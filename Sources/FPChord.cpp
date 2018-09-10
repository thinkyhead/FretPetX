/*
 *  FPChord.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPChord.h"

#include "FPGuitarPalette.h"
#include "FPScalePalette.h"
#include "FPApplication.h"
#include "FPPreferences.h"

#include "TFile.h"
#include "TDictionary.h"
#include "TString.h"

FPChord globalChord;

#define kPrefDefaultSeq		CFSTR("defaultSequence")

#define kStoredTones		CFSTR("tones")
#define kStoredRoot			CFSTR("root")
#define kStoredRootStep		CFSTR("rootStep")
#define kStoredRootModifier	CFSTR("rootModifier")
#define kStoredKey			CFSTR("key")
#define kStoredLock			CFSTR("lock")
#define kStoredBracketFlag	CFSTR("bracketFlag")
#define kStoredBracketLo	CFSTR("brakLow")
#define kStoredBracketHi	CFSTR("brakHi")
#define kStoredFingering	CFSTR("fingering")
#define kStoredPattern		CFSTR("pickPattern")
#define kStoredGroupBeats	CFSTR("beats")
#define kStoredGroupRepeat	CFSTR("repeat")
#define kStoredGroup		CFSTR("group")

#define HASONLY(b)		(test==(b))
#define HASBITS(b)		((test&(b)) == (b))
#define NOTBITS(b)		((test&(b)) == 0)

#define TESTBITS(b)		( HASBITS(b) && (used=(b)) )
#define TESTONLY(b)		( HASONLY(b) && (used=(b)) )

#define HASBIT(m,b)		( BIT(b) & m )
#define MOVEBIT(m,b)	{ if (HASBIT(m,b)) { m &= ~BIT(b); m |= BIT(b+OCTAVE); } }
#define MOVEBITS(b)		{ fat.mask = (fat.mask & (~(b))) | ((fat.mask & (b)) << OCTAVE); }

//
// FPChord		Constructor
//
FPChord::FPChord() {
	Init();
}


//
// TODO: Use a copy constructor (see deepspace for examples)
//
FPChord::FPChord(const FPChord &other) {
	*this = other;
}


FPChord::FPChord(UInt16 c, UInt16 k, UInt16 r) {
	Init();
	Set(c, k, r);
}


FPChord::FPChord(const OldChordInfo &info) {
	Set(info);
}


FPChord::FPChord(const TDictionary &inDict, UInt16 b, UInt16 r) {
	key				= inDict.GetInteger(kStoredKey);
	root			= inDict.GetInteger(kStoredRoot);
	rootScaleStep	= inDict.GetInteger(kStoredRootStep, kUndefinedRootValue);
	rootModifier	= inDict.GetInteger(kStoredRootModifier, kUndefinedRootValue);
	rootLock		= inDict.GetBool(kStoredLock);
	tones			= inDict.GetInteger(kStoredTones);
	bracketFlag		= inDict.GetBool(kStoredBracketFlag);
	brakLow			= inDict.GetInteger(kStoredBracketLo);
	brakHi			= inDict.GetInteger(kStoredBracketHi);

	beats		= b;
	repeat		= r;

	SInt32 held32[NUM_STRINGS];

	inDict.GetIntArray(kStoredFingering, held32, NUM_STRINGS);

	for (int p=NUM_STRINGS; p--;)
		fretHeld[p] = held32[p];

	SInt32 pick32[NUM_STRINGS];

	inDict.GetIntArray(kStoredPattern, pick32, NUM_STRINGS);

	for (int p=NUM_STRINGS; p--;)
		pick[p] = pick32[p];
}


void FPChord::Set(const OldChordInfo &info) {
	tones		= EndianU16_BtoN(info.tones);
	key			= EndianU16_BtoN(info.key);
	root		= EndianU16_BtoN(info.root);
	rootLock	= info.rootLock;

	bracketFlag	= info.bracketFlag;
	brakLow		= EndianU16_BtoN(info.brakLow);
	brakHi		= EndianU16_BtoN(info.brakHi);

	beats		= EndianU16_BtoN(info.beats);
	repeat		= EndianU16_BtoN(info.repeat);

	for (int s=NUM_STRINGS; s--;) {
		fretHeld[s] = EndianS16_BtoN(info.fretHeld[s]);
		pick[s] = EndianU16_BtoN(info.pick[s]);
	}

	ResetStepInfo();
}


/*
FPChord& FPChord::operator=(const FPChord &src) {
	tones			= src.tones;
	root			= src.root;
	key				= src.key;
	rootLock		= src.rootLock;
	rootModifier	= src.rootModifier;
	rootScaleStep	= src.rootScaleStep;
	bracketFlag		= src.bracketFlag;
	brakLow			= src.brakLow;
	brakHi			= src.brakHi;
	beats			= src.beats;
	repeat			= src.repeat;

	for (int s=NUM_STRINGS; s--;) {
		fretHeld[s] = src.fretHeld[s];
		pick[s]		= src.pick[s];
	}
}


*/

int FPChord::operator==(const FPChord &inChord) const {
	if (this != &inChord) {
		if (tones != inChord.tones || beats != inChord.beats || root != inChord.root || key != inChord.key)
			return false;

		for (int s=NUM_STRINGS; s--;)
			if (fretHeld[s] != inChord.fretHeld[s] || pick[s] != inChord.pick[s])
				return false;
	}

	return true;
}


//
// Init
//
// Set up a new empty chord in the key of C with a root of C
//
void FPChord::Init() {
	key				= 0;
	root			= 0;
	rootScaleStep	= 0;
	rootModifier	= 0;
	rootLock		= false;
	tones			= 0;
	bracketFlag		= true;
	brakLow			= 0;
	brakHi			= 4;

	for (int s=NUM_STRINGS; s--;) {
		fretHeld[s] = -1;
		pick[s] = 0;
	}

	beats		= MAX_BEATS;
	repeat		= 1;
}


void FPChord::ChordName(TString &n, TString &e, TString &m, bool bRoman) const {
	Str31	name, ext, miss;
	ChordName(Root(), name, ext, miss, bRoman);

	n.SetFromPascalString(name);
	e.SetFromPascalString(ext);
	m.SetFromPascalString(miss);
}


void FPChord::ChordName(UInt16 inRoot, StringPtr outName, StringPtr outExtend, StringPtr outMissing, bool bRoman) const {
	char		*string1, *string2, *string3;

	string1 = ChordName(inRoot, bRoman);

	if ((string2 = strstr(string1, ":"))) {
		*string2++ = '\0';

		if ((string3 = strstr(string2, ":"))) {
			*string3++ = '\0';
			CopyCStringToPascal(string3, outMissing);
		}
		else
			outExtend[0] = 0;

		CopyCStringToPascal(string2, outExtend);
	}
	else
		outMissing[0] = 0;

	CopyCStringToPascal(string1, outName);
}


//-----------------------------------------------
//
//	ChordName
//
//	Calculate a name for your chord based on a root & key
//
char* FPChord::ChordName(UInt16 r, bool bRoman) const {
	const char		*romanNumStr[] = { "I", "bII", "II", "bIII", "III", "IV", "bV", "V", "bVI", "VI", "bVII", "VII" };

	UInt16 mask = 0, test = 0, used = 0;

	static char		string[64];
	char			toneString[32];
	char			needString[32];
	char			extString[32];

	toneString[0] = '\0';
	needString[0] = '\0';
	extString[0] = '\0';

	// Empty chord ?
	if (tones == 0) {
		strcpy(string, "None::");
	}
	else {

		r = NOTEMOD(r);

		if (bRoman || fretpet->romanMode)
			strcpy(toneString, romanNumStr[NOTEMOD(r - key)]);
		else
			strcpy(toneString, scalePalette->NameOfNote(key, r));

		// Generate a normalized chord mask
		mask = ((tones >> r) | (tones << (OCTAVE - r))) & ((1 << OCTAVE) - 1);

		//
		// ALTERNATIVE BASE TRIAD TESTING
		// The root is ignored in all these tests
		// Uses a strict precedence rule to establish identity
		// major/minor/sus, perfect/aug/dim
		//

		Boolean	noRoot = ((mask & B_ROOT) == 0),
				noThird = false,
				noFifth = false,
				promote13 = false;

		// Add a root
		test = mask|B_ROOT;

		//
		// Add a perfect 5th if:
		// - There is no diminished, perfect, or augmented 5th at all
		// - The chord is a mb6 and there is no b5 or 5
		//
		if ( NOTBITS(B_DIM5|B_PER5|B_AUG5) || (HASBITS(B_MIN3|B_MIN6) && NOTBITS(B_MAJ3|B_DIM5|B_PER5)) ) {
			test |= B_PER5;
			noFifth = true;
		}

		// Add default 3rd if there is none - leave sus2 and 5 chords as-is
		if ( NOTBITS(B_MIN3|B_MAJ3|B_PER4) && !HASONLY(B_ROOT|B_MAJ2|B_PER5) && !HASONLY(B_ROOT|B_PER5)) {
			test |= B_MAJ3;
			noThird = true;
		}

		Boolean	hasMajThird		= HASBITS(B_MAJ3),
				hasPerfFifth	= HASBITS(B_PER5);

// 13TH CHORDS (7 TONES - 1 3 5 7 9 11 13)

		if (TESTBITS(DOM13_CHORD)) {																		// 13
			strcpy(extString, "13");
		} else if (!hasPerfFifth && TESTBITS(AUG13_CHORD)) {												// 13+
			strcpy(extString, "13+");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_DOM7|B_PER9|B_DOM11|B_DOM13)) {					// 13-
			strcpy(extString, "13-");
		} else if (TESTBITS(MAJ13_CHORD)) {																	// ∆13
			strcpy(extString, "\30613");
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ13_CHORD)) {												// ∆13+
			strcpy(extString, "\30613+");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7|B_PER9|B_DOM11|B_DOM13)) {					// ∆13-
			strcpy(extString, "\30613-");
		} else if (!hasMajThird && TESTBITS(MIN13_CHORD)) {													// m13
			strcpy(extString, "m13");
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_PER9|B_DOM11|B_DOM13)) {					// m13-
			strcpy(extString, "m13-");
		} else if (!hasMajThird && TESTBITS(MIM13_CHORD)) {													// m∆13
			strcpy(extString, "m\30613");
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM_TRIAD|B_MAJ7|B_PER9|B_DOM11|B_DOM13)) {			// m∆13-
			strcpy(extString, "m\30613-");
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7|B_PER9) && TESTBITS(DIM13_CHORD)) {					// o13
			strcpy(extString, "o13");
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ6|B_MAJ7|B_PER9) && TESTBITS(HAD13_CHORD)) {					// ø13
			strcpy(extString, "\27713");

// 7/9/13 CHORDS (6 TONES - 1 3 5 7 9 13)

		} else if (TESTBITS(DOM9_CHORD|B_DOM13)) {															// 7/9/13
			strcpy(extString, "7/9/13");
		} else if (!hasPerfFifth && TESTBITS(AUG9_CHORD|B_DOM13)) {											// 7/9/13+
			strcpy(extString, "7/9/13+");
		} else if (TESTBITS(MAJ9_CHORD|B_DOM13)) {															// ∆7/9/13
			strcpy(extString, "\3067/9/13");
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ9_CHORD|B_DOM13)) {										// ∆7/9/13+
			strcpy(extString, "\3067/9/13+");

// 11TH CHORDS (6 TONES - 1 3 5 7 9 11)

		} else if (TESTBITS(DOM11_CHORD)) {																	// 11
			strcpy(extString, "11");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUG11_CHORD)) {												// 11+
			strcpy(extString, "11+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_DOM7|B_PER9|B_DOM11)) {							// 11-
			strcpy(extString, "11-");
			promote13 = true;
		} else if (TESTBITS(MAJ11_CHORD)) {																	// ∆11
			strcpy(extString, "\30611");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ11_CHORD)) {												// ∆11+
			strcpy(extString, "\30611+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7|B_PER9|B_DOM11)) {							// ∆11-
			strcpy(extString, "\30611-");
			promote13 = true;
		} else if (!hasMajThird && TESTBITS(MIN11_CHORD)) {													// m11
			strcpy(extString, "m11");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_PER9|B_DOM11)) {							// m11-
			strcpy(extString, "m11-");
			promote13 = true;
		} else if (!hasMajThird && TESTBITS(MIM11_CHORD)) {													// m∆11
			strcpy(extString, "m\30611");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM_TRIAD|B_MAJ7|B_PER9|B_DOM11)) {					// m∆11-
			strcpy(extString, "m\30611-");
			promote13 = true;
		} else if (NOTBITS(B_PER5|B_DOM7|B_MAJ7|B_PER9|B_DOM11) && TESTBITS(DIM11_CHORD)) {					// o11
			strcpy(extString, "o11");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7|B_PER9) && TESTBITS(HAD11_CHORD)) {							// ø11
			strcpy(extString, "\27711");
			promote13 = true;

// 9♯11 CHORDS (6 TONES - 1 3 5 7 9 #11)

		} else if (TESTBITS(MAJ9_CHORD|B_SH11)) {															// ∆9♯11
			strcpy(extString, "\3069#11");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ9_CHORD|B_SH11)) {										// ∆9♯11+
			strcpy(extString, "\3069#11+");
			promote13 = true;
		} else if (TESTBITS(MAJ7_CHORD|B_FLA9|B_SH11)) {													// ∆7♭9♯11
			strcpy(extString, "\3067b9#11");
			promote13 = true;
		} else if (TESTBITS(AUGMAJ7_CHORD|B_FLA9|B_SH11)) {													// ∆7♭9♯11+
			strcpy(extString, "\3067b9#11+");
			promote13 = true;

// 9♭6 CHORDS (6 TONES - 1 3 5 ♭6 7 9)

		} else if (!hasMajThird && TESTBITS(MIN9_CHORD|B_MIN6)) {											// m9♭6
			strcpy(extString, "m9b6");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_PER9|B_MIN6)) {							// m9♭6-
			strcpy(extString, "m9b6-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7|B_PER9) && TESTBITS(DIM9_CHORD|B_MIN6)) {			// o9♭6
			strcpy(extString, "o9b6");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7|B_PER9) && TESTBITS(HAD9_CHORD|B_MIN6)) {					// ø9♭6
			strcpy(extString, "\2779b6");
			promote13 = true;
		} else if (!hasMajThird && TESTBITS(MIM9_CHORD|B_MIN6)) {											// m∆9♭6
			strcpy(extString, "m\3069b6");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM_TRIAD|B_MIN6|B_MAJ7|B_PER9)) {					// m∆9♭6-
			strcpy(extString, "m\3069b6-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS9_CHORD|B_MIN6)) {									// sus9♭6
			strcpy(extString, "sus9b6");
			promote13 = true;

// 7/6/11 CHORDS (6 TONES - 1 3 5 6 7 11)

		} else if (!hasMajThird && TESTBITS(MIN7_CHORD|B_MAJ6|B_DOM11)) {									// m7/6/11
			strcpy(extString, "m7/6/11");
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(B_MIN3|B_AUG5|B_DOM7|B_MAJ6|B_DOM11)) {				// m7/6/11+
			strcpy(extString, "m7/6/11+");
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_MAJ6|B_DOM11)) {							// m7/6/11-
			strcpy(extString, "m7/6/11-");
			// or strcpy(extString, "\2777/11/13"); ??

// 7/6/9 CHORDS (6 TONES - 1 3 5 6 7 9)

		} else if (TESTBITS(DOM7_CHORD|B_MAJ6|B_PER9)) {													// 7/6/9
			strcpy(extString, "7/6/9");
		} else if (!hasPerfFifth && TESTBITS(AUG7_CHORD|B_MAJ6|B_PER9)) {									// 7/6/9+
			strcpy(extString, "7/6/9+");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_DOM7|B_MAJ6|B_PER9)) {							// 7/6/9-
			strcpy(extString, "7/6/9-");
		} else if (NOTBITS(B_PER9) && TESTBITS(DOM7_CHORD|B_MAJ6|B_FLA9)) {									// 7/6/♭9
			strcpy(extString, "7/6/b9");
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(AUG7_CHORD|B_MAJ6|B_FLA9)) {							// 7/6/♭9+
			strcpy(extString, "7/6/b9+");
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(DIM5_TRIAD|B_DOM7|B_MAJ6|B_FLA9)) {					// 7/6/♭9-
			strcpy(extString, "7/6/b9-");
		} else if (TESTBITS(MAJ7_CHORD|B_MAJ6|B_PER9)) {													// ∆7/6/9
			strcpy(extString, "\3067/6/9");
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ7_CHORD|B_MAJ6|B_PER9)) {								// ∆7/6/9+
			strcpy(extString, "\3067/6/9+");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6|B_PER9)) {							// ∆7/6/9-
			strcpy(extString, "\3067/6/9-");
		} else if (NOTBITS(B_PER9) && TESTBITS(MAJ7_CHORD|B_MAJ6|B_FLA9)) {									// ∆7/6/♭9
			strcpy(extString, "\3067/6/b9");
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(AUGMAJ7_CHORD|B_MAJ6|B_FLA9)) {						// ∆7/6/♭9+
			strcpy(extString, "\3067/6/b9+");
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6|B_FLA9)) {					// ∆7/6/♭9-
			strcpy(extString, "\3067/6/b9-");

// 7 FLAT 9 CHORDS (5 TONES - 1 3 5 7 ♭9)

		} else if (NOTBITS(B_PER9) && TESTBITS(DOM7_CHORD|B_FLA9)) {										// 7♭9
			strcpy(extString, "7b9");
			promote13 = true;
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(AUG7_CHORD|B_FLA9)) {									// 7♭9+
			strcpy(extString, "7b9+");
			promote13 = true;
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(DIM5_TRIAD|B_DOM7|B_FLA9)) {							// 7♭9-
			strcpy(extString, "7b9-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER9) && TESTBITS(MIN7_CHORD|B_FLA9)) {									// m7♭9
			strcpy(extString, "m7b9");
			promote13 = true;
		} else if (NOTBITS(B_PER9) && TESTBITS(MAJ7_CHORD|B_FLA9)) {										// ∆7♭9
			strcpy(extString, "\3067b9");
			promote13 = true;
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(AUGMAJ7_CHORD|B_FLA9)) {								// ∆7♭9+
			strcpy(extString, "\3067b9+");
			promote13 = true;
		} else if (NOTBITS(B_PER5|B_PER9) && TESTBITS(DIM5_TRIAD|B_MAJ7|B_FLA9)) {							// ∆7♭9-
			strcpy(extString, "\3067b9-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER9) && TESTBITS(SUS4_TRIAD|B_MAJ7|B_FLA9)) {					// ∆7♭9sus
			strcpy(extString, "\3067b9sus");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5|B_PER9) && TESTBITS(B_ROOT|B_PER4|B_AUG5|B_MAJ7|B_FLA9)) {	// ∆7♭9sus+
			strcpy(extString, "\3067b9sus+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5|B_PER9) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7|B_FLA9)) {	// ∆7♭9sus-
			strcpy(extString, "\3067b9sus-");
			promote13 = true;

// 7/11 CHORDS (5 TONES - 1 3 5 7 11)

		} else if (TESTBITS(DOM7_CHORD|B_DOM11)) {															// 7/11
			strcpy(extString, "7/11");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUG7_CHORD|B_DOM11)) {											// 7/11+
			strcpy(extString, "7/11+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_DOM7|B_DOM11)) {									// 7/11-
			strcpy(extString, "7/11-");
			promote13 = true;
		} else if (TESTBITS(MAJ7_CHORD|B_DOM11)) {															// ∆7/11
			strcpy(extString, "\3067/11");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ7_CHORD|B_DOM11)) {										// ∆7/11+
			strcpy(extString, "\3067/11+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7|B_DOM11)) {									// ∆7/11-
			strcpy(extString, "\3067/11-");
			promote13 = true;
		} else if (TESTBITS(MAJ7_CHORD|B_SH11)) {															// ∆7/♯11
			strcpy(extString, "\3067/#11");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ7_CHORD|B_SH11)) {										// ∆7/♯11+
			strcpy(extString, "\3067/#11+");
			promote13 = true;
		} else if (!hasMajThird && TESTBITS(MIN7_CHORD|B_DOM11)) {											// m7/11
			strcpy(extString, "m7/11");
			promote13 = true;

// NINTH CHORDS (5 TONES - 1 3 5 7 9)

		} else if (TESTBITS(DOM9_CHORD)) {																	// 9
			strcpy(extString, "9");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUG7_CHORD|B_PER9)) {											// 9+
			strcpy(extString, "9+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_DOM7|B_PER9)) {									// 9-
			strcpy(extString, "9-");
			promote13 = true;
		} else if (TESTBITS(MAJ9_CHORD)) {																	// ∆9
			strcpy(extString, "\3069");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ7_CHORD|B_PER9)) {										// ∆9+
			strcpy(extString, "\3069+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7|B_PER9)) {									// ∆9-
			strcpy(extString, "\3069-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7|B_PER9) && TESTBITS(DIM9_CHORD)) {					// o9
			strcpy(extString, "o9");
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7|B_PER9) && TESTBITS(HAD9_CHORD)) {							// ø9
			strcpy(extString, "\2779");
			promote13 = true;

// MINOR NINTH CHORDS (5 TONES - 1 m3 5 7 9)

		} else if (!hasMajThird && TESTBITS(MIN9_CHORD)) {													// m9
			strcpy(extString, "m9");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(B_MIN3|B_AUG5|B_DOM7|B_PER9)) {						// m9+
			strcpy(extString, "m9+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_PER9)) {									// m9-
			strcpy(extString, "m9-");
			promote13 = true;
		} else if (!hasMajThird && TESTBITS(MIM9_CHORD)) {													// m∆9
			strcpy(extString, "m\3069");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(B_MIN3|B_AUG5|B_MAJ7|B_PER9)) {						// m∆9+
			strcpy(extString, "m\3069+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM_TRIAD|B_MAJ7|B_PER9)) {							// m∆9-
			strcpy(extString, "m\3069-");
			promote13 = true;

// SUS9 CHORDS (5 TONES - 1 4 5 7 9)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS9_CHORD)) {										// sus9
			strcpy(extString, "sus9");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_AUG5|B_DOM7|B_PER9)) {			// sus9+
			strcpy(extString, "sus9+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_DOM7|B_PER9)) {			// sus9-
			strcpy(extString, "sus9-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS4_TRIAD|B_MAJ7|B_PER9)) {							// ∆9sus
			strcpy(extString, "\3069sus");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_AUG5|B_MAJ7|B_PER9)) {			// ∆9sus+
			strcpy(extString, "\3069sus+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7|B_PER9)) {			// ∆9sus-
			strcpy(extString, "\3069sus-");
			promote13 = true;

// 6/9 CHORDS (5 TONES - 1 3 5 6 9)

		} else if (TESTBITS(MAJ_TRIAD|B_MAJ6|B_PER9)) {														// 6/9
			strcpy(extString, "6/9");
		} else if (!hasPerfFifth && TESTBITS(AUG_TRIAD|B_MAJ6|B_PER9)) {									// 6/9+
			strcpy(extString, "6/9+");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ6|B_PER9)) {									// 6/9-
			strcpy(extString, "6/9-");
		} else if (!hasMajThird && TESTBITS(MIN_TRIAD|B_MAJ6|B_PER9)) {										// m6/9
			strcpy(extString, "m6/9");
		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS4_TRIAD|B_MAJ6|B_PER9)) {							// sus6/9
			strcpy(extString, "sus6/9");

// MINOR 7 PLUS (5 TONES - 1 3 5 7 ???)

		} else if (!hasMajThird && TESTBITS(MIN7_CHORD|B_DOM11)) {											// m7/11
			strcpy(extString, "m7/11");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_DOM11)) {								// m7/11-
			strcpy(extString, "m7/11-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(HAD7_CHORD|B_MAJ6)) {									// m7/6-
			strcpy(extString, "m7/6-");

// 7/6 CHORDS (5 TONES - 1 3 5 6 7)

		} else if (TESTBITS(DOM7_CHORD|B_MAJ6)) {															// 7/6
			strcpy(extString, "7/6");
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM5_TRIAD|B_DOM7|B_MAJ6)) {							// 7/6-
			strcpy(extString, "7/6-");
		} else if (TESTBITS(MAJ7_CHORD|B_MAJ6)) {															// ∆7/6
			strcpy(extString, "\3067/6");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6)) {									// ∆7/6-
			strcpy(extString, "\3067/6-");
		} else if (!hasMajThird && TESTBITS(MIN7_CHORD|B_MAJ6)) {											// m7/6
			strcpy(extString, "m7/6");
		} else if (!hasMajThird && TESTBITS(MIM7_CHORD|B_MAJ6)) {											// m∆7/6
			strcpy(extString, "m\3067/6");
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7) && TESTBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6)) {					// m∆7/6-
			strcpy(extString, "m\3067/6-");

// 7/6SUS CHORDS (5 TONES - 1 4 5 6 7)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS7_CHORD|B_MAJ6)) {									// 7/6sus
			strcpy(extString, "7/6sus");
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_DOM7|B_MAJ6)) {			// 7/6sus-
			strcpy(extString, "7/6sus-");
		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS4_TRIAD|B_MAJ7|B_MAJ6)) {							// ∆7/6sus
			strcpy(extString, "\3067/6sus");
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7|B_MAJ6)) {			// ∆7/6sus-
			strcpy(extString, "\3067/6sus-");

// SEVENTH CHORDS (4 TONES - 1 3 5 7)

		} else if (TESTBITS(DOM7_CHORD)) {																	// 7
			strcpy(extString, "7");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUG7_CHORD)) {													// 7+
			strcpy(extString, "7+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_DOM7)) {											// 7-
			strcpy(extString, "7-");
			promote13 = true;
		} else if (TESTBITS(MAJ7_CHORD)) {																	// ∆7
			strcpy(extString, "\3067");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(AUGMAJ7_CHORD)) {												// ∆7+
			strcpy(extString, "\3067+");
			promote13 = true;
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ7)) {											// ∆7-
			strcpy(extString, "\3067-");
			promote13 = true;

// MINOR 7 CHORDS (4 TONES - 1 m3 5 7)

		} else if (!hasMajThird && TESTBITS(MIN7_CHORD)) {													// m7
			strcpy(extString, "m7");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_DIM5|B_PER5) && TESTBITS(B_MIN3|B_AUG5|B_DOM7)) {						// m7+
			strcpy(extString, "m7+");
			promote13 = true;
		} else if (!hasMajThird && TESTBITS(MIM7_CHORD)) {													// m∆7
			strcpy(extString, "m\3067");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(B_MIN3|B_AUG5|B_MAJ7)) {								// m∆7+
			strcpy(extString, "m\3067+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM5_TRIAD|B_MAJ7)) {									// m∆7-
			strcpy(extString, "m\3067-");
			promote13 = true;

// 6TH CHORDS (4 TONES - 1 3 5 6)

//		} else if (NOTBITS(B_PER5) && TESTBITS(B_MAJ3|B_MAJ6|B_SH11) {										// 6♯11 (1 3 6 ♯11)
//			strcpy(extString, "6#11");
		} else if (TESTBITS(MAJ_TRIAD|B_MAJ6)) {															// 6
			strcpy(extString, "6");
		} else if (!hasMajThird && TESTBITS(MIN_TRIAD|B_MAJ6)) {											// m6
			strcpy(extString, "m6");
		} else if (!hasPerfFifth && TESTBITS(AUG_TRIAD|B_MAJ6)) {											// 6+
			strcpy(extString, "6+");
		} else if (!hasPerfFifth && TESTBITS(DIM5_TRIAD|B_MAJ6)) {											// 6-
			strcpy(extString, "6-");

// DIMINISHED 7 CHORDS (4 TONES - 1 m3 b5 b7/bb7)

		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7) && TESTBITS(HAD7_CHORD)) {									// ø7
			strcpy(extString, "\2777");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7) && TESTBITS(DIM7_CHORD)) {							// o7
			strcpy(extString, "o7");
			promote13 = true;

// SUS7 CHORDS (4 TONES - 1 4 5 7)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS7_CHORD)) {										// sus7
			strcpy(extString, "sus7");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_AUG5|B_DOM7)) {				// sus7+
			strcpy(extString, "sus7+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_DOM7)) {				// sus7-
			strcpy(extString, "sus7-");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS4_TRIAD|B_MAJ7)) {									// ∆7sus
			strcpy(extString, "\3067sus");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_AUG5|B_MAJ7)) {				// ∆7sus+
			strcpy(extString, "\3067sus+");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7)) {				// ∆7sus-
			strcpy(extString, "\3067sus-");
			promote13 = true;

// SUS6 CHORDS (4 TONES - 1 4 5 6)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS4_TRIAD|B_MAJ6)) {									// sus6
			strcpy(extString, "sus6");
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ6)) {				// sus6-
			strcpy(extString, "sus6-");

// MINOR FLAT 6 (4 TONES - 1 b3 5 b6)

		// TODO: Maybe skip if there is a Major 6th?
		} else if (!hasMajThird && TESTBITS(MIN_TRIAD|B_MIN6)) {											// m♭6
			strcpy(extString, "mb6");
			promote13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM_TRIAD|B_MIN6)) {									// o♭6
			strcpy(extString, "ob6");

/*
// ADD11 CHORDS (4 TONES - 1 3 5 11)

		} else if (TESTBITS(MAJ_TRIAD|B_DOM11)) {															// /11
			strcpy(extString, "/11");
		} else if (!hasMajThird && TESTBITS(MIN_TRIAD|B_DOM11)) {											// m/11
			strcpy(extString, "m/11");
		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM5_TRIAD|B_DOM11) {								// -/11
			strcpy(extString, "-/11");

// ADD 9 CHORDS (4 TONES - 1 3 5 9)

		} else if (TESTBITS(MAJ_TRIAD|B_PER9)) {															// /9
			strcpy(extString, "/9");
			promote = 2;
		} else if (TESTBITS(MAJ_TRIAD|B_FLA9)) {															// /♭9
			strcpy(extString, "/b9");
			promote = 2;
		} else if (TESTBITS(MAJ_TRIAD|B_AUG9)) {															// /♯9
			strcpy(extString, "/#9");
			promote = 2;
*/

// MAJOR TRIAD

		} else if (TESTBITS(MAJ_TRIAD)) {																	// (∆)
			strcpy(extString, "");

// MINOR TRIAD

		} else if (TESTBITS(MIN_TRIAD)) {																	// m
			strcpy(extString, "m");

// DIMINISHED 5TH TRIAD

		} else if (TESTBITS(DIM5_TRIAD)) {																	// - (b5)
			strcpy(extString, "-");

// DIMINISHED TRIAD

		} else if (NOTBITS(B_MAJ3|B_PER5) && TESTBITS(DIM_TRIAD)) {											// o
			strcpy(extString, "o");

// SUS TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS4_TRIAD)) {										// sus
			strcpy(extString, "sus");

// SUS+ TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_AUG5)) {						// sus+
			strcpy(extString, "sus+");

// SUS- TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && TESTBITS(B_ROOT|B_PER4|B_DIM5)) {						// sus-
			strcpy(extString, "sus-");

// SUS2 TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3) && TESTBITS(SUS2_TRIAD)) {										// sus2
			strcpy(extString, "sus2");

// AUGMENTED TRIAD

		} else if (!hasPerfFifth && TESTBITS(AUG_TRIAD)) {													// +
			strcpy(extString, "+");

// TONIC ONLY

		} else if (mask == B_ROOT) {																		// (3 5)
			noThird = noFifth = true;

// POWER CHORD ONLY

		} else if (TESTONLY(B_ROOT|B_PER5) || TESTONLY(B_PER5)) {											// 5
			strcpy(extString, "5");
			noThird = false;

// ???  NOT HANDLED YET!

		} else {																							// ???
			strcpy(extString, "???");
		}


