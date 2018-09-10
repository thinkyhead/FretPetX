/*
 *  TSpriteControl.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TSpriteControl.h"
#include "FPSprite.h"
#include "FPApplication.h"

//-----------------------------------------------
//
// TSpriteControl
//

TSpriteControl::TSpriteControl(WindowRef wind, OSType sig, UInt32 cid, FPSprite *sprite) : TCustomControl(wind, sig, cid) {
	Init();
	controlSprite	= sprite;
}


TSpriteControl::TSpriteControl(WindowRef wind, UInt32 cmd, FPSprite *sprite) : TCustomControl(wind, cmd) {
	Init();
	controlSprite	= sprite;
}


void TSpriteControl::Init() {
	pressed			= false;
	state			= false;
}


bool TSpriteControl::Track(MouseTrackingResult eventType, Point where) {
	Rect		bounds = Bounds();
	static bool	isInside = false;

	switch (eventType) {
		case kMouseTrackingMouseDown:
		case kMouseTrackingMouseEntered:
			pressed = true;
			isInside = true;
			Draw(bounds);
			break;

		case kMouseTrackingMouseExited:
			isInside = false;

		case kMouseTrackingMouseUp:
			pressed = false;
			Draw(bounds);
			break;
	}

	return true;
}


bool TSpriteControl::Draw(const Rect &bounds) {
	UInt16  frame = 0;
	bool	enabled = IsActive();

	switch (controlSprite->FrameCount()) {
		case 1:	frame = 0; break;

		case 2:	frame = state ? (pressed ? 0 : 1) : (pressed ? 1 : 0); break;

		case 3: frame = state ? (pressed ? 1 : 2) : (pressed ? 1 : 0); break;

		case 4: frame = state ? (pressed ? 3 : 2) : (pressed ? 1 : 0); break;

		case 5:	frame = enabled ? (state ? 2 : 0) : 4; break;
	}

	controlSprite->SetAnimationFrameIndex(frame);

	return true;
}


void TSpriteControl::Register() {
	fretpet->RegisterCustomControl(CFSTR("com.thinkyhead.fretpet.sprite"));
}

