/*
 *  TCustomControl.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TCustomControl.h"
#include "TCarbonEvent.h"

#define kMyPartCode 666

UInt32	TCustomControl::lastClick = 0;
int		TCustomControl::clickCount = 1;

TCustomControl::TCustomControl(WindowRef wind, OSType sig, UInt32 cid) : TControl(wind, sig, cid) {
	Init();
}


TCustomControl::TCustomControl(WindowRef wind, UInt32 cmd) : TControl(wind, cmd) {
	Init();
}


TCustomControl::~TCustomControl() {
}


void TCustomControl::Init() {
	const EventTypeSpec	custEvents[] = {
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlHit }
	};

	RegisterForEvents(GetEventTypeCount(custEvents), custEvents);
}


//-----------------------------------------------
//
//	RegisterCustomControl
//
//	Custom controls are the only kind that receive
//	mouse tracking, so we include that here.
//
//	A better solution will be to use HIView-based
//	controls and use their Construct event to create
//	the control instances as they are loaded into
//	the interface.
//
void TCustomControl::RegisterCustomControl(CFStringRef classID, ToolboxObjectClassRef *customDef, EventHandlerUPP *customHandler) {
	const EventTypeSpec	trackEvents[] = {
//		{ kEventClassControl, kEventControlInitialize },
		{ kEventClassControl, kEventControlTrack }
	};

	*customHandler = NewEventHandlerUPP(TCustomControl::CustomHandler);

	RegisterToolboxObjectClass(
		classID,
		NULL,
		GetEventTypeCount(trackEvents),
		trackEvents,
		*customHandler,
		NULL,									// pass "&ThisClass::Construct" instead of null.
		customDef
	);
}


pascal OSStatus
TCustomControl::CustomHandler(EventHandlerCallRef inCallRef, EventRef inEvent, void* nullData) {
	ControlPartCode		part;
	MouseTrackingResult trackingResult;
	Rect				bounds;
	Boolean				isInRegion, wasInRegion;
	ControlRef			theControl;
	Point				dragPoint;
	TCarbonEvent		event(inEvent);

	(void) event.GetParameter(kEventParamDirectObject, &theControl);
	TCustomControl  *controlInstance = (TCustomControl*)GetControlReference(theControl);

	switch ( event.GetClass() ) {
		case kEventClassControl:
			switch ( event.GetKind() ) {
//				case kEventControlInitialize:
//					break;

				case kEventControlTrack:

					(void) event.GetParameter( kEventParamMouseLocation, &dragPoint );

					trackingResult = kMouseTrackingMouseDown;
					isInRegion = true;
					wasInRegion = false;

					while (true) {
						if (trackingResult == kMouseTrackingMouseDragged && isInRegion != wasInRegion)
							trackingResult = isInRegion ? kMouseTrackingMouseEntered : kMouseTrackingMouseExited;

						wasInRegion = isInRegion;
						controlInstance->GetLocalPoint(&dragPoint);

						switch (trackingResult) {
							case kMouseTrackingMouseDown:
							case kMouseTrackingMouseUp:
							case kMouseTrackingMouseEntered:
							case kMouseTrackingMouseExited:
							case kMouseTrackingMouseDragged:
							case kMouseTrackingMouseMoved:
								controlInstance->Track(trackingResult, dragPoint);
								break;

							default:
								break;
						}

						if (trackingResult == kMouseTrackingMouseUp)
							break;

						// TrackMouseLocation gets drag events but not enter/exit
						// So we do our own enter/exit testing and fake the events
						controlInstance->FocusWindow();
						TrackMouseLocation(NULL, &dragPoint, &trackingResult);
						GetControlBounds(theControl, &bounds);
						isInRegion = PtInRect(dragPoint, &bounds);
					}

					// Set the control part on exit
					part = isInRegion ? kMyPartCode : kControlNoPart;
					event.SetParameter(kEventParamControlPart, part);
					break;
			}
			break;
	}

	return noErr;
}


//-----------------------------------------------
//
//  CountClicks
//
//	Counts the number of clicks close enough
//	together to be counted as a unit.
//
UInt16 TCustomControl::CountClicks(bool same, UInt16 max) {
	if (same && (TickCount() < lastClick + GetDblTime())) {
		if (++clickCount > max)
			clickCount = 1;
	}
	else
		clickCount = 1;

	lastClick = TickCount();

	return clickCount;
}

