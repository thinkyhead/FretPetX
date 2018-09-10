/*
 *  FPPaletteGL.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPPaletteGL.h"

#include "TWindow.h"
#include "FPMacros.h"
#include "FPSpriteLayer.h"
#include "FPSprite.h"
#include "FPContext.h"

FPPaletteGL::FPPaletteGL(CFStringRef windowName, UInt16 numLayers) : FPPalette(windowName) {
	Init();

	for (int i=numLayers; i--;)
		AppendLayer();
}


FPPaletteGL::~FPPaletteGL() {
	DisposeSpriteAssets();	// After this the layers are deleted
}


void FPPaletteGL::Init() {
	processBkgd = true;
	timerUPP = NULL;
	spriteLoop = NULL;
	context = new FPContext(this);
}


void FPPaletteGL::AppendLayer(FPSpriteLayer *layer) {
	if (layer == NULL)
		layer = new FPSpriteLayer(context);

	layerArray.push_back(layer);
}


void FPPaletteGL::AppendSprite(FPSprite *sprite, int layer) {
	if (sprite->Layer() == NULL) {
		FPSpriteLayer *l = layerArray[layer];
		sprite->SetLayer(l);
		l->AppendSprite(sprite);
	}
}


FPSprite* FPPaletteGL::NewSprite(CFURLRef imageFile, int layer) {
	FPSprite *sprite = new FPSprite(context, imageFile);
	if (layer >= 0) AppendSprite(sprite, layer);
	return sprite;
}


FPSprite* FPPaletteGL::NewSprite(FPSprite *s, int layer) {
	FPSprite *sprite = new FPSprite(*s);
	if (layer >= 0) AppendSprite(sprite, layer);
	return sprite;
}


void FPPaletteGL::DisposeAllSprites() {
	for (UInt16 i=0; i<LayerCount(); i++)
		layerArray[i]->clear();
}


void FPPaletteGL::Focus() {
	FPPalette::Focus();
}


void FPPaletteGL::Resized() {
	context->DidResize();
}


void FPPaletteGL::SetContentSize( short x, short y ) {
	TWindow::SetContentSize(x, y);
	context->DidResize();
}


void FPPaletteGL::SetClearColor(float color[4]) {
	context->SetClearColor(color);
}


void FPPaletteGL::Draw() {
	context->Focus();

	glClear(GL_COLOR_BUFFER_BIT);

	for (UInt16 i=0; i<LayerCount(); i++)
		layerArray[i]->Draw();

	context->SwapBuffers();
}


void FPPaletteGL::ProcessAndAnimateSprites() {
	context->UpdateTime();

	if (IsVisible())
		Draw();

	for (UInt16 i=0; i<LayerCount(); i++)
		layerArray[i]->ProcessAndAnimate();
}


void FPPaletteGL::StartSpriteTimer() {
	if (spriteLoop == NULL) {
		timerUPP = NewEventLoopTimerUPP(FPPaletteGL::SpriteLoopTimerProc);

		(void)InstallEventLoopTimer(
			GetMainEventLoop(),
			0.0, 1.0 / 30.0,
			timerUPP,
			this,
			&spriteLoop
			);
	}
}


void FPPaletteGL::DisposeSpriteTimer() {
	if (spriteLoop != NULL) {
		(void)RemoveEventLoopTimer(spriteLoop);
		DisposeEventLoopTimerUPP(timerUPP);
		spriteLoop = NULL;
		timerUPP = NULL;
	}
}


void FPPaletteGL::SpriteLoopTimerProc( EventLoopTimerRef timer, void *palette ) {
	FPPaletteGL *pal = ((FPPaletteGL*)palette);
	if (pal->IsVisible() || pal->ProcessInBackground())
		pal->ProcessAndAnimateSprites();
}

void FPPaletteGL::SetNextFireTime(EventTimerInterval next) {
	SetEventLoopTimerNextFireTime(spriteLoop, next);
}
