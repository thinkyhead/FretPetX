/*
 *  FPPaletteSW.h
 *
 *	FretPet X
 *  Copyright © 2007 Scott Lahteine. All rights reserved.
 *
 *	The FPPaletteSW class represents a palette window that utilizes
 *	a SpriteWorld to store and display its contents.
 *
 */

#ifndef FPPALETTESW_H
#define FPPALETTESW_H

#include "FPPalette.h"

class FPPaletteSW : public FPPalette {
	private:
		EventLoopTimerUPP		timerUPP;			// UPP for an Event Loop Timer
		EventLoopTimerRef		spriteLoop;			// Event Loop timer object

	protected:
		SpriteWorldPtr			world;				// My SpriteWorld
		UInt16					layerCount;			// Number of layers
		SpriteLayerPtr			layer[4];			// Layer pointers

	public:
		bool					worldActive;		// Flag to stop processing temporarily

								FPPaletteSW(CFStringRef windowTitle, UInt16 numLayers);
		virtual					~FPPaletteSW();

		virtual UInt32			Type() { return 'SW  '; }

		virtual void			InitSpriteWorld(UInt16 numLayers);
		void					ReinitSpriteWorld();
		virtual void			DisposeSpriteWorld();

		virtual const Rect		SizeForView() { return GetContentSize(); }
		virtual inline Rect		SizeForVirtual() { return SizeForView(); }
		void					Focus() { SWSetPortToBackground(world); }
		virtual void			Draw();
		SpriteLayerPtr			Layer(UInt16 index) { return layer[index]; }

		void					AddLayer();
		inline SpriteWorldPtr	SpriteWorld() { return world; }
		virtual inline void		LockSprites() { SWLockSpriteWorld(world); worldActive = true; }
		inline void				UnlockSprites()  { worldActive = false; SWUnlockSpriteWorld(world); }
		virtual void			ProcessAndAnimateSprites();

		void					StartSpriteWorldTimer();
		void					DisposeSpriteWorldTimer();
		static void				SpriteLoopTimerProc( EventLoopTimerRef timer, void *palette );

		SpritePtr				CreateSpriteFromCICN(SInt16 cIconID, SInt16 maxFrames);
		SpritePtr				CreateSpriteFromImage(const FSSpec *spritespec, short frameDimension, const FSSpec *maskspec=NULL);
		SpritePtr				CreateSpriteFromImage(const char *spritefile, short frameDimension, const char *maskfile=NULL, bool abspath=false);
		FramePtr				CreateFrameFromImage(const char *filename);
		GWorldPtr				CreateGWorldFromImage(const char *spritefile);

		virtual void			SetBackgroundImage(const char *filename);
		void					ColorFill(const RGBColor &color);
		void					PictureFill(SInt16 pictID, Rect* cliprect=NULL);
		void					DrawSpriteToBack(SpritePtr pSprite);

		void					RefreshBackground();
};

#endif
