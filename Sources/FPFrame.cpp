/*
 *  FPFrame.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPFrame.h"
#include "FPContext.h"
#include "CGHelper.h"

FPFrame::FPFrame(FPContext *c) : context(c) {
	Init();
}


FPFrame::FPFrame(FPContext *c, CFURLRef imageFile, bool keep) : context(c) {
	Init();
	LoadImageFile(imageFile, keep);
}


FPFrame::FPFrame(FPContext *c, CGContextRef contextRef) : context(c) {
	Init();
	(void)LoadCGContext(contextRef);
}


FPFrame::~FPFrame() {
	DisposeDisplayLists();
	DisposeTexture();
	DisposeImage();
}


void FPFrame::Init() {
	use_count		= 0;
	cg_image		= NULL;
	cg_context		= NULL;
	gl_texture		= 0;
	gl_list			= 0;
	gl_flips		= 0;
	texw = texh		= 0;
	xhandle			= 0;
	yhandle			= 0;
	width			= 0;
	height			= 0;
}


void FPFrame::Draw(float x, float y, float rot, float xscale, float yscale, bool flipx, bool flipy) {
	GLubyte aTint[] = { 255, 255, 255, 255 };
//	MultiplyColorQuads(tint, tintAdd, aTint);

//	context->SetTextureEnabled(true);
//	context->SetBlendEnabled(true);

	glMatrixMode(GL_MODELVIEW);
	glPushMatrix();

	glTranslatef(x, y, 0.0f);
	glRotatef(rot, 0.0f, 0.0f, 1.0f);
	glScalef(xscale, yscale, 1.0f);
	glColor4ubv(aTint);

	if (flipx || flipy)
		glCallList(gl_flips-1 + (flipx ? 1 : 0) + (flipy ? 2 : 0));
	else
		glCallList(gl_list);

	glPopMatrix();
}


void FPFrame::SendGeometry(bool flipx, bool flipy) {
	float tx, ty, l, r, t, b;

	if (flipx) {
		tx = (width-texw)-xhandle;
		l = 1.0; r = 0.0;
	} else {
		tx = -xhandle;
		l = 0.0; r = 1.0;
	}

	if (flipy) {
		ty = (height-texh)-yhandle;
		t = 1.0; b = 0.0;
	} else {
		ty = -yhandle;
		t = 0.0; b = 1.0;
	}

	glBindTexture(GL_TEXTURE_2D, gl_texture);
	glTranslatef(tx, ty, 0.0f);
	glBegin(GL_QUADS);
	glTexCoord2f(l, t); glVertex2f(0.0f, 0.0f);
	glTexCoord2f(r, t); glVertex2f(texw, 0.0f);
	glTexCoord2f(r, b); glVertex2f(texw, texh);
	glTexCoord2f(l, b); glVertex2f(0.0f, texh);
	glEnd();
}


void FPFrame::SetHandle(float x, float y) {
	xhandle = x;
	yhandle = y;
	InitDisplayList();
	if (gl_flips) MakeFlippable();
}


void FPFrame::InitDisplayList() {
	context->Focus();

	if (gl_list == 0)
		gl_list = glGenLists(1);

	glNewList(gl_list, GL_COMPILE);
	SendGeometry(false, false);
	glEndList();
}


void FPFrame::MakeFlippable() {
	context->Focus();

	if (gl_flips == 0)
		gl_flips = glGenLists(3);

	glNewList(gl_flips, GL_COMPILE);
	SendGeometry(true, false);
	glEndList();

	glNewList(gl_flips+1, GL_COMPILE);
	SendGeometry(false, true);
	glEndList();

	glNewList(gl_flips+2, GL_COMPILE);
	SendGeometry(true, true);
	glEndList();
}


void FPFrame::DisposeDisplayLists() {
	context->Focus();

	if (gl_list) {
		glDeleteLists(gl_list, 1);
		gl_list = 0;
	}

	if (gl_flips) {
		glDeleteLists(gl_flips, 3);
		gl_flips = 0;
	}
}


int FPFrame::LoadImageFile(CFURLRef imageFile, bool keep) {
	DisposeImage();
	CGImageRef *outStore = keep ? &cg_image : NULL;
	gl_texture = context->MakeTextureWithImageFile(imageFile, &width, &height, &texw, &texh, outStore);
	CenterHandle();
	return gl_texture;
}


int FPFrame::LoadImageSegment(CGImageRef imageRef, CGRect segment) {
	DisposeImage();
	gl_texture = context->MakeTextureWithImageSegment(imageRef, segment, &width, &height, &texw, &texh);
	CenterHandle();
	return gl_texture;
}


int FPFrame::LoadCGContext(CGContextRef contextRef) {
	gl_texture = context->MakeTextureWithCGContext(contextRef, &width, &height, &texw, &texh);
	CenterHandle();
	return gl_texture;
}


CGContextRef FPFrame::MakeCGContext(int width, int height) {
	DisposeCGContext();
	cg_context = CGCreateContext(width, height);
	(void)LoadCGContext(cg_context);
	return cg_context;
}


void FPFrame::RefreshTexture() {
	if (cg_context) {
		DisposeTexture();
		(void)LoadCGContext(cg_context);
	}
	else if (cg_image) {
		DisposeTexture();
	}
}


void FPFrame::DisposeImage() {
	if (cg_image) {
		CGImageRelease(cg_image);
		cg_image = NULL;
	}
}


void FPFrame::DisposeTexture() {
	GLuint t = gl_texture;
	if (t) {
		gl_texture = 0;
		context->Focus();
		glDeleteTextures(1, &t);
	}
}


void FPFrame::DisposeCGContext() {
	if (cg_context) {
		CGContextRelease(cg_context);
		cg_context = NULL;
	}
}


