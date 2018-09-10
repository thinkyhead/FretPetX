//
//  FPBankControl.h
//  FretPetX
//
//  Created by Scott Lahteine on 12/1/12.
//
//

#ifndef __FretPetX__FPBankControl__
#define __FretPetX__FPBankControl__

#include "FPDocWindow.h"
#include "TControls.h"

class FPBankControl : public TCustomControl {
public:
	FPBankControl(WindowRef wind);
	~FPBankControl() {}
	
	virtual bool	Draw(const CGContextRef gc, const Rect &bounds);
	//	virtual bool	Draw(const Rect &bounds);
	
	ControlPartCode	HitTest(Point where);
	bool			MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
	bool			Track(MouseTrackingResult eventType, Point where);
	void			GetPatternPosition(Point where, SInt16 &string, SInt16 &step);
	SInt16			GetLineFromPoint(Point where)	{ return (where.v + kLineHeight) / kLineHeight - 1; }
	
	UInt32			EnableFeatures() { return kControlSupportsDragAndDrop; }
	bool			DragEnter(DragRef drag) { return false; }
	bool			DragLeave(DragRef drag) { return false; }
	bool			DragWithin(DragRef drag) { return false; }
	bool			DragReceive(DragRef drag) { return false; }
};

#endif /* defined(__FretPetX__FPBankControl__) */
