/*!
	@file TTrackingRegion.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "TWindow.h"
#include "TCarbonEvent.h"
#include "TTrackingRegion.h"

SInt32				TTrackingRegion::nextID = 1;
TTrackingRegion*	TTrackingRegion::globalEnteredRegion = NULL;


TTrackingRegion::TTrackingRegion() {
	Init();
}


TTrackingRegion::TTrackingRegion(TWindow *inWindow, RgnHandle inRegion, SInt32 inID, OSType signature) {
	Init();
	Specify(inWindow, inRegion, inID, signature);
}


TTrackingRegion::TTrackingRegion(TWindow *inWindow, Rect &inRect, SInt32 inID, OSType signature) {
	Init();
	Specify(inWindow, inRect, inID, signature);
}


TTrackingRegion::~TTrackingRegion() {
	SimulateExitEvent();

	if (fTrackingRef) {
		if (fWindow)
			fWindow->RemoveTrackingRegion(this);

		(void)ReleaseMouseTrackingRegion(fTrackingRef);
	}

	if (fRegion)
		DisposeRgn(fRegion);
}


void TTrackingRegion::Init() {
	fWindow			= NULL;
	fTrackingID		= 0;
	fTrackingRef	= NULL;
	isEntered		= false;
	enabled			= true;
	active			= true;
	fRegion			= NULL;
}


void TTrackingRegion::Specify(TWindow *inWindow, RgnHandle inRegion, SInt32 inID, OSType signature) {
	fWindow		= inWindow;
	fTrackingID	= inID ? inID : nextID++;

	fRegion = NewRgn();
	MacCopyRgn(inRegion, fRegion);

	MouseTrackingRegionID regionID = { signature, fTrackingID };

	(void)CreateMouseTrackingRegion(
						inWindow->Window(),					// The window that retains this
						inRegion,							// The region
						NULL,								// No clip
						kMouseTrackingOptionsStandard,		// Local coordinates
						regionID,							// A synthetic tracking region id
						this,								// The refcon refers to this instance
						NULL,								// The event target is the window
						&fTrackingRef						// The tracking ref is stored
					);

	const EventTypeSpec	rgnEvents[] = {
			{ kEventClassMouse, kEventMouseEntered },
			{ kEventClassMouse, kEventMouseExited }
		};

	fWindow->RegisterForEvents(GetEventTypeCount(rgnEvents), rgnEvents);
}


void TTrackingRegion::Specify(TWindow *inWindow, Rect &inRect, SInt32 inID, OSType signature) {
	RgnHandle rgn = NewRgn();
	RectRgn(rgn, &inRect);
	Specify(inWindow, rgn, inID, signature);
	DisposeRgn(rgn);
}


void TTrackingRegion::Change(Rect &inRect) {
	RgnHandle rgn = NewRgn();
	RectRgn(rgn, &inRect);
	Change(rgn);
	DisposeRgn(rgn);
}


void TTrackingRegion::Change(RgnHandle inRegion) {
	(void)ChangeMouseTrackingRegion(fTrackingRef, inRegion, NULL);
	MacCopyRgn(inRegion, fRegion);
}


void TTrackingRegion::SimulateExitEvent() {
	if (IsEnabled() && WasEntered()) {
		TCarbonEvent event(kEventClassMouse, kEventMouseExited);
		event.SetParameter(kEventParamMouseTrackingRef, typeMouseTrackingRef, sizeof(MouseTrackingRef), &fTrackingRef);
		event.SetParameter(kEventParamWindowRef, typeWindowRef, sizeof(WindowRef), fWindow->Window());
		(void)event.SendToTarget(fWindow->EventTarget());
	}
}


void TTrackingRegion::SimulateEnterEvent() {
	if (IsEnabled() /* && !WasEntered() */ ) {
		TCarbonEvent event(kEventClassMouse, kEventMouseEntered);
		UInt32	mods = GetCurrentEventKeyModifiers();
		event.SetParameter(kEventParamKeyModifiers, mods);
		event.SetParameter(kEventParamWindowRef, typeWindowRef, sizeof(WindowRef), fWindow->Window());
		event.SetParameter(kEventParamMouseTrackingRef, typeMouseTrackingRef, sizeof(MouseTrackingRef), &fTrackingRef);
		(void)event.SendToTarget(fWindow->EventTarget());
	}
}


void TTrackingRegion::SimulateIdleEvent() {
	TCarbonEvent event(kEventClassMouse, kEventMouseEntered);
	event.SetParameter(kEventParamMouseTrackingRef, typeMouseTrackingRef, sizeof(MouseTrackingRef), &fTrackingRef);
	(void)event.SendToTarget(fWindow->EventTarget());
}


void TTrackingRegion::MarkEntered(bool b) {
	isEntered = b;
}


