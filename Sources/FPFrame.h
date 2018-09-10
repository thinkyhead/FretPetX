/*
 *  FPFrame.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPFRAME_H
#define FPFRAME_H

class FPContext;

class FPFrame {
private:
	int				use_count;			//!< Number of times this frame is in use
	FPContext		*context;			//!< Sharing textures between contexts requires creating all contexts up-front, so it's not done here
	int				gl_texture;			//!< OpenGL texture
	int				gl_list;			//!< OpenGL display list
	int				gl_flips;			//!< OpenGL display lists for flipped views
	UInt16			texw,				//!< OpenGL texture size width (powers of 2)
					texh;				//!< OpenGL texture size height (powers of 2)
	CGContextRef	cg_context;			//!< The frame may own a drawable context
	CGImageRef		cg_image;		//!< Source Image, if preserved
	float			xhandle,			//!< x handle for relative positioning
					yhandle;			//!< y handle for relative positioning
	float			width,				//!< width of image area
					height;				//!< height of image area

public:
	FPFrame(FPContext *c);
	FPFrame(FPContext *c, CFURLRef imageFile, bool keep=false);
	FPFrame(FPContext *c, CGContextRef contextRef);
	~FPFrame();

	void	Init();

	int LoadImageFile(CFURLRef imageFile, bool keep=false);
	int LoadImageSegment(CGImageRef imageRef, CGRect segment);
	int LoadCGContext(CGContextRef contextRef);
	void RefreshTexture();

	CGContextRef MakeCGContext(int width, int height);

	inline void SendGeometry(bool flipx, bool flipy);

	void InitDisplayList();
	void MakeFlippable();
	void DisposeDisplayLists();

	//! Throw away the source image
	void DisposeImage();

	//! Throw away the drawable context
	void DisposeCGContext();

	//! Throw away the OpenGL texture
	void DisposeTexture();

	//! Set the rotation/motion handle
	void SetHandle(float x, float y);

	//! Move the rotation/motion handle to the center
	inline void CenterHandle() { SetHandle(floor(width / 2.0f), floor(height / 2.0f)); }

	//! Width of the frame
	float Width() const { return width; }

	//! Height of the frame
	float Height() const { return height; }

	//! Send the texture to the output buffer
	void Draw(float x, float y, float rot=0.0, float xscale=1.0, float yscale=1.0, bool flipx=false, bool flipy=false);

	//! Increase reference counter
	int Retain() { return ++use_count; }

	//! Decrease reference counter and delete if zero
	int Release() { if (--use_count <= 0) delete this; return use_count; }
};

#include <deque>
typedef std::deque<FPFrame*> FPFrameArray;
typedef FPFrameArray::iterator FrameIterator;

#endif
