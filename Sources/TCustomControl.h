/*
 *  TCustomControl.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TCUSTOMCONTROL_H
#define TCUSTOMCONTROL_H

#include "TControl.h"

//-----------------------------------------------
//
//  TCustomControl
//
class TCustomControl : public TControl {
	protected:
		static UInt32	lastClick;
		static int		clickCount;

	public:
								TCustomControl();
								TCustomControl(WindowRef wind, OSType sig, UInt32 cid);
								TCustomControl(WindowRef wind, UInt32 cmd);
								TCustomControl(ControlRef controlRef) : TControl(controlRef) { Init(); }
		virtual 				~TCustomControl();

		void					Init();

		static TCustomControl*	Construct(ControlRef theControl) { return new TCustomControl(theControl); }
		static void				RegisterCustomControl(CFStringRef classID, ToolboxObjectClassRef *customDef, EventHandlerUPP *customHandler);
		static pascal OSStatus  CustomHandler(EventHandlerCallRef inCallRef, EventRef inEvent, void* userData);
		UInt16					CountClicks(bool same, UInt16 max);
};

#endif
