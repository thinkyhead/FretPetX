/*
 *  FPGuitarPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPGUITARPALETTE_H
#define FPGUITARPALETTE_H

#include "FPGuitarArray.h"
#include "FPGuitar.h"
#include "FPMacros.h"
#include "FPPaletteGL.h"
#include "FPTuningInfo.h"

#include "FPSprite.h"

#include "TControls.h"
#include "TString.h"

enum {
	kDotStyleDots,
	kDotStyleLetters,
	kDotStyleNumbers
};


class TCarbonEvent;
class FPChord;
class FPHistoryEvent;

typedef struct GuitarMetrics {
	TString	titleFont, labelFont;
	UInt16	titleFontSize, labelFontSize;
	UInt8	titleFontStyle, labelFontStyle;
	bool	usingPegs;
	CGPoint	labelPegPosition[NUM_STRINGS];
	int		bracketTopHeight, bracketBottomHeight,
			bracketMiddleHeight, bracketStretchHeight,
			bracketWidth, bracketPosX,
			bracketTopOffset, bracketBottomOffset,
			numberOfStrings, numberOfFrets,
			pixelsWide, pixelsHigh;
	float	stringSpacing,
			*stringx, stringy[MAX_FRETS+2],
			frettop[MAX_FRETS+2], *fretbottom,
			titleMargin;
	RGBColor titleColor, labelColor;
} GuitarMetrics;

#pragma mark -
//-----------------------------------------------
//
//	FPBracketControl
//
class FPBracketControl : public TCustomControl {
private:
	const GuitarMetrics	&metrics;
	FPHistoryEvent		*bracketEvent;
	int					dragPart;
	int					origLow, origHigh;

public:
	FPBracketControl(WindowRef wind, int controlID, const GuitarMetrics &inMetrics);

	~FPBracketControl() {}

	bool			Draw(const Rect &bounds);	/* { return true; } */
	ControlPartCode	HitTest(Point where);
	bool			Track(MouseTrackingResult eventType, Point where);
	int				GetFretFromPosition(int y, int middleAdjustment=0);
	int				OrientedPosition(Point where);
	void			FinishDrag();
};


#pragma mark -
//-----------------------------------------------
//
//	FPTuningControl
//
class FPTuningControl : public TCustomControl {
private:
	MenuRef	tuningMenu;

public:
	FPTuningControl(WindowRef wind);
	void	ContextClick();
};


#pragma mark -
//-----------------------------------------------
//
//	FPGuitarControl
//
class FPGuitarControl : public TCustomControl {
private:
	EventLoopTimerUPP	overrideUPP;
	EventLoopTimerRef	overrideLoop;
	const GuitarMetrics	&metrics;

public:
	FPGuitarControl(WindowRef wind, const GuitarMetrics &inMetrics);

	~FPGuitarControl();

	bool			Draw(const Rect &bounds) { return true; }
	bool			Track(MouseTrackingResult eventType, Point where);
	void			GetPositionFromPoint(Point where, SInt16 *string, SInt16 *fret);

	static void		OverrideTimerProc( EventLoopTimerRef timer, void *ignoreme );
};


#pragma mark -
//-----------------------------------------------
//
// FPGuitarPalette
//
class FPGuitarPalette : public FPPaletteGL {
private:
	FPGuitarArray		*guitarArray;
	FPGuitar			*currentGuitar;
	FPGuitar			*defaultGuitar;

	FPGuitarControl		*guitarControl;				// The fretboard custom control
	FPBracketControl	*bracketControl;			// The fret bracket control
	FPTuningControl		*tuneNameControl;			// The tuning name
	Rect				guitarBounds;
	Rect				bracketBounds;
	Rect				tuneNameBounds;

	FPSpritePtr			backgroundSprite,			// A convenient way to store backgrounds
						cursorSprite,				// The fret cursor
						bracketTopSprite,			// Bracket parts
						bracketMiddleSprite,
						bracketStretchSprite1,
						bracketStretchSprite2,
						bracketBottomSprite,
						masterTwinkle,
						tuningInfoSprite,
						dotSprite[NUM_STRINGS][MAX_FRETS],
						numberDotSprite[NUM_STRINGS][MAX_FRETS];

	CGContextRef		tuningInfoContext;

	UInt16				dotColor[OCTAVE];			//!< Dot colors based on chord
	UInt16				fingeredChord;				//!< Fingered chord may be different
	bool				redoDots;					//!< Flag to recalculate the fingering

	FPTuningInfo		currentTuning;				//!< Current tuning object

	bool				showRedDots;				//!< Flag to show the outer dots
	UInt16				dotStyle;					//!< Type of dots to show
	bool				isHorizontal;				//!< Is the window sideways?
	bool				reverseStrung;				//!< The strings are reversed?
	bool				isLefty;					//!< The guitar is aligned for lefties
	UInt16				currentTone;				//!< The tone

	UInt16				currentString;				//!< The string is the same as X sometimes
	UInt16				currentFret;				//!< Fret 0 is the open string
	char*				currentImageName;			//!< The name of the current image, derived from the filename
	bool				tuningViewName;				//!< Viewing tuning by name

	UInt16				initWidth, initHeight;		//!< Starting size for flipping
	UInt16				initRight, initBottom;		//!< Starting edges for flipping

	UInt32				bracketDragType;
	UInt16				bracketTopBase,
						bracketBottomBase;
	int					bracketDragStart,
						bracketDragEnd,
						bracketDragActiveTop,
						bracketDragActiveBottom;

	float				grip_at_fret_0,
						grip_at_fret_1,
						grip_at_fret_max,
						top_mult_special,
						bot_mult_special,
						top_mult_normal,
						bot_mult_normal;

public:
	GuitarMetrics		metrics;
	
