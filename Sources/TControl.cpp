/*
 *  TControl.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TControl.h"
#include "TCarbonEvent.h"
#include "TError.h"

//-----------------------------------------------
//
//  TControl
//
TControl::TControl() {
	Init();
}


TControl::TControl(WindowRef wind, OSType sig, UInt32 cid) {
	Init();
	SpecifyByControlID(wind, sig, cid);
	InstallEventHandler();
}


TControl::TControl(WindowRef wind, UInt32 cmd) {
	Init();
	SpecifyByCommandID(wind, cmd);
	InstallEventHandler();
}


TControl::TControl(ControlRef controlRef) {
	Init();
	SpecifyByControlRef(controlRef);
	InstallEventHandler();
}


TControl::TControl(DialogRef dialog, SInt16 item) {
	Init();
	SpecifyByDialogItem(dialog, item);
	InstallEventHandler();
}


TControl::~TControl() {
	DisableAction();
}


void TControl::Init() {
	fControl	= NULL;
	fHandler	= NULL;
	fWindow		= NULL;
	actionUPP	= NULL;
	state		= 0;
	command		= 0;
}


CGRect TControl::CGBounds() {
	Rect	rect1 = Bounds(),
			rect2 = GetTWindow()->GetContentSize();
	CGRect cgRect = { { rect1.left, rect2.bottom - rect1.bottom }, { rect1.right - rect1.left, rect1.bottom - rect1.top } };
	return cgRect;
}


//-----------------------------------------------
//
// SpecifyByCommandID
//
// Find a control anywhere in the hierarchy with the
//	specified command ID. Will search up to ten nested
//	levels of embedded controls, which is plenty.
//
OSStatus TControl::SpecifyByCommandID(WindowRef wind, UInt32 cmd) {
	UInt32		foundID;
	UInt16		stCount[10], count;
	ControlRef	stControl[10], cont;
	int			level = 0;

	OSStatus err = GetRootControl(wind, &stControl[0]);
	verify_action( stControl[0]!=NULL, throw TError(err, CFSTR("No root control.")) );

	CountSubControls(stControl[0], &stCount[0]);

	while(level >= 0 && stCount[0] < 1000) {
		err = GetIndexedSubControl(stControl[level], stCount[level], &cont);
		err = GetControlCommandID(cont, &foundID);
		--stCount[level];

		if (foundID == cmd) {
			SpecifyByControlRef(cont);
			break;
		}

		CountSubControls(cont, &count);
		if (count > 0) {
			level++;
			stControl[level] = cont;
			stCount[level] = count;
		} else if (stCount[level] < 1)
			level--;
	}

	verify_action( fControl!=NULL, throw TError(err, CFSTR("No control with given CommandID found.")) );

	return err;
}


OSStatus TControl::SpecifyByControlID(WindowRef wind, OSType sig, UInt32 cid) {
	ControlRef	cont;
	ControlID	inID = { sig, cid };
	OSStatus err = GetControlByID(wind, &inID, &cont);
	verify_action( err==noErr, throw TError(err, CFSTR("Unable to find the control.")) );

	SpecifyByControlRef(cont);
	return err;
}


void TControl::SpecifyByControlRef(ControlRef controlRef) {
	fControl = controlRef;
	fWindow = GetControlOwner(controlRef);
	SetControlReference(controlRef, (SInt32)this);
}


OSStatus TControl::SpecifyByDialogItem(DialogRef dialog, SInt16 item) {
	ControlRef	control;
	OSStatus err = GetDialogItemAsControl(dialog, item, &control);
	verify_action( err==noErr, throw TError(err, CFSTR("Unable to find the control.")) );

	SpecifyByControlRef(control);
	return err;
}


/* Provide a draw method for HIView/Quartz drawing */
bool TControl::Draw(const CGContextRef context, const Rect &bounds) {
	return false;
}

bool TControl::Draw(const Rect &bounds) {
/*
	ForeColor(blackColor);
	FrameRect(&bounds);

	Rect inRect = bounds;
	InsetRect(&inRect, 1, 1);

	ForeColor(whiteColor);
	PaintRect(&inRect);
*/

	return false;
}


void TControl::DrawText(const Rect &bounds, CFStringRef inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext) {
	CGContextRef	gc;
	OSStatus		err;

	gc = inContext ? inContext : FocusContext();

	err = DrawThemeTextBox( inString, fontID, GetDrawState(), false, &bounds, inJust, gc );

	if (inContext == NULL)
		EndContext();
}


void TControl::DrawText(const Rect &bounds, StringPtr inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext) {
	CFStringRef aString = CFStringCreateWithPascalString(kCFAllocatorDefault, inString, kCFStringEncodingMacRoman);
	DrawText(bounds, aString, fontID, inJust, inContext);
	CFRELEASE(aString);
}


void TControl::SetAndLatch(bool enable) {
	SetState(enable);
	SetEnabled(!enable);
}


void TControl::SetState(bool yesno) {
	state = yesno;
	DrawNow();
}


void TControl::SetVisible(bool show) {
	if (show)
		ShowControl(fControl);
	else
		HideControl(fControl);
}


void TControl::SetActivated(bool active) {
	if (active)
		ActivateControl(fControl);
	else
		DeactivateControl(fControl);
}


void TControl::SetEnabled(bool enable) {
	if (enable)
		EnableControl(fControl);
	else
		DisableControl(fControl);
}


OSStatus TControl::SetHelpTag(char *inString, bool localType) {
	CFStringRef stringRef = CFStringCreateWithCString(kCFAllocatorDefault, inString, kCFStringEncodingMacRoman);
	OSStatus err = SetHelpTag(stringRef, localType);
	CFRELEASE(stringRef);
	return err;
}


OSStatus TControl::SetHelpTagPascal(StringPtr inString, bool localType) {
	CFStringRef stringRef = CFStringCreateWithPascalString(kCFAllocatorDefault, inString, kCFStringEncodingMacRoman);
	OSStatus err = SetHelpTag(stringRef, localType);
	CFRELEASE(stringRef);
	return err;
}


OSStatus TControl::SetHelpTag(CFStringRef inString, bool localType) {
    HMHelpContentRec helpTag;
    helpTag.version = kMacHelpVersion;
    helpTag.tagSide = kHMDefaultSide;
    SetRect(&helpTag.absHotRect, 0, 0, 0, 0);
    helpTag.content[kHMMinimumContentIndex].contentType = (localType ? kHMCFStringLocalizedContent : kHMCFStringContent);
    helpTag.content[kHMMinimumContentIndex].u.tagCFString = inString;
    helpTag.content[kHMMaximumContentIndex].contentType = kHMNoContent;

	return HMSetControlHelpContent(fControl, &helpTag);
}


void TControl::GetLocalPoint(Point *point) {
	Rect	bounds;

	GetControlBounds(fControl, &bounds);

	point->h -= bounds.left;
	point->v -= bounds.top;
}


#pragma mark -
void TControl::InstallEventHandler() {
	const EventTypeSpec	baseEvents[] = {
//		{ kEventClassControl, kEventControlInitialize },
		{ kEventClassControl, kEventControlDraw },
		{ kEventClassControl, kEventControlHiliteChanged },
		{ kEventClassControl, kEventControlDeactivate }
	};

	::InstallControlEventHandler(
		fControl,
		NewEventHandlerUPP(TControl::EventHandler),
		GetEventTypeCount(baseEvents),
		baseEvents,
		this,
		&fHandler
	);
}


//-----------------------------------------------
//
//  EventHandler
//
//  The eventhandler calls the handleevent method,
//  for the object indicated by the userData parameter.
//
pascal OSStatus
TControl::EventHandler(EventHandlerCallRef inRef, EventRef inEvent, void* userData) {
	OSStatus		err = noErr;
	TCarbonEvent	event(inEvent);

	if (((TControl*)userData)->HandleEvent(inRef, event) == false)
		err = CallNextEventHandler( inRef, inEvent );

	return err;
}


bool TControl::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
//	fprintf( stderr, "[%08X] Control Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus			result = eventNotHandledErr;
	ControlPartCode		part;
	Point				where;
	Rect				bounds;

	switch ( event.GetClass() ) {
		case kEventClassControl:

			switch ( event.GetKind() ) {
				case kEventControlDeactivate:
					DidDeactivate();
					break;

				case kEventControlHiliteChanged:
					HiliteChanged();
					break;

				case kEventControlInitialize: {
					UInt32 features;
					(void) event.GetParameter( kEventParamControlFeatures, &features );
					features |= EnableFeatures();
					features &= ~ DisableFeatures();
					(void) event.SetParameter( kEventParamControlFeatures, features );
					break;
				}

				case kEventControlDraw: {
					CGContextRef theContext = NULL;
					(void) event.GetParameter( kEventParamCGContextRef, &theContext );
					if ( Draw(theContext, Bounds()) )
						result = noErr;
					else if ( Draw(Bounds()) )	// no hiview yet?
						result = noErr;
					break;
				}

				case kEventControlHitTest:
					(void) event.GetParameter( kEventParamMouseLocation, &where );
					bounds = Bounds();
					where.h -= bounds.left; where.v -= bounds.top;
					part = HitTest(where);
					event.SetParameter(kEventParamControlPart, part);
					result = noErr;
					break;

				case kEventControlHit:
					(void) event.GetParameter( kEventParamMouseLocation, &where );
					if (ControlHit(where))
						result = noErr;
					break;

				case kEventControlContextualMenuClick:
					ContextClick();
					result = noErr;
					break;

				case kEventControlDragEnter:
				case kEventControlDragLeave:
				case kEventControlDragWithin: {
					DragRef drag;
					bool likesDrag;

					event.GetParameter( kEventParamDragRef, &drag );

					switch ( event.GetKind() ) {
						case kEventControlDragEnter:
						likesDrag = DragEnter( drag );
						result = event.SetParameter( kEventParamControlWouldAcceptDrop, likesDrag );
						break;

						case kEventControlDragLeave:
						likesDrag = DragLeave( drag );
						result = event.SetParameter( kEventParamControlWouldAcceptDrop, likesDrag );
						break;

						case kEventControlDragWithin:
						likesDrag = DragWithin( drag );
						result = event.SetParameter( kEventParamControlWouldAcceptDrop, likesDrag );
						break;
					}
				}
				break;

				case kEventControlDragReceive: {
					DragRef drag;
					event.GetParameter( kEventParamDragRef, &drag );
					result = DragReceive( drag );
				}
				break;


			}
			break;

		case kEventClassMouse: {
			switch ( event.GetKind() ) {
				case kEventMouseWheelMoved: {
					SInt32				delta;
					EventMouseWheelAxis	axis;

					OSStatus err = event.GetParameter(kEventParamMouseWheelDelta, typeSInt32, sizeof(SInt32), &delta);
					err = event.GetParameter(kEventParamMouseWheelAxis, typeMouseWheelAxis, sizeof(typeMouseWheelAxis), &axis);
					if ( err == noErr && MouseWheelMoved(axis ? delta : 0, axis ? 0 : delta) )
						result = noErr;

					break;
				}
			}
			break;
		}

	}

	return result != eventNotHandledErr;
}


