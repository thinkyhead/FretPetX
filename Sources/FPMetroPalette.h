/*
 *  FPMetroPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPMETROPALETTE_H
#define FPMETROPALETTE_H

#include "FPPalette.h"
#include "TControls.h"

//
// FPMetroPalette
//
class FPMetroPalette : public FPPalette {
	public:
		FPMetroPalette();
		~FPMetroPalette() {}

		OSStatus	HandleEvent(EventHandlerCallRef inRef, TCarbonEvent &event);
		void		InitSprites();

		const CFStringRef PreferenceKey()	{ return CFSTR("metro"); }
};

#endif
