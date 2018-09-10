/*
 *  TPictureControl.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TPICTURECONTROL_H
#define TPICTURECONTROL_H

#include "TCustomControl.h"
#include "TControl.h"

#define MAX_PICTS 6

enum {
	kFrameNormal = 0,
	kFramePressed,
	kFrameSet,
	kFrameSetPressed,
	kFrameBlink,
	kFrameDisabled
	};

//-----------------------------------------------
//
//  TPictureHelper
//
//	Provides a utility to store and draw pictures
//	and adds a Blinking feature for controls like
//	"Play" that blink while active.
//
class TPictureHelper {
	private:
		TControl			*owningControl;			// The control, which is also "this"

	public:
		UInt16				pictCount;
		GWorldPtr			graphic[MAX_PICTS];		// GWorlds rendered by DrawPicture
		IconRef				icon[MAX_PICTS];		// IconRefs with masks, draw better
		bool				blinking;				// Flag that it's blinking
		bool				blinkState;				// Odd or even blink state

							TPictureHelper(TControl *theControl, UInt16 pictID, UInt16 numPicts=0);
		virtual 			~TPictureHelper();

		// Picture creation and storage
		void				LoadPictResources(UInt16 pictID, UInt16 numPicts=0);
		void				AddPictResource(UInt16 pictID);
		void				AddPicture(PicHandle hPict);
		void				DisposePicts();

		// Custom draw for pictures
		void				Draw(const Rect &bounds);

		virtual bool		GetDimmedState();
		virtual UInt16		GetFrameForState();

		void				Blink(bool on)			{ blinking = on; }
		inline void			DoBlink()				{ blinkState ^= true; }
		inline bool			IsBlinking()			{ return blinking; }
};

#pragma mark -
//-----------------------------------------------
//
//  TPictureControl
//
class TPictureControl : public TControl, public TPictureHelper {
	private:
		EventLoopTimerUPP	timerUPP;				// UPP for an Event Loop Timer
		EventLoopTimerRef	blinkLoop;				// Event Loop timer object

	public:
							TPictureControl(WindowRef wind, OSType sig, UInt32 cid, UInt16 pictID, UInt16 numPicts=0);
							TPictureControl(WindowRef wind, UInt32 cmd, UInt16 pictID, UInt16 numPicts=0);
							TPictureControl(ControlRef controlRef, UInt16 pictID, UInt16 numPicts=0);

		virtual				~TPictureControl() {}

		void				Init();

		virtual bool		Draw(const Rect &bounds)	{ TPictureHelper::Draw(bounds); return true; }

		void				Blink(bool on);
		static void			BlinkLoopTimerProc( EventLoopTimerRef timer, void *control ) { ((TPictureControl*)control)->DoBlink(); }
		void				DoBlink();

		virtual void		SetState(bool enable)		{ TControl::SetState(enable); if (/* pictHelper. */ pictCount == 5) Blink(enable); }
};


#pragma mark -
//-----------------------------------------------
//
//  TSegmentedPictureButton
//
class TSegmentedPictureButton : public TPictureControl {
	public:
		TSegmentedPictureButton(WindowRef wind, OSType sig, UInt32 cid, UInt16 pictID, UInt16 numPicts=0)
			: TPictureControl(wind, sig, cid, pictID, numPicts) {}

		TSegmentedPictureButton(WindowRef wind, UInt32 cmd, UInt16 pictID, UInt16 numPicts=0)
			: TPictureControl(wind, cmd, pictID, numPicts) {}

		TSegmentedPictureButton(ControlRef controlRef, UInt16 pictID, UInt16 numPicts=0)
			: TPictureControl(controlRef, pictID, numPicts) {}

	virtual				~TSegmentedPictureButton() {}

	virtual bool		GetDimmedState() {
		UInt16 frame = GetFrameForState();
		return !IsActive() ||
			(	TPictureControl::GetDimmedState()
				&& (frame == kFrameNormal || (pictCount != 3 && frame == kFrameSet))
			);
	}

};


#pragma mark -
//-----------------------------------------------
//
//  TPicturePopup
//	A picture control that hosts a popup menu.
//	The control should only need two images.
//
class TPicturePopup : public TPictureControl {
	protected:
		MenuRef			popupMenu;
		MenuItemIndex	itemUnderMouse;

	public:
		TPicturePopup();
		TPicturePopup(WindowRef wind, OSType sig, UInt32 cid, CFStringRef menuName, UInt16 pictID, UInt16 numPicts=0);
		TPicturePopup(WindowRef wind, UInt32 cmd, CFStringRef menuName, UInt16 pictID, UInt16 numPicts=0);
		TPicturePopup(ControlRef controlRef, CFStringRef menuName, UInt16 pictID, UInt16 numPicts=0);

		virtual ~TPicturePopup();

		void			Init();
		void			AttachMenu(CFStringRef menuName);
		void			SetActiveItem(MenuItemIndex i) { itemUnderMouse = i; }
		ControlPartCode	HitTest(Point where);

		virtual bool	ControlHit(Point where);
		void			ContextClick();
};


#pragma mark -
//-----------------------------------------------
//
//  TPictureDragger
//
//	A picture control that has a range of values.
//	It needs to be a Custom Control to support mouse
//	tracking. Presumably the mouse pointer should
//	change when over the draggable area, and clicking
//	should hide the mouse pointer as the control
//	becomes active. Dragging the mouse left-right or
//	up-down then moves the value up and down.
//
//	The best interface would be to allow typing in
//	the values. This might be accomplished with some
//	controls. I haven't played with keyboard focus,
//	but currently the active window gets first dibs
//	on keystrokes. By default the active control can
//	intercept all plain keystrokes. So there ought
//	to be an option to combine a dragging element
//	with a text edit control. (Which of course at
//	the moment I haven't implemented.)
//
//	To draw the text call the DrawText function
//	passing all the necessary parameters. The value
//	will be drawn in the font, size, and at the given
//	offset using CG.
//
class TPictureDragger : public TCustomControl, public TPictureHelper {
	private:
		TTextField		*editField;

	public:						// the tedium of c++
								TPictureDragger(WindowRef wind, OSType sig, UInt32 cid, UInt16 pictID, UInt16 numPicts=0);
								TPictureDragger(WindowRef wind, UInt32 cmd, UInt16 pictID, UInt16 numPicts=0);
								TPictureDragger(ControlRef controlRef, UInt16 pictID, UInt16 numPicts=0);

		virtual					~TPictureDragger() {}

		void					Init();

		inline bool				Draw(const Rect &bounds)	{ TPictureHelper::Draw(bounds); return true; }
		virtual void			DrawText(const Rect &bounds, CFStringRef inString);

		virtual ControlPartCode	HitTest(Point where);
		virtual bool			ControlHit(Point where);
		virtual bool			Track(MouseTrackingResult eventType, Point where);
		virtual bool			MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
		void					AssociateEditField(TTextField *field)	{ editField = field; }
		void					ActivateField(bool b=true);

//		virtual UInt16			GetFrameForState()			{ return pictHelper.GetFrameForState(this); }
//		virtual bool			GetDimmedState(UInt16 frame){ return pictHelper.GetDimmedState(this, frame); }
};


#endif
