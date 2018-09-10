/*
 *  FPCirclePalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPCIRCLEPALETTE_H
#define FPCIRCLEPALETTE_H

#include "TSpriteControl.h"
#include "FPSprite.h"
#include "FPPaletteGL.h"


//-----------------------------------------------
//
// FPCircleControl
//
class FPCircleControl : public TCustomControl {
	public:
		FPCircleControl(WindowRef wind);

		ControlPartCode	HitTest(Point where);
		bool	Track(MouseTrackingResult eventType, Point where);
};


#pragma mark -
//-----------------------------------------------
//
// FPHarmonyControl
//
class FPHarmonyControl : public TCustomControl {
	public:
		FPHarmonyControl(WindowRef wind);

		bool	Track(MouseTrackingResult eventType, Point where);
};


#pragma mark -
//-----------------------------------------------
//
// FPCirclePalette
//
class FPCirclePalette : public FPPaletteGL {
	friend class FPCircleControl;

	private:
		UInt16			circleTop;
		bool			spinLock;
		bool			visualPlay;

	protected:
		FPCircleControl		*circleControl;
		FPHarmonyControl	*harmonyControl;
		TSpriteControl		*eyeControl, *holdControl, *leftControl, *rightControl, *upControl, *downControl, *majMinControl;
		TTrackingRegion		*circleRollover;

		CGContextRef		circleContext, functionContext;
		FPSpritePtr			circleSprite, functionSprite;
		FPSpritePtr			backSprite, overlaySprite, eyeSprite, holdSprite;
		FPSpritePtr			masterBlob, masterWedge;
		FPSpritePtr			blob[NUM_STEPS];
		FPSpritePtr			harmLeftSprite, harmRightSprite;	// harmony arrows
		FPSpritePtr			transUpSprite, transDownSprite;		// transpose arrows
		FPSpritePtr			majorMinorSprite;					// major/minor gadget

	public:
		FPCirclePalette();
		~FPCirclePalette();

		void		InitSprites();
		void		UpdateCircle();
		void		UpdateHarmonyDots();
		void		ToggleEye()				{ visualPlay = eyeControl->Toggle(); }
		void		ToggleHold()			{ spinLock = holdControl->Toggle(); }
		inline void	RotateCircle(SInt16 d)	{ if (spinLock) { circleTop = NOTEMOD(circleTop + d); UpdateCircle(); } }
		const CFStringRef PreferenceKey()	{ return CFSTR("circle"); }

		void		HandleTrackingRegion(TTrackingRegion *region, TCarbonEvent &event);

		void		ProcessAndAnimateSprites();	// so we can reduce animation frequency
		void		TwinkleTone(UInt16 tone);
//		static pascal void TwinkleSpriteAnimProc(SpritePtr pSprite, FramePtr pFrame, long *pIndex);

};


extern FPCirclePalette *circlePalette;


#endif
