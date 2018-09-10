/*
 *  TSpriteControl.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TSPRITECONTROL_H
#define TSPRITECONTROL_H

#include "TCustomControl.h"

#pragma mark -

class FPSprite;

//-----------------------------------------------
//
// TSpriteControl
//
class TSpriteControl : public TCustomControl {
	private:
		FPSprite	*controlSprite;
		bool		pressed;

	public:
						TSpriteControl(WindowRef wind, OSType sig, UInt32 cid, FPSprite *sprite);
						TSpriteControl(WindowRef wind, UInt32 cmd, FPSprite *sprite);
						TSpriteControl(ControlRef controlRef) : TCustomControl(controlRef) { Init(); }
		virtual			~TSpriteControl() {}

		void			Init();

		virtual bool	Track(MouseTrackingResult eventType, Point where);
		virtual bool	Draw(const Rect &bounds);
		static void		Register();
};

#endif
