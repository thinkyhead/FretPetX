/*
 *  FPScalePalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPScalePalette.h"
#include "FPCirclePalette.h"
#include "FPApplication.h"
#include "FPHistory.h"
#include "FPMusicPlayer.h"
#include "FPUtilities.h"
#include "TControls.h"
#include "TString.h"
#include "TCarbonEvent.h"


FPScalePalette *scalePalette = NULL;

CFStringRef FPScaleInfo::scaleNames[] = {
	CFSTR("1 Ionian (Major)"),
	CFSTR("2 Dorian Minor"),
	CFSTR("3 Phrygian Minor"),
	CFSTR("4 Lydian Major"),
	CFSTR("5 Mixolydian Major"),
	CFSTR("6 Aeolian Minor"),
	CFSTR("7 Locrian Diminished"),
	CFSTR("Harmonic Minor"),
	CFSTR("Fixed Melodic Minor"),
	CFSTR("Oriental Scale")
};

FPScaleInfo::FPScaleInfo() {
	enharmonic		= 4;

	//
	// Init scaleMask
	//
	bzero(scaleMask, sizeof(scaleMask));
	for (int mode=0; mode<NUM_SCALES; mode++) {
		int offset = 0;
		for (int step=0; step<NUM_STEPS*2; step++) {
			// Every key gets the bit shifted over one more
			// offset will always be under 11 when 14-note scales go away
			if (offset < OCTAVE)
				for (int key=0; key<OCTAVE; key++)
					scaleMask[mode][key] |= BIT(NOTEMOD(offset + key));

			offset += interval[mode][step];
		}
	}

	//
	// Init the scale tone numbers
	//
	SInt16	tone;

	for (int mode=0; mode<NUM_SCALES; mode++) {
		for (int key=0; key<OCTAVE; key++) {
			tone = key;
			for (int step=0; step<NUM_STEPS; step++) {
				scale[mode][key][step]  = tone;
				ADD_WRAP(tone, interval[mode][step], OCTAVE);
			}
		}

	}

	//
	// Init the tone names
	//
	InitToneNames();

	//
	// Init stepInfo[mode][step] which will be used
	// for drawing boxes.
	//
	for (int mode=0; mode<NUM_SCALES; mode++) {
		UInt16	ttype;
		for (int step=0; step<=6; step++) {
			ttype = GetTriadType(mode, step);
			stepInfo[mode][step].toneOffset		= ScaleTone(mode, 0, step);
			stepInfo[mode][step].triadType		= ttype;

			switch(ttype) {
			case kTriadMajor:
				stepInfo[mode][step].lightColor = &rgbMajBack;
				stepInfo[mode][step].darkColor = &rgbMaj;
				break;

			case kTriadMinor:
				stepInfo[mode][step].lightColor = &rgbMinBack;
				stepInfo[mode][step].darkColor = &rgbMin;
				break;

			case kTriadDiminished:
				stepInfo[mode][step].lightColor = &rgbDimBack;
				stepInfo[mode][step].darkColor = &rgbDim;
				break;

			default:
				stepInfo[mode][step].lightColor = &rgbOthBack;
				stepInfo[mode][step].darkColor = &rgbOth;
			}

			stepInfo[mode][step].darkTextColor = stepInfo[mode][step].lightColor;
		}

	}
}


//-----------------------------------------------
//
// InitToneNames
//
// This initializes the names of all scale tones.
// Called during scale init and whenever the enharmonic
// changes.
//
// Fills these arrays:
//	scale[mode][key][step]					// The tone numbers of the scale	ScaleTone(mode, 0, step)
//	noteNameIndex[mode][key][tone]			// The letter name of a tone		BoxToneName(UInt16 key, UInt16 tone, SInt16 mod)
//	noteMarksIndex[mode][key][tone]			// The flats/sharps lookup			BoxToneName(UInt16 key, UInt16 tone, SInt16 mod)
//	noteName[mode][key][tone]				// The note's name in each mode/key	NameOfNote(UInt16 key, UInt16 tone)
//
// Requires these arrays:
//	letterIndex[sharp][key]
//	interval[mode][step]
//	theCScale[letter]
//	UNoteS[marks][letter]
//
void FPScaleInfo::InitToneNames() {
	SInt16	k;
	SInt16	tone, marks, letter;

	for (int mode=0; mode<NUM_SCALES; mode++) {
		for (int key=0; key<OCTAVE; key++) {
			letter = letterIndex[(FIFTHS_POSITION(key) > 11 - enharmonic) ? 1 : 0][key];

			tone = key;
			for (int step=0; step<NUM_STEPS; step++) {
				for (k=1; k<=interval[mode][step]; k++) {
					//
					//	Get the marks index (Cbbb Cbb Cb C C# C## C###)
					//
					marks = tone - theCScale[letter];		// distance between the tone and the ideal tone
					if (marks > 6)	marks -= 12;			// if over 6 subtract 12
					if (marks < -6)	marks += 12;			// if under -6 add 12
					marks += 3;								// add 3

					//
					//	Save stats so we can grab a note name later
					//
					noteNameIndex[mode][key][tone] = letter;
					noteMarksIndex[mode][key][tone] = marks;

					//
					//	And the string address too, which may be all we need
					//
					noteName[mode][key][tone] = UNoteS[marks][letter];

					//
					//	Get the next letter and tone
					//
					INC_WRAP(tone, OCTAVE);

					//
					//	The letter only increments once
					//
					if (k == 1)
						INC_WRAP(letter, NUM_STEPS);
				}
			}
		}
	}
}


UInt16 FPScaleInfo::GetTriadType(UInt16 mode, UInt16 step) {
	switch(GetBaseTriad(mode, step)) {
		case B_ROOT | B_MAJ3 | B_PER5:
			return kTriadMajor;

		case B_ROOT | B_MIN3 | B_PER5:
			return kTriadMinor;

		case B_ROOT | B_MIN3 | B_DIM5:
			return kTriadDiminished;

		default:
			return kTriadOther;
	}
}


UInt16 FPScaleInfo::GetTriad(UInt16 mode, UInt16 key, UInt16 step, SInt16 modifier) {
	return	BIT(NOTEMOD(ScaleTone(mode, key, step) + modifier)) |
			BIT(NOTEMOD(ScaleTone(mode, key, (step + 2) % NUM_STEPS) + modifier)) |
			BIT(NOTEMOD(ScaleTone(mode, key, (step + 4) % NUM_STEPS) + modifier));
}


#pragma mark -
FPScaleControl::FPScaleControl(WindowRef wind) : TCustomControl(wind, 'scal', 101) {
}


FPScaleControl::~FPScaleControl() {
}


bool FPScaleControl::Draw(const Rect &bounds) {
	FPScalePalette *thePal = (FPScalePalette*)GetTWindow();
	thePal->DrawScaleBoxes();
	return true;
}


ControlPartCode FPScaleControl::HitTest( Point where ) {
	int horz, vert;
	GetBoxFromPoint(where, &vert, &horz);

	if (horz || vert >= 0)
		return kControlIndicatorPart;
	else
		return kControlNoPart;
}


bool FPScaleControl::Track(MouseTrackingResult eventType, Point where) {
	int						horz, vert, newBox;
	static int				origVert, origHorz;
	static int				lastBox;
	static bool				keyDrag, funcDrag;
//	static FPChord			origChord;
	static UInt16			totalClicks;
	static FPHistoryEvent	*event;
	static bool				shouldMerge;
	static FPChord			oldChord;

	bool transforming = fretpet->IsTransformModeEnabled();

	FPScalePalette *thePal = (FPScalePalette*)GetTWindow();

	GetBoxFromPoint(where, &vert, &horz);

	if (eventType == kMouseTrackingMouseDown) {
		if ( (keyDrag = (horz == 0)) )
			origHorz = globalChord.RootScaleStep();	// thePal->CurrentFunction();

		if ( (funcDrag = (vert < 0)) )
			origVert = INDEX_FOR_KEY(transforming ? globalChord.Key() : thePal->CurrentKey());
	}

	if (funcDrag) vert = origVert;
		else if (vert < 0) vert = 0;

	if (keyDrag) horz = origHorz;

	newBox = vert * 8 + horz;

	switch (eventType) {
		case kMouseTrackingMouseDown:

			totalClicks = CountClicks(newBox==lastBox, (transforming || keyDrag || funcDrag) ? 1 : 3);

			switch(totalClicks) {
				case 1: {
					oldChord = globalChord;
					event = fretpet->StartUndo(UN_EDIT, transforming ? CFSTR("Direct Transform") : CFSTR("Root Change"));
					event->SaveCurrentBefore();
					shouldMerge = false;
					break;
				}

				case 2: {
					if (shouldMerge)
						event->SetMerge();

					fretpet->DoToggleScaleTone();
					break;
				}

				case 3: {
					event->SetMerge();

					IS_OPTION(GetCurrentKeyModifiers())
						? fretpet->DoSubtractScaleTriad()
						: fretpet->DoAddScaleTriad();
					break;
				}
			}

			lastBox = -1;

			break;

		case kMouseTrackingMouseUp: {
			//
			//	If the chord changed on the first click then
			//	commit the undo and flag that a double-click
			//	should throw out the original "root change"
			//	event.
			//
			if (totalClicks == 1 && oldChord != globalChord) {
				event->SaveCurrentAfter();
				event->Commit();
				shouldMerge = true;
				fretpet->EndTransformOnce();
			}
			break;
		}
	}

	if (lastBox != newBox) {
		fretpet->DoMoveScaleCursor(keyDrag ? 0 : horz, vert);
		lastBox = newBox;
	}

	return true;
}


void FPScaleControl::GetBoxFromPoint(Point where, int *vert, int *horz) {
	*vert = (where.v - 2) / FUNCV - 1;

	CONSTRAIN(*vert, -1, OCTAVE - 1);

	if (where.h < FUNCH-2)
		*horz = 0;
	else {
		*horz = (where.h - FUNCH-2) / FUNCH + 1;
		if (*horz > NUM_STEPS) *horz = NUM_STEPS;
	}
}


#pragma mark -
FPScaleNameControl::FPScaleNameControl(WindowRef wind) : TControl(wind, 'scal', 100) {}


bool FPScaleNameControl::Draw(const Rect &bounds) {
	FPScalePalette *thePal = (FPScalePalette*)GetTWindow();

	//
	// Clear to background
	//
	EraseRect(&bounds);

	//
	// Use a system appearance widget as the backdrop
	// Primary Group box looks pretty
	//
	(void) DrawThemePrimaryGroup(&bounds, kThemeStateActive);

	// Set the port font
	FontInfo	font;
	SetFont(&font, (StringPtr)"\pLucida Grande", 11, bold);

	// Get the text color
	RGBColor	*rgb;
	switch (thePal->ScaleType()) {
		case kTriadMajor:
			rgb = &rgbMaj;
			break;
		case kTriadMinor:
			rgb = &rgbMin;
			break;
		case kTriadDiminished:
			rgb = &rgbDim;
			break;
		default:
			rgb = &rgbBlack;
			break;
	}

	// Draw the string
	CGContextRef	gc = FocusContext();
	CGContextSetAlpha(gc, IsEnabled() && IsActive() ? 1.0 : 0.25);
	CGContextSetRGBFillColor(gc, CG_RGBCOLOR(rgb), 1.0);

	TString name;
	name.SetLocalized(thePal->ScaleName());
	Rect inner = bounds;
	inner.top += 2; inner.left += 4;
	name.Draw( gc, inner, kThemeCurrentPortFont, GetDrawState() );

	EndContext();

	thePal->DrawScaleHeading();

	return true;
}


#pragma mark -
FPScalePalette::FPScalePalette() : FPPalette(CFSTR("Scale")) {
	scalePalette = this;

	redrawScale		= true;
	currMode		= -1;
	bulbFlag		= false;
	noteModifier	= 0;
	currKey			= 0;
	currFunction	= 0;

	//
	// Force all boxes to be drawn
	//
	for (int col=0; col<=NUM_STEPS; col++)
		for (int key=0; key<OCTAVE; key++)
			boxState[key][col].force = true;

	WindowRef wind = Window();
	backButton		= new TPictureControl(wind, kFPCommandScaleBack, kPictBack1);
	fwdButton		= new TPictureControl(wind, kFPCommandScaleFwd, kPictFwd1);
	flatButton		= new TSegmentedPictureButton(wind, kFPCommandFlat, kPictFlat1);
	naturalButton   = new TSegmentedPictureButton(wind, kFPCommandNatural, kPictNatural1);
	sharpButton		= new TSegmentedPictureButton(wind, kFPCommandSharp, kPictSharp1);
	bulbButton		= new TPictureControl(wind, kFPCommandBulb, kPictBulb1);

	nameControl		= new FPScaleNameControl(wind);
	scaleControl	= new FPScaleControl(wind);

	Rect bounds = scaleControl->Bounds();
	scaleBoxX = bounds.left;
	scaleBoxY = bounds.top;
	InitBoxesForKeyOrder();

//	globalChord.Set(0,0,0);		// 0x91 is the C chord

	SetNoteModifier(0);
	SetScale(kModeIonian);
	SetIllumination(true);
	MoveScaleCursor(0, 0);

}


FPScalePalette::~FPScalePalette() {
	delete backButton;
	delete fwdButton;
	delete flatButton;
	delete naturalButton;
	delete sharpButton;
	delete bulbButton;

	delete nameControl;
	delete scaleControl;
}


void FPScalePalette::Draw() {
	Rect bounds = GetContentSize();
/*
	RGBForeColor(&rgbPalette);
	PaintRect(&bounds);
	ForeColor(blackColor);
*/

	EraseRect(&bounds);
	DrawControls(Window());
}


