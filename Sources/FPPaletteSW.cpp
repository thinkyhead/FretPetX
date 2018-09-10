/*
 *  FPPaletteSW.cpp
 *
 *	FretPet X
 *  Copyright © 2007 Scott Lahteine. All rights reserved.
 *
 */

#include "FPUtilities.h"
#include "FPPaletteSW.h"

#define kHardwareSprites		true		// Uses OpenGL/RAVE (true) or BlitPixie/CopyBits (false).
#define kUseAlphaChannels		true		// 1 bit (false) or 8 bit mask (true)

//-----------------------------------------------
//
//  FPPaletteSW					* CONSTRUCTOR *
//
FPPaletteSW::FPPaletteSW(CFStringRef windowName, UInt16 numLayers) : FPPalette(windowName)) {
	layerCount	= 0;
	spriteLoop	= NULL;
	worldActive	= false;
	world		= NULL;

	InitSpriteWorld(numLayers);
}


//-----------------------------------------------
//
//  FPPaletteSW					* DESTRUCTOR *
//
FPPaletteSW::~FPPaletteSW() {
	DisposeSpriteWorld();
}


//-----------------------------------------------
//
//  DisposeSpriteWorld
//
void FPPaletteSW::DisposeSpriteWorld() {
	DisposeSpriteWorldTimer();
	SWDisposeSpriteWorld(&world);
	layerCount = 0;
	worldActive = false;
}


//-----------------------------------------------
//
//  InitSpriteWorld
//
void FPPaletteSW::InitSpriteWorld(UInt16 numLayers) {
	Rect	view = SizeForView();
	Rect	virt = SizeForVirtual();

	if (world)
		DisposeSpriteWorld();

	OSErr err = SWCreateSpriteWorldFromWindow(&world, Window(), &view, &virt, 0);
	if ( noErr != err )
		throw FPError(err, CFSTR("Failed to create SpriteWorld."));

//	fprintf(stderr, "Created world [%08X] with Window Frame [%08X]\n", world, world->windowFrameP);

#if kHardwareSprites
	err = SWInitHardwareSpriteWorld( world, false, kActivateHardwareNow );
	if ( noErr != err )
		throw FPError(err, CFSTR("Sorry, your graphics card is unsupported by FretPet."));
#endif

	for (int i=numLayers; i--;)
		AddLayer();

    SWLockSpriteWorld(world);

	SWSetSpriteWorldOffscreenDrawProc(world, ::BlitPixieRectDrawProc);
	SWSetDoubleRectDrawProc(world, ::BlitPixieDoubleRectDrawProc);
	SWSetPartialMaskDrawProc(world, ::BlitPixiePartialMaskDrawProc);

	SWSetSpriteWorldMaxFPS(world, 30);
//	SWSetTranslucencyAdjustments(world, true);

#if kHardwareSprites
    SWSetSpriteWorldOffscreenHardwareDrawProc(world, SWHardwareDrawProc);
    SWSetTileMaskHardwareDrawProc(world, SWHardwareDrawProc);
    SWSetPartialMaskHardwareDrawProc(world, SWHardwareDrawProc);
#endif

	StartSpriteWorldTimer();
}


//-----------------------------------------------
//
//  ReinitSpriteWorld
//
void FPPaletteSW::ReinitSpriteWorld() {
	InitSpriteWorld(layerCount);
}


#pragma mark -
//-----------------------------------------------
//
//  AddLayer
//
void FPPaletteSW::AddLayer() {
	if (SWCreateSpriteLayer(&layer[layerCount]) == noErr) {
		SWAddSpriteLayer(world, layer[layerCount]);
		layerCount++;
	}
}


//-----------------------------------------------
//
//  CreateSpriteFromImage
//
SpritePtr FPPaletteSW::CreateSpriteFromImage(const FSSpec *spritespec, short frameDimension, const FSSpec *maskspec) {
	SpritePtr	newSprite	= NULL;
	bool		hasMask		= (maskspec != NULL);

	OSErr err = SWCreateSpriteFromFSSpecs(
		world,
		&newSprite,
		NULL,
		spritespec,
		(FSSpec*)maskspec,
		frameDimension,
		0,
		hasMask ? (kUseAlphaChannels ? kAlphaChannelMask : kFatMask) : kNoMask
		);

	if (err == noErr) {
		SWLockSprite(newSprite);

		SWSetSpriteDrawProc(newSprite, hasMask ? (kUseAlphaChannels ? BlitPixieAlphaMaskDrawProc : BlitPixieMaskDrawProc) : BlitPixieRectDrawProc);

		#if kHardwareSprites
			err = SWLoadHardwareSprite(world, newSprite);
			verify_action(err==noErr, throw FPError(err, CFSTR("Unable to create a hardware sprite.")) );
			SWSetSpriteHardwareDrawProc(newSprite, SWHardwareDrawProc);
		#endif	
	}

	return newSprite;
}


//-----------------------------------------------
//
//  CreateSpriteFromImage
//
SpritePtr FPPaletteSW::CreateSpriteFromImage(const char *spritefile, short frameDimension, const char *maskfile, bool abspath) {
	SpritePtr	newSprite	= NULL;
	bool		hasMask		= (maskfile != NULL);

	const char	*path1 = abspath ? spritefile : ImagePath(spritefile);
	const char	*path2 = hasMask ? (abspath ? maskfile : ImagePath(maskfile)) : NULL;

	OSErr err = SWCreateSpriteFromSingleFile(
		world,
		&newSprite,
		NULL,
		path1,
		path2,
		frameDimension,
		0,
		hasMask ? (kUseAlphaChannels ? kAlphaChannelMask : kFatMask) : kNoMask
		);

	if (err == noErr) {
		SWLockSprite(newSprite);

		SWSetSpriteDrawProc(newSprite, hasMask ? (kUseAlphaChannels ? BlitPixieAlphaMaskDrawProc : BlitPixieMaskDrawProc) : BlitPixieRectDrawProc);

		#if kHardwareSprites
			err = SWLoadHardwareSprite(world, newSprite);
			verify_action(err==noErr, throw FPError(err, CFSTR("Unable to create a hardware sprite.")) );
			SWSetSpriteHardwareDrawProc(newSprite, SWHardwareDrawProc);
		#endif	
	}

	return newSprite;
}


//-----------------------------------------------
//
//  CreateGWorldFromImage
//
GWorldPtr FPPaletteSW::CreateGWorldFromImage(const char *filename) {
	char		*temp = ImagePath(filename);
	char		*pathString = new char[strlen(temp)+1]; strcpy(pathString, temp);
	GWorldPtr	newGWorld = NULL;

	OSErr err = SWCreateGWorldFromFile(world, pathString, &newGWorld);

	delete [] pathString;
	return newGWorld;
}


//-----------------------------------------------
//
//  CreateFrameFromImage
//
FramePtr FPPaletteSW::CreateFrameFromImage(const char *filename) {
	OSErr		err;
	FramePtr	newFrame = NULL;
	GWorldPtr	newGWorld = CreateGWorldFromImage(filename);

	if (newGWorld)
		err = SWCreateFrameFromGWorld(world, &newFrame, newGWorld, NULL, kNoMask, false);

	return newFrame;
}


//-----------------------------------------------
//
//  CreateSpriteFromCICN
//
SpritePtr FPPaletteSW::CreateSpriteFromCICN(SInt16 cIconID, SInt16 maxFrames) {
	SpritePtr	newSprite = NULL;
	OSErr err;

//	err = SWCreateSpriteFromCicnResource(
//		world,
//		&newSprite,
//		NULL,
//		cIconID,
//		maxFrames,
//		kFatMask);

//	SWLockSprite(newSprite);
//	SWSetSpriteDrawProc(newSprite, kUseAlphaChannels ? BlitPixieAlphaMaskDrawProc : BlitPixieMaskDrawProc);

#if kHardwareSprites
    err = SWLoadHardwareSprite(world, newSprite);
    verify_action(err==noErr, throw FPError(err, CFSTR("Unable to create a hardware sprite for the CICN.")) );
    SWSetSpriteHardwareDrawProc(newSprite, SWHardwareDrawProc);
#endif	

	return newSprite;
}


//-----------------------------------------------
//
//  Draw
//
void FPPaletteSW::Draw() {
	if (worldActive) {
		#if kHardwareSprites
			::SWUpdateHardwareSpriteWorld(world);
		#else
			::SWUpdateSpriteWorld(world, true);
		#endif

		::DrawControls(Window());
	}
}


//-----------------------------------------------
//
// ProcessAndAnimateSprites
//
void FPPaletteSW::ProcessAndAnimateSprites() {
	::SWProcessSpriteWorld(world);

	if (IsVisible()) {
		#if kHardwareSprites

			::SWUpdateHardwareSpriteWorld(world);

		#else

			GrafPtr	oldPort;
			GetPort(&oldPort);
			::SWAnimateSpriteWorld(world);
			SetPort(oldPort);

		#endif
	}
}

//-----------------------------------------------
//
//  StartSpriteWorldTimer
//
void FPPaletteSW::StartSpriteWorldTimer() {
	if (spriteLoop == NULL) {
		timerUPP = NewEventLoopTimerUPP(FPPaletteSW::SpriteLoopTimerProc);

		(void)InstallEventLoopTimer(
			GetMainEventLoop(),
			1.0 / 1000.0,
			1.0 / 30.0,
			timerUPP,
			this,
			&spriteLoop
			);
	}
}


//-----------------------------------------------
//
//  DisposeSpriteWorldTimer
//
void FPPaletteSW::DisposeSpriteWorldTimer() {
	if (spriteLoop != NULL) {
		(void)RemoveEventLoopTimer(spriteLoop);
		DisposeEventLoopTimerUPP(timerUPP);
		spriteLoop = NULL;
		timerUPP = NULL;
	}
}

//-----------------------------------------------
//
//	SpriteLoopTimerProc
//
void FPPaletteSW::SpriteLoopTimerProc( EventLoopTimerRef timer, void *palette ) {
	if (((FPPaletteSW*)palette)->worldActive)
		((FPPaletteSW*)palette)->ProcessAndAnimateSprites();
}


//-----------------------------------------------
//
//	ColorFill
//
void FPPaletteSW::ColorFill(const RGBColor &color) {
	GrafPtr			oldPort;

	GetPort(&oldPort);
	SWSetPortToBackground(world);
	RGBForeColor(&color);
	PaintRect(&world->backRect);
	ForeColor(blackColor);
	SetPort(oldPort);

	RefreshBackground();
}

//-----------------------------------------------
//
//	PictureFill
//
void FPPaletteSW::PictureFill(SInt16 pictID, Rect* cliprect) {
	OSErr			err = noErr;
	SInt16			hh, vv;
	SpritePtr		texture = NULL;
	GrafPtr			oldPort;

// TODO: Load a picture file instead
//	err = SWCreateSpriteFromPictResource(world, &texture, NULL, pictID, pictID, 1, kFatMask);
	require_noerr( err, NoPattern );

	SWLockSprite(texture);

	#if kHardwareSprites
	SWHardwareFillWithPattern(texture->curFrameP, world->backFrameP, &texture->curFrameP->frameRect, &world->backFrameP->frameRect);
	#endif

/*
	SWMoveSprite(texture, 0, 0);
	hh = texture->curFrameP->frameRect.right;
	vv = texture->curFrameP->frameRect.bottom;

	GetPort(&oldPort);
	SWSetPortToBackground(world);

	// If we have a clip then use it
	RgnHandle oldClip;
	if (cliprect) {
		oldClip = NewRgn();
		GetClip(oldClip);
		ClipRect(cliprect);
	}

	// Draw patterns over the whole background
	do {
		SWMoveSprite(texture, 0, texture->destFrameRect.top);
		do {
			if (cliprect) {
				if (cliprect->left <= texture->destFrameRect.right
					&& cliprect->right >= texture->destFrameRect.left
					&& cliprect->top <= texture->destFrameRect.bottom
					&& cliprect->bottom >= texture->destFrameRect.top) {
					DrawSpriteToBack(texture);
				}
			}
			else {
				DrawSpriteToBack(texture);
			}

			SWOffsetSprite(texture, hh, 0);
		} while (texture->destFrameRect.left < world->backRect.right);
		SWOffsetSprite(texture, 0, vv);
	} while (texture->destFrameRect.top < world->backRect.bottom);

	if (cliprect) {
		SetClip(oldClip);
		DisposeRgn(oldClip);
	}

	SetPort(oldPort);
*/
	SWDisposeSprite(&texture);
	RefreshBackground();

NoPattern:
	return;
}


//-----------------------------------------------
//
//	RefreshBackground
//
void FPPaletteSW::RefreshBackground() {
	#if kHardwareSprites
	(void)SWQuickReloadHardwareFrame(world, world->backFrameP);
	#endif

    SWFlagRectAsChanged(world, &world->backFrameP->frameRect);
}


//-----------------------------------------------
//
//	SetBackgroundImage
//
//	Load a single image onto the background of the
//	window's spriteworld. The image must be the original
//	size of the spriteworld. In other words, it must be
//	exactly the same dimensions as the window content.
//
void FPPaletteSW::SetBackgroundImage(const char *filename) {
	OSStatus	err;
	FSSpec		fileSpec;

	err = SWGetApplicationRelativeFSSpec(ImagePath(filename), &fileSpec, kAppBundleRelativeInternal);
	verify_action(err==noErr, throw FPError(err, CFSTR("Unable to load background.")) );

	SWUpdateGWorldFromFSSpec(world->backFrameP->framePort, &fileSpec);
	RefreshBackground();
}


//-----------------------------------------------
//
//	DrawSpriteToBack
//
//  Stamp a sprite onto the background of the window
//
void FPPaletteSW::DrawSpriteToBack(SpritePtr pSprite) {
	(*pSprite->frameDrawProc)
		(pSprite->curFrameP, world->backFrameP,
		&pSprite->curFrameP->frameRect, &pSprite->destFrameRect);

	RefreshBackground();
}


