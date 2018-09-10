/*!
	@file TCustomWindow.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "TCustomWindow.h"
#include "TCarbonEvent.h"

TCustomWindow::TCustomWindow( WindowDefSpec *inSpec, WindowClass inClass, WindowAttributes inAttributes, const Rect& inBounds ) {
//	fprintf(stderr, "TCustomWindow::TCustomWindow(WindowClass, WindowAttributes, Rect&) = %08X\n", this);

	WindowRef	theWindow;

	(void) CreateCustomWindow(
			inSpec,
			inClass,
			inAttributes,
			&inBounds,
			&theWindow);

	InitWithPlatformWindow(theWindow);
}

void TCustomWindow::RegisterCustomClass(CFStringRef signature, WindowDefSpec *windowSpec, const EventTypeSpec *eventSpec, ItemCount eventCount, ToolboxObjectClassRef *customRef, EventHandlerUPP *customHandler) {
	*customHandler = NewEventHandlerUPP(TCustomWindow::CustomHandler);

	RegisterToolboxObjectClass(
		signature, 
		NULL,
		eventCount,
		eventSpec,
		*customHandler,
		NULL,
		customRef);

	windowSpec->defType = kWindowDefObjectClass;
	windowSpec->u.classRef = *customRef;
}

pascal OSStatus TCustomWindow::CustomHandler(EventHandlerCallRef inRef, EventRef inEvent, void* userData) {
	WindowRef		window;
	TCarbonEvent	event(inEvent);

	event.GetParameter(kEventParamDirectObject, &window);
	TCustomWindow  *object = (TCustomWindow*)TWindow::GetTWindow(window);
	return object ? object->HandleEvent( inRef, event ) : eventNotHandledErr;
}

