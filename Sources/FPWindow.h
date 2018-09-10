/*
 *  FPWindow.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *  The FPWindow adds:
 *
 *  - Load and Save window positions with the global preferences
 *  - Timer-driven rollovers that can handle modifiers key changes
 *
 */

#ifndef FPWINDOW_H
#define FPWINDOW_H

#include "TWindow.h"
#include "FPUtilities.h"

//
// mouseRect is a rollover region
// Any window that wants it can install a mouseRect Carbon Timer
// that will poll for the mouse region. For the current mouseRect
// the HandleMouseRectEvent() method will fire ten times per second
// passing idleMouseEvent as the event id.
//
typedef struct {
	int		id_number;
	Rect	rect;
} mouseRect;

//
// FPWindow
//
// A window that saves its position and visibility
// state using the global preferences object.
//
class FPWindow : public TWindow {
	private:
		bool			veryHidden;
		CFStringRef		positionKey, sizeKey, visibleKey;
		Rect			originalBounds;

	public:

		FPWindow(CFStringRef windowName);
		virtual ~FPWindow();

		virtual UInt32	Type()						{ return 'FP  '; }
		virtual const CFStringRef	PreferenceKey()	{ return NULL; }
		virtual const CFStringRef	PositionKey()	{ if (positionKey == NULL) { positionKey = CFStringCombine(CFSTR("wpos:"), PreferenceKey()); } return positionKey; }
		virtual const CFStringRef	SizeKey()		{ if (sizeKey == NULL) { sizeKey = CFStringCombine(CFSTR("wsiz:"), PreferenceKey()); } return sizeKey; }
		virtual const CFStringRef	VisibleKey()	{ if (visibleKey == NULL) { visibleKey = CFStringCombine(CFSTR("wvis:"), PreferenceKey()); } return visibleKey; }
		virtual bool				LoadPreferences(bool showing=true);
		virtual void				SavePreferences();
		void						ReallyHide();
		void						Show();
		void						ShowIfShown();
		virtual void				SetDefaultBounds() { originalBounds = Bounds(); }
		virtual void				SetDefaultBounds(Rect &b) { originalBounds = b; }
		virtual void				SetDefaultSize(int w, int h) { originalBounds.right = originalBounds.left + w; originalBounds.bottom = originalBounds.top + h; }
		virtual void				Reset();

		OSErr						EnableDragReceive() { return InstallReceiveHandler(HandleDragReceive, Window(), this); }
		OSErr						DisableDragReceive() { return RemoveReceiveHandler(HandleDragReceive, Window()); }
		OSErr						EnableDragTracking() { return InstallTrackingHandler(HandleDragTracking, Window(), this); }
		OSErr						DisableDragTracking() { return RemoveTrackingHandler(HandleDragTracking, Window()); }

		virtual OSErr				DragEnter(DragRef theDrag) { return dragNotAcceptedErr; }
		virtual OSErr				DragLeave(DragRef theDrag) { return noErr; }
		virtual OSErr				DragWithin(DragRef theDrag) { return noErr; }
		virtual OSErr				DragReceive(DragRef theDrag) { return dragNotAcceptedErr; }

		static OSErr HandleDragReceive(WindowRef theWindow, void *refCon, DragRef theDrag) {
			FPWindow *wind = (FPWindow*)refCon;
			return wind->DragReceive(theDrag);
		}

		static OSErr HandleDragTracking(DragTrackingMessage message, WindowRef theWindow, void *refCon, DragRef theDrag) {
			OSErr result = noErr;
			FPWindow *wind = (FPWindow*)refCon;
			switch(message) {
				case kDragTrackingEnterHandler:
				case kDragTrackingLeaveHandler:
					break;
				case kDragTrackingEnterWindow:
					result = wind->DragEnter(theDrag);
					break;
				case kDragTrackingInWindow:
					result = wind->DragWithin(theDrag);
					break;
				case kDragTrackingLeaveWindow:
					result = wind->DragLeave(theDrag);
					break;
			}
			return result;
		}

};

#endif
