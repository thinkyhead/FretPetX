/*!
	@file TCustomWindow.h

	A custom window can receive extra events

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef TCUSTOMWINDOW_H
#define TCUSTOMWINDOW_H

#include "TWindow.h"

class TCustomWindow : public TWindow {
	private:

	public:
				TCustomWindow( WindowDefSpec *inSpec, WindowClass inClass, WindowAttributes inAttributes, const Rect& bounds );
		virtual ~TCustomWindow() {}

	static void RegisterCustomClass(CFStringRef signature, WindowDefSpec *windowSpec, const EventTypeSpec *eventSpec, ItemCount eventCount, ToolboxObjectClassRef *customRef, EventHandlerUPP *customHandler);
	static pascal OSStatus CustomHandler(EventHandlerCallRef inRef, EventRef inEvent, void* userData);
};

#endif