//
// ADD ALL THE UNUSED NOTES
//

		// Get the tones that aren't yet accounted for
		UInt16 unused = ((mask & ~used) & ~B_ROOT);
		if (unused != 0) {

			//
			// 7TH
			//
			if ((unused & B_MAJ6) != 0)
				strcat(extString, promote13 ? "/13" : "/6");

			if ((unused & B_DOM7) != 0)
				strcat(extString, "/7");

			if ((unused & B_MAJ7) != 0)
				strcat(extString, "/\3067");

			//
			// 9TH
			//
			if ((unused & B_PER9) != 0)
				strcat(extString, "/9");
			if ((unused & B_FLA9) != 0)
				strcat(extString, "/b9");

			//
			// MINOR 3RD / SHARP 9TH
			//
			if ((unused & B_AUG9) != 0)
				strcat(extString, "/#9");

			//
			// 3RD / 10TH
			//
			if (!noThird) {
				// This should never happen:
				// There was a third, but the Major 3rd was ignored
				// when the chord was being named.
				if ((unused & B_MAJ3) != 0)
					strcat(extString, "/10");
			}

			//
			// 11TH
			//
			if ((unused & B_DOM11) != 0)
				strcat(extString, "/11");

			//
			// 5TH (No /♯11 - ♯11 is always explicit)
			//
			if ((unused & B_DIM5) != 0)
				strcat(extString, "/b5");

			if ((unused & B_AUG5) != 0)
				strcat(extString, "/#5");

			if (!noFifth) {
				// This should never happen:
				// There was a 5th originally, but the Perfect 5th
				// was ignored when the chord was being named.
				if ((unused & B_PER5) != 0)
					strcat(extString, "/5");
			}
		}

		if (noRoot)		strcpy(needString, " R");
		if (noThird)	strcat(needString, " 3");
		if (noFifth)	strcat(needString, " 5");

		if (strlen(needString) > 0)
			sprintf(needString, "(%s)", &needString[1]);

		sprintf(string, "%s:%s:%s", toneString, extString, needString);
	}

	return string;
}


