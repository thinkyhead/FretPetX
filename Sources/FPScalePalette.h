/*
 *  FPScalePalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPSCALEPALETTE_H
#define FPSCALEPALETTE_H

class TString;

#include "FPPalette.h"
#include "TControls.h"
#include "FPChord.h"

#include "FPApplication.h"

enum {
	// Gadget Coordinates
	FUNCX		=	5,							// function header
	FUNCY		=	48,
	FUNCH		=	32,
	FUNCV		=	13,

};

//
// BoxState
//
typedef struct {
	Rect		rect;
	RGBColor	*boxColor, *textColor, *frameColor, halfColor;
	bool		isRound, drawFrame;
	short		tx, ty;
	UInt16		key, note;
	int			mod;
	bool		force;
	const char	*text;
} BoxState;


//
// Statistics for the scale steps in every key
// Generated when the scale is set and used to
// optimize scale drawing slightly.
//
typedef struct {
	UInt16		toneOffset;
	UInt16		triadType;
	RGBColor	*darkColor;
	RGBColor	*lightColor;
	RGBColor	*darkTextColor;
} FPScaleStep;


#pragma mark -
//-----------------------------------------------
//
//	FPScaleControl
//
class FPScaleControl : public TCustomControl {
	public:
		FPScaleControl(WindowRef wind);
		~FPScaleControl();

		bool			Draw(const Rect &bounds);
		ControlPartCode	HitTest( Point where );
		bool			Track(MouseTrackingResult eventType, Point where);
		void			GetBoxFromPoint(Point where, int *vert, int *horz);
};

#pragma mark -
//-----------------------------------------------
//
//	FPScaleNameControl
//
class FPScaleNameControl : public TControl {
	public:
		FPScaleNameControl(WindowRef wind);
		~FPScaleNameControl() {}

		bool	Draw(const Rect &bounds);
};

#pragma mark -
//-----------------------------------------------
//
//	FPScaleInfo
//
class FPScaleInfo {
	private:
		UInt16			enharmonic;									//!< current flat-sharp naming
		SInt16			scaleMask[NUM_SCALES][OCTAVE];				//!< Bitmasks for ALL Scales in all keys
		SInt16			scale[NUM_SCALES][OCTAVE][16];				//!< All tones in the current mode
		char const		*noteName[NUM_SCALES][OCTAVE][OCTAVE];		//!< All scale note names in the current mode
		UInt16			noteMarksIndex[NUM_SCALES][OCTAVE][OCTAVE];	//!< values for deriving note names
		UInt16			noteNameIndex[NUM_SCALES][OCTAVE][OCTAVE];	//!< values for deriving note names
		FPScaleStep		stepInfo[NUM_SCALES][NUM_STEPS];
		TString			*romanFunc[NUM_SCALES][NUM_STEPS];			//!< Names like I, IIm, VIIo, etc.
		static CFStringRef scaleNames[NUM_SCALES];

	public:
		FPScaleInfo();
		~FPScaleInfo() {}

		void			Init();
		void			InitToneNames();

		inline const char*	ToneName(UInt16 mode, UInt16 key, UInt16 tone, SInt16 mod)
						{ return UNoteS[noteMarksIndex[mode][key][tone] + mod][noteNameIndex[mode][key][tone]]; }

		inline SInt16*	Scale(UInt16 mode, UInt16 key)							{ return scale[mode][key]; }
		inline SInt16	ScaleTone(UInt16 mode, UInt16 key, UInt16 step)			{ return scale[mode][key][step]; }

		inline SInt16	FunctionOfTone(UInt16 mode, UInt16 key, SInt16 tone)	{ int step = NUM_STEPS; while (step-- && ScaleTone(mode, key, step) != NOTEMOD(tone)); return step; }
		inline const char*	NameOfNote(UInt16 mode, UInt16 key, UInt16 tone)		{ return noteName[mode][key][tone]; }
		inline const char*	NameOfKey(UInt16 key)									{ return noteName[0][key][key]; }

		inline CFStringRef ScaleName(UInt16 mode)								{ return scaleNames[mode]; }
		inline UInt16	ScaleType(UInt16 mode)									{ return GetTriadType(mode, 0); }

		inline bool		ScaleHasTones(UInt16 mode, UInt16 key, UInt16 mask)		{ return ((MaskForMode(mode, key) & mask) == mask); }
		inline bool		ScaleHasTone(UInt16 mode, UInt16 key, UInt16 tone)		{ return ((MaskForMode(mode, key) & BIT(NOTEMOD(tone))) != 0); }

		UInt16			GetTriad(UInt16 mode, UInt16 key, UInt16 step, SInt16 mod);
		UInt16			GetTriadType(UInt16 mode, UInt16 step);

		inline UInt16	GetBaseTriad(UInt16 mode, UInt16 key, UInt16 step)		{ return GetTriad(mode, key, step, -ScaleTone(mode, key, step)); }
		inline UInt16	GetBaseTriad(UInt16 mode, UInt16 step)					{ return GetBaseTriad(mode, 0, step); }
		inline UInt16	MaskForMode(UInt16 mode, UInt16 key)					{ return scaleMask[mode][key]; }

		FPScaleStep&	StepInfo(UInt16 mode, UInt16 key)						{ return stepInfo[mode][key]; }

		UInt16			Enharmonic()											{ return enharmonic; }
		void			SetEnharmonic(UInt16 e)									{ enharmonic = e; }
};


#pragma mark -
//-----------------------------------------------
//
//	FPScalePalette
//
class FPScalePalette : public FPPalette {
	private:
		SInt16			currMode;							// the current scale
		SInt16			bulbFlag;							// illumination
		SInt16			noteModifier;						// the offset of the scale tone

		BoxState		boxState[OCTAVE][NUM_STEPS+1];		// per scale and function (including left heading)
		bool			redrawScale;						// flag to refresh the scale, background and all

		SInt16			currFunction;						// the function at the cursor (0-7)
		SInt16			currKey;							// the key at the cursor (0-11)
		SInt16			scaleBoxX,
						scaleBoxY;

//		SInt16			keyAlter;							// current key's modifier (#/b) ... noteS[keyAlter][tone]

		FPScaleInfo		info;
		char			romanSteps[NUM_STEPS][10];			// the name of all 7 steps

	public:
		TPictureControl		*backButton, *fwdButton;
		TPictureControl		*flatButton, *naturalButton, *sharpButton;
		TPictureControl		*bulbButton;

		FPScaleControl		*scaleControl;
		FPScaleNameControl	*nameControl;

						FPScalePalette();
						~FPScalePalette();

		void			Draw();

		inline UInt16	CurrentMode()		{ return currMode; }
		inline UInt16	CurrentKey()		{ return currKey; }
		inline UInt16	CurrentFunction()	{ return currFunction; }
		inline SInt16	NoteModifier()		{ return noteModifier; }
		inline UInt16	Enharmonic()		{ return info.Enharmonic(); }

		inline const char*	BoxToneName(UInt16 key, UInt16 tone, SInt16 mod) { return info.ToneName(CurrentMode(), key, tone, mod); }

		inline const char*	BoxToneName() {
				BoxState box = boxState[INDEX_FOR_KEY(currKey)][currFunction];
				return BoxToneName(box.key, box.note, box.mod);
			}

		// Scale Tone Accessors
		inline UInt16	ScaleTone(UInt16 mode, UInt16 key, UInt16 step)	{ return info.ScaleTone(mode, key, step); }
		inline UInt16	ScaleTone(UInt16 key, UInt16 step)				{ return info.ScaleTone(CurrentMode(), key, step); }
		inline UInt16	ScaleTone(UInt16 step)							{ return info.ScaleTone(CurrentMode(), CurrentKey(), step); }
		inline UInt16	FunctionTone()									{ return info.ScaleTone(CurrentMode(), CurrentKey(), CurrentFunction()); }
		inline UInt16	CurrentTone()									{ return NOTEMOD(FunctionTone() + NoteModifier()); }

		// Convert Tones to Scale Functions
		inline SInt16	FunctionOfTone(SInt16 tone)						{ int step = NUM_STEPS; while (step-- && ScaleTone(step) != NOTEMOD(tone)); return step; }
		inline SInt16	FunctionOfTone(UInt16 key, SInt16 tone)			{ int step = NUM_STEPS; while (step-- && ScaleTone(key, step) != NOTEMOD(tone)); return step; }

		// Tone Name Accessors
		inline const char*	NameOfNote(UInt16 mode, UInt16 key, UInt16 tone){ return info.NameOfNote(mode, key, tone); }
		inline const char*	NameOfNote(UInt16 key, UInt16 tone)				{ return info.NameOfNote(currMode, key, tone); }
		inline const char*	NameOfNote(UInt16 tone)							{ return info.NameOfNote(currMode, currKey, tone); }
		inline const char*	NameOfCurrentNote()								{ return info.NameOfNote(currMode, currKey, CurrentTone()); }
		inline const char*	NameOfKey(UInt16 key)							{ return info.NameOfKey(key); }

		inline char*	RomanFunctionName(UInt16 step)					{ return romanSteps[step]; }

		void			SetScale(SInt16 newMode);
		inline void		SetEnharmonic(UInt16 enh)						{ info.SetEnharmonic(enh); info.InitToneNames(); }

		inline void		SetScaleForward()								{ SetScale(CurrentMode()+1); }
		inline void		SetScaleBack()									{ SetScale(CurrentMode()-1); }

		void			SetBoxDirty(SInt16 key, SInt16 col)				{ boxState[key][col].force = true; }
		void			InitBoxesForKeyOrder();
		void			UpdateBoxState(SInt16 keyindex, SInt16 func);
		void			DrawScaleBox(SInt16 keyindex, SInt16 func);
		void			DrawScaleBoxes();
		void			DrawChanges();
		void			DrawScaleHeading();
		inline void		UpdateScaleControl()							{ scaleControl->DrawNow(); }
		inline void		UpdateNameControl()								{ nameControl->DrawNow(); }

		void			SetCursorWithChord(FPChord &chord);
		void			MoveCursorToNote(UInt16 note, SInt16 newkey=-1);
		void			MoveScaleCursor(SInt16 nh, SInt16 nv);
		void			SetNoteModifier(SInt16 nm);

		//
		// Scale Illumination
		//
		void			SetIllumination(bool onoff);
		inline bool		IsIlluminated()										{ return bulbFlag; }
		inline void		ToggleIllumination()								{ SetIllumination(!bulbFlag); }

		//
		// Scale Info Accessors
		//
		inline CFStringRef ScaleName()										{ return info.ScaleName(CurrentMode()); }
		inline UInt16	ScaleType()											{ return info.GetTriadType(CurrentMode(), 0); }

		void			PlayCurrentTone();
		inline SInt16*	CurrentScale()										{ return info.Scale(CurrentMode(), CurrentKey()); }
		inline bool		KeyHasTones(UInt16 key, UInt16 mask)				{ return info.ScaleHasTones(CurrentMode(), key, mask); }
		inline bool		ScaleHasTones(UInt16 mask)							{ return info.ScaleHasTones(CurrentMode(), CurrentKey(), mask); }
		inline bool		ScaleHasTone(UInt16 key, UInt16 tone)				{ return info.ScaleHasTone(CurrentMode(), key, tone); }
		inline bool		ScaleHasTone(UInt16 tone)							{ return info.ScaleHasTone(CurrentMode(), CurrentKey(), tone); }

//		char*			RomanForTone(UInt16 tone, UInt16 key);

		UInt16			GetTriad(UInt16 mode, UInt16 key, UInt16 step, SInt16 mod)	{ return info.GetTriad(mode, key, step, mod); }
		inline UInt16	GetTriad(UInt16 key, UInt16 step, SInt16 mod)		{ return info.GetTriad(CurrentMode(), key, step, mod); }
		inline UInt16	GetTriad(UInt16 step, SInt16 mod=0)					{ return info.GetTriad(CurrentMode(), CurrentKey(), step, mod); }
		inline UInt16	GetTriad()											{ return info.GetTriad(CurrentMode(), CurrentKey(), CurrentFunction(), 0); }
		UInt16			CurrentTriad(bool noMod=false)						{ return info.GetTriad(CurrentMode(), CurrentKey(), CurrentFunction(), noMod ? 0 : NoteModifier()); }

		inline UInt16	GetBaseTriad(UInt16 mode, UInt16 key, UInt16 step)	{ return info.GetTriad(mode, key, step, -ScaleTone(mode, key, step)); }
		inline UInt16	GetBaseTriad(UInt16 mode, UInt16 step)				{ return info.GetBaseTriad(mode, 0, step); }
		inline UInt16	GetBaseTriad(UInt16 step)							{ return info.GetBaseTriad(CurrentMode(), 0, step); }
		inline UInt16	GetBaseTriad()										{ return info.GetBaseTriad(CurrentMode(), 0, CurrentFunction()); }

		UInt16			GetTriadType(UInt16 mode, UInt16 step)				{ return info.GetTriadType(mode, step); }
		inline UInt16	GetTriadType(UInt16 step)							{ return info.GetTriadType(CurrentMode(), step); }
		inline UInt16	GetTriadType()										{ return info.GetTriadType(CurrentMode(), CurrentFunction()); }

		inline UInt16	MaskForMode(UInt16 mode, UInt16 key)				{ return info.MaskForMode(mode, key); }
		inline UInt16	MaskForKey(UInt16 key)								{ return info.MaskForMode(CurrentMode(), key); }
		inline UInt16	CurrentScaleMask()									{ return info.MaskForMode(CurrentMode(), CurrentKey()); }

		inline RGBColor*	LightColor(UInt16 step)							{ return info.StepInfo(CurrentMode(), step).lightColor; }
		inline RGBColor*	DarkColor(UInt16 step)							{ return info.StepInfo(CurrentMode(), step).darkColor; }

		const CFStringRef PreferenceKey()	{ return CFSTR("scale"); }
};


extern FPScalePalette *scalePalette;


//
// Scale images
//
enum {
	kPictBack1 = 1100, kPictBack2,
	kPictFwd1, kPictFwd2,
	kPictFlat1, kPictFlat2, kPictFlat3,
	kPictNatural1, kPictNatural2, kPictNatural3,
	kPictSharp1, kPictSharp2, kPictSharp3,
	kPictBulb1, kPictBulb2, kPictBulb3, kPictBulb4
};

//
// Scales
//
enum {
	kModeIonian = 0,	kModeDorian,
	kModePhrygian,		kModeLydian,
	kModeMixolydian,	kModeAeolian,
	kModeLocrian,		kModeHarmonic,
	kModeMelodic,		kModeOriental
};

#endif
