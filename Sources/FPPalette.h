/*
 *  FPPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *  The FPPalette adds a handler to the basic window that
 *  makes it magnetize towards the edges of the screen.
 *
 */

#ifndef FPPALETTE_H
#define FPPALETTE_H

#include "FPWindow.h"

class FPPalette;

#include <list>
typedef std::list<FPPalette*>		FPPaletteList;
typedef FPPaletteList::iterator		FPPaletteIterator;

enum {
	kDockingDistance 	= 10,
	kTitleBarFudge		= 16,
	MAGNET_EDGES		= 0x0001,
	MAGNET_PALETTES		= 0x0002,
	MAGNET_INNER		= 0x0004,
	MAGNET_ALL			= 0x0007
};


class FPPalette : public FPWindow {
	private:
		UInt32					behavior;
		Point					originalPosition;
		static FPPaletteList	paletteList;

	public:
						FPPalette(CFStringRef windowName);
		virtual			~FPPalette();

		virtual UInt32	Type()						{ return 'Base'; }

		void			Init() {}
		virtual void	Draw() {}
		void			ToggleVisiblity()			{ if (IsVisible()) { ReallyHide(); } else { Show(); Select(); } }

		OSStatus		HandleEvent(EventHandlerCallRef inRef, TCarbonEvent &event);
		virtual void	Magnetize(TCarbonEvent &event);

		static void		RefreshDisplayData(bool displaysChanged=false);

};

#endif