//-----------------------------------------------
//
//	TwoOctaveChord
//
//	Get a tone mask based on two octaves
//
FatChord FPChord::TwoOctaveChord() const {
	UInt16 mask = 0, test = 0;
	FatChord fat = { 0, false, false, false, false };

	// Empty chord ?
	if (tones != 0) {
		Tone r = NOTEMOD(root);

		// Generate a root-relative chord mask
		fat.mask = test = mask = ((tones >> r) | (tones << (OCTAVE - r))) & ((1 << OCTAVE) - 1);

		// Add a root to the test mask
		test |= B_ROOT;

		//
		// Add a perfect 5th if:
		// - There is no diminished, perfect, or augmented 5th at all
		// - The chord is a mb6 and there is no b5 or 5
		//
		if ( NOTBITS(B_DIM5|B_PER5|B_AUG5) || (HASBITS(B_MIN3|B_MIN6) && NOTBITS(B_MAJ3|B_DIM5|B_PER5)) ) {
			test |= B_PER5;
		}

		// Add default 3rd if there is none - leave sus2 and 5 chords as-is
		if ( NOTBITS(B_MIN3|B_MAJ3|B_PER4) && !HASONLY(B_ROOT|B_MAJ2|B_PER5) && !HASONLY(B_ROOT|B_PER5)) {
			test |= B_MAJ3;
		}

		Boolean	hasMajThird		= HASBITS(B_MAJ3),
				hasPerfFifth	= HASBITS(B_PER5);

#pragma mark 13TH CHORDS (7 TONES - 1 3 5 7 9 11 13)

		if (HASBITS(DOM13_CHORD)) {																					// 13
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(AUG13_CHORD)) {															// 13+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7|B_PER9|B_DOM11|B_DOM13)) {							// 13-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (HASBITS(MAJ13_CHORD)) {																			// ∆13
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ13_CHORD)) {														// ∆13+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7|B_PER9|B_DOM11|B_DOM13)) {							// ∆13-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasMajThird && HASBITS(MIN13_CHORD)) {															// m13
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_PER9|B_DOM11|B_DOM13)) {							// m13-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (!hasMajThird && HASBITS(MIM13_CHORD)) {															// m∆13
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM_TRIAD|B_MAJ7|B_PER9|B_DOM11|B_DOM13)) {					// m∆13-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7|B_PER9) && HASBITS(DIM13_CHORD)) {							// o13
			MOVEBITS(B_DIM9|B_PER9|B_DOM11|B_DIM13);
			fat.flatFlat7 = true;
			fat.flat13 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ6|B_MAJ7|B_PER9) && HASBITS(HAD13_CHORD)) {							// ø13
			MOVEBITS(B_DIM9|B_PER9|B_DOM11|B_DIM13);
			fat.flat13 = true;

#pragma mark 7/9/13 CHORDS (6 TONES - 1 3 5 7 9 13)

		} else if (HASBITS(DOM9_CHORD|B_DOM13)) {																	// 7/9/13
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(AUG9_CHORD|B_DOM13)) {													// 7/9/13+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (HASBITS(MAJ9_CHORD|B_DOM13)) {																	// ∆7/9/13
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ9_CHORD|B_DOM13)) {												// ∆7/9/13+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);

#pragma mark 11TH CHORDS (6 TONES - 1 3 5 7 9 11)

		} else if (HASBITS(DOM11_CHORD)) {																			// 11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUG11_CHORD)) {															// 11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7|B_PER9|B_DOM11)) {									// 11-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (HASBITS(MAJ11_CHORD)) {																			// ∆11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ11_CHORD)) {														// ∆11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7|B_PER9|B_DOM11)) {									// ∆11-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIN11_CHORD)) {															// m11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_PER9|B_DOM11)) {									// m11-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIM11_CHORD)) {															// m∆11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM_TRIAD|B_MAJ7|B_PER9|B_DOM11)) {							// m∆11-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_PER5|B_DOM7|B_MAJ7|B_PER9|B_DOM11) && HASBITS(DIM11_CHORD)) {							// o11
			MOVEBITS(B_FLA9|B_PER9|B_DIM11);
			fat.flatFlat7 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7|B_PER9) && HASBITS(HAD11_CHORD)) {									// ø11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);

#pragma mark 9♯11 CHORDS (6 TONES - 1 3 5 7 9 #11)

		} else if (HASBITS(MAJ9_CHORD|B_SH11)) {																	// ∆9♯11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_SH11);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ9_CHORD|B_SH11)) {												// ∆9♯11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_SH11);
		} else if (HASBITS(MAJ7_CHORD|B_FLA9|B_SH11)) {																// ∆7♭9♯11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_SH11);
		} else if (HASBITS(AUGMAJ7_CHORD|B_FLA9|B_SH11)) {															// ∆7♭9♯11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_SH11);

#pragma mark MINOR 9♭6 CHORDS (6 TONES - 1 3 5 ♭6 7 9)

		} else if (!hasMajThird && HASBITS(MIN9_CHORD|B_MIN6)) {													// m9♭6
			MOVEBITS(B_FLA9|B_PER9);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_PER9|B_MIN6)) {									// m9♭6-
			MOVEBITS(B_FLA9|B_PER9);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7|B_PER9) && HASBITS(DIM9_CHORD|B_MIN6)) {						// o9♭6
			MOVEBITS(B_FLA9|B_PER9);
			fat.flatFlat7 = true;
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7|B_PER9) && HASBITS(HAD9_CHORD|B_MIN6)) {							// ø9♭6
			MOVEBITS(B_FLA9|B_PER9);
			fat.flat6 = true;
		} else if (!hasMajThird && HASBITS(MIM9_CHORD|B_MIN6)) {													// m∆9♭6
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM_TRIAD|B_MIN6|B_MAJ7|B_PER9)) {								// m∆9♭6-
			MOVEBITS(B_FLA9|B_PER9);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS9_CHORD|B_MIN6)) {											// sus9♭6
			MOVEBITS(B_FLA9|B_PER9);
			fat.flat6 = true;

#pragma mark 7/6/11 CHORDS (6 TONES - 1 3 5 6 7 11)

		} else if (!hasMajThird && HASBITS(MIN7_CHORD|B_MAJ6|B_DOM11)) {											// m7/6/11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(B_MIN3|B_AUG5|B_DOM7|B_MAJ6|B_DOM11)) {						// m7/6/11+
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_MAJ6|B_DOM11)) {									// m7/6/11-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);

#pragma mark 7/6/9 CHORDS (6 TONES - 1 3 5 6 7 9)

		} else if (HASBITS(DOM7_CHORD|B_MAJ6|B_PER9)) {																// 7/6/9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUG7_CHORD|B_MAJ6|B_PER9)) {											// 7/6/9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7|B_MAJ6|B_PER9)) {										// 7/6/9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (NOTBITS(B_PER9) && HASBITS(DOM7_CHORD|B_MAJ6|B_FLA9)) {											// 7/6/♭9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(AUG7_CHORD|B_MAJ6|B_FLA9)) {									// 7/6/♭9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(DIM5_TRIAD|B_DOM7|B_MAJ6|B_FLA9)) {							// 7/6/♭9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (HASBITS(MAJ7_CHORD|B_MAJ6|B_PER9)) {																// ∆7/6/9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ7_CHORD|B_MAJ6|B_PER9)) {											// ∆7/6/9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6|B_PER9)) {										// ∆7/6/9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (NOTBITS(B_PER9) && HASBITS(MAJ7_CHORD|B_MAJ6|B_FLA9)) {											// ∆7/6/♭9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(AUGMAJ7_CHORD|B_MAJ6|B_FLA9)) {								// ∆7/6/♭9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6|B_FLA9)) {							// ∆7/6/♭9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);

#pragma mark 7 FLAT 9 CHORDS (5 TONES - 1 3 5 7 ♭9)

		} else if (NOTBITS(B_PER9) && HASBITS(DOM7_CHORD|B_FLA9)) {													// 7♭9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(AUG7_CHORD|B_FLA9)) {											// 7♭9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(DIM5_TRIAD|B_DOM7|B_FLA9)) {									// 7♭9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_PER9) && HASBITS(MIN7_CHORD|B_FLA9)) {											// m7♭9
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_PER9) && HASBITS(MAJ7_CHORD|B_FLA9)) {													// ∆7♭9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(AUGMAJ7_CHORD|B_FLA9)) {										// ∆7♭9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_PER5|B_PER9) && HASBITS(DIM5_TRIAD|B_MAJ7|B_FLA9)) {									// ∆7♭9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER9) && HASBITS(SUS4_TRIAD|B_MAJ7|B_FLA9)) {							// ∆7♭9sus
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5|B_PER9) && HASBITS(B_ROOT|B_PER4|B_AUG5|B_MAJ7|B_FLA9)) {			// ∆7♭9sus+
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5|B_PER9) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7|B_FLA9)) {			// ∆7♭9sus-
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);