void FPScalePalette::DrawScaleHeading() {

	static char const *scaleHeadings[] = {
		" 1 2 3 4 5 6 7",
		" 1 2b3 4 5 6b7",
		" 1b2b3 4 5b6b7",
		" 1 2 3#4 5 6 7",
		" 1 2 3 4 5 6b7",
		" 1 2b3 4 5b6b7",
		" 1b2b3 4b5b6b7",
		" 1 2b3 4 5b6 7",
		" 1 2b3 4 5 6 7",
		" 1b2 3 4b5 6b7"
	};

	if (!IsVisible())
		return;

	unsigned char	string[3];

	Rect			headRect = { scaleBoxY, scaleBoxX + FUNCH + 1, scaleBoxY + FUNCV, scaleBoxX + 8 * FUNCH + 2 };
	EraseRect(&headRect);

	FontInfo		font;
	SetFont(&font, (StringPtr)"\pLucida Grande", 11, normal);

	PenSize(1, 1);

/*
	RGBForeColor(&rgbPalette);
	PaintRect(&headRect);
*/

	// PRINT THE FUNCTION HEADER
	for (int step=0; step<NUM_STEPS; step++) {
		string[0] = 2;
		BlockMoveData(&scaleHeadings[CurrentMode()][step*2], &string[1], 2);
		MoveTo(scaleBoxX + FUNCH * (step+1) + (FUNCH - StringWidth(string)) / 2, scaleBoxY + FUNCV - 2);
		RGBForeColor( LightColor(step) );
		DrawString(string);
	}

	ForeColor(blackColor);
}


