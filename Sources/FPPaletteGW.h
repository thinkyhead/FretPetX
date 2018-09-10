/*
 *  FPPaletteGW.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPPALETTEGW_H
#define FPPALETTEGW_H

#include "FPPalette.h"

class FPPaletteGW : public FPPalette {
	private:
		UInt16		gworldCount;
		GWorldPtr   gworld[4];

	public:
		FPPaletteGW(CFStringRef windowTitle, UInt16 numGWorlds);
		virtual ~FPPaletteGW();

		virtual UInt32	Type() { return 'GW  '; }

		void			Init();
		virtual void	Draw();

		void			AddLayer();
};

#endif