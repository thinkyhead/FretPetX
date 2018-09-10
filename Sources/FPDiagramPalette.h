/*
 *  FPDiagramPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPDIAGRAMPALETTE_H
#define FPDIAGRAMPALETTE_H

#include "FPPalette.h"
#include "TControls.h"

//-----------------------------------------------
//
//	FPDiagramControl
//
class FPDiagramControl : public TCustomControl {
	public:
		FPDiagramControl(WindowRef wind);

		bool			Draw( const Rect &bounds );
		ControlPartCode	HitTest( Point where );
		bool			Track( MouseTrackingResult eventType, Point where );
		void			GetPositionFromPoint( Point where, SInt16 *string, SInt16 *fret );
};

#pragma mark -
//
// FPDiagramPalette
//
class FPDiagramPalette : public FPPalette {
	private:
		FPDiagramControl	*diagramControl;

	public:
		FPDiagramPalette();
		~FPDiagramPalette() {}

		void		Draw();

		void		UpdateDiagram()			{ diagramControl->DrawNow(); }
		void		DrawDiagram(const Rect &bounds);
		void		DrawRootType();

		const CFStringRef PreferenceKey()	{ return CFSTR("diagram"); }
};


extern FPDiagramPalette *diagramPalette;


#endif
