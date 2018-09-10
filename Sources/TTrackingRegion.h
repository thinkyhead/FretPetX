/*!
	@file TTrackingRegion.h

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef TTRACKINGREGION_H
#define TTRACKINGREGION_H

class TWindow;

class TTrackingRegion {
	private:
		static SInt32			nextID;
		SInt32					fTrackingID;
		bool					isEntered;
		bool					enabled;
		bool					active;

		MouseTrackingRef		fTrackingRef;
		RgnHandle				fRegion;
		TWindow					*fWindow;

	public:
		static TTrackingRegion	*globalEnteredRegion;

							TTrackingRegion();
							TTrackingRegion(TWindow *inWindow, RgnHandle inRegion, SInt32 inID=0, OSType signature=CREATOR_CODE);
							TTrackingRegion(TWindow *inWindow, Rect &inRect, SInt32 inID=0, OSType signature=CREATOR_CODE);
		virtual				~TTrackingRegion();
		void				Init();

		void				Specify(TWindow *inWindow, RgnHandle inRegion, SInt32 inID=0, OSType signature=CREATOR_CODE);
		void				Specify(TWindow * inWindow, Rect &inRect, SInt32 inID=0, OSType signature=CREATOR_CODE);

		void				Change(RgnHandle inRegion);
		void				Change(Rect &inRect);

		inline TWindow*		GetTWindow()			{ return fWindow; }

		inline void			SetEnabled(bool ena)	{ (void)SetMouseTrackingRegionEnabled(fTrackingRef, ena ? IsActive() : false); enabled = ena; }
		inline void			Enable()				{ SetEnabled(true); }
		inline void			Disable()				{ SetEnabled(false); }
		inline bool			IsEnabled()				{ return enabled; }

		inline void			SetActive(bool act)		{ (void)SetMouseTrackingRegionEnabled(fTrackingRef, act ? IsEnabled() : false); active = act; }
		inline void			Activate()				{ SetActive(true); }
		inline void			Deactivate()			{ SetActive(false); }
		inline bool			IsActive()				{ return active; }

		inline int			GetID()					{ return fTrackingID; }
		inline RgnHandle	GetRegion()				{ return fRegion; }

		void				MarkEntered(bool b);
		bool				WasEntered()			{ return isEntered; }

		void				SimulateEnterEvent();
		void				SimulateExitEvent();
		void				SimulateIdleEvent();	// To send modifier keys
};

#include <list>
typedef	std::list<TTrackingRegion*>				TrackingRegionList;
typedef	TrackingRegionList::iterator			TrackingRegionIterator;
typedef	TrackingRegionList::reverse_iterator	TrackingRegionIteratorRev;

#endif