#pragma mark 7/11 CHORDS (5 TONES - 1 3 5 7 11)

		} else if (HASBITS(DOM7_CHORD|B_DOM11)) {																	// 7/11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUG7_CHORD|B_DOM11)) {													// 7/11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7|B_DOM11)) {											// 7/11-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (HASBITS(MAJ7_CHORD|B_DOM11)) {																	// ∆7/11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ7_CHORD|B_DOM11)) {												// ∆7/11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7|B_DOM11)) {											// ∆7/11-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (HASBITS(MAJ7_CHORD|B_SH11)) {																	// ∆7/♯11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_SH11);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ7_CHORD|B_SH11)) {												// ∆7/♯11+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_SH11);
		} else if (!hasMajThird && HASBITS(MIN7_CHORD|B_DOM11)) {													// m7/11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_SH11);

#pragma mark NINTH CHORDS (5 TONES - 1 3 5 7 9)

		} else if (HASBITS(DOM9_CHORD)) {																			// 9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(AUG7_CHORD|B_PER9)) {													// 9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7|B_PER9)) {											// 9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (HASBITS(MAJ9_CHORD)) {																			// ∆9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ7_CHORD|B_PER9)) {												// ∆9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7|B_PER9)) {											// ∆9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7|B_PER9) && HASBITS(DIM9_CHORD)) {							// o9
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flatFlat7 = true;
			fat.flat11 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7|B_PER9) && HASBITS(HAD9_CHORD)) {									// ø9
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);

