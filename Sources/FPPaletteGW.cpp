/*
 *  FPPaletteGW.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TWindow.h"
#include "FPPaletteGW.h"

FPPaletteGW::FPPaletteGW(CFStringRef windowName, UInt16 numLayers) : FPPalette(windowName)) {
	Init();

	for (int i=numLayers; i--;)
		AddLayer();
}



FPPaletteGW::~FPPaletteGW() {
}



void FPPaletteGW::Init() {
}



void FPPaletteGW::AddLayer() {
	gworldCount++;
}



void FPPaletteGW::Draw() {
}


