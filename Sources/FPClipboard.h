/*!
 *  @file FPClipboard.h
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#ifndef FPCLIPBOARD_H
#define FPCLIPBOARD_H

#include "FPChord.h"

//-----------------------------------------------
//
// FPClipboard
//
class FPClipboard {
	private:
		int					copiedPart;			// Part that was showing at copy
		FPChordGroupArray	chordGroupArray;	// All four voices are copied

	public:
						FPClipboard();
						~FPClipboard() {}

		void				Init();
		void				Copy(FPChordGroupArray &sourceArray, int part, ChordIndex start, ChordIndex end);
		FPChordGroupArray&	Contents()			{ return chordGroupArray; }
		inline int			Part()				{ return copiedPart; }
		inline ChordIndex	Size()				{ return chordGroupArray.size(); }
		inline bool			HasChords()			{ return Size() > 0; }
		void				PasteContents(FPChordGroupArray &dest, ChordIndex afterIndex);
};

#endif