#pragma mark MINOR NINTH CHORDS (5 TONES - 1 m3 5 7 9)

		} else if (!hasMajThird && HASBITS(MIN9_CHORD)) {															// m9
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(B_MIN3|B_AUG5|B_DOM7|B_PER9)) {								// m9+
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_PER9)) {											// m9-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (!hasMajThird && HASBITS(MIM9_CHORD)) {															// m∆9
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(B_MIN3|B_AUG5|B_MAJ7|B_PER9)) {								// m∆9+
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM_TRIAD|B_MAJ7|B_PER9)) {									// m∆9-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11|B_DOM13);
			fat.flat6 = true;

#pragma mark SUS9 CHORDS (5 TONES - 1 4 5 7 9)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS9_CHORD)) {													// sus9
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_AUG5|B_DOM7|B_PER9)) {					// sus9+
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_DOM7|B_PER9)) {					// sus9-
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS4_TRIAD|B_MAJ7|B_PER9)) {									// ∆9sus
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_AUG5|B_MAJ7|B_PER9)) {					// ∆9sus+
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7|B_PER9)) {					// ∆9sus-
			MOVEBITS(B_FLA9|B_PER9|B_DOM13);

#pragma mark 6/9 CHORDS (5 TONES - 1 3 5 6 9)

		} else if (HASBITS(MAJ_TRIAD|B_MAJ6|B_PER9)) {																// 6/9
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUG_TRIAD|B_MAJ6|B_PER9)) {												// 6/9+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ6|B_PER9)) {											// 6/9-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIN_TRIAD|B_MAJ6|B_PER9)) {												// m6/9
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS4_TRIAD|B_MAJ6|B_PER9)) {									// sus6/9
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark MINOR 7 PLUS (5 TONES - 1 3 5 7 ?)

		} else if (!hasMajThird && HASBITS(MIN7_CHORD|B_DOM11)) {													// m7/11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_DOM11)) {											// m7/11-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(HAD7_CHORD|B_MAJ6)) {											// m7/6-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);

#pragma mark 7/6 CHORDS (5 TONES - 1 3 5 6 7) ... promote b9,9,(#9/10),11,#11,b13

		} else if (HASBITS(DOM7_CHORD|B_MAJ6)) {																	// 7/6
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_MIN3|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7|B_MAJ6)) {											// 7/6-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (HASBITS(MAJ7_CHORD|B_MAJ6)) {																	// ∆7/6
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7|B_MAJ6)) {											// ∆7/6-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIN7_CHORD|B_MAJ6)) {													// m7/6
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIM7_CHORD|B_MAJ6)) {													// m∆7/6
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7) && HASBITS(B_MIN3|B_DIM5|B_MAJ7|B_MAJ6)) {							// m∆7/6-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);

#pragma mark 7/6SUS CHORDS (5 TONES - 1 4 5 6 7)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS7_CHORD|B_MAJ6)) {											// 7/6sus
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_DOM7|B_MAJ6)) {					// 7/6sus-
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS4_TRIAD|B_MAJ7|B_MAJ6)) {									// ∆7/6sus
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7|B_MAJ6)) {					// ∆7/6sus-
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark SEVENTH CHORDS (4 TONES - 1 3 5 7)

		} else if (HASBITS(DOM7_CHORD)) {																			// 7
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUG7_CHORD)) {															// 7+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_DOM7)) {													// 7-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (HASBITS(MAJ7_CHORD)) {																			// ∆7
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUGMAJ7_CHORD)) {														// ∆7+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ7)) {													// ∆7-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);

#pragma mark MINOR 7 CHORDS (4 TONES - 1 m3 5 7)

		} else if (!hasMajThird && HASBITS(MIN7_CHORD)) {															// m7
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_DIM5|B_PER5) && HASBITS(B_MIN3|B_AUG5|B_DOM7)) {								// m7+
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIM7_CHORD)) {															// m∆7
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(B_MIN3|B_AUG5|B_MAJ7)) {										// m∆7+
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(B_MIN3|B_DIM5|B_MAJ7)) {										// m∆7-
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);

#pragma mark 6TH CHORDS (4 TONES - 1 3 5 6)

//		} else if (NOTBITS(B_PER5) && HASBITS(B_MAJ3|B_MAJ6|B_SH11) {												// 6♯11 (1 3 6 ♯11)
//			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11|B_SH11);
		} else if (HASBITS(MAJ_TRIAD|B_MAJ6)) {																		// 6
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIN_TRIAD|B_MAJ6)) {														// m6
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(AUG_TRIAD|B_MAJ6)) {													// 6+
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasPerfFifth && HASBITS(DIM5_TRIAD|B_MAJ6)) {													// 6-
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);

#pragma mark DIMINISHED 7 CHORDS (4 TONES - 1 m3 b5 b7/bb7)

		} else if (NOTBITS(B_MAJ3|B_PER5|B_MAJ7) && HASBITS(HAD7_CHORD)) {											// ø7
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5|B_DOM7|B_MAJ7) && HASBITS(DIM7_CHORD)) {									// o7
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flatFlat7 = true;

#pragma mark SUS7 CHORDS (4 TONES - 1 4 5 7)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS7_CHORD)) {													// sus7
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_AUG5|B_DOM7)) {							// sus7+
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_DOM7)) {							// sus7-
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS4_TRIAD|B_MAJ7)) {											// ∆7sus
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_AUG5|B_MAJ7)) {							// ∆7sus+
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ7)) {							// ∆7sus-
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark SUS6 CHORDS (4 TONES - 1 4 5 6)

		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS4_TRIAD|B_MAJ6)) {											// sus6
			MOVEBITS(B_FLA9|B_PER9);
		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5|B_MAJ6)) {							// sus6-
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark MINOR FLAT 6 (4 TONES - 1 b3 5 b6)

		} else if (!hasMajThird && HASBITS(MIN_TRIAD|B_MIN6)) {														// m♭6
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flat6 = true;
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM_TRIAD|B_MIN6)) {											// o♭6
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flatFlat7 = true;
			fat.flat6 = true;

/*
#pragma mark ADD11 CHORDS

		} else if (HASBITS(MAJ_TRIAD|B_DOM11)) {																	// /11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);
		} else if (!hasMajThird && HASBITS(MIN_TRIAD|B_DOM11)) {													// m/11
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM5_TRIAD|B_DOM11)) {											// -/11
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);

#pragma mark ADD 9 CHORDS (4 TONES - 1 3 5 9)

		} else if (HASBITS(MAJ_TRIAD|B_PER9)) {																		// /9
			[extString setString:@"/9"];
			promote = 2;
		} else if (HASBITS(MAJ_TRIAD|B_FLA9)) {																		// /♭9
			[extString setString:@"/♭9"];
			promote = 2;
		} else if (HASBITS(MAJ_TRIAD|B_AUG9)) {																		// /♯9
			[extString setString:@"/♯9"];
			promote = 2;
*/

#pragma mark MAJOR TRIAD

		} else if (HASBITS(MAJ_TRIAD)) {																			// (∆)
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);

#pragma mark MINOR TRIAD

		} else if (HASBITS(MIN_TRIAD)) {																			// m
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flat6 = true;

#pragma mark DIMINISHED 5TH TRIAD

		} else if (HASBITS(DIM5_TRIAD)) {																			// - (b5)
			MOVEBITS(B_FLA9|B_PER9|B_AUG9|B_DOM11);

#pragma mark DIMINISHED TRIAD

		} else if (NOTBITS(B_MAJ3|B_PER5) && HASBITS(DIM_TRIAD)) {													// o
			MOVEBITS(B_FLA9|B_PER9|B_DOM11);
			fat.flatFlat7 = true;
			fat.flat6 = true;

#pragma mark SUS TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS4_TRIAD)) {													// sus
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark SUS+

		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_AUG5)) {								// sus+
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark SUS- TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3|B_PER5) && HASBITS(B_ROOT|B_PER4|B_DIM5)) {								// sus-
			MOVEBITS(B_FLA9|B_PER9);

#pragma mark SUS2 TRIAD

		} else if (NOTBITS(B_MAJ3|B_MIN3) && HASBITS(SUS2_TRIAD)) {													// sus2
			MOVEBITS(B_FLA9);

#pragma mark AUGMENTED TRIAD

		} else if (!hasPerfFifth && HASBITS(AUG_TRIAD)) {															// +
			MOVEBITS(B_FLA9|B_PER9|B_AUG9);

#pragma mark TONIC ONLY

		} else if (mask == B_ROOT) {																				// (3 5)
			MOVEBITS(B_FLA9);

#pragma mark POWER CHORD ONLY

		} else if (HASONLY(B_ROOT|B_PER5) || HASONLY(B_PER5)) {														// 5
			MOVEBITS(B_FLA9);

#pragma mark ???  NOT HANDLED YET!

		} else {																									// ???
			MOVEBITS(B_FLA9);
		}

	}

	return fat;
}


