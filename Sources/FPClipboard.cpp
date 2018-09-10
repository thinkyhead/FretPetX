/*
 *  FPClipboard.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPClipboard.h"
#include "FPChord.h"
#include "FPDocument.h"

//-----------------------------------------------
//
// FPClipboard		Constructor
//
FPClipboard::FPClipboard() {
}

//-----------------------------------------------
//
// Copy
//
void FPClipboard::Copy(FPChordGroupArray &sourceArray, int part, ChordIndex start, ChordIndex end) {
	copiedPart = part;
	chordGroupArray.clear();
	chordGroupArray.append_copies(sourceArray, start, end);
}

//-----------------------------------------------
//
// PasteContents
//
void FPClipboard::PasteContents(FPChordGroupArray &dest, ChordIndex afterIndex) {
	dest.insert_copies(chordGroupArray, afterIndex);
}

