/*
 *  FPGlobals.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPApplication.h"
#include "FPChord.h"

//
// Static Music Info
//
short theCScale[]   = { NOTE_C, NOTE_D, NOTE_E, NOTE_F, NOTE_G, NOTE_A, NOTE_B };

//
//	Intervals of the above scales
//
short interval[][14] = {
	{ 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1 },   // Ionian
	{ 2, 1, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2 },   // Dorian
	{ 1, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2 },   // Phrygian
	{ 2, 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2, 1 },   // Lydian
	{ 2, 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 2 },   // Mixolydian
	{ 2, 1, 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 2, 2 },   // Aeolian
	{ 1, 2, 2, 1, 2, 2, 2, 1, 2, 2, 1, 2, 2, 2 },   // Locrian
	{ 2, 1, 2, 2, 1, 3, 1, 2, 1, 2, 2, 1, 3, 1 },   // Harmonic
	{ 2, 1, 2, 2, 2, 2, 1, 2, 1, 2, 2, 2, 2, 1 },   // Melodic
	{ 1, 3, 1, 1, 3, 1, 2, 1, 3, 1, 1, 3, 1, 2 }	// Oriental
};


//
// Indices of letter-names for sharp / flat keys
//
short letterIndex[][OCTAVE] = {
	{ 0, 0, 1, 1, 2, 3, 3, 4, 4, 5, 5, 6 },			// C C D E E F F G G A A B		(#)
	{ 0, 1, 1, 2, 2, 3, 4, 4, 5, 5, 6, 6 }			// C D D E E F G G A A B B		(b)
};

//
// Quick reference for black keys
//
bool isBlack[OCTAVE] = { 0, 1, 0, 1, 0, 0, 1, 0, 1, 0, 1, 0 };

//
// Scale shorthand
//
StringPtr   modalS[] = {
	(StringPtr)"\pION",
	(StringPtr)"\pDOR",
	(StringPtr)"\pPHR",
	(StringPtr)"\pLYD",
	(StringPtr)"\pMIX",
	(StringPtr)"\pAEO",
	(StringPtr)"\pLOC",
	(StringPtr)"\pHAR",
	(StringPtr)"\pMEL"
};


//
// Unique Notes
//
char const *UNoteS[][7] = {
	{ "Cbbb","Dbbb","Ebbb","Fbbb","Gbbb","Abbb","Bbbb" },
	{ "Cbb ","Dbb ","Ebb ","Fbb ","Gbb ","Abb ","Bbb " },
	{ "Cb  ","Db  ","Eb  ","Fb  ","Gb  ","Ab  ","Bb  " },
	{ "C   ","D   ","E   ","F   ","G   ","A   ","B   " },
	{ "C#  ","D#  ","E#  ","F#  ","G#  ","A#  ","B#  " },
	{ "C## ","D## ","E## ","F## ","G## ","A## ","B## " },
	{ "C###","D###","E###","F###","G###","A###","B###" }
};


//
// Names of functions and intervals
//
char const* NFPrimary[]	= { "R", "b2", "2", "b3", "3", "4", "b5", "5", "#5", "6", "b7", "7" };
char const* NFSecondary[]	= { "1", "b9", "9", "#9", "10", "11", "#11", "5", "b13", "13", "#13", "?" };
char const* NFSolFa[]		= { "DO", "ra", "RE", "ma", "MI", "FA", "si", "SOL", "le", "LA", "ta", "TI" };
char const* NFSolFaSharp[]= { "DO", "de", "RE", "reh", "MI", "FA", "fe", "SOL", "se", "LA", "le", "TI" };
char const* NFSolFaFlat[]	= { "DO", "ra", "RE", "ma", "MI", "FA", "sa", "SOL", "la", "LA", "ta", "TI" };
char const* NFInterval[]	= { "---", "MIN 2", "MAJ 2", "MIN 3", "MAJ 3", "PER 4", "DIM 5", "PER 5", "MIN 6", "MAJ 6", "MIN 7", "MAJ 7" };
char const* NFModal[]		= { "0", "m2", "2", "m3", "3", "4", "-5", "5", "m6", "6", "m7", "7" };


//
// Colors
//
RGBColor rgbBlack	= { 0x0000, 0x0000, 0x0000 };
RGBColor rgbGray4	= { 0x4000, 0x4000, 0x4000 };
RGBColor rgbGray8	= { 0x7FFF, 0x7FFF, 0x7FFF };
RGBColor rgbGray9	= { 0x9000, 0x9000, 0x9000 };
RGBColor rgbGray12	= { 0xCC00, 0xCC00, 0xCC00 };
RGBColor rgbGray14	= { 0xDDDD, 0xDDDD, 0xDDDD };
RGBColor rgbWhite	= { 0xFFFF, 0xFFFF, 0xFFFF };
RGBColor rgbRed		= { 0xFFFF, 0x0000, 0x0000 };
RGBColor rgbDkRed	= { 0x7FFF, 0x0000, 0x0000 };
RGBColor rgbGreen	= { 0x0000, 0x64AF, 0x11B0 };
RGBColor rgbLtGreen	= { 0x0000, 0xF000, 0x1200 };
RGBColor rgbBlue	= { 0x0000, 0x0000, 0xB000 };
RGBColor rgbLtBlue	= { 0xA000, 0xA000, 0xFFFF };
RGBColor rgbLtBlue2	= { 0xE000, 0xE000, 0xFFFF };
RGBColor rgbBrown	= { 0x9000, 0x9000, 0x0000 };
RGBColor rgbYellow	= { 0xFFFF, 0xFFFF, 0x6000 };

RGBColor rgbMaj		= { 0x7000, 0x2000, 0x2000 };
RGBColor rgbMin		= { 0x2000, 0x2000, 0x8000 };
RGBColor rgbDim		= { 0x2000, 0x7000, 0x2000 };
RGBColor rgbOth		= { 0x5000, 0x5000, 0x5000 };
RGBColor rgbMajBack	= { 0xC000, 0x4000, 0x4000 };
RGBColor rgbMinBack	= { 0x4000, 0x4000, 0xE000 };
RGBColor rgbDimBack	= { 0x3000, 0xC000, 0x3000 };
RGBColor rgbOthBack	= { 0x8000, 0x8000, 0x8000 };
RGBColor rgbTone	= { 0xC000, 0xC000, 0xC000 };
RGBColor rgbCurr	= { 0x4000, 0x6000, 0x4000 };
RGBColor rgbShadow0	= { 0x6000, 0x6000, 0x6000 };
RGBColor rgbShadow1	= { 0x2000, 0x6000, 0x2000 };
RGBColor rgbChord1	= { 0x0000, 0xFFFF, 0x0000 };
RGBColor rgbChord2	= { 0xCC00, 0xCC00, 0xCC00 };

RGBColor rgbPalette	= rgbGray14;
RGBColor rgbOdd		= { 0xEE00, 0xEE00, 0xFFFF };
RGBColor rgbEven	= rgbWhite;
RGBColor rgbKeyFits	= { 0x3000, 0x6000, 0x6000 };

RGBColor rgbNatural	= { 0xA000, 0x2000, 0x2000 };

RGBColor rgbTickGreen1	= { 0x4000, 0x9000, 0x4000 };
RGBColor rgbTickGreen2	= rgbGreen;
RGBColor rgbTickGray1	= rgbGray9;
RGBColor rgbTickGray2	= rgbGray4;


