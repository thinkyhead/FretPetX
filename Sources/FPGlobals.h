/*
 *  FPGlobals.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPGLOBALS_H
#define FPGLOBALS_H

#include "FPMacros.h"
#include <QuickTime/QuickTimeMusic.h>

//#define DEBUG_ASSERT_PRODUCTION_CODE 0

#define BANK_FILETYPE	'BANK'

// Data Parameters
enum {
	LOWEST_C		= 36,		// The lowest C note to send to QT
	THE_POLY		= 12,		// The polyphonic value for QT
	TYPICAL_POLY	= 6,		// Typical polyphony
	OLD_NUM_PARTS	= 4,		// The original program had 4 parts
	DOC_PARTS		= 4,		// The new one has 4, but this should be changeable
	TOTAL_PARTS		= 4,		// Total parts may differ if we add a metronome or drumkit
	MAX_FRETS		= 24,		// The most frets allowed on a guitar or in the music data
	NUM_STRINGS		= 6,		// A name for 6, but could become variable
	NUM_STEPS		= 7,		// A name for the 7 steps in the scale
	NUM_MODES		= 7,		// A name for the number of modes - 7 for 7 tone scales
	NUM_SCALES		= 10,		// The total number of scales
	TOTAL_CHANNELS	= 16,		// The number of channels in MIDI
	NUM_OCTAVES		= 12,		// The number of octaves the music data is limited to
	MAX_BEATS		= 16,		// The most beats allowed in a document sequencer
	MAX_REPEAT		= 16		// The highest number of repeats allowed
};


enum {
	kFilterEnabled	= 1,
	kFilterCurrent	= 2,
	kFilterAll		= 3,

	kCurrentChannel		= -1,
	kAllChannels		= -50,
	kAllChannelsMask	= (1 << DOC_PARTS) - 1,
	kCurrentChannelMask	= 0xFF,

	kChannel1		= 0,
	kChannel2,	kChannel3,	kChannel4,	kChannel5,
	kChannel6,	kChannel7,	kChannel8,	kChannel9,

	kCurrentKey		= -1,

	kInvalidItemIndex = 999,

	kErrorHandled	= -9999,

	// GM Instruments, Drums, and GS Instruments
	kDefinedGMInstruments	= kLastGMInstrument - kFirstGMInstrument + 1,
	kDefinedDrums			= 9,
	kDefinedGSInstruments	= 98,

	kFirstFauxGM			= kFirstGMInstrument,
	kLastFauxGM				= kLastGMInstrument,

	kFirstFauxDrum			= kLastGMInstrument + 1,
	kLastFauxDrum			= kFirstFauxDrum + kDefinedDrums - 1,
	
	kFirstFauxGS			= kFirstFauxDrum + kDefinedDrums,
	kLastFauxGS				= kFirstFauxGS + kDefinedGSInstruments - 1,

	kMenuIndexDrums			= 17,
	kMenuIndexGS
	
};

//
// Names for intervals, tone indexes, and base chords
//
enum {
	MIN2 = 1,
	MAJ2, MIN3, MAJ3, PER4, DIM5, PER5, AUG5, MAJ6, DOM7, MAJ7,
	OCTAVE,

	MIN6	= MAJ6 - 1,
	DIM7	= DOM7 - 1,
	PER9	= MAJ2,
	FLA9	= PER9 - 1,
	DIM9	= FLA9,
	AUG9	= PER9 + 1,
	IN10	= MAJ3,
	IN11	= PER4,
	FL11	= IN11 - 1,
	DIM11	= FL11,
	SH11	= IN11 + 1,
	IN13	= MAJ6,
	DIM13	= IN13 - 1,

	B_ROOT = BIT(0),
	B_MIN2 = BIT(MIN2),	B_FLA9 = BIT(FLA9), B_DIM9 = BIT(DIM9),
	B_MAJ2 = BIT(MAJ2),	B_PER9 = BIT(PER9),
	B_MIN3 = BIT(MIN3),	B_AUG9 = BIT(AUG9),
	B_MAJ3 = BIT(MAJ3),	B_DOM10 = BIT(IN10), B_DIM11 = BIT(FL11),
	B_PER4 = BIT(PER4),	B_DOM11 = BIT(IN11),
	B_DIM5 = BIT(DIM5),	B_SH11 = BIT(SH11),
	B_PER5 = BIT(PER5),
	B_AUG5 = BIT(AUG5),	B_MIN6  = BIT(MIN6), B_DIM13 = BIT(DIM13),
	B_MAJ6 = BIT(MAJ6),	B_DIM7 = BIT(DIM7), B_DOM13  = BIT(IN13),
	B_DOM7 = BIT(DOM7),
	B_MAJ7 = BIT(MAJ7),

	SUS2_TRIAD	= B_ROOT|B_MAJ2|B_PER5,
	MIN_TRIAD	= B_ROOT|B_MIN3|B_PER5,
	MAJ_TRIAD	= B_ROOT|B_MAJ3|B_PER5,
	SUS4_TRIAD	= B_ROOT|B_PER4|B_PER5,
	DIM_TRIAD	= B_ROOT|B_MIN3|B_DIM5,
	DIM5_TRIAD	= B_ROOT|B_MAJ3|B_DIM5,
	AUG_TRIAD	= B_ROOT|B_MAJ3|B_AUG5,

	DOM7_CHORD	= MAJ_TRIAD|B_DOM7,
	MAJ7_CHORD	= MAJ_TRIAD|B_MAJ7,
	MIN7_CHORD	= MIN_TRIAD|B_DOM7,
	SUS7_CHORD	= SUS4_TRIAD|B_DOM7,
	MIM7_CHORD	= MIN_TRIAD|B_MAJ7,
	AUG7_CHORD	= AUG_TRIAD|B_DOM7,
	AUGMAJ7_CHORD = AUG_TRIAD|B_MAJ7,
	HAD7_CHORD	= DIM_TRIAD|B_DOM7,
	DIM7_CHORD	= DIM_TRIAD|B_DIM7,

	DOM9_CHORD	= DOM7_CHORD|B_PER9,
	MAJ9_CHORD	= MAJ7_CHORD|B_PER9,
	MIN9_CHORD	= MIN7_CHORD|B_PER9,
	SUS9_CHORD	= SUS7_CHORD|B_PER9,
	MIM9_CHORD	= MIM7_CHORD|B_PER9,
	AUG9_CHORD	= AUG7_CHORD|B_PER9,
	AUGMAJ9_CHORD = AUGMAJ7_CHORD|B_PER9,
	HAD9_CHORD	= HAD7_CHORD|B_DIM9,
	DIM9_CHORD	= DIM7_CHORD|B_DIM9,

	DOM11_CHORD	= DOM9_CHORD|B_DOM11,
	MIN11_CHORD	= MIN9_CHORD|B_DOM11,
	MAJ11_CHORD	= MAJ9_CHORD|B_DOM11,
	MIM11_CHORD	= MIM9_CHORD|B_DOM11,
	AUG11_CHORD	= AUG9_CHORD|B_DOM11,
	AUGMAJ11_CHORD = AUGMAJ9_CHORD|B_DOM11,
	HAD11_CHORD	= HAD9_CHORD|B_DOM11,
	DIM11_CHORD	= DIM9_CHORD|B_DIM11,

	DOM13_CHORD	= DOM11_CHORD|B_DOM13,
	MIN13_CHORD	= MIN11_CHORD|B_DOM13,
	MAJ13_CHORD	= MAJ11_CHORD|B_DOM13,
	MIM13_CHORD	= MIM11_CHORD|B_DOM13,
	AUG13_CHORD	= AUG9_CHORD|B_DOM13,
	AUGMAJ13_CHORD = AUGMAJ9_CHORD|B_DOM13,
	HAD13_CHORD	= HAD9_CHORD|B_DOM11|B_DIM13,
	DIM13_CHORD	= DIM9_CHORD|B_DOM11|B_DIM13,

	NOTE_C			= 0,
	NOTE_C_SHARP	= 1,
	NOTE_D_FLAT		= 1,
	NOTE_D			= 2,
	NOTE_D_SHARP	= 3,
	NOTE_E_FLAT		= 3,
	NOTE_E			= 4,
	NOTE_F			= 5,
	NOTE_F_SHARP	= 6,
	NOTE_G_FLAT		= 6,
	NOTE_G			= 7,
	NOTE_G_SHARP	= 8,
	NOTE_A_FLAT		= 8,
	NOTE_A			= 9,
	NOTE_A_SHARP	= 10,
	NOTE_B_FLAT		= 10,
	NOTE_B			= 11,
};

//
// Types of triads
//
enum {
	kTriadOther,
	kTriadMajor,
	kTriadMinor,
	kTriadDiminished
};

//
// Global Music Info
//
extern short		theCScale[NUM_STEPS];
extern short		interval[NUM_SCALES][14];
extern short		letterIndex[2][OCTAVE];
extern StringPtr	modalS[9];
extern char const*	UNoteS[7][NUM_STEPS];
extern char const*		NFPrimary[OCTAVE];
extern char const*		NFSecondary[OCTAVE];
extern char const*		NFSolFa[OCTAVE];
extern char const*		NFInterval[OCTAVE];
extern char const*		NFModal[OCTAVE];
extern short		indexOf[OCTAVE];
extern bool			isBlack[OCTAVE];


//
// Colors
//
extern	RGBColor	rgbBlack,	rgbWhite,
					rgbRed,		rgbDkRed,
					rgbGray8,	rgbGray14,	rgbGray9,	rgbGray12,	rgbGray4,
					rgbGreen,	rgbLtGreen,
					rgbBlue,	rgbLtBlue,	rgbLtBlue2,
					rgbBrown,	rgbYellow,

					rgbTickGreen1,	rgbTickGreen2,
					rgbTickGray1,	rgbTickGray2,

					rgbFlat,	rgbNatural,	rgbSharp,
					rgbTone,
					rgbCurr,
					rgbMaj,		rgbMin,		rgbDim,		rgbOth,
					rgbMajBack,	rgbMinBack,	rgbDimBack,	rgbOthBack,
					rgbShadow0,	rgbShadow1,
					rgbKeyFits,
					rgbChord1,	rgbChord2,
					rgbPalette,
					rgbOdd,		rgbEven
					;
//
// FretPet Error Numbers
//
enum {
	kFPErrorNoteAlloc	=	1001,
	kFPErrorNoteChannel,
	kFPErrorBadFormat,
	kFPErrorExportMID,
	kFPErrorExportMOV,
	kFPErrorExportSunvox,
	kFPErrorKRM
};

//
// Custom cursors
//
enum {
	kCursTrans	= 128,
	kCursBar	,
	kCursRepeat	,
	kCursAdd	,
	kCursColumn	,
	kCursLine	,
	kCursCLine	,
	kCursMove	,
	kCursDupe	,
	kCursRotate	,
	kCursHear	,
	kCursOne	,
	kCursTwo	,
	kCursThree	,
	kCursFour	,
	kCursHalf	,
	kCursMaj2	,
	kCursMin3	,
	kCursMaj3	,
	kCursPerf4	,
	kCursNoSmoking	,
	kCursMovePart	,
	kCursCopyPart	,
	kCursDrumUp		,
	kCursDrumDown	,
	kCursWebLink	,
	kFirstCursor	=	kCursTrans,
	kLastCursor		=	kCursWebLink,
	kNumCursors		=	kLastCursor - kFirstCursor + 1
};

//
// Shared pictures
//
enum {
	kPictLittleEye = 1400
	};

#endif