	FPGuitarPalette();
	~FPGuitarPalette() { if (metrics.stringx) delete metrics.stringx; }

	void					Init();
	void					InitSprites();
	void					LoadCursorSprite();
	void					LoadDotSprites();
	void					LoadBracketSprites();
	void					LoadEffectSprites();
	void					PositionDotSprites();
	void					DotPosition(UInt16 string, UInt16 fret, int *x, int *y);
	void					PositionControls();
	void					DisposeSpriteAssets();

	// Accessors
	inline UInt16			InitHeight() const						{ return initHeight; }
	inline UInt16			InitWidth() const						{ return initWidth; }

	inline UInt16			CurrentString() const					{ return currentString; }
	inline UInt16			CurrentFret() const						{ return currentFret; }
	inline FPTuningInfo&	CurrentTuning()							{ return currentTuning; }
	inline SInt16			OpenTone(UInt16 string) const			{ return currentTuning.tone[string]; }
	inline SInt16			Tone(UInt16 string, UInt16 fret) const	{ return OpenTone(string) + fret; }
	inline SInt16			CurrentTone() const						{ return Tone(currentString, currentFret); }

	inline bool				IsHorizontal() const					{ return isHorizontal; }
	inline bool				IsLeftHanded() const					{ return isLefty; }
	inline bool				IsReverseStrung() const					{ return reverseStrung; }
	inline bool				ShowingUnusedTones() const				{ return showRedDots; }

	inline UInt16			DotStyle() const						{ return dotStyle; }
	inline bool				ShowingDottedDots() const				{ return dotStyle == kDotStyleDots; }
	inline bool				ShowingLetterDots() const				{ return dotStyle == kDotStyleLetters; }
	inline bool				ShowingNumberedDots() const				{ return dotStyle == kDotStyleNumbers; }

	// Metrics from the Guitar
	inline UInt16			NumberOfFrets() const					{ return currentGuitar->NumberOfFrets(); }

	// Tuning stuff
	void					SetTuning(const FPTuningInfo &t, bool skipRedraw=false);
	void					ToggleTuningName();
	void					UpdateTuningInfo();
	void					UpdateTuningOrientation();

	// Setters
	void					MoveFretCursor(UInt16 string, UInt16 fret);

	void					ToggleHorizontal();
	void					ToggleLefty();
	void					ToggleReverseStrung();
	void					ToggleUnusedTones();
	void					ToggleFancyDots();
	void					SetDotStyle(UInt16 style);

	// Guitar images
	void					UpdateGuitarImageMenu();
	bool					LoadGuitar(CFStringRef shortName);
	bool					LoadGuitar(SInt16 index);
	void					UpdateMetrics();
	void					UpdateBackground();
	inline SInt16			SelectedGuitarImageMenuIndex()	{ return guitarArray->SelectedGuitarMenuIndex(); }
	inline SInt16			GuitarIndexForMenuIndex(MenuItemIndex i) { return guitarArray->GuitarIndexForMenuIndex(i); }

	// Display Elements
	inline void				UpdateCursor()					{ MoveFretCursor(currentString, currentFret); }
	void					UpdateBracket();
	void					DrawBracket(UInt16 ytop, UInt16 ybot);
	void					GetBracketCapInfo(int &caph1, int &caph2, int &capo1, int &capo2);

	void					FinishBracketDrag()				{ bracketControl->FinishDrag(); }
	void					BracketDragStart(UInt32 type, int y);
	void					BracketDragStop();
	void					DragBracket(SInt16 y);
	int						DragTop()						{ return bracketDragActiveTop; }
	int						DragBottom()					{ return bracketDragActiveBottom; }

	bool					MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);

	void					UpdateDotColors();
	UInt16					DotColor(int tone) const		{ return dotColor[tone % OCTAVE]; }
	void					ArrangeDots();
	void					NewFingering(FPChord &chord);
	bool					OverrideFingering(FPChord &chord);
	void					TwinkleTone(SInt16 string, SInt16 fret);

	void					ShowRedDots(bool yes=true)		{ showRedDots = yes; }
	void					DropFingering()					{ redoDots = true; }

	void					SavePreferences();
	void					Reset();

protected:
	OSStatus				HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );
	bool					Zoom();

	const CFStringRef		PreferenceKey()					{ return CFSTR("guitar"); }
};


extern FPGuitarPalette *guitarPalette;


enum {
	kCicnFretCursor	= 200,
	kCursorFrames	= 4
};

enum {
	kDotSize		= 11,
	kCursorSize		= 13,
	kTwinkleSize	= 19,

	kDotColorWhite	= 0,
	kDotColorYellow	,
	kDotColorGray	,
	kDotColorRed	,
	kDotColorCount	,
	kDotFrames		= 9,

	kDotIndexX			= 8,
	kDotNumCount		= 24,
	kDotLetterCount		= 24,
	kDotIndexNumWhite	= 12,
	kDotIndexNumYellow	= kDotIndexNumWhite + kDotNumCount * 1,
	kDotIndexNumGray	= kDotIndexNumWhite + kDotNumCount * 2,
	kDotIndexNumRed		= kDotIndexNumWhite + kDotNumCount * 3,
	kDotIndexToneWhite	= kDotIndexNumWhite + kDotNumCount * 4,
	kDotIndexToneYellow	= kDotIndexToneWhite + kDotLetterCount * 1,
	kDotIndexToneGray	= kDotIndexToneWhite + kDotLetterCount * 2,
	kDotIndexToneRed	= kDotIndexToneWhite + kDotLetterCount * 3,
	kDotIndexSquare		= kDotIndexToneWhite + kDotLetterCount * 4
};

#endif