void FPScalePalette::SetScale(SInt16 newMode) {
	// Set the current mode, which will be passed to the scale info
	// when generating the info below
	currMode = LIMIT(newMode, NUM_SCALES);

	Str255	namePart, extPart, missPart, fullName;
	for (int step=7; step--;) {
		// Get a triad based on the scale step
		UInt16	triad = GetTriad(0, step, 0);

		// The root is the first tone in the triad
		UInt16	fakeRoot = ScaleTone(0, step);

		// Create a chord so we can generate a name
		FPChord harmChord(triad, 0, fakeRoot);

		// Get a name using roman notation, clean up the parts
		harmChord.ChordName(fakeRoot, namePart, extPart, missPart, true);
		prtrim(namePart); prtrim(extPart);

		pstrcpy(fullName, namePart);
		pstrcat(fullName, extPart);
		CopyPascalStringToC(fullName, romanSteps[step]);
	}
}


void FPScalePalette::DrawChanges() {
	redrawScale = false;
	scaleControl->DrawNow();
	redrawScale = true;
}


void FPScalePalette::DrawScaleBoxes() {
	static BoxState prevState[OCTAVE][NUM_STEPS + 1] = {
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 },
						{ 0, 0, 0, 0, 0, 0, 0, 0 }	};

	if (IsVisible()) {
		if (redrawScale) {
			Rect testRect = {
				scaleBoxY + FUNCV + 1,
				scaleBoxX + FUNCH + 1,
				scaleBoxY + FUNCV * 13 + 2,
				scaleBoxX + FUNCH * 8 + 2 };

			RGBForeColor(&rgbBlack);
			PaintRect(&testRect);

			for (int col=0; col<=NUM_STEPS; col++)
				for (int key=0; key<OCTAVE; key++)
					SetBoxDirty(key, col);
		}

		for (int col=0; col<=NUM_STEPS; col++) {
			for (int key=0; key<OCTAVE; key++) {
				UpdateBoxState(key, col);
				if (boxState[key][col].force || bcmp(&prevState[key][col], &boxState[key][col], sizeof(BoxState))) {
					DrawScaleBox(key, col);
					boxState[key][col].force = false;
					prevState[key][col] = boxState[key][col];
//					bcopy(&boxState[key][col], &prevState[key][col], sizeof(BoxState));
				}
			}
		}
	}
}


void FPScalePalette::InitBoxesForKeyOrder() {
	//
	// Initialize (and update) the box states
	//
	for (int func=0; func<=NUM_STEPS; func++) {
		for (int keyindex=0; keyindex<=11; keyindex++) {
			int key = KEY_FOR_INDEX(keyindex);
			int x = scaleBoxX + FUNCH * func + (func ? 2 : 0);
			int y = scaleBoxY + FUNCV + 2 + FUNCV * keyindex;
			SetHVRect(&boxState[keyindex][func].rect, x, y, FUNCH - 1, FUNCV - 1);
			boxState[keyindex][func].tx			= x + 3;
			boxState[keyindex][func].ty			= y + FUNCV - 3;
			boxState[keyindex][func].key		= key;
		}
	}
}


void FPScalePalette::UpdateBoxState(SInt16 keyindex, SInt16 func) {
	RGBColor		*boxColor, *frameColor, *textColor;
	RGBColor		halfColor;
	int				key, n, mod=0;
	bool			drawFrame = false;
	SInt16			realFunc = func ? func-1 : 0;
	bool			isOffset = false;
	const FPChord	&chord = globalChord;

	key = KEY_FOR_INDEX(keyindex);

	//
	//  DETERMINE THE COLOR OF THE FRAME AND BACKGROUND
	//
	UInt16	lock = chord.IsLocked();
	UInt16	note = ScaleTone(key, realFunc);

	// Cursor matching?
	bool	keyMatch = false, funcMatch = false;
	bool	lockKeyMatch = false, lockFuncMatch = false;

	if (lock) {
		lockKeyMatch = (key == chord.Key());			// CurrentKey of global chord (Root Y)
		lockFuncMatch = (chord.RootScaleStep() == realFunc);
	}

	keyMatch = (key == CurrentKey());
	funcMatch = (realFunc == CurrentFunction());


	if (func == 0) {
		//
		// A Key Column box
		//
		drawFrame	= lock ? lockKeyMatch : keyMatch;
		textColor	= &rgbWhite;
		frameColor	= &rgbLtGreen;

		if	(	bulbFlag
				&&	chord.HasTones()
				&&	KeyHasTones(key, chord.tones)
			)
			boxColor = &rgbKeyFits;
		else
			boxColor = &rgbBlack;
	}
	else {
		//
		// A Scale box
		//
		if (keyMatch && funcMatch) {			// *** CURSOR?
			drawFrame = true;
			mod = NoteModifier();
			isOffset = (mod != 0);

			n = NOTEMOD(note + mod);
			bool isRoot = (n == chord.root);
			textColor = isRoot ? &rgbYellow : &rgbWhite;
			frameColor = (isRoot && (!lock || lockKeyMatch)) ? textColor : &rgbWhite;
		}
		else {
			if (lockKeyMatch && lockFuncMatch) { // *** ROOT?
				drawFrame = true;
				mod = chord.RootModifier();
				isOffset = (mod != 0);
			}

			n = NOTEMOD(note + mod);
			frameColor = textColor = (n == chord.root) ? &rgbYellow : &rgbWhite;
		}

		//
		// Lighter box for illuminated notes
		//
		if (bulbFlag) {
			if (chord.tones & BIT(n))
				boxColor = LightColor(realFunc);
			else {
				boxColor = DarkColor(realFunc);
				if (chord.tones)
					textColor = LightColor(realFunc);				// The light color looks good on the dark
			}
		}
		else
			boxColor = DarkColor(realFunc);
	}

	BoxState &bs = boxState[keyindex][func];

	bs.boxColor		= boxColor;
	bs.isRound		= isOffset;
	bs.textColor	= textColor;
	bs.drawFrame	= drawFrame;
	bs.note			= note;
	bs.mod			= mod;
	bs.text			= NameOfNote(bs.key, note);

	if (drawFrame) {
		bs.frameColor = frameColor;

		halfColor.red	= frameColor->red / 2;
		halfColor.green	= frameColor->green / 2;
		halfColor.blue	= frameColor->blue / 2;

		bs.halfColor = halfColor;
	}
}


