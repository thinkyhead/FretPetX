/*
 *  FPPaletteGL.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPPALETTEGL_H
#define FPPALETTEGL_H

#include "FPPalette.h"
#include "TWindow.h"
#include "TObjectDeque.h"

class FPContext;
class FPSprite;

#include "FPSpriteLayer.h"
typedef TObjectDeque<FPSpriteLayer>	FPLayerArray;

#define SPRITE_TIMER_DELAY (1000 / 60)

class FPPaletteGL : public FPPalette {
	protected:
		Boolean				processBkgd;
		EventLoopTimerUPP	timerUPP;			// UPP for an Event Loop Timer
		EventLoopTimerRef	spriteLoop;			// Event Loop timer object
		FPLayerArray		layerArray;
		FPContext			*context;

	public:
								FPPaletteGL(CFStringRef windowTitle, UInt16 numLayers);
		virtual					~FPPaletteGL();

		virtual void			DisposeSpriteAssets() {}

		virtual UInt32			Type() { return 'GL  '; }
		virtual void			Focus();

		void					Init();
		virtual void			Draw();
		void					SetContentSize( short x, short y );
		void					Resized();
		void					SetClearColor(float color[4]);

		void					AppendLayer(FPSpriteLayer *layer=NULL);
		inline FPSpriteLayer*	Layer(UInt16 i) { return (i < LayerCount()) ? layerArray[i] : NULL; }
		inline unsigned			LayerCount()	{ return layerArray.size(); }

		inline void				DisposeLayer(unsigned index)	{ layerArray.erase(index); }
		inline void				DisposeAllLayers()				{ layerArray.clear(); }
		void					DisposeAllSprites();

		void					AppendSprite(FPSprite *sprite, int layer=0);
		FPSprite*				NewSprite(CFURLRef imageFile=NULL, int layer=-1);
		FPSprite*				NewSprite(FPSprite *s, int layer=-1);

		inline Boolean			ProcessInBackground() { return processBkgd; }

		virtual void			ProcessAndAnimateSprites();
		void					StartSpriteTimer();
		void					DisposeSpriteTimer();
		static void				SpriteLoopTimerProc( EventLoopTimerRef timer, void *palette );
		void					SetNextFireTime(EventTimerInterval next);
};

#endif