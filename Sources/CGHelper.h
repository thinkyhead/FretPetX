/*!
	@file CGHelper.h

	Functions for some common Core Graphics operations

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef CGHELPER_H
#define CGHELPER_H

CGImageRef			CGLoadImage(CFURLRef url);
CGPoint				CGGetImageDimensions(CFURLRef url);
CGContextRef		CGCreateContext(int width, int height, void **outData=NULL);
CGColorSpaceRef		CGGetColorSpace();
void				CGSetFont(CGContextRef gc, CFStringRef cfString);
void				CGMakeRoundedRectPath(CGContextRef gc, CGRect &inRect, float radius);
void				CGDraw3DBox(CGContextRef gc, CGRect cgRect, bool indent, RGBColor *med);

#endif
