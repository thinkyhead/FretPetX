/*
    File:		TWindow.cp
    
    Version:	Mac OS X

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Copyright © 2000-2001 Apple Computer, Inc., All Rights Reserved
*/

#include "TWindow.h"
#include "TCarbonEvent.h"
#include "TTrackingRegion.h"

const EventTypeSpec	kEvents[] = { 
	{ kEventClassWindow, kEventWindowClose },
	{ kEventClassWindow, kEventWindowActivated },
	{ kEventClassWindow, kEventWindowDeactivated },
	{ kEventClassWindow, kEventWindowDrawContent },
	{ kEventClassWindow, kEventWindowBoundsChanged },
	{ kEventClassWindow, kEventWindowGetIdealSize }
};

TWindow::TWindow() {
	Init();
}

TWindow::TWindow( WindowClass inClass, WindowAttributes inAttributes, const Rect& inBounds ) {
//	fprintf(stderr, "TWindow::TWindow(WindowClass, WindowAttributes, Rect&) = %08X\n", this);

	Init();

	OSStatus		err;
	WindowRef		window;

	err = CreateNewWindow( 	inClass,
							inAttributes,
							&inBounds,
							&window );

	InitWithPlatformWindow( window );							
}

TWindow::TWindow( CFStringRef nibName, CFStringRef name ) {
//	fprintf(stderr, "TWindow::TWindow(CFStringRef, CFStringRef) = %08X\n", this);

	Init();

	OSStatus		err;
	WindowRef		window;
	IBNibRef		ref;
	
	err = CreateNibReference( nibName, &ref );
	if ( err == noErr ) {
		err = CreateWindowFromNib( ref, name, &window );
		DisposeNibReference( ref );
	}
	
	InitWithPlatformWindow( window );							
}

void TWindow::Init() {
	isActive			= false;
	fWindow				= NULL;
	fPort				= NULL;
	fHandler			= NULL;
	localEnteredRegion	= NULL;
	context				= NULL;
	currentFocus		= NULL;
}

TWindow* TWindow::GetTWindow( WindowRef window ) {
	return (TWindow*)GetWRefCon( window );
}

void TWindow::InitWithPlatformWindow( WindowRef window ) {
	WindowClass		theClass;
	
	fWindow = window;
	fPort = GetWindowPort( window );

	SetWindowKind( window, 2001 );
	SetWRefCon( window, (long)this );
	
	InstallWindowEventHandler( window, GetEventHandlerProc(), GetEventTypeCount( kEvents ),
						kEvents, this, &fHandler );

	GetWindowClass( Window(), &theClass );
	if ( theClass == kDocumentWindowClass )
		ChangeWindowAttributes( window, kWindowInWindowMenuAttribute, 0 );
}

TWindow::~TWindow() {
	if ( fWindow )
		DisposeWindow( fWindow );
}

void TWindow::PlatformWindowDisposed() {
	fWindow = NULL;
	fPort = NULL;
}

#pragma mark -

CGrafPtr TWindow::Port() const {
	return fPort;
}

WindowRef TWindow::Window() const {
	return fWindow;
}

CFStringRef TWindow::CopyTitle() const {
	CFStringRef	outTitle;
	CopyWindowTitleAsCFString(fWindow, &outTitle);
	return outTitle;
}

CFStringRef TWindow::CopyAlternateTitle() const {
	CFStringRef	outTitle;
	CopyWindowAlternateTitle(fWindow, &outTitle);
	return outTitle;
}

void TWindow::Show() {
	ShowWindow( fWindow );
	AdvanceKeyboardFocus( fWindow );
}

void TWindow::ShowSheet( WindowRef inParentWindow ) {
	(void) ShowSheetWindow( fWindow, inParentWindow );
	AdvanceKeyboardFocus( fWindow );
}

void TWindow::HideSheet() {
	(void)HideSheetWindow( fWindow );
}

WindowRef TWindow::SheetParent() {
	WindowRef parent = NULL;
	(void) GetSheetWindowParent(fWindow, &parent);
	return parent;
}

DialogRef TWindow::MakeStandardSheet(
				AlertType	alertType,
				CFStringRef	error,
				CFStringRef	explanation,						/* can be NULL */
				const AlertStdCFStringAlertParamRec *param		/* can be NULL */
				) {
	DialogRef	outDialog;
	(void) CreateStandardSheet(alertType, error, explanation, param, NULL /* EventTarget() */, &outDialog);
	return outDialog;
}

Point TWindow::GetIdealSize() {
	Point size = { 0, 0 };
	return size;
}

void TWindow::TransitionSize( short w, short h ) {
	Rect structure = StructureBounds(), bounds = Bounds();
	structure.right = structure.left + w;
	structure.bottom = bounds.top + h;
	(void)TransitionWindow(fWindow, kWindowSlideTransitionEffect, kWindowResizeTransitionAction, &structure);
}

#pragma mark -

void TWindow::SetOpaque(bool op) {
	HIWindowChangeFeatures( fWindow, op ? kWindowIsOpaque : 0 , op ? 0 : kWindowIsOpaque );
	ReshapeCustomWindow( fWindow );
	SetAlpha( op ? 1.0 : 0.999 );
}

void TWindow::Cascade(SInt16 h, SInt16 v) {
	WindowRef	front = FrontNonFloatingWindow();
	TWindow		*twind = NULL;
	if (front) {
		twind = TWindow::GetTWindow( front );
		if (twind != NULL) {
			Rect bounds = twind->Bounds();
			Move( bounds.left + h, bounds.top + v );
		}
	}
}

void TWindow::Center(SInt16 v) {

	Rect screenRect, windowRect = GetContentSize();
	OSStatus err = GetWindowGreatestAreaDevice(Window(), kWindowStructureRgn, NULL, &screenRect);
	
	Move(
		(screenRect.left + screenRect.right - windowRect.right) / 2,
		(screenRect.top + screenRect.bottom - windowRect.bottom) / 2 + v);
}

Rect TWindow::Bounds() {
	Rect bounds;
	GetWindowBounds( fWindow, kWindowContentRgn, &bounds );
	return bounds;
}

Rect TWindow::StructureBounds() {
	Rect bounds;
	GetWindowBounds( fWindow, kWindowStructureRgn, &bounds );
	return bounds;
}

Rect TWindow::GetContentSize() {
	Rect portBounds;
	GetWindowPortBounds( fWindow, &portBounds );
	return portBounds;
}

CGRect TWindow::CGBounds() {
	Rect rect1 = GetContentSize();
	return CGRectMake(0, 0, rect1.right - rect1.left, rect1.bottom - rect1.top);
}

#pragma mark -

void TWindow::InvalidateWindow() {
	Rect rect = GetContentSize();
	InvalWindowRect( fWindow, &rect );
}

#pragma mark -

bool TWindow::UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	if ( cid == kHICommandClose ) {
		EnableMenuCommand( NULL, cid );
		return true;
	}
	
	return false; // not handled
}

bool TWindow::HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	if ( cid == kHICommandClose ) {
		EventRef		event;
		
		if ( CreateEvent( NULL, kEventClassWindow, kEventWindowClose,
				GetCurrentEventTime(), 0, &event ) == noErr ) {
			WindowRef	window = Window();
			SetEventParameter( event, kEventParamDirectObject, typeWindowRef,
					sizeof( WindowRef ), &window );
			SendEventToEventTarget( event, GetWindowEventTarget( window ) );
			ReleaseEvent( event );
		}
		return true;
	}
	
	return false; // not handled
}

CGContextRef TWindow::FocusContext() {
	if (context != NULL)
		QDEndCGContext(fPort, &context);

	(void)QDBeginCGContext(fPort, &context);

	return context;
}

void TWindow::EndContext() {
	if (context != NULL)
		(void)QDEndCGContext(fPort, &context);
}

void TWindow::DrawText(const Rect &bounds, CFStringRef inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext) {
	CGContextRef	gc;
	OSStatus		err;

	gc = inContext ? inContext : FocusContext();

	err = DrawThemeTextBox( inString, fontID, GetDrawState(), false, &bounds, inJust, gc );

	if (inContext == NULL)
		EndContext();
}

void TWindow::DrawText(const Rect &bounds, StringPtr inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext) {
	CFStringRef aString = CFStringCreateWithPascalString(kCFAllocatorDefault, inString, kCFStringEncodingMacRoman);
	DrawText(bounds, aString, fontID, inJust, inContext);
	CFRELEASE(aString);
}

