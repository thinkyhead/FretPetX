/*!
 *  @file FPMidiHelper.h
 *
 *  FretPetX
 *
 *  Created by Scott Lahteine on 7/2/11.
 *  Copyright 2012 Thinkyhead Software. All rights reserved.
 *
 */

class FPMidiHelper {
private:
	static UInt16 gs_instruments[kDefinedGSInstruments + kDefinedDrums];

public:
	/*! GM Number conversion for older files and UI indices
	 */
	static UInt16 FauxGMToTrueGM(UInt16 fauxGM);

	/*! GM Number conversion for older files and UI indices
	 */
	static UInt16 TrueGMToFauxGM(UInt16 trueGM);
	static void	GroupAndIndexForInstrument(UInt16 inInst, UInt16 &outGroup, UInt16 &outIndex);
};
