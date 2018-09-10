/*
	File:		TWindow.h

    Version:	Mac OS X

	Disclaimer:	IMPORTANT:  This Apple software is supplied to you by Apple Computer, Inc.
				("Apple") in consideration of your agreement to the following terms, and your
				use, installation, modification or redistribution of this Apple software
				constitutes acceptance of these terms.  If you do not agree with these terms,
				please do not use, install, modify or redistribute this Apple software.

				In consideration of your agreement to abide by the following terms, and subject
				to these terms, Apple grants you a personal, non-exclusive license, under AppleÕs
				copyrights in this original Apple software (the "Apple Software"), to use,
				reproduce, modify and redistribute the Apple Software, with or without
				modifications, in source and/or binary forms; provided that if you redistribute
				the Apple Software in its entirety and without modifications, you must retain
				this notice and the following text and disclaimers in all such redistributions of
				the Apple Software.  Neither the name, trademarks, service marks or logos of
				Apple Computer, Inc. may be used to endorse or promote products derived from the
				Apple Software without specific prior written permission from Apple.  Except as
				expressly stated in this notice, no other rights or licenses, express or implied,
				are granted by Apple herein, including but not limited to any patent rights that
				may be infringed by your derivative works or by other works in which the Apple
				Software may be incorporated.

				The Apple Software is provided by Apple on an "AS IS" basis.  APPLE MAKES NO
				WARRANTIES, EXPRESS OR IMPLIED, INCLUDING WITHOUT LIMITATION THE IMPLIED
				WARRANTIES OF NON-INFRINGEMENT, MERCHANTABILITY AND FITNESS FOR A PARTICULAR
				PURPOSE, REGARDING THE APPLE SOFTWARE OR ITS USE AND OPERATION ALONE OR IN
				COMBINATION WITH YOUR PRODUCTS.

				IN NO EVENT SHALL APPLE BE LIABLE FOR ANY SPECIAL, INDIRECT, INCIDENTAL OR
				CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE
				GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION)
				ARISING IN ANY WAY OUT OF THE USE, REPRODUCTION, MODIFICATION AND/OR DISTRIBUTION
				OF THE APPLE SOFTWARE, HOWEVER CAUSED AND WHETHER UNDER THEORY OF CONTRACT, TORT
				(INCLUDING NEGLIGENCE), STRICT LIABILITY OR OTHERWISE, EVEN IF APPLE HAS BEEN
				ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

	Copyright © 2000-2001 Apple Computer, Inc., All Rights Reserved
*/

#ifndef TWINDOW_H
#define TWINDOW_H

#if !BUILDING_FOR_CARBON_8
#include <Carbon/Carbon.h>
#else
#include <Carbon.h>
#endif

#include "FPUtilities.h"
#include "TTrackingRegion.h"

class TCarbonEvent;
class TTextField;

class TWindow {
	friend class TTrackingRegion;

	protected:
		TTextField		*currentFocus;
		CGContextRef	context;

	private:
		bool					isActive;
		TTrackingRegion			*localEnteredRegion;
		TrackingRegionList		trackingRegionList;

	public:
			TWindow();
			TWindow( CFStringRef inNib, CFStringRef inName );
			TWindow( WindowClass inClass, WindowAttributes inAttributes, const Rect& bounds );
			TWindow( WindowRef inWindow )								{ InitWithPlatformWindow( inWindow ); }
		virtual ~TWindow();

		inline bool			IsActive() const							{ return isActive;	/* IsWindowActive(fWindow); */ }
		inline bool			IsVisible() const							{ return IsWindowVisible(fWindow); }

		virtual inline void	Close()										{ Hide(); delete this; }

        inline CGrafPtr		Focus()										{ CGrafPtr oldPort; GetPort(&oldPort); SetPortWindowPort(fWindow); return oldPort; }
		inline void			SetUserFocus()								{ (void)SetUserFocusWindow(fWindow); }
		CGrafPtr			Port() const;
		WindowRef			Window() const;
		static TWindow*		GetTWindow( WindowRef window );

//		inline void			SetTitle(const StringPtr inTitle)			{ SetWTitle( fWindow, inTitle ); }
		inline void			SetTitle(CFStringRef inTitle)				{ SetWindowTitleWithCFString(fWindow, inTitle); }
		CFStringRef			CopyTitle() const;
		
		inline void			SetAlternateTitle(CFStringRef inTitle)		{ SetWindowAlternateTitle( fWindow, inTitle ); }
		CFStringRef			CopyAlternateTitle() const;

		inline EventTargetRef EventTarget()								{ return GetWindowEventTarget(fWindow); }

		virtual void		Show();
		virtual inline void	Hide()										{ if (IsSheet()) HideSheet(); else HideWindow( fWindow ); }
		inline void			ToggleVisibility()							{ if (IsVisible()) { Hide(); } else { Show(); Select(); } }

		void				SendBehind(TWindow *behindWindow)			{ ::SendBehind(Window(), behindWindow->Window()); }

		virtual void		ShowSheet(WindowRef inParentWindow);
		virtual void		HideSheet();
		WindowRef			SheetParent();
		bool				IsSheet()									{ return SheetParent() != NULL; }
		DialogRef			MakeStandardSheet(AlertType alertType, CFStringRef error, CFStringRef explanation, const AlertStdCFStringAlertParamRec *param);

