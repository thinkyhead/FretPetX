/*
 *  FPPianoPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPPIANOPALETTE_H
#define FPPIANOPALETTE_H

#include "FPPaletteGL.h"
#include "FPGuitarPalette.h"

#define kPianoKeys	53
#define kPianoSizes	3

#pragma mark -
//-----------------------------------------------
//
//	FPPianoControl
//
class FPPianoControl : public TCustomControl {
	private:

	public:
		FPPianoControl(WindowRef wind);
		~FPPianoControl() {}

		bool			Draw(const Rect &bounds) { return true; }
		bool			Track(MouseTrackingResult eventType, Point where);
		UInt16			GetKeyFromPoint(Point where);
};


typedef struct {
	float		x, y;
	FPSpritePtr	sprite;
} FPPianoKey;

typedef struct {
	FPSpritePtr		whiteMaster, blackMaster, dotsMaster,
					dotSprite[OCTAVE];
	FPSpriteLayer	*whiteLayer, *blackLayer, *dotsLayer;
	FPPianoKey		key[kPianoKeys];
} FPPianoKeyData;

//-----------------------------------------------
//
//	FPPianoPalette
//
class FPPianoPalette : public FPPaletteGL {
	private:
		UInt16				keyboardSize;
		FPPianoKeyData		keyData[kPianoSizes];	
		FPPianoControl		*pianoControl;

	public:
		FPPianoPalette();
		~FPPianoPalette();

		void				InitSprites();
		bool				Zoom();
		void				ToggleKeyboardSize();
		void				SetKeyboardSize(UInt16 size);

		void				UpdatePianoKeys();
		void				PositionPianoDots();
		void				SetKeyPlaying(SInt16 key, bool isPlaying);

		inline UInt16		PianoSize() const { return keyboardSize; }

		void				SavePreferences();
		const CFStringRef	PreferenceKey()	{ return CFSTR("keyboard"); }

		void				ProcessAndAnimateSprites();
};


extern FPPianoPalette *pianoPalette;


#endif