void blobcat(char *inStr, const char *inPiece);
void blobcat(char *inStr, const char *inPiece) {
	if (strlen(inStr) > 0) strcat(inStr, " ");
	strcat(inStr, inPiece);
}

//-----------------------------------------------
//
//	ChordToneFunctions
//
//	Calculate the tone functions of your chord
//	based on a root & key just like the chord name
//
char* FPChord::ChordToneFunctions() const {
	bool			min = false;
	bool			sus = false;
	bool			dim = false;
	bool			bit[OCTAVE];
	static char		string[64];
	char			rootString[4];
	char			ninthString[16];
	char			thirdString[16];
	char			fourthString[16];
	char			eleventhString[16];
	char			fifthString[16];
	char			sixthString[16];
	char			thirteenthString[16];
	char			seventhString[16];

	UInt16 inRoot = NOTEMOD(root);

	// Empty chord ?
	if (tones == 0) {
		*string = '\0';
	}
	else {
		*string				= '\0';
		*rootString			= '\0';
		*ninthString		= '\0';
		*thirdString		= '\0';
		*fourthString		= '\0';
		*eleventhString		= '\0';
		*fifthString		= '\0';
		*sixthString		= '\0';
		*thirteenthString	= '\0';
		*seventhString		= '\0';

		// Set up the chord bit booleans
		for (int i=OCTAVE; i--;)
			bit[i] = HasTone(inRoot + i);

		if (bit[0])
			strcpy(rootString, "R");

		//
		// THIRDS
		//
		if (bit[MAJ3]) {
			blobcat(thirdString, "3");

			if (bit[PER4])
				blobcat(eleventhString, "11");
		}
		else if (bit[MIN3]) {
			blobcat(thirdString, "m3");
			min = true;

			if (bit[PER4])
				blobcat(eleventhString, "11");
		}
		else if (bit[PER4]) {
			strcpy(fourthString, "4");
			sus = true;
		}

		//
		// FIFTHS
		//
		if (bit[PER5]) {
			if (bit[DIM5])
				blobcat(fifthString, "b5");

			blobcat(fifthString, "5");

			if (bit[AUG5]) {
				if (min)
					blobcat(sixthString, "b6");
				else
					blobcat(fifthString, "#5");
			}
		}
		else {
			if (!bit[DIM5]) {
				if (bit[AUG5])
					blobcat(fifthString, "+5");
			}
			else {
				if (bit[AUG5])
					blobcat(fifthString, "#5");

				dim = true;
				blobcat(fifthString, (char*)(min ? "o5" : "-5"));
			}
		}


		bool sharp9 = (bit[AUG9] && !min);


		//
		// SEVENTHS
		//
		if (bit[DOM7]) {
			blobcat(seventhString, "b7");

			if (bit[MAJ7])
				blobcat(seventhString,"7");

			if (bit[MAJ6]) {
				if (bit[PER9] || bit[FLA9] || sharp9)
					blobcat(thirteenthString, "13");
				else
					blobcat(sixthString, "6");
			}
		}
		else if (bit[MAJ7]) {
			blobcat(seventhString, "7");

			if (bit[MAJ6]) {
				if (bit[PER9] || bit[FLA9] || sharp9)
					blobcat(thirteenthString, "13");
				else
					blobcat(sixthString, "6");
			}
		}
		else if (bit[MAJ6])
			blobcat(sixthString, (char*)((min && dim) ? "o7" : "6"));

		if (bit[FLA9])
			blobcat(ninthString, "b9");

		if (bit[PER9])
			blobcat(ninthString, "9");

		if (sharp9)
			blobcat(ninthString, "#9");

		sprintf(string, "%s %s %s %s %s %s %s %s %s", rootString, ninthString, thirdString, fourthString, eleventhString, fifthString, sixthString, thirteenthString, seventhString);
	}

	return string;
}


void FPChord::UpdateStepInfo() {
	static FPScalePalette &scale = *scalePalette;
	SInt16 rs = scale.FunctionOfTone(key, root);

	if (rs >= 0)
		rootModifier = 0;
	else {
		rs = scale.FunctionOfTone(key, root + 1);

		if (rs >= 0)
			rootModifier = -1;
		else {
			rs = scale.FunctionOfTone(key, root - 1);
			rootModifier = 1;
		}
	}

	rootScaleStep = rs;
}


void FPChord::HarmonizeBy(const SInt16 steps) {
	if ( RootNeedsScaleInfo() )
		UpdateStepInfo();

	ADD_MOD(rootScaleStep, steps, NUM_STEPS);

	if (rootModifier != 0) {
		root -= rootModifier;
		rootModifier = 0;
	}

	UInt16	mask = scalePalette->MaskForKey(key),
			d = (steps < 0) ? OCTAVE-1 : 1,
			asteps = ABS(steps),
			j;

	// Get the adjacent tone for the root
	for (j=asteps; j--;) {
		do { root = (root + d) % OCTAVE; }
		while (!(mask & BIT(root)));
	}

	// Get adjacent tones for all tones
	if (tones) {
		SInt16	newtones = 0;
		UInt16	n, j;
		for (int i=OCTAVE; i--;) {
			if (tones & BIT(i)) {
				n = i;

				// Move each tone by the number of steps
				for (j=asteps; j--;) {
					do { n = (n + d) % OCTAVE; }
					while (!(mask & BIT(n)));
				}

				newtones |= BIT(n);
			}
		}
		tones = newtones;
	}
}


bool FPChord::TransposeTo(const UInt16 newKey) {
	UInt16 diff = NOTEMOD(newKey - key);

//	fprintf(stderr, "Transpose %d to %d (%d)\n", key, k, diff);

	if (diff) {
		UInt16 newtones = 0;
		for (int i=OCTAVE; i--;)
			if (tones & BIT(i))
				newtones |= BIT((i + diff) % OCTAVE);

		tones = newtones;
		ADD_MOD(root, diff, OCTAVE);
		key = newKey % OCTAVE;
	}

	return (diff != 0);
}


UInt16 FPChord::StringsUsedInPattern() const {
	UInt16 stringMask = 0;
	if (bracketFlag && tones) {
		UInt16 mask = BIT(beats) - 1;
		for (int s=NUM_STRINGS;s--;)
			if ( (fretHeld[s] >= 0) && (pick[s] & mask) )
				stringMask |= BIT(s);
	}
	return stringMask;
}


bool FPChord::HasPattern(bool noChordRequired) const {
	UInt16 mask = BIT(beats) - 1;
	for (int i=NUM_STRINGS;i--;) {
		if ( (noChordRequired || fretHeld[i] >= 0) && (pick[i] & mask) )
			return true;
	}

	return false;
}


bool FPChord::HasFingering() const {
	if (HasTones() && IsBracketEnabled()) {
		for (int i=NUM_STRINGS;i--;)
			if ( fretHeld[i] >= 0 )
				return true;
	}

	return false;
}


void FPChord::ResetPattern() {
	beats	= MAX_BEATS;
	repeat	= 1;

	ClearPattern();
}

void FPChord::SetDefaultPattern() {
	beats	= MAX_BEATS;
	repeat	= 1;

	SInt32 intList[NUM_STRINGS] = { 0x0001,0x0182,0x8244,0x4428,0x2810,0x1000 };

	CFStringRef	stringRef = (CFStringRef)preferences.GetCopy(kPrefDefaultSeq);
	if (stringRef) {
		CFRETAIN(stringRef);	
		(void)CFStringToIntArray(stringRef, intList, NUM_STRINGS);
		CFRELEASE(stringRef);
	}

	for(int s=NUM_STRINGS; s--; pick[s]=intList[s]);
}

void FPChord::SavePatternAsDefault() {
	SInt32 intList[NUM_STRINGS];
	for (int s=NUM_STRINGS; s--;)
		intList[s] = globalChord.pick[s];
	CFStringRef listRef = CFStringCreateWithIntList(intList, NUM_STRINGS);
	preferences.SetString(kPrefDefaultSeq, listRef);
	CFRELEASE(listRef);
}

void FPChord::FlipPattern() {
	for (int i=NUM_STRINGS / 2; i--;) {
		UInt16 p = pick[i];
		pick[i] = pick[NUM_STRINGS-1-i];
		pick[NUM_STRINGS-1-i] = p;
	}
}


