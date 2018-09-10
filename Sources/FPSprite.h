/*
 *  FPSprite.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPSPRITE_H
#define FPSPRITE_H

#include "FPFrame.h"
#include "FPContext.h"

class FPSpriteLayer;

class FPSprite {
protected:
	FPSpriteLayer	*layer;
	FPFrameArray	frameArray;
	FPContext		*context;
	bool			visible;
	bool			flipx, flipy;
	float			rotation;
	float			xpos, ypos;
	float			hvel, vvel;
	float			scalex, scaley;
	float			width, height;
	int				frameIndex, frameStart, frameEnd, frameIncrement;
//		int				frameICounter;
	int				disposeFlag;
	double			framePreviousTime;
	double			frameInterval;
	UInt32			userData;

public:
	FPSprite() { Init(); }
	FPSprite(FPContext *c, CFURLRef imageFile=NULL);
	FPSprite(const FPSprite &src) { Init(); *this = src; }

	virtual ~FPSprite();

	void Init();

	void AppendFrame(FPFrame *f);
	void PrependFrame(FPFrame *f);
	void RemoveFrame(UInt16 index);

	inline void SetLayer(FPSpriteLayer *l)	{ layer = l; }
	inline FPSpriteLayer* Layer() const		{ return layer; }

	inline void MarkDisposed()				{ disposeFlag = 2; }
	inline bool IsDisposed() const			{ return disposeFlag == 2; }
	inline void MarkForDisposal()			{ disposeFlag = 1; }
	inline bool MarkedForDisposal() const	{ return disposeFlag == 1; }

	void AdjustWidth(FPFrame *f=NULL);

	void ReleaseFrames();

	inline void	SetVisible(bool v)			{ visible = v; }
	inline void	SetHidden(bool v)			{ SetVisible(!v); }
	inline void	Show()						{ SetVisible(true); }
	inline void	Hide()						{ SetVisible(false); }

	inline void	Move(float x, float y)		{ xpos = x; ypos = y; }

	void		SetHandle(float x, float y);
	inline void	SetHandle(UInt16 i, float x, float y) { frameArray[i]->SetHandle(x, y); }

	inline void	SetRotation(float r)		{ rotation = r; }

	inline void	SetScaleX(float x)			{ scalex=x; }
	inline void	SetScaleY(float y)			{ scaley=y; }
	inline void	SetScale(float x, float y)	{ scalex=x; scaley=y; }

	void		MakeFlippable();
	inline void	SetFlipX(bool x)			{ flipx=x; }
	inline void	SetFlipY(bool y)			{ flipy=y; }
	inline void	SetFlip(bool x, bool y)		{ flipx=x; flipy=y; }

	//! A single image to make a single frame
	void LoadImage(CFURLRef imageFile);

	//! A grid of regular-sized images
	void LoadImageGrid(CFURLRef imageFile, int xsize, int ysize=0);

	//! A matrix of regular-sized images
	void LoadImageMatrix(CFURLRef imageFile, int xcount, int ycount=0);

	//! One dimension is an even multiple of the other
	void LoadImageSquares(CFURLRef imageFile);

	//! An image with a single color dividing the frames
	void LoadImageKeyed(CFURLRef imageFile);

	//! An image broken up into strips
	void LoadImageStrips(CFURLRef imageFile, CFArrayRef sizeArray, bool horiz);

	//! A pre-existing CGContext for on-the-fly frames
	void LoadCGContext(CGContextRef inContext);

	//! An empty CGContext for on-the-fly frames
	CGContextRef MakeCGContext(int width, int height);

	float Width() const			{ return width * scalex; }
	float Height() const		{ return height * scaley; }
	float BaseWidth() const		{ return width; }
	float BaseHeight() const	{ return height; }
	int	 FrameCount() const		{ return (int)frameArray.size(); }

	void SetUserData(UInt32 u)	{ userData = u; }
	UInt32 UserData() const		{ return userData; }

	void Draw();

	inline void SetAnimationFrameIndex(int i)			{ if (i>=frameStart && i<=frameEnd) { frameIndex = i; } else { frameIndex = frameStart; } }
	inline void ConstrainFrameIndex()					{ if (frameIndex < frameStart || frameIndex > frameEnd) frameIndex = frameStart; }
	inline void SetAnimationInterval(double t)			{ frameInterval = t; ResetAnimationCounter(); }
	inline void SetAnimationFrameStart(int s)			{ frameStart = s; ConstrainFrameIndex(); }
	inline void SetAnimationFrameEnd(int e)				{ frameEnd = e; ConstrainFrameIndex(); }
	inline void SetAnimationFrameRange(int s, int e)	{ frameStart = s; frameEnd = e; ConstrainFrameIndex(); }
	inline void SetAnimationFrameIncrement(int i)		{ frameIncrement = i; ConstrainFrameIndex(); }
	inline void SetAnimation(double t, int s, int e, int i=1) { frameInterval = t; SetAnimationFrameRange(s, e); frameIncrement = i; ConstrainFrameIndex(); }
	inline void ResetAnimationCounter()					{ framePreviousTime = context->runningTimeCount; }

	inline int AnimationFrameIndex()					{ return frameIndex; }

	virtual void ProcessAndAnimate();
	virtual void Process() {}
	virtual void Animate();
	virtual const FPSprite& operator=(const FPSprite &src);
};

typedef FPSprite* FPSpritePtr;

#endif