void TWindow::UpdateNow() {
	if ( IsWindowUpdatePending( Window() ) ) {
		TCarbonEvent	event(kEventClassWindow, kEventWindowUpdate);
		
		WindowRef	window = Window();

		event.SetParameter(kEventParamDirectObject, window);
	
		SendEventToEventTarget( event, GetWindowEventTarget( window ) );
		ReleaseEvent( event );
	}
}

#pragma mark -

OSStatus TWindow::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
//	fprintf( stderr, "[%08X] TWindow Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus		result = eventNotHandledErr;
	OSStatus		err;
	UInt32			attributes;
	HICommand		command;
	
	switch ( event.GetClass() ) {
		case kEventClassKeyboard: {
			switch ( event.GetKind() ) {
				case kEventRawKeyModifiersChanged: {
					if ( TTrackingRegion::globalEnteredRegion != NULL )
						TTrackingRegion::globalEnteredRegion->GetTWindow()->HandleTrackingRegion(TTrackingRegion::globalEnteredRegion, event);
					break;
				}
			}
			break;
		}

		case kEventClassCommand: {
			event.GetParameter(kEventParamDirectObject, &command);
		
			switch ( event.GetKind() ) {
				case kEventCommandUpdateStatus:
//						fprintf(stderr, "TWindow kEventCommandUpdateStatus '%c%c%c%c'...\n", FOUR_CHAR(command.commandID));
					if ( UpdateCommandStatus( command.commandID, command.menu.menuRef, command.menu.menuItemIndex ) )
						result = noErr;
					break;

				case kEventCommandProcess:
//						fprintf(stderr, "TWindow kEventCommandProcess...\n");
					if ( HandleCommand( command.commandID, command.menu.menuRef, command.menu.menuItemIndex ) )
						result = noErr;
					break;
			}
			break;
		}

		case kEventClassMouse: {
			MouseTrackingRef	trackingRef;
			UInt32				eventKind = event.GetKind();
			OSStatus			err = event.GetParameter(kEventParamMouseTrackingRef, typeMouseTrackingRef, sizeof(MouseTrackingRef), &trackingRef);
			if (err == noErr) {
				TTrackingRegion *trackRegion;
				err = GetMouseTrackingRegionRefCon(trackingRef, (void**)&trackRegion);

				switch (eventKind) {
					case kEventMouseEntered: {
						if (IsActive()) {
//							fprintf(stderr, "Entered: %08X\n", trackRegion);

							ExitPreviousRegion();

							// Mark this region as entered
							trackRegion->MarkEntered(true);
							TrackingRegionEntered(trackRegion);

							// Send the enter event to the window
							HandleTrackingRegion( trackRegion, event );
							result = noErr;
						}
						break;
					}

					case kEventMouseExited:

						if (localEnteredRegion == trackRegion) {
							HandleTrackingRegion( trackRegion, event );
							TrackingRegionEntered( NULL );
						}

						// Mark the region as exited
						trackRegion->MarkEntered(false);
						result = noErr;
						break;
				}

			}
			else {
				switch ( eventKind ) {
					case kEventMouseWheelMoved: {
						SInt32				delta;
						EventMouseWheelAxis	axis;
						err = event.GetParameter(kEventParamMouseWheelDelta, typeSInt32, sizeof(SInt32), &delta);
						err = event.GetParameter(kEventParamMouseWheelAxis, typeMouseWheelAxis, sizeof(typeMouseWheelAxis), &axis);
						if ( err == noErr && MouseWheelMoved(axis ? delta : 0, axis ? 0 : delta) )
							result = noErr;

						break;
					}
				}
			}

			break;
		}

		case kEventClassWindow:
			switch ( event.GetKind() ) {
				case kEventWindowClose:
					Close();
					result = noErr;
					break;
				
				case kEventWindowDrawContent:
					CallNextEventHandler( inRef, event.GetEventRef() );
					Draw();
					result = noErr;
					break;
				
				case kEventWindowActivated:
					CallNextEventHandler( inRef, event.GetEventRef() );
					isActive = true;
					Activated();
					result = noErr;
					break;

				case kEventWindowDeactivated:
					CallNextEventHandler( inRef, event.GetEventRef() );
					isActive = false;
					Deactivated();
					result = noErr;
					break;

				case kEventWindowBoundsChanged:
					err = event.GetParameter(kEventParamAttributes, &attributes);
					if ( err == noErr ) {
						if ( attributes & kWindowBoundsChangeSizeChanged ) {
							Resized();
							result = noErr;
						}
						else if ( attributes & kWindowBoundsChangeOriginChanged ) {
							Moved();
							result = noErr;
						}
					}
					break;
				
				case kEventWindowGetIdealSize: {
					Point	size = GetIdealSize();
					
					if ( (size.h != 0) && (size.v != 0) ) {
						event.SetParameter(kEventParamDimensions, size);
						result = noErr;
					}
					break;
				}

				case kEventWindowZoom:
					if (Zoom())
						result = noErr;
					else
						CallNextEventHandler( inRef, event.GetEventRef() );
					break;
			}
	};
	
	return result;
}

