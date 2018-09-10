/*
 *  FPAboutBox.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPABOUTBOX_H
#define FPABOUTBOX_H

#include "TCustomWindow.h"

class FPAboutBox : public TCustomWindow {
	private:
		CGImageRef	aboutImage;

	public:
		static bool boxIsActive;

		FPAboutBox();
		~FPAboutBox();

		void		Draw();
		OSStatus	HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );

//		static pascal OSStatus	CustomHandler(EventHandlerCallRef handler, EventRef inEvent, void* userData);
		static void				RegisterCustomClass(ToolboxObjectClassRef *customWindow, EventHandlerUPP *customHandler);
		void					Deactivated()	{ QuitModal(); }
};

enum {
	kPictSplash		= 128,
	kPictSplashMask
};

#endif
