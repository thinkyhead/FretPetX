/*!
	@file CGHelper.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "CGHelper.h"

extern long systemVersion;

#define CG_RGBCOLOR(x)		(float)(x)->red / 65535.0, (float)(x)->green / 65535.0, (float)(x)->blue / 65535.0


#pragma mark -
CGImageRef CGLoadImage(CFURLRef url) {
	CGImageRef imageRef = NULL;

#if defined(CGImageSourceRef)
	if (systemVersion >= 0x1040) {
		CGImageSourceRef imageSourceRef = CGImageSourceCreateWithURL(url, NULL);
		if (imageSourceRef) {
			imageRef = CGImageSourceCreateImageAtIndex(imageSourceRef, 0, NULL);
			CFRELEASE(imageSourceRef);
		}
	}
	else
#endif
	{
		CGDataProviderRef provider = CGDataProviderCreateWithURL(url);
		if (provider) {
			CFStringRef ext = CFURLCopyPathExtension(url);

			if (ext) {
				if (CFStringCompare(ext, CFSTR("jpg"), kCFCompareCaseInsensitive) == kCFCompareEqualTo || CFStringCompare(ext, CFSTR("jpeg"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
					imageRef = CGImageCreateWithJPEGDataProvider(provider, NULL, true, kCGRenderingIntentDefault);
				else if (CFStringCompare(ext, CFSTR("png"), kCFCompareCaseInsensitive) == kCFCompareEqualTo)
					imageRef = CGImageCreateWithPNGDataProvider(provider, NULL, true, kCGRenderingIntentDefault);
				else
					fprintf(stderr, "CGLoadImage: Only JPEG and PNG images are supported!\n");

				CFRELEASE(ext);
			}
			CGDataProviderRelease(provider);
		}
	}

	return imageRef;
}

//-----------------------------------------------
//
//	CGGetImageDimensions
//
CGPoint CGGetImageDimensions(CFURLRef url) {
	CGImageRef imageRef = CGLoadImage(url);
	CGPoint size = { CGImageGetWidth(imageRef), CGImageGetHeight(imageRef) };
	CGImageRelease(imageRef);
	return size;
}

//-----------------------------------------------
//
//	CGCreateContext
//
CGContextRef CGCreateContext(int width, int height, void **outData) {
	void *myData = calloc(width * 4, height);
	CGColorSpaceRef space = CGGetColorSpace();
	CGContextRef ctx = CGBitmapContextCreate(myData, width, height, 8, width*4, space, kCGImageAlphaPremultipliedFirst);
	if (outData) *outData = myData;
	return ctx;
}

#define kGenericRGBProfilePathStr "/System/Library/ColorSync/Profiles/Generic RGB Profile.icc"
 
CMProfileRef OpenGenericProfile() {
	static CMProfileRef cachedRGBProfileRef = NULL;

	// Create the profile reference only once
	if (cachedRGBProfileRef == NULL) {
		OSStatus			err;
		CMProfileLocation	loc;

		loc.locType = cmPathBasedProfile;
		strcpy(loc.u.pathLoc.path, kGenericRGBProfilePathStr);

		err = CMOpenProfile(&cachedRGBProfileRef, &loc);

		if (err != noErr) {
			cachedRGBProfileRef = NULL;
			// Log a message to the console
			fprintf (stderr, "Couldn't open generic profile due to error %d\n", (int)err);
		}
	}

	if (cachedRGBProfileRef) {
		// Clone the profile reference so that the caller has
		// their own reference, not our cached one.
		CMCloneProfileRef(cachedRGBProfileRef);
	}

	return cachedRGBProfileRef;
}

//-----------------------------------------------
//
//  CGGetColorSpace
//
CGColorSpaceRef CGGetColorSpace() {
	static CGColorSpaceRef genericRGBColorSpace = NULL;

	if (genericRGBColorSpace == NULL) {
		CMProfileRef genericRGBProfile = OpenGenericProfile();

		if (genericRGBProfile) {
			genericRGBColorSpace = CGColorSpaceCreateWithPlatformColorSpace(genericRGBProfile);
			if (genericRGBColorSpace == NULL)
				fprintf(stderr, "Couldn't create the generic RGB color space\n");

			CMCloseProfile(genericRGBProfile); 
		}
	}
	return genericRGBColorSpace;
}

//-----------------------------------------------
//
//	CGDraw3DBox
//
//	Make a nifty-looking 3D box, either raised or indented
//
//	This assumes the context is already established
//
void CGDraw3DBox(CGContextRef gc, CGRect cgRect, bool indent, RGBColor *med) {
	if (cgRect.size.width == 0 || cgRect.size.height == 0)
		return;

	RGBColor	lite, dark;

	lite.red	= (med->red + 0xFFFF) / 2;
	lite.green	= (med->green + 0xFFFF) / 2;
	lite.blue	= (med->blue + 0xFFFF) / 2;

	dark.red	= (med->red + 0x2000) / 2;
	dark.green	= (med->green + 0x2000) / 2;
	dark.blue	= (med->blue + 0x2000) / 2;

	CGContextSetRGBFillColor(gc, CG_RGBCOLOR(med), 1);

	CGContextBeginPath(gc);
	CGContextAddRect(gc, cgRect);
//	CGContextClosePath(gc);
	CGContextFillPath(gc);

	float	left	= cgRect.origin.x,	// left
			bottom	= cgRect.origin.y,	// bottom
			right	= cgRect.origin.x + cgRect.size.width,
			top		= cgRect.origin.y + cgRect.size.height;

	RGBColor *tlColor = indent ? &dark : &lite;
	CGContextSetRGBStrokeColor(gc, CG_RGBCOLOR(tlColor), 1.0f);
	CGContextSetLineWidth(gc, 1.0f);
	CGContextSetShouldAntialias(gc, false);

	CGPoint	points[3];
	points[0].x = right - 1;
	points[0].y = top;
	points[1].x = left;
	points[1].y = top;
	points[2].x = left;
	points[2].y = bottom + 1;

	CGContextBeginPath(gc);
	CGContextAddLines(gc, points, 3);
	CGContextStrokePath(gc);

	RGBColor *brColor = indent ? &lite : &dark;
	CGContextSetRGBStrokeColor(gc, CG_RGBCOLOR(brColor), 1);

	points[0].x = right;
	points[0].y = top - 1;
	points[1].x = right;
	points[1].y = bottom;
	points[2].x = left + 1;
	points[2].y = bottom;

	CGContextBeginPath(gc);
	CGContextAddLines(gc, points, 3);
	CGContextStrokePath(gc);
}

//-----------------------------------------------
//
//	CGSetFont
//
void CGSetFont(CGContextRef gc, CFStringRef cfString) {
	ATSFontRef	atsFont = ATSFontFindFromName(cfString, kATSOptionFlagsDefault);
	CGFontRef	cgFont = CGFontCreateWithPlatformFont((void*)&atsFont);
	CGContextSetFont(gc, cgFont);
}

//-----------------------------------------------
//
//	CGMakeRoundedRectPath
//
void CGMakeRoundedRectPath(CGContextRef gc, CGRect &inRect, float radius) {
	float	left	= inRect.origin.x,
			right	= inRect.origin.x + inRect.size.width,
			top		= inRect.origin.y + inRect.size.height,
			bottom	= inRect.origin.y;

	CGContextBeginPath(gc);

	// left-bottom
	CGContextMoveToPoint(gc, left, bottom + radius);

	// left-top
	CGContextAddLineToPoint(gc, left, top - radius);
	CGContextAddQuadCurveToPoint(gc, left, top, left + radius, top);

	// right-top
	CGContextAddLineToPoint(gc, right - radius, top);
	CGContextAddQuadCurveToPoint(gc, right, top, right, top - radius);

	// right-bottom
	CGContextAddLineToPoint(gc, right, bottom + radius);
	CGContextAddQuadCurveToPoint(gc, right, bottom, right - radius, bottom);

	// left-bottom
	CGContextAddLineToPoint(gc, left + radius, bottom);
	CGContextAddQuadCurveToPoint(gc, left, bottom, left, bottom + radius);

	CGContextClosePath(gc);
}

