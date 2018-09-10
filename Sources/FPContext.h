/*
 *  FPContext.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPCONTEXT_H
#define FPCONTEXT_H

#include <OpenGL/gl.h>
#include <OpenGL/glext.h>
#include <AGL/agl.h>

class TWindow;

class FPContext {
private:
	TWindow		*window;
	AGLContext	ctx;
	int			width, height;
	CGrafPtr	port;
	GLboolean	blendEnabled;
	GLboolean	textureEnabled;
	GLuint		textureID;
	GLint		maxTextureSize;
	GLenum		sFactor, dFactor;

public:
	double		runningTimeCount;

public:
				FPContext(TWindow *window);
				~FPContext();

	inline void	SwapBuffers()	{ aglSwapBuffers(ctx); }

	GLboolean	Focus()		{ return ctx ? aglSetCurrentContext(ctx) : false; }
	void		Update();
	void		DidResize();
	void		Begin();
	void		End();

	void		SetBlendEnabled(bool on) { if (blendEnabled != on) { (blendEnabled = on) ? glEnable(GL_BLEND) : glDisable(GL_BLEND); } }
	void		SetBlendFunction(GLenum sf, GLenum df) { if (sFactor != sf || dFactor != df) { glBlendFunc(sf,df); sFactor = sf; dFactor = df; } }

	static void		SetTextureAlias(GLboolean alias) {
					GLenum q = alias ? GL_LINEAR : GL_NEAREST;
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, q);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, q);
					}

	static void		SetTextureRepeat(GLboolean rept) {
					GLenum q = rept ? GL_REPEAT : GL_CLAMP;
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, q);
					glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, q);
					}

	void		SetClearColor(float color[4]);

	void		SetTextureEnabled(bool on) { if (textureEnabled != on) { (textureEnabled = on) ? glEnable(GL_TEXTURE_2D) : glDisable(GL_TEXTURE_2D); } }
	void		SetTextureID(GLuint tid) { if (textureID != tid) { glBindTexture(GL_TEXTURE_2D, textureID = tid); } }

	int			MakeTextureWithImageFile(CFURLRef imageFile, float *w, float *h, UInt16 *tw, UInt16 *th, CGImageRef *outImage=NULL);
	int			MakeTextureWithImage(CGImageRef imageRef, float *dw, float *dh, UInt16 *tw, UInt16 *th);
	int			MakeTextureWithImageSegment(CGImageRef imageRef, CGRect segment, float *w, float *h, UInt16 *tw, UInt16 *th);
	int			MakeTextureWithCGContext(CGContextRef contextRef, float *dw, float *dh, UInt16 *tw, UInt16 *th);

	// Call if the screen resolution, screen depth, or window size changes
	void		UpdateContext();

	//! Update the clock
	void UpdateTime();
};

#endif
