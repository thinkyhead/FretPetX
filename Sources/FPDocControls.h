/*
 *  FPDocControls.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPDOCCONTROLS_H
#define FPDOCCONTROLS_H

#include "FPDocWindow.h"
#include "TControl.h"
#include "TCustomControl.h"
#include "TPictureControl.h"

class	FPHistoryEvent;
class	TString;

#pragma mark -
class FPTabsControl : public TCustomControl {
public:
			FPTabsControl(WindowRef wind);
			~FPTabsControl() {}

	bool			Draw(const Rect &bounds) { return true; }
	bool			Track(MouseTrackingResult eventType, Point where);
};


#pragma mark -
class FPDocPopup : public TControl {
private:
	bool				numberMode;
	MenuRef				instrumentMenu;

public:
						FPDocPopup( WindowRef w );
	bool				Draw( const Rect &bounds );
	ControlPartCode		HitTest(Point where);
	bool				ControlHit(Point where);
	bool				MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
	void				ToggleNumberMode() { numberMode ^= true; DrawNow(); }
};


#pragma mark -
class FPDocSlider : public TCustomControl {
private:
	bool			ispressed;
	SInt32			oldValue;

protected:
	FPHistoryEvent	*event;
	UInt32			snap;

public:
							FPDocSlider( WindowRef wind, OSType sig, UInt32 cid );
	virtual					~FPDocSlider() {}
	bool					Draw(const Rect &bounds);
	bool					Track(MouseTrackingResult eventType, Point where);
	inline ControlPartCode	HitTest(Point where)				{ return kControlIndicatorPart; }
	virtual TString*		ValueString()						{ return NULL; }
	inline void				SetValue(SInt32 v, bool d=false)	{ TCustomControl::SetValue(v); if (d) ValueChanged(); }
	inline void				SetSnap(UInt32 s)					{ snap = s; }
	virtual bool			MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
	virtual void			ValueChanged() = 0;
	virtual bool			LeftAlign()							{ return true; }
};


#pragma mark -
class FPTempoDoubler : public TPictureControl {
	public:
					FPTempoDoubler( WindowRef w );
		UInt16		GetFrameForState();
};


#pragma mark -
class FPTempoSlider : public FPDocSlider {
public:
				FPTempoSlider( WindowRef w ) : FPDocSlider(w, 'docu', 104) { SetSnap(40); }
	void		ValueChanged();
	TString*	ValueString();
	bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
	void		DoMouseDown();
	void		DoMouseUp();
	bool		LeftAlign()				{ return false; }

};


#pragma mark -
/*!
 *	Custom slider for instrument velocity
 */
class FPVelocitySlider : public FPDocSlider {
public:
				FPVelocitySlider( WindowRef w ) : FPDocSlider(w, 'docu', 105) { SetSnap(20); }
	void		ValueChanged();
	bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
	void		DoMouseDown();
	void		DoMouseUp();
};


#pragma mark -
/*!
 *	Custom slider for instrument sustain
 */
class FPSustainSlider : public FPDocSlider {
public:
				FPSustainSlider( WindowRef w ) : FPDocSlider(w, 'docu', 106) { SetSnap(10); }
	void		ValueChanged();
	bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
	void		DoMouseDown();
	void		DoMouseUp();
};


#pragma mark -

enum {
	kPictDoubleButton = 1202, kPictDoubleButton2, kPictDoubleButton3, kPictDoubleButton4,
	kPictChordGray, kPictChordSel, kPictChordCurs,
	kPictOverlayGray, kPictOverlaySel, kPictOverlayCurs,
	kPictStretch1, kPictTop1,
	kPictStretch2, kPictTop2,
};

enum {
	CHORDW	= kDocWidth - kScrollbarSize,		// Width of the entire chord graphic
	CORDL	= 137,								// Name and Pattern Left
	CORDR	= CORDL + 206,						// Name and Pattern Right
	
	LOCKX	= CORDR - 13, LOCKY = 7,			// Lock position
	
	SEQX	= CORDL+7,	SEQY	= 20,
	SEQH	= 12,		SEQV	= 6,
	
	SEQTOP	= SEQY + 2,
	SEQBOT	= (SEQV * 6) + SEQY,
	
	GTAB	= 116,								// Tab numbers center

	CIRC	= 40,		CIRCX	= 44,			// Small Circle Pos / Size
	CIRCY	= 11,
	
	REPTH	= 9,		REPTX	= 95,			// Repeat Slider Pos / Size
	REPTV	= 34,		REPTY	= 9,
	
	SEQHH	= (SEQH * 17),
	SEQVV	= (SEQV * 6) + 4,
	
	LOCKH	= 10,
	LOCKV	= 13,
	
	WTABH	= 85,		WTABV	= 17,			// Part Tab size
	WTABX	= 5,
	WTABD	= 89,								// distance
	
	OVERH	= 33,		OVERX	= 136,			// Roman Mode name overlay
	OVERV	= 17,		OVERY	= 3
};

#endif