EventHandlerUPP TWindow::GetEventHandlerProc() {
	static EventHandlerUPP handlerProc = NULL;
	
	if ( handlerProc == NULL )
		handlerProc = NewEventHandlerUPP( EventHandlerProc );
	
	return handlerProc;
}

pascal OSStatus TWindow::EventHandlerProc( EventHandlerCallRef handler, EventRef inEvent, void* userData ) {
	TCarbonEvent event(inEvent);
	return ((TWindow*)userData)->HandleEvent( handler, event );
}

void TWindow::EnableControlByID( ControlID theID ) {
	ControlRef	control;
	OSStatus	err;
	
	err = GetControlByID( Window(), &theID, &control );
	if ( err == noErr )
		(void)EnableControl( control );
}

void TWindow::DisableControlByID( ControlID theID ) {
	ControlRef	control;
	OSStatus	err;
	
	err = GetControlByID( Window(), &theID, &control );
	if ( err == noErr )
		(void)DisableControl( control );
}

void TWindow::ShowControlByID( ControlID theID ) {
	ControlRef	control;
	OSStatus	err;
	
	err = GetControlByID( Window(), &theID, &control );
	if ( err == noErr )
		ShowControl( control );
}

void TWindow::HideControlByID( ControlID theID ) {
	ControlRef	control;
	OSStatus	err;
	
	err = GetControlByID( Window(), &theID, &control );
	if ( err == noErr )
		HideControl( control );
}

#pragma mark -

TTrackingRegion* TWindow::NewTrackingRegion(RgnHandle inRegion, SInt32 inID, OSType signature) {
	TTrackingRegion	*trgn = new TTrackingRegion(this, inRegion, inID, signature);
	AddTrackingRegion(trgn);
	return trgn;
}

TTrackingRegion* TWindow::NewTrackingRegion(Rect &inRect, SInt32 inID, OSType signature) {
	TTrackingRegion	*trgn = new TTrackingRegion(this, inRect, inID, signature);
	AddTrackingRegion(trgn);
	return trgn;
}

void TWindow::AddTrackingRegion(TTrackingRegion *region) {
	trackingRegionList.push_back(region);
}

void TWindow::RemoveTrackingRegion(TTrackingRegion *region) {
	if (localEnteredRegion == region)
		ExitPreviousRegion();

	trackingRegionList.remove(region);
}

void TWindow::TrackingRegionEntered(TTrackingRegion *region) {
	localEnteredRegion = region;
	TTrackingRegion::globalEnteredRegion = region;
}

void TWindow::ExitPreviousRegion() {
	if (localEnteredRegion != NULL)
		localEnteredRegion->SimulateExitEvent();
}

TTrackingRegion* TWindow::TrackingRegionUnderMouse() {
	// This only returns active enabled regions.
	return TTrackingRegion::globalEnteredRegion;
}

void TWindow::SetAllRegionsActive(bool act) {
	if (!act)
		ExitPreviousRegion();

	for (TrackingRegionIterator itr = trackingRegionList.begin();
			itr != trackingRegionList.end();
			(*(itr++))->SetActive(act)
		);

	if (act)
		EnterRegionUnderMouse();
}

void TWindow::Activated() {
	ActivateAllRegions();
}

void TWindow::EnterRegionUnderMouse() {
	TTrackingRegion *reg = TrackingRegionUnderMouse();
	if (reg != NULL)
		reg->SimulateEnterEvent();
}
