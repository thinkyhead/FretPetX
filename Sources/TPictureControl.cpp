/*
 *  TPictureControl.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TPictureControl.h"
#include "TControls.h"
#include "FPUtilities.h"
#include "TString.h"
#include "TError.h"

TPictureHelper::TPictureHelper(TControl *inControl, UInt16 pictID, UInt16 numPicts) {
	owningControl	= inControl;
	blinkState		= false;
	blinking		= false;
	pictCount		= 0;

	for (int i=MAX_PICTS; i--;) {
		graphic[i] = NULL;
		icon[i] = NULL;
	}

	LoadPictResources(pictID, numPicts);
}


TPictureHelper::~TPictureHelper() {
	DisposePicts();
}


void TPictureHelper::LoadPictResources(UInt16 pictID, UInt16 numPicts) {
	DisposePicts();

	int count = numPicts ? numPicts : owningControl->GetID();

	for (int i=0; i<count; i++)
		AddPictResource(pictID + i);
}


void TPictureHelper::AddPictResource(UInt16 pictID) {
	PicHandle	hPict;

	hPict = GetPicture(pictID);
	verify_action( hPict != NULL, throw TError(0, CFSTR("Can't load the PICT.")) );

	AddPicture(hPict);
	ReleaseResource((Handle)hPict);
}


void TPictureHelper::AddPicture(PicHandle hPict) {
	icon[pictCount++] = MakeIconWithPicture(hPict);
}


void TPictureHelper::DisposePicts() {
	if (pictCount) {
		for (int i=pictCount; i--;) {
			if (graphic[i]) {
				DisposeGWorld(graphic[i]);
				graphic[i] = NULL;
			}

			if (icon[i]) {
				ReleaseIconRef(icon[i]);
				icon[i] = NULL;
			}
		}

		pictCount = 0;
	}
}


bool TPictureHelper::GetDimmedState() {
	return !owningControl->IsEnabled() || !owningControl->IsActive();
}


//-----------------------------------------------
//
//  GetFrameForState
//
//	Depending on the number of frames and the state of
//	the given TControl determine which picture to show.
//
//	Number 0 and 5
//	can be drawn by the system (normal and disabled).
//
//	0 - Normal
//	1 - Pressed
//	2 - State On
//	3 - State On Pressed
//	4 - Blink Frame (timer driven while active)
//	5 - Disabled (system draws as dimmed Normal)
//
UInt16 TPictureHelper::GetFrameForState() {
	UInt16  frame = kFrameNormal;
	bool	pressed = owningControl->IsPressed();
	bool	state = owningControl->State();

	switch (pictCount) {
		case 2:
			frame =	pressed ? (state ? kFrameNormal : kFramePressed)
							: (state ? kFramePressed : kFrameNormal);
			break;

		case 3:
			frame =	pressed ? kFramePressed
							: (state ? kFrameSet : kFrameNormal);

			break;

		case 4:
			frame = pressed ? (state ? kFrameSetPressed : kFramePressed)
							: (state ? kFrameSet : kFrameNormal);
			break;

		case 5:
			frame = pressed ? (state ? kFrameSetPressed : kFramePressed)
							: (state ? (blinkState ? kFrameBlink : kFrameSet) : kFrameNormal);

			break;

		default:
			fprintf(stderr, "Not sure how to handle this picture button!\n");
			break;
	}

	return frame;
}


void TPictureHelper::Draw(const Rect &bounds) {
	const UInt16	frame = GetFrameForState();
	const bool		dimmed = GetDimmedState();

	EraseRect(&bounds);

	if (graphic[frame]) {
		const BitMap	*inBitmap = GetPortBitMapForCopyBits(owningControl->WindowPort());

		if (dimmed)
			RGBForeColor(&rgbGray8);

		Rect gsize;
		GetPortBounds(graphic[frame], &gsize);
		LockPortBits(graphic[frame]);
		CopyBits(GetPortBitMapForCopyBits(graphic[frame]), inBitmap, &gsize, &bounds, srcCopy, NULL);
		UnlockPortBits(graphic[frame]);
	}
	else if (icon[frame]) {
		Rect iconRect = { bounds.top, bounds.left, bounds.top + ICON_SIZE, bounds.left + ICON_SIZE };
		(void)PlotIconRef(
					&iconRect,
					kAlignTopLeft,
					dimmed ? kTransformDisabled : kTransformNone,
					kIconServicesNormalUsageFlag,
					icon[frame]
				);
	}
}


#pragma mark -
TPictureControl::TPictureControl(WindowRef wind, OSType sig, UInt32 cid, UInt16 pictID, UInt16 numPicts)
	: TControl(wind, sig, cid)
	, TPictureHelper(this, pictID, numPicts) {
	Init();
}


TPictureControl::TPictureControl(WindowRef wind, UInt32 cmd, UInt16 pictID, UInt16 numPicts)
	: TControl(wind, cmd)
	, TPictureHelper(this, pictID, numPicts) {
	Init();
}


TPictureControl::TPictureControl(ControlRef controlRef, UInt16 pictID, UInt16 numPicts)
	: TControl(controlRef)
	, TPictureHelper(this, pictID, numPicts) {
	Init();
}


void TPictureControl::Init() {
	timerUPP	= NULL;
	blinkLoop	= NULL;
}


void TPictureControl::DoBlink() {
	if (IsEnabled() && !IsPressed()) {
		TPictureHelper::DoBlink();
		DrawNow();
	}
}


void TPictureControl::Blink(bool on) {
	if (on != IsBlinking()) {
		if (on) {
			if (blinkLoop == NULL) {
				timerUPP = NewEventLoopTimerUPP(TPictureControl::BlinkLoopTimerProc);

				(void)InstallEventLoopTimer(
					GetMainEventLoop(),
					0.0, 0.5,
					timerUPP,
					this,
					&blinkLoop
					);
			}
		}
		else {
			if (blinkLoop != NULL) {
				(void)RemoveEventLoopTimer(blinkLoop);
				DisposeEventLoopTimerUPP(timerUPP);
				blinkLoop = NULL;
				timerUPP = NULL;
			}
		}

		TPictureHelper::Blink(on);
	}
}


#pragma mark -
TPicturePopup::TPicturePopup(WindowRef wind, OSType sig, UInt32 cid, CFStringRef menuName, UInt16 pictID, UInt16 numPicts)
	: TPictureControl(wind, sig, cid, pictID, numPicts) {
	Init();
	AttachMenu(menuName);
}


TPicturePopup::TPicturePopup(WindowRef wind, UInt32 cmd, CFStringRef menuName, UInt16 pictID, UInt16 numPicts)
	: TPictureControl(wind, cmd, pictID, numPicts) {
	Init();
	AttachMenu(menuName);
}


TPicturePopup::TPicturePopup(ControlRef controlRef, CFStringRef menuName, UInt16 pictID, UInt16 numPicts)
	: TPictureControl(controlRef, pictID, numPicts) {
	Init();
	AttachMenu(menuName);
}


TPicturePopup::~TPicturePopup() {
	if (popupMenu)
		DisposeMenu(popupMenu);
}


void TPicturePopup::Init() {
	popupMenu = NULL;

	const EventTypeSpec	popEvents[] = {
		{ kEventClassControl, kEventControlContextualMenuClick }
		};

	RegisterForEvents(GetEventTypeCount(popEvents), popEvents);
}


void TPicturePopup::AttachMenu(CFStringRef menuName) {
	if (popupMenu)
		DisposeMenu(popupMenu);

	popupMenu = GetMiniMenu(menuName);

	const EventTypeSpec	popEvents[] = {
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlHit }
	};

	RegisterForEvents( GetEventTypeCount( popEvents ), popEvents );
}


ControlPartCode TPicturePopup::HitTest(Point where) {
	Rect bounds = Bounds();
	where.h += bounds.left;
	where.v += bounds.top;

	if (PtInRect(where, &bounds))
		return kControlButtonPart;
	else
		return kControlNoPart;
}


bool TPicturePopup::ControlHit(Point where) {
	Rect			bounds = Bounds();
	Point			globPoint = { bounds.top, bounds.left };
	TWindow			*owningWindow = GetTWindow();

	owningWindow->Focus();
	LocalToGlobal(&globPoint);
	(void) PopUpMenuSelect(popupMenu, globPoint.v, globPoint.h, itemUnderMouse);
	return true;
}


void TPicturePopup::ContextClick() {
	Point where = { 0, 0 };
	ControlHit(where);
}


#pragma mark -
TPictureDragger::TPictureDragger(WindowRef wind, OSType sig, UInt32 cid, UInt16 pictID, UInt16 numPicts)
	: TCustomControl(wind, sig, cid)
	, TPictureHelper(this, pictID, numPicts) {
	Init();
}


TPictureDragger::TPictureDragger(WindowRef wind, UInt32 cmd, UInt16 pictID, UInt16 numPicts)
	: TCustomControl(wind, cmd)
	, TPictureHelper(this, pictID, numPicts) {
	Init();
}


TPictureDragger::TPictureDragger(ControlRef controlRef, UInt16 pictID, UInt16 numPicts)
	: TCustomControl(controlRef)
	, TPictureHelper(this, pictID, numPicts) {
	Init();
}


void TPictureDragger::Init() {
	const EventTypeSpec	wheelEvents[] = {
			{ kEventClassMouse, kEventMouseWheelMoved }
		};
	RegisterForEvents(GetEventTypeCount(wheelEvents), wheelEvents);

	editField = NULL;
	SetRange(1, 100);
}

bool TPictureDragger::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {

	if (editField->GetOwner() == this)
		ActivateField(false);

	SInt32 val = Value();
	SetValue(val + delta);
	if (val != Value()) ValueChanged();
	return true;
}


void TPictureDragger::DrawText(const Rect &bounds, CFStringRef inString) {
	TControl::DrawText(bounds, inString, kThemeMiniSystemFont, teFlushRight);
}


ControlPartCode TPictureDragger::HitTest(Point where) {
	return kControlIndicatorPart;
}

bool TPictureDragger::ControlHit(Point where) {
	return true;
}


void TPictureDragger::ActivateField(bool b) {
	if (editField->IsVisible()) {
		editField->UnFocus();
		editField->Hide();
	}

	if (b) {
		Rect rect = Bounds();
		InsetRect(&rect, 3, 3);
		editField->SetOwner(this);
		editField->SetBounds(&rect);
		editField->SetText(TString(Value()).GetCFStringRef());
		editField->Show();
		editField->Enable();
		editField->DoFocus();
	}
}


bool TPictureDragger::Track(MouseTrackingResult eventType, Point where) {
	static Point	lastPoint;
	static bool		didDrag, draggable;
	SInt32			diff = 0;
	bool			mouseUpped = false;

	switch (eventType) {
		case kMouseTrackingMouseDown: {
			Rect size = Size();
			draggable = (where.h > size.right - 8);

			if (draggable) {
				Hilite(kControlIndicatorPart);
				didDrag = false;
				DoMouseDown();
			}

			break;
		}

		case kMouseTrackingMouseDragged:

			if (draggable) {
				diff = lastPoint.v - where.v;
				didDrag = true;
				break;
			}

		case kMouseTrackingMouseUp:

			if (draggable) {
				if (didDrag)
					diff = 0;
				else {
					Rect size = Size();
					diff = (where.v <= (size.bottom + 1) / 2) ? 1 : -1;
				}

				Hilite(kControlNoPart);
				draggable = false;
				mouseUpped = true;
				break;
			}
			else if (editField && CountClicks(true, 2) == 2) {
				// Double-click makes an edit field appear
				// TODO: Make the value apply when return is pressed
				ActivateField();
			}
	}

	if (diff) {
		SInt32 val = Value() + diff;
		CONSTRAIN(val, Minimum(), Maximum());
		if (val != Value()) {
			SetValue(val);
			ValueChanged();
		}
	}

	if (mouseUpped)
		DoMouseUp();

	lastPoint = where;

	return true;
}