bool FPChord::CanFlipPattern() const {
	for (int i=NUM_STRINGS / 2; i--;)
		if (pick[i] != pick[NUM_STRINGS-1-i])
			return true;

	return false;
}


void FPChord::ReversePattern() {
	for (int i=NUM_STRINGS; i--;) {
		int q = pick[i], r = 0;
		for (int z=beats; z--;)
			if ((q&(1<<z)) != 0)
				r += (1<<(beats-1-z));

		pick[i] = r;
	}
}


bool FPChord::CanReversePattern() const {
	for (int i=NUM_STRINGS; i--;) {
		PatternMask q = pick[i];
		for (int z=0; z<beats/2; z++) {
			bool q1 = (q & BIT(z)) != 0;
			bool q2 = (q & BIT(beats-1-z)) != 0;
			if (q1 != q2) return true;
		}
	}

	return false;
}


void FPChord::InvertTones() {
	tones ^= scalePalette->MaskForKey(key);
}


bool FPChord::CanInvertTones() const {
	UInt16 newTones = tones ^ scalePalette->MaskForKey(key);
	return tones != newTones;
}


void FPChord::CleanupTones() {
	UInt16	newTones = 0;

	for (int s=NUM_STRINGS; s--;) {
		int f = fretHeld[s];
		if (f >= 0)
			newTones |= BIT(NOTEMOD(guitarPalette->Tone(s, f)));
	}

	SetTones(newTones);
}


bool FPChord::CanCleanupTones() const {
	UInt16	newTones = 0;

	for (int s=NUM_STRINGS; s--;) {
		int f = fretHeld[s];
		if (f >= 0)
			newTones |= BIT(NOTEMOD(guitarPalette->Tone(s, f)));
	}

	return tones != newTones;
}


void FPChord::StretchPattern(bool latter) {
	for (int i=NUM_STRINGS; i--;) {
		UInt32 sbits = 0;
		for (int b=0; b<MAX_BEATS; b++)
			if ((pick[i] & BIT(b)) != 0)
				sbits |= BIT(b*2);

		pick[i] = (latter ? (sbits >> beats) : sbits) & (BIT(beats) - 1);
	}

}


void FPChord::RandomPattern(bool very) {
	ClearPattern();

	if (very) {
		for (int i=MAX_BEATS; i--;)
			pick[RANDINT(0, NUM_STRINGS-1)] |= BIT(i);
	}
	else {
		UInt16	components = RANDINT(1, 6);
		for (int c=components; c--;) {
			UInt16 string	= RANDINT(0, NUM_STRINGS-1);
			UInt16 start	= RANDINT(0, 3);
			UInt16 interval	= (1 << RANDINT(1, 3));

			int t = 0, i = start;
			while (t < MAX_BEATS) {
				pick[string] |= BIT(i % MAX_BEATS);

				i += interval;
				t += interval;
			}
		}
	}
}


void FPChord::NewFingering() {
	guitarPalette->NewFingering(*this);
}


OSErr FPChord::Write(TFile* const file) const {
	return file->Write(this, sizeof(this));
}


OSErr FPChord::WriteOldStyle(TFile* const file) const {
	OldChordInfo chordInfo = {
		EndianU16_NtoB( tones ),
		EndianU16_NtoB( root ),
		EndianU16_NtoB( key ),
		0,					// rootType always 0
		rootLock,
		bracketFlag,
		EndianU16_NtoB(brakLow),
		EndianU16_NtoB(brakHi),
		{	EndianS16_NtoB( fretHeld[0] ),
			EndianS16_NtoB( fretHeld[1] ),
			EndianS16_NtoB( fretHeld[2] ),
			EndianS16_NtoB( fretHeld[3] ),
			EndianS16_NtoB( fretHeld[4] ),
			EndianS16_NtoB( fretHeld[5] ) },
		EndianU16_NtoB( PatternSize() ),
		EndianU16_NtoB( Repeat() ),
		{	EndianU16_NtoB( pick[0] ),
			EndianU16_NtoB( pick[1] ),
			EndianU16_NtoB( pick[2] ),
			EndianU16_NtoB( pick[3] ),
			EndianU16_NtoB( pick[4] ),
			EndianU16_NtoB( pick[5] ) },
		};

	return file->Write(&chordInfo, sizeof(chordInfo));
}


TDictionary* FPChord::GetDictionary() const {
	TDictionary *outDict = new TDictionary();
	
	outDict->SetInteger(kStoredTones, tones);
	outDict->SetInteger(kStoredRoot, root);
	outDict->SetInteger(kStoredKey, key);
	outDict->SetBool(kStoredLock, rootLock);
	outDict->SetBool(kStoredBracketFlag, bracketFlag);
	outDict->SetInteger(kStoredBracketLo, brakLow);
	outDict->SetInteger(kStoredBracketHi, brakHi);
	outDict->SetInteger(kStoredRootStep, rootScaleStep);
	outDict->SetInteger(kStoredRootModifier, rootModifier);

	SInt32 held32[NUM_STRINGS];
	for (int s=NUM_STRINGS; s--;)
		held32[s] = fretHeld[s];

	outDict->SetIntArray(kStoredFingering, held32, NUM_STRINGS);

	SInt32 pick32[NUM_STRINGS];
	for (int s=NUM_STRINGS; s--;)
		pick32[s] = pick[s];

	outDict->SetIntArray(kStoredPattern, pick32, NUM_STRINGS);

	return outDict;
}


#pragma mark -
FPChordGroup::FPChordGroup(const FPChord &chord) {
	for (PartIndex p=DOC_PARTS; p--;)
		chordList[p] = chord;
}


FPChordGroup::FPChordGroup(const FPChordGroup &group) {
	*this = group;
}


FPChordGroup::FPChordGroup(const TDictionary &inDict) {
	UInt16 b = inDict.GetInteger(kStoredGroupBeats);
	UInt16 r = inDict.GetInteger(kStoredGroupRepeat);

	CFArrayRef grpArray = inDict.GetArray(kStoredGroup);
	CFRETAIN(grpArray);

	for (PartIndex p=DOC_PARTS; p--;) {
		TDictionary chordDict( (CFDictionaryRef)CFArrayGetValueAtIndex(grpArray, p) );
		chordList[p] = FPChord(chordDict, b, r);
	}

	CFRELEASE(grpArray);
}


FPChordGroup::FPChordGroup(const CFArrayRef grpArray, UInt16 b, UInt16 r) {
	for (PartIndex p=DOC_PARTS; p--;) {
		TDictionary chordDict( (CFDictionaryRef)CFArrayGetValueAtIndex(grpArray, p) );
		chordList[p] = FPChord(chordDict, b, r);
	}
}


bool FPChordGroup::HasPattern(bool anyChord) const {
	for (PartIndex p=0;p<DOC_PARTS;p++)
		if (chordList[p].HasPattern(anyChord))
			return true;

	return false;
}


bool FPChordGroup::HasFingering() const {
	for (PartIndex p=0;p<DOC_PARTS;p++)
		if (chordList[p].HasFingering())
			return true;

	return false;
}


void FPChordGroup::ResetPattern() {
	for (PartIndex p=DOC_PARTS; p--;)
		chordList[p].ResetPattern();
}


void FPChordGroup::StretchPattern(bool latter) {
	for (PartIndex p=DOC_PARTS; p--;)
		chordList[p].StretchPattern(latter);
}


int FPChordGroup::operator==(const FPChordGroup &inGroup) const {
	for (PartIndex p=DOC_PARTS; p--;)
		if (chordList[p] != inGroup[p])
			return false;

	return true;
}


TDictionary* FPChordGroup::GetDictionary() const {
	TDictionary		*partDictList[DOC_PARTS];
	CFDictionaryRef	partDictRef[DOC_PARTS];

	TDictionary *groupDict = new TDictionary(3);
	groupDict->SetInteger(kStoredGroupBeats, PatternSize());
	groupDict->SetInteger(kStoredGroupRepeat, Repeat());

	for (PartIndex p=0; p<DOC_PARTS; p++) {
		TDictionary *dict = chordList[p].GetDictionary();
		partDictList[p] = dict;
		partDictRef[p] = dict->GetDictionaryRef();
	}

	CFArrayRef groupArrayRef = CFArrayCreate(
						kCFAllocatorDefault,
						(const void **)partDictRef,
						DOC_PARTS,
						&kCFTypeArrayCallBacks );

	groupDict->SetArray(kStoredGroup, groupArrayRef);

	for (PartIndex p=DOC_PARTS; p--;)
		delete partDictList[p];

	return groupDict;
}


#pragma mark -
void FPChordGroupArray::InsertCopyBefore(ChordIndex index, const FPChord &chord) {
	const FPChordGroup group(chord);
	insert_copy(index, group);
}