void FPScalePalette::DrawScaleBox(SInt16 keyindex, SInt16 func) {
	Str15		tone;
	BoxState	box = boxState[keyindex][func];

	//
	// Draw the background of the cell
	//
	if (box.isRound) {
		ForeColor(blackColor);
		PaintRect(&box.rect);
		RGBForeColor(box.boxColor);
		PaintRoundRect(&box.rect, 10, 10);
	}
	else {
		RGBForeColor(box.boxColor);
		PaintRect(&box.rect);
	}

	//
	// Draw the frame
	//
	if (box.drawFrame) {
		PenSize(1, 1);

		RGBForeColor(box.frameColor);

		box.isRound
			? FrameRoundRect(&box.rect, 10, 10)
			: FrameRect(&box.rect);

		Rect innerRect = box.rect;
		InsetRect(&innerRect, 1, 1);
		RGBForeColor(&box.halfColor);

		box.isRound
			? FrameRoundRect(&innerRect, 9, 9)
			: FrameRect(&innerRect);
	}

	//
	// Draw the text
	//
	RGBForeColor(box.textColor);
	RGBBackColor(box.boxColor);
	SetFont(NULL, (StringPtr)"\pLucida Grande", 11, normal);
	MoveTo(box.tx, box.ty);
	CopyCStringToPascal(BoxToneName(box.key, box.note, box.mod), tone);
	DrawString(tone);

	ForeColor(blackColor);
	BackColor(whiteColor);
}


void FPScalePalette::MoveCursorToNote(UInt16 note, SInt16 newkey) {
	int	offset = 99;
	int	func = 99;
	int	x, i, oldO, oldF;

	UInt16 destkey = (newkey == kCurrentKey) ? CurrentKey() : newkey;

	bool findSharps = ( FIFTHS_POSITION(destkey) > (OCTAVE - 1 - info.Enharmonic()) );

	note = MIN_NOTE(note, newkey);

	do {
		oldO = offset; oldF = func; offset = 99;
		for (i=NUM_STEPS; i--;) {
			x = note - MIN_NOTE(ScaleTone(destkey, i), destkey);
			if ((findSharps && x >= 0) || (!findSharps && x <= 0))
				if (ABS(x) < ABS(offset)) { func = i; offset = x; }
		}
		findSharps ^= true;
	} while ( (offset>1 || offset<-1) && oldO==99 );

	if (ABS(oldO)==ABS(offset))
		offset=oldO,	func=oldF;

	SetNoteModifier(offset);
	MoveScaleCursor( func + 1, INDEX_FOR_KEY(destkey) );		// Assumes if newkey was provided it's passive
}


void FPScalePalette::MoveScaleCursor(SInt16 nh, SInt16 nv) {
	if (nv >= 0)
		currKey = KEY_FOR_INDEX(nv);

	if (nh != 0)
		currFunction = nh - 1;

}


//-----------------------------------------------
//
//  SetCursorWithChord
//
//	This is called by ReflectChordChanges after any
//	change to the global chord that affects the
//	position of the scale cursor.
//
//	This is currently called after App::UpdateGlobalChord
//
void FPScalePalette::SetCursorWithChord(FPChord &chord) {
	if ( chord.RootNeedsScaleInfo() ) {
		MoveCursorToNote( chord.root, chord.key );
		chord.rootModifier = NoteModifier();
		chord.rootScaleStep = CurrentFunction();
	}
	else {
		SetNoteModifier( chord.rootModifier );
		MoveScaleCursor( chord.rootScaleStep + 1, INDEX_FOR_KEY(chord.key) );
	}
}


void FPScalePalette::SetNoteModifier(SInt16 nm) {
	noteModifier = nm;
	flatButton->SetAndLatch(nm == -1);
	naturalButton->SetAndLatch(nm == 0);
	sharpButton->SetAndLatch(nm == 1);
}


void FPScalePalette::SetIllumination(bool onoff) {
	bulbFlag = onoff;
	bulbButton->SetState(onoff);

	if (IsVisible())
		scaleControl->DrawNow();
}


void FPScalePalette::PlayCurrentTone() {
	UInt16 tone = CurrentTone();

	if (tone < NOTEMOD(CurrentKey()+NoteModifier()))
		tone += OCTAVE;

	player->PlayNote(0,  tone + OCTAVE);
}


