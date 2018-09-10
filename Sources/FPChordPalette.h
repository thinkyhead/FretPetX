/*
 *  FPChordPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPCHORDPALETTE_H
#define FPCHORDPALETTE_H

#include "FPPalette.h"
#include "TControls.h"

#define kChordClosedHeight	66
#define kChordOpenHeight	216

//-----------------------------------------------
//
// FPTonesControl
//
class FPTonesControl : public TControl {
	public:
		FPTonesControl(WindowRef wind);
		bool	Draw(const Rect &bounds);
};


#pragma mark -
//-----------------------------------------------
//
// FPChordNameControl
//
class FPChordNameControl : public TControl {
	public:
		FPChordNameControl(WindowRef wind);
		bool	Draw(const Rect &bounds);
};


#pragma mark -
//-----------------------------------------------
//
// FPMoreNamesControl
//
class FPMoreNamesControl : public TCustomControl {
	public:
							FPMoreNamesControl(WindowRef wind);
		bool				Draw(const Rect &bounds);
		void				DrawMoreLine(UInt16 line, bool hilite=false, CGContextRef inContext=NULL);
		ControlPartCode		HitTest( Point where );
		bool				Track(MouseTrackingResult eventType, Point where);
		SInt16				GetLineFromPoint(Point where);
};


#pragma mark -
//-----------------------------------------------
//
// TTriangleControl
//
class TTriangleControl : public TTriangle {
	private:
		ThemeEraseUPP	eraseUPP;

	public:
						TTriangleControl(WindowRef wind);
		static void		EraseCallback(const Rect *bounds, UInt32 eraseData, SInt16 depth, Boolean isColorDev);
};


#pragma mark -
//-----------------------------------------------
//
// FPChordPalette
//
class FPChordPalette : public FPPalette {
	protected:
		FPTonesControl		*tonesControl;
		FPChordNameControl	*nameControl;
		FPMoreNamesControl	*moreControl;
		TTriangleControl	*triangleControl;

	public:
		TPictureControl		*lockButton;

		FPChordPalette();
		~FPChordPalette();

		void		Draw();
		void		Reset();
		OSStatus	HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );
		void		UpdateNow();
		void		UpdateMoreNow()			{ moreControl->DrawNow(); }
		void		UpdateExtendedView();
		const CFStringRef PreferenceKey()	{ return CFSTR("chord"); }
		void		SavePreferences();
		void		UpdateLockButton();
};

enum {
	kPictNewLock1 = 1060,
	kPictNewLock2, kPictNewLock3, kPictNewLock4
};

#endif