		inline virtual void	SetModified(bool modified)					{ (void)SetWindowModified(fWindow, modified); }
		inline void			SetAlpha(float inAlpha)						{ (void)SetWindowAlpha(fWindow, inAlpha); }
		void				SetOpaque(bool op);

		inline void			Select()									{ SelectWindow( fWindow ); }

		inline void			Move(SInt16 x, SInt16 y)					{ MoveWindow( fWindow, x, y, false ); }
		virtual void		Cascade(SInt16 h, SInt16 v=0);
		void				Center(SInt16 v=-20);
		inline void			SetContentSize( short x, short y )			{ SizeWindow( fWindow, x, y, true ); }
		void				TransitionSize( short x, short y );

		inline UInt16		Height()									{ Rect bounds = Bounds(); return bounds.bottom - bounds.top; }
		inline UInt16		Width()										{ Rect bounds = Bounds(); return bounds.right - bounds.left; }
		Rect				Bounds();
		CGRect				CGBounds();
		Rect				StructureBounds();
		Rect				GetContentSize();

		inline void			InvalidateArea( RgnHandle region )			{ InvalWindowRgn( fWindow, region ); }
		inline void			InvalidateArea( const Rect& rect )			{ InvalWindowRect( fWindow, &rect ); }
		void				InvalidateWindow();
		inline void			ValidateArea( RgnHandle region )			{ ValidWindowRgn( fWindow, region ); }
		inline void			ValidateArea( const Rect& rect )			{ ValidWindowRect( fWindow, &rect ); }

		void				UpdateNow();
		inline void			RunModal()									{ RunAppModalLoopForWindow(fWindow); }
		inline void			QuitModal()									{ QuitAppModalLoopForWindow(fWindow); }
		virtual bool		RunDialogsModal()							{ return false; }

		void				SetDefaultButton( ControlRef control )		{ SetWindowDefaultButton( fWindow, control ); }
		void				SetCancelButton( ControlRef control )		{ SetWindowCancelButton( fWindow, control ); }

		void				EnableControlByID( ControlID theID );
		void				DisableControlByID( ControlID theID );

		void				ShowControlByID( ControlID theID );
		void				HideControlByID( ControlID theID );

		inline void			SetUserFocus(TTextField *focus)					{ currentFocus = focus; SetUserFocus(); }
		CGContextRef		FocusContext();
		void				EndContext();

	protected:
		void				InitWithPlatformWindow( WindowRef window );

		ThemeDrawState		GetDrawState()	{ return IsActive() ? kThemeStateActive : kThemeStateInactive; }
		void				DrawText(const Rect &bounds, CFStringRef inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext);
		void				DrawText(const Rect &bounds, StringPtr inString, ThemeFontID fontID, SInt16 inJust, CGContextRef inContext);

		virtual void		Draw() {}

		virtual void		Activated();

		virtual void		Deactivated()	{ DeactivateAllRegions(); }
		virtual inline bool	MouseWheelMoved(SInt16 delta, SInt16 deltaX=0) { return false; }

		virtual void		Moved() {}
		virtual void		Resized() {}
		virtual inline bool	Zoom() { return false; }

		virtual Point		GetIdealSize();
		
		virtual bool		UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index);
		virtual bool		HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index);
		
		inline void			RegisterForEvents( UInt32 numEvents, const EventTypeSpec* list )	{ AddEventTypesToHandler( fHandler, numEvents, list ); }
		inline void			UnregisterForEvents( UInt32 numEvents, const EventTypeSpec* list )	{ RemoveEventTypesFromHandler( fHandler, numEvents, list ); }

		virtual OSStatus	HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );

		void				SetAutomaticControlDragTrackingEnabled(bool enabled) { SetAutomaticControlDragTrackingEnabledForWindow(Window(), enabled); }

		// Mouse Tracking Region support
		virtual void		HandleTrackingRegion(TTrackingRegion *region, TCarbonEvent &event ) {}
		TTrackingRegion*	NewTrackingRegion(RgnHandle inRegion, SInt32 inID=0, OSType signature=CREATOR_CODE);
		TTrackingRegion*	NewTrackingRegion(Rect &inRect, SInt32 inID=0, OSType signature=CREATOR_CODE);
		void				AddTrackingRegion(TTrackingRegion *region);
		void				RemoveTrackingRegion(TTrackingRegion *region);
		void				TrackingRegionEntered(TTrackingRegion *region);
		TTrackingRegion*	TrackingRegionUnderMouse();
		void				ExitPreviousRegion();
		void				EnterRegionUnderMouse();
		void				SetAllRegionsActive(bool act=true);
		inline void			ActivateAllRegions()		{ SetAllRegionsActive(true); }
		inline void			DeactivateAllRegions()		{ SetAllRegionsActive(false); }

		void				PlatformWindowDisposed();
		
		
	private:

		void				Init();
		static pascal OSStatus		EventHandlerProc( EventHandlerCallRef handler, EventRef event, void* userData );
		static EventHandlerUPP		GetEventHandlerProc();

		
		WindowRef			fWindow;
		CGrafPtr			fPort;
		EventHandlerRef		fHandler;
};

#endif
