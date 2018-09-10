/*
 *  FPContext.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPContext.h"
#include "CGHelper.h"

extern long systemVersion;

#include "TWindow.h"
#include "TError.h"

#if defined(__BIG_ENDIAN__)
	#define ARGB_IMAGE_TYPE GL_UNSIGNED_INT_8_8_8_8_REV
#else
	#define ARGB_IMAGE_TYPE GL_UNSIGNED_INT_8_8_8_8
#endif


FPContext::FPContext(TWindow *inWindow) {
	UpdateTime();

	ctx = NULL;
	textureID = 0;
	maxTextureSize = 256;
	window = inWindow;
   	port = inWindow->Port();

    GLint			attrib_wind[] = { AGL_RGBA, AGL_DOUBLEBUFFER, AGL_ACCELERATED, AGL_ALL_RENDERERS, AGL_NO_RECOVERY, AGL_NONE };
	GLboolean		glOK;

	verify_action( (Ptr) aglGetVersion != (Ptr) kUnresolvedCFragSymbolAddress, throw TError(1000, CFSTR("OpenGL library not loaded.")) );

	Rect windRect = inWindow->GetContentSize();
	width = windRect.right - windRect.left;
	height = windRect.bottom - windRect.top;

	inWindow->Focus();
	LocalToGlobal((Point *)&windRect.top);
	LocalToGlobal((Point *)&windRect.bottom);
	GDHandle device = GetMaxDevice(&windRect);
	if (device == NULL) device = GetMainDevice();

    // Choose an rgb pixel format
	AGLPixelFormat	fmt = aglChoosePixelFormat((AGLDevice *) &device, 1, attrib_wind);
	verify_action( fmt!=NULL, throw TError(1000, CFSTR("No RGB pixel format found.")) );

	// Create an AGL context
	ctx = aglCreateContext(fmt, (AGLContext) NULL);
	verify_action( ctx!=NULL, throw TError(1000, CFSTR("Could not create AGL context.")) );

	// Attach the window to the context
	glOK = aglSetDrawable(ctx, port);
	verify_action( glOK, throw TError(1000, CFSTR("Could not attach window to AGL context.")) );

	// Make the context the current context
	glOK = aglSetCurrentContext(ctx);
	verify_action( glOK, throw TError(1000, CFSTR("Could not activate AGL context.")) );

	// Pixel format is no longer needed
	aglDestroyPixelFormat(fmt);

	textureEnabled = glIsEnabled(GL_TEXTURE_2D);
	blendEnabled = glIsEnabled(GL_BLEND);

	// setup 2D mode
	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
 	glLoadIdentity();
	glOrtho(0, width, height, 0, -1.0, 1.0);

	glShadeModel(GL_FLAT);
	glDisable(GL_DEPTH_TEST);
	glEnable(GL_COLOR_MATERIAL);
	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);			// A default light that points straight on

	SetTextureID(0);
	SetTextureEnabled(true);
	SetBlendEnabled(true);

	glClearColor(1, 1, 1, 1);
	glClear(GL_COLOR_BUFFER_BIT);

	glGetIntegerv(GL_MAX_TEXTURE_SIZE, &maxTextureSize);

	// some cards (Rage 128) report 1024, but can't really handle it
	if (maxTextureSize >= 1024)
		maxTextureSize = 512;

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	// Prime SetBlendFunction
	GLint s, d;
	glGetIntegerv(GL_BLEND_SRC, &s);
	glGetIntegerv(GL_BLEND_DST, &d);
	sFactor = s;
	dFactor = d;

	SetBlendFunction(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
}


void FPContext::SetClearColor(float color[4]) {
	Focus();
	glMatrixMode(GL_PROJECTION);
	glClearColor(color[0], color[1], color[2], color[3]);
}


void FPContext::DidResize() {
	Update();
}


void FPContext::Update() {
	Focus();

	Rect windRect = window->GetContentSize();
	width = windRect.right - windRect.left;
	height = windRect.bottom - windRect.top;

	glViewport(0, 0, width, height);
	glMatrixMode(GL_PROJECTION);
 	glLoadIdentity();
	glOrtho(0, width, height, 0, -1.0, 1.0);

	aglUpdateContext(ctx);
}


int FPContext::MakeTextureWithImageFile(CFURLRef imageFile, float *dw, float *dh, UInt16 *tw, UInt16 *th, CGImageRef *outImage) {
	CGImageRef imageRef = CGLoadImage(imageFile);

	GLuint texture = MakeTextureWithImage(imageRef, dw, dh, tw, th);

	if (outImage != NULL)
		*outImage = imageRef;
	else
		CGImageRelease(imageRef);

	return texture;
}


int FPContext::MakeTextureWithImage(CGImageRef imageRef, float *dw, float *dh, UInt16 *tw, UInt16 *th) {
	return MakeTextureWithImageSegment(imageRef, CGRectMake(0, 0, CGImageGetWidth(imageRef), CGImageGetHeight(imageRef)), dw, dh, tw, th);
}


int FPContext::MakeTextureWithCGContext(CGContextRef inContext, float *dw, float *dh, UInt16 *tw, UInt16 *th) {
	GLuint texture = 0;

#if defined(CGBitmapContextCreateImage)
	if (systemVersion >= 0x1040) {
		CGImageRef img = CGBitmapContextCreateImage(inContext);
		texture = MakeTextureWithImage(img, dw, dh, tw, th);
		CFRELEASE(img);
	}
	else
#endif
	{	//	In 10.3.9 and earlier just do a direct data copy

		float	sw = CGBitmapContextGetWidth(inContext),
				sh = CGBitmapContextGetHeight(inContext);

		*dw = sw; *dh = sh;
		UInt16	expw = (UInt16)sw - 1; expw |= expw >> 1; expw |= expw >> 2; expw |= expw >> 4; expw |= expw >> 8; expw++;
		UInt16	exph = (UInt16)sh - 1; exph |= exph >> 1; exph |= exph >> 2; exph |= exph >> 4; exph |= exph >> 8; exph++;
		*tw = expw; *th = exph;

		// Create an enlarged context, if needed
		void *txData, *srcData = CGBitmapContextGetData(inContext);
		CGContextRef gc = NULL;
		if (sw != expw || sh != exph) {
			gc = CGCreateContext(expw, exph, &txData);
			const register size_t linesize = (size_t)(sw * 4);
			const register size_t widesize = (size_t)(expw * 4);
			UInt8 *src = (UInt8*)srcData, *dst = (UInt8*)txData;
			for (int y=(int)sh; y--;) {
				memcpy(dst, src, linesize);
				src += linesize; dst += widesize;
			}
		}
		else
			txData = srcData;

		Focus();
		glPixelStorei(GL_UNPACK_ROW_LENGTH, expw);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glGenTextures(1, &texture);
		glBindTexture(GL_TEXTURE_2D, texture);
		SetTextureAlias(false);
		SetTextureRepeat(false);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, expw, exph, 0, GL_BGRA_EXT, ARGB_IMAGE_TYPE, txData);

		if (txData != srcData) {
			CGContextRelease(gc);
			free(txData);
		}
	}

	return texture;
}


int FPContext::MakeTextureWithImageSegment(CGImageRef imageRef, CGRect segment, float *dw, float *dh, UInt16 *tw, UInt16 *th) {
	GLuint texture = 0;
	float	sw = CGRectGetWidth(segment), sh = CGRectGetHeight(segment);
	UInt16	expw = (UInt16)sw - 1; expw |= expw >> 1; expw |= expw >> 2; expw |= expw >> 4; expw |= expw >> 8; expw++;
	UInt16	exph = (UInt16)sh - 1; exph |= exph >> 1; exph |= exph >> 2; exph |= exph >> 4; exph |= exph >> 8; exph++;

	*dw = sw; *dh = sh;
	*tw = expw; *th = exph;
	void *myData;

	CGRect dstRect = {{-segment.origin.x, -segment.origin.y}, {CGImageGetWidth(imageRef), CGImageGetHeight(imageRef)}};

	CGContextRef gc = CGCreateContext(expw, exph, &myData);
	CGContextTranslateCTM(gc, 0, exph-sh);	// move y upwards
   	CGContextClipToRect(gc, CGRectMake(0, 0, sw, sh));
	CGContextDrawImage(gc, dstRect, imageRef);
	CGContextFlush(gc);

	Focus();
	glPixelStorei(GL_UNPACK_ROW_LENGTH, expw);
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glGenTextures(1, &texture);
	glBindTexture(GL_TEXTURE_2D, texture);
	SetTextureAlias(false);
	SetTextureRepeat(false);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, expw, exph, 0, GL_BGRA_EXT, ARGB_IMAGE_TYPE, myData);

	CGContextRelease(gc);
 	free(myData);

	return texture;
}


void FPContext::UpdateTime() {
	UnsignedWide curMicrosecondsWide;
	Microseconds( &curMicrosecondsWide );
	runningTimeCount = ((((double) curMicrosecondsWide.hi) * 0xffffffff) + curMicrosecondsWide.lo) * 0.001;

//	fprintf(stderr, "runningTimeCount = %.0lf\r", runningTimeCount);
}


