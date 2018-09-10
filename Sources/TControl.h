/*
 *  TControl.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TCONTROL_H
#define TCONTROL_H

class TCarbonEvent;

#include "TWindow.h"

#pragma mark -
//-----------------------------------------------
//
//  TControl
//
class TControl {
	protected:
		ControlRef			fControl;
		EventHandlerRef		fHandler;
		WindowRef			fWindow;
		bool				state;
		ControlActionUPP	actionUPP;

	public:
		UInt32	command;
								TControl();
								TControl(WindowRef wind, OSType sig, UInt32 cid);
								TControl(WindowRef wind, UInt32 cmd);
								TControl(DialogRef dialog, SInt16 item);
								TControl(ControlRef controlRef);
		virtual					~TControl();

		void					Init();

		OSStatus				SpecifyByControlID(WindowRef wind, OSType sig, UInt32 cid);
		OSStatus				SpecifyByCommandID(WindowRef wind, UInt32 cmd);
		OSStatus				SpecifyByDialogItem(DialogRef dialog, SInt16 item);
		void					SpecifyByControlRef(ControlRef controlRef);

		inline void				SetBounds(Rect rect)		{ SetBounds(&rect); }
		inline void				SetBounds(Rect *rect)		{ SetControlBounds(fControl, rect); }
		inline void				SetViewSize(SInt32 size)	{ SetControlViewSize(fControl, size); }

		inline void				SetTitle(CFStringRef str)	{ (void)SetControlTitleWithCFString(fControl, str); }
		inline CFStringRef		GetTitle()					{ CFStringRef str; (void)CopyControlTitleAsCFString(fControl, &str); return str; }

		inline void				SetStaticText(CFStringRef str)	{ (void)SetControlData(fControl, 0, kControlStaticTextCFStringTag, sizeof(CFStringRef), &str); }
		inline CFStringRef		GetStaticText()					{ CFStringRef str; (void)GetControlData(fControl, 0, kControlStaticTextCFStringTag, sizeof(CFStringRef), &str, NULL); return str; }

		inline void				EnableAction() {
									actionUPP = NewControlActionUPP(LACallback);
									SetControlAction(fControl, actionUPP);
								}

		inline void				DisableAction() {
									if (actionUPP) {
										DisposeControlActionUPP(actionUPP);
										actionUPP = NULL;
									}
								}

		static pascal void		LACallback(ControlRef inControl, ControlPartCode inPart) {
									TControl *myself = (TControl*)GetControlReference(inControl);
									myself->Action(inPart);
								}

		virtual void			Action(ControlPartCode inPart)		{}

		// Accessors
		inline ControlRef		Control()					{ return fControl; }
		inline OSType			Signature()					{ ControlID contID; (void) GetControlID(fControl, &contID); return contID.signature; }
		inline SInt32			GetID()						{ ControlID contID; (void) GetControlID(fControl, &contID); return contID.id; }
		inline WindowRef		Window()					{ return fWindow; }
		inline TWindow*			GetTWindow()				{ return TWindow::GetTWindow(Window()); }
		inline GrafPtr			WindowPort()				{ return GetWindowPort(Window()); }
		inline EventTargetRef	EventTarget()				{ return GetControlEventTarget(fControl); }

		inline Rect				Bounds()					{ Rect rect; GetControlBounds(fControl, &rect); return rect; }
		inline UInt16			Width()						{ Rect rect = Bounds(); return rect.right - rect.left; }
		inline UInt16			Height()					{ Rect rect = Bounds(); return rect.bottom - rect.top; }
		CGRect					CGBounds();
		inline Rect				Size()						{ Rect rect = Bounds(); rect.right -= rect.left; rect.bottom -= rect.top; rect.left = rect.top = 0; return rect; }
		inline SInt32			Maximum()					{ return GetControl32BitMaximum(fControl); }
		inline SInt32			Minimum()					{ return GetControl32BitMinimum(fControl); }
		inline bool				IsVisible()					{ return IsControlVisible(fControl); }
		inline bool				IsHidden()					{ return !IsVisible(); }
		inline bool				IsActive()					{ return IsControlActive(fControl); }
		inline bool				IsEnabled()					{ return IsControlEnabled(fControl); }
		inline bool				IsHilited()					{ return IsControlHilited(fControl); }
		inline bool				IsPressed()					{ return IsEnabled() && IsActive() && IsHilited(); }
		inline bool				HasFocus()
								{	ControlRef maybe;
									(void)GetKeyboardFocus(fWindow, &maybe);
									return (maybe == fControl);
								}


		inline bool				State()						{ return state; }
		inline SInt32			Value()						{ return GetControl32BitValue(fControl); }
		ThemeDrawState			GetDrawState() {
			return	IsEnabled()
					? (IsActive() ? (IsPressed() ? kThemeStatePressed : kThemeStateActive) : kThemeStateInactive)
					: (IsActive() ? kThemeStateUnavailable : kThemeStateUnavailableInactive);
		}

		inline void				SetMinimum(SInt32 min)		{ SetControl32BitMinimum(fControl, min); }
		inline void				SetMaximum(SInt32 max)		{ SetControl32BitMaximum(fControl, max); }
		inline void				SetRange(SInt32 lo, SInt32 hi)	{ SetMinimum(lo); SetMaximum(hi); }
		virtual inline void		SetValue(SInt32 val)		{ SetControl32BitValue(fControl, val); }

		virtual void			SetState(bool yesno);
		virtual void			SetAndLatch(bool enable);

		virtual void			SetVisible(bool show);
		inline void				Show()						{ SetVisible(true); }
		inline void				Hide()						{ SetVisible(false); }

		virtual void			SetActivated(bool enable);
		inline void				Activate()					{ SetActivated(true); }
		inline void				Deactivate()				{ SetActivated(false); }

		virtual void			SetEnabled(bool enable);
		inline void				Enable()					{ SetEnabled(true); }
		inline void				Disable()					{ SetEnabled(false); }

		inline virtual void		Focus()						{ (void)SetKeyboardFocus(fWindow, fControl, kControlEditTextPart); }
		CGContextRef			FocusContext()				{ TWindow *wind = GetTWindow(); return wind ? wind->FocusContext() : NULL; }
		void					EndContext()				{ TWindow *wind = GetTWindow(); if (wind) wind->EndContext(); }
        CGrafPtr                FocusWindow()               { return GetTWindow()->Focus(); }

		inline bool				Toggle()					{ SetState(!state); return state; }
		inline void				Hilite(ControlPartCode partCode) { HiliteControl(fControl, partCode); }

		inline void				ReceiveEvent(EventRef event){ SendEventToEventTarget(event, EventTarget()); }
		void					GetLocalPoint(Point *point);

		void					InstallEventHandler();
		inline void				RegisterForEvents( UInt32 numEvents, const EventTypeSpec* list )	{ AddEventTypesToHandler( fHandler, numEvents, list ); }
		inline void				UnregisterForEvents( UInt32 numEvents, const EventTypeSpec* list )	{ RemoveEventTypesFromHandler( fHandler, numEvents, list ); }
		static pascal OSStatus  EventHandler(EventHandlerCallRef inCallRef, EventRef inEvent, void* userData);

		virtual bool			HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );
		virtual inline bool		ControlHit(Point where)								{ return false; }
		virtual bool			Draw(const CGContextRef context, const Rect &bounds);
		virtual bool			Draw(const Rect &bounds);
		virtual void			DrawText(const Rect &bounds, CFStringRef inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext=NULL);
		void					DrawText(const Rect &bounds, StringPtr inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext=NULL);
//		virtual bool			BoundsChanged();
		virtual inline ControlPartCode	HitTest( Point where )						{ return kControlIndicatorPart; }
		virtual inline void		ContextClick()										{}
		virtual inline bool		Track(MouseTrackingResult eventType, Point where)	{ return false; }
		virtual inline void		IndicatorMoved()									{}
		virtual inline void		ValueChanged()										{}
		virtual inline bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0)		{ return false; }
		virtual inline void		DoMouseDown()										{}
		virtual inline void		DoMouseUp()											{}

		virtual inline void		DidDeactivate()										{}
		virtual inline void		HiliteChanged()										{}

		virtual inline UInt32	EnableFeatures()									{ return 0; }
		virtual inline UInt32	DisableFeatures()									{ return 0; }
		virtual inline bool		DragEnter(DragRef drag)								{ return false; }
		virtual inline bool		DragLeave(DragRef drag)								{ return false; }
		virtual inline bool		DragWithin(DragRef drag)							{ return false; }
		virtual inline bool		DragReceive(DragRef drag)							{ return false; }
		void					SetDragTrackingEnabled(bool enable)					{ SetControlDragTrackingEnabled(fControl, enable); }

		inline void				DrawNow()					{ DrawOneControl(fControl); }

		OSStatus				SetHelpTag(CFStringRef inString, bool localType=false);
		OSStatus				SetHelpTag(char *inString, bool localType=false);
		OSStatus				SetHelpTagPascal(StringPtr inString, bool localType=false);
		OSStatus				RemoveHelpTag()				{ return HMSetControlHelpContent(fControl, NULL); }
};

#endif