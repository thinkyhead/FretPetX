/*
 *  FPMidiHelper.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 7/2/11.
 *  Copyright 2012 Thinkyhead Software. All rights reserved.
 *
 */

#include "FPMidiHelper.h"

UInt16 FPMidiHelper::gs_instruments[kDefinedGSInstruments + kDefinedDrums] = {
	16385,16393,16401,16409,16410,16417,16425,16433,16441,
	167,186,189,209,210,227,231,233,249,250,
	251,252,253,254,255,256,359,377,379,380,
	381,382,383,384,507,508,509,510,511,512,
	635,637,638,639,763,765,766,767,894,1022,
	1025,1026,1027,1028,1029,1030,1031,1036,1037,1039,
	1041,1042,1044,1046,1049,1050,1051,1052,1053,1055,
	1056,1063,1064,1065,1073,1075,1086,1087,1088,1105,
	1106,1132,1140,1141,1142,1143,1150,1167,1271,1278,
	2049,2053,2054,2055,2065,2068,2073,2074,2077,2088,
	2111,2112,3077,3079,4113,4114,4121,4149 };

UInt16 FPMidiHelper::FauxGMToTrueGM(UInt16 fauxGM) {
	int i = fauxGM - kFirstFauxDrum;
	return (i >= 0 && i < kDefinedDrums + kDefinedGSInstruments) ? gs_instruments[i] : fauxGM;
}

UInt16 FPMidiHelper::TrueGMToFauxGM(UInt16 trueGM) {
	for (int i=0; i<kDefinedDrums; i++) {
		if (trueGM == gs_instruments[i]) return kFirstFauxDrum + i;
	}
	UInt16 *gs_array = gs_instruments + kDefinedDrums;
	int lo = 0, hi = kDefinedGSInstruments - 1;
	for(;;) {
		int mid = (lo + hi) / 2, val = gs_array[mid];
		if (trueGM == val) return mid + kFirstFauxGS;
		else {
			if (lo >= hi) break;
			else if (trueGM < val) hi = mid - 1;
			else if (trueGM > val) lo = mid + 1;
			if (lo > hi) break;
		}
	}

	return trueGM;
}

void FPMidiHelper::GroupAndIndexForInstrument(UInt16 inInst, UInt16 &outGroup, UInt16 &outIndex) {
	UInt16 fauxGM = TrueGMToFauxGM(inInst);
	outGroup = ((fauxGM <= kLastFauxGM) ? fauxGM - 1 : (fauxGM <= kLastFauxDrum) ? kFirstFauxDrum - 1 : kFirstFauxGS - 1) / 8 + 1;
	outIndex = fauxGM - ((fauxGM <= kLastFauxGM) ? kFirstFauxGM : (fauxGM <= kLastFauxDrum) ? kFirstFauxDrum : kFirstFauxGS) + 1;
}
