/*
 *  FPSprite.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPSprite.h"
#include "FPContext.h"
#include "CGHelper.h"
#include "FPMacros.h"

FPSprite::FPSprite(FPContext *c, CFURLRef imageFile) : context(c) {
	Init();

	if (imageFile)
		LoadImage(imageFile);
}


void FPSprite::Init() {
	layer			= NULL;
	visible			= true;
	userData		= 0;
	disposeFlag		= false;
	width			= 0;
	height			= 0;
	xpos			= 0;
	ypos			= 0;
	rotation		= 0;
	scalex			= 1.0;
	scaley			= 1.0;
	flipx			= false;
	flipy			= false;
	hvel			= 0;
	vvel			= 0;
	frameIndex		= 0;
	frameInterval	= -1;
	framePreviousTime = 0;
	frameStart		= 0;
	frameEnd		= 0;
	frameIncrement	= 1;
}


const FPSprite& FPSprite::operator=(const FPSprite &src) {
	if (&src != this) {

		layer			= NULL;
		framePreviousTime = 0;
		disposeFlag		= false;

		ReleaseFrames();

		context = src.context;
		for (int i=0; i < src.FrameCount(); i++)
			AppendFrame(src.frameArray[i]);

		userData		= src.userData;
		visible			= src.visible;
		xpos			= src.xpos;
		ypos			= src.ypos;
		rotation		= src.rotation;
		scalex			= src.scalex;
		scaley			= src.scaley;
		flipx			= src.flipx;
		flipy			= src.flipy;
		hvel			= src.hvel;
		vvel			= src.vvel;
		frameIndex		= src.frameIndex;
		frameStart		= src.frameStart;
		frameEnd		= src.frameEnd;
		frameIncrement	= src.frameIncrement;
		frameInterval	= src.frameInterval;
	}

	return *this;
}


FPSprite::~FPSprite() {
	ReleaseFrames();
}


void FPSprite::PrependFrame(FPFrame *f) {
	f->Retain();
	frameArray.push_front(f);
	frameEnd++;

	AdjustWidth(f);
}


void FPSprite::AppendFrame(FPFrame *f) {
	f->Retain();
	frameArray.push_back(f);
	frameEnd = FrameCount() - 1;

	AdjustWidth(f);
}


void FPSprite::MakeFlippable() {
	for (UInt16 i=0; i<FrameCount(); i++)
		frameArray[i]->MakeFlippable();
}


void FPSprite::AdjustWidth(FPFrame *f) {
	if (f == NULL) {
		width = height = 0;
		for (UInt16 i=0; i<FrameCount(); i++) {
			f = frameArray[i];
			width = MAX(width, f->Width());
			height = MAX(height, f->Height());
		}
	}
	else {
		width = MAX(width, f->Width());
		height = MAX(height, f->Height());
	}
}


void FPSprite::ReleaseFrames() {
	for (UInt16 i=0; i<FrameCount(); i++)
		frameArray[i]->Release();

	width = height = 0;

	frameArray.clear();
}


void FPSprite::RemoveFrame(UInt16 index) {
	if (index < frameArray.size()) {
		FrameIterator itr = frameArray.begin() + index;
		FPFrame *frame = *itr;
		frameArray.erase(itr);
		frame->Release();
		AdjustWidth();
	}
}


//
// LoadImage(imageFile)
//
// Create a single-frame sprite from an image file
//
void FPSprite::LoadImage(CFURLRef imageFile) {
	ReleaseFrames();

	AppendFrame(new FPFrame(context, imageFile));
}


//
// LoadImageSquares(imageFile)
//
// Given an image, create a set of frames with dimensions
// based on the shorter side of the image.
//
// For example, if you pass an image that is 100 x 10 pixels,
// all frames will be 10 x 10 and there will be 10 frames.
//
void FPSprite::LoadImageSquares(CFURLRef imageFile) {
	ReleaseFrames();

	CGImageRef myImageRef = CGLoadImage(imageFile);

	size_t w = CGImageGetWidth(myImageRef);
	size_t h = CGImageGetHeight(myImageRef);
	size_t dim = MIN(w, h);

	CGRect rect = {{0, 0}, {dim, dim}};

	if (w > h) {
		do {
			FPFrame *frame = new FPFrame(context);
			frame->LoadImageSegment(myImageRef, rect);
			AppendFrame(frame);
			rect.origin.x += dim;
		} while (rect.origin.x < w);
	}
	else {
		rect.origin.y = rect.size.height - dim;
		do {
			FPFrame *frame = new FPFrame(context);
			frame->LoadImageSegment(myImageRef, rect);
			AppendFrame(frame);
			rect.origin.y -= dim;
		} while (rect.origin.y < h);
	}

	CGImageRelease(myImageRef);
}


//
// LoadImageGrid(file, xsize, ysize)
//
// Given an image file and the size of each grid segment,
// create a sprite with all frames of the given size.
//
void FPSprite::LoadImageGrid(CFURLRef imageFile, int xsize, int ysize) {
	if (ysize == 0) ysize = xsize;

	ReleaseFrames();

	CGImageRef myImageRef = CGLoadImage(imageFile);

	size_t	w = CGImageGetWidth(myImageRef),
			h = CGImageGetHeight(myImageRef);

	CGRect rect = {{0, h-ysize}, {xsize, ysize}};

	do {
		do {
			FPFrame *frame = new FPFrame(context);
			frame->LoadImageSegment(myImageRef, rect);
			AppendFrame(frame);
			rect.origin.x += xsize;
		} while (rect.origin.x < w);

		rect.origin.x = 0;
		rect.origin.y -= ysize;

	} while (rect.origin.y >= 0);

	CGImageRelease(myImageRef);
}


//
// LoadImageMatrix(file, xcount[, ycount])
//
// Given an image file and the number of horizonal and vertical divisions,
// create a sprite with all frames being the same size.
//
// This works like LoadImageGrid, except instead of specifying the
// size of the grid segments you specify the number of segments
// and the segment size is calculated by dividing up the full size.
//
void FPSprite::LoadImageMatrix(CFURLRef imageFile, int xcount, int ycount) {
	ReleaseFrames();

	CGImageRef myImageRef = CGLoadImage(imageFile);

	size_t	w = CGImageGetWidth(myImageRef),
			h = CGImageGetHeight(myImageRef);

	int xsize = w / xcount;
	int ysize = ycount ? h / ycount : xsize;

	CGRect rect = {{0, h-ysize}, {xsize, ysize}};

	do {
		do {
			FPFrame *frame = new FPFrame(context);
			frame->LoadImageSegment(myImageRef, rect);
			AppendFrame(frame);
			rect.origin.x += xsize;
		} while (rect.origin.x < w);

		rect.origin.x = 0;
		rect.origin.y -= ysize;

	} while (rect.origin.y >= 0);

	CGImageRelease(myImageRef);
}


//
// LoadImageStrips(imageFile, sizeArray, horiz)
//
// Given an image file and an array of strip-widths, create a set of frames
// By default strips are assumed to be vertical, but pass YES for horizontal strips
//
void FPSprite::LoadImageStrips(CFURLRef imageFile, CFArrayRef sizeArray, bool horiz) {
	ReleaseFrames();

	CGImageRef myImageRef = CGLoadImage(imageFile);

	size_t	w = CGImageGetWidth(myImageRef),
			h = CGImageGetHeight(myImageRef);

	CGRect rect =  {{0, 0}, {w, h}};
	int sizeCount = CFArrayGetCount(sizeArray);

	if (horiz) {
		rect.origin.y = h;
		for (int i=0; i < sizeCount; i++) {
			int hi = 0;
			CFNumberRef numRef = (CFNumberRef)CFArrayGetValueAtIndex(sizeArray, (CFIndex)i);
			if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
				(void)CFNumberGetValue(numRef, kCFNumberIntType, &hi);

			if (hi > 0) {
				rect.origin.y -= hi;
				rect.size.height = hi;
				FPFrame *frame = new FPFrame(context);
				frame->LoadImageSegment(myImageRef, rect);
				AppendFrame(frame);
				if (rect.origin.y <= 0)
					break;
			}
		}
	}
	else {
		for (int i=0; i < sizeCount; i++) {
			int wi = 0;
			CFNumberRef numRef = (CFNumberRef)CFArrayGetValueAtIndex(sizeArray, (CFIndex)i);
			if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
				(void)CFNumberGetValue(numRef, kCFNumberIntType, &wi);

			if (wi > 0) {
				rect.size.width = wi;
				FPFrame *frame = new FPFrame(context);
				frame->LoadImageSegment(myImageRef, rect);
				AppendFrame(frame);
				rect.origin.x += wi;
				if (rect.origin.x >= w)
					break;
			}
		}
	}
	CGImageRelease(myImageRef);
}


CGContextRef FPSprite::MakeCGContext(int width, int height) {
	ReleaseFrames();
	FPFrame *frame = new FPFrame(context);
	CGContextRef ctx = frame->MakeCGContext(width, height);
	AppendFrame(frame);
	return ctx;
}


void FPSprite::LoadCGContext(CGContextRef inContext) {
	ReleaseFrames();
	AppendFrame(new FPFrame(context, inContext));
}


void FPSprite::SetHandle(float x, float y) {
	for (int i=0; i<FrameCount(); i++)
		frameArray[i]->SetHandle(x, y);
}


void FPSprite::Draw() {
	if (visible && !disposeFlag && FrameCount() > 0)
		frameArray[frameIndex]->Draw(xpos, ypos, rotation, scalex, scaley, flipx, flipy);
}


void FPSprite::ProcessAndAnimate() {
	if (!disposeFlag) {
		xpos += hvel;
		ypos += vvel;

		double t = context->runningTimeCount;

//		fprintf(stderr, "Sprite Got Time = %.0lf (interval=%.0lf) (prev=%.0lf) (delta=%.0lf)\r", t, frameInterval, framePreviousTime, t - framePreviousTime);

		if (frameInterval >= 0) {
//			fprintf(stderr, "...\r");
			if (t - framePreviousTime > frameInterval * 10)
				framePreviousTime = t;

			while ( (t - framePreviousTime) >= frameInterval ) {
				if (frameInterval == 0)
					framePreviousTime = t;
				else
					framePreviousTime += frameInterval;

				Animate();
			}
		}

//		if (frameInterval >= 0 && --frameICounter <= 0)
//		{
//			frameICounter = frameInterval;
//			Animate();
//		}
	}
}


void FPSprite::Animate() {
	frameIndex += frameIncrement;
	if (frameIncrement < 0) {
		if (frameIndex < frameEnd)
			frameIndex = frameStart;
	}
	else {
		if (frameIndex > frameEnd)
			frameIndex = frameStart;
	}

	if (frameIndex < 0)
		frameIndex = 0;
	else if (frameIndex >= FrameCount())
		frameIndex = FrameCount() - 1;
}

