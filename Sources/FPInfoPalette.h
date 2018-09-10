/*
 *  FPInfoPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPINFOPALETTE_H
#define FPINFOPALETTE_H

#include "FPPalette.h"
#include "TControls.h"

//-----------------------------------------------
//
// FPInfoPalette
//
class FPInfoPalette : public FPPalette {
	private:
		TControl	*scaleTone1, *scaleTone2, *scaleTone3;
		TControl	*scaleFunc1, *scaleFunc2;
		TControl	*solfaName;
		TControl	*guitarTone1, *guitarTone2, *guitarTone3;
		TControl	*guitarFunc1, *guitarFunc2;
		TControl	*interval1, *interval2;

	public:
					FPInfoPalette();
					~FPInfoPalette();

		void				UpdateFields(bool doScale=true, bool doGuitar=true);
		const CFStringRef	PreferenceKey()	{ return CFSTR("info"); }
};


extern FPInfoPalette *infoPalette;


#endif
