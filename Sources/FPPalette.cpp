/*
 *  FPPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TCarbonEvent.h"
#include "FPPalette.h"
#include "FPApplication.h"
#include "FPPreferences.h"
#include "FPDocWindow.h"

FPPaletteList	FPPalette::paletteList;

//-----------------------------------------------
//
//  FPPalette
//
FPPalette::FPPalette(CFStringRef windowName) : FPWindow(windowName) {
	behavior = MAGNET_ALL;

	paletteList.push_back(this);

	//
	// Register to receive these events in addition
	// to the ones TWindow already handles.
	//
	const EventTypeSpec	paletteEvents[] = {
		{ kEventClassWindow, kEventWindowBoundsChanging }
	};

	RegisterForEvents( GetEventTypeCount( paletteEvents ), paletteEvents );
//    Show();
}


FPPalette::~FPPalette() {
	paletteList.remove(this);
}


OSStatus FPPalette::HandleEvent(EventHandlerCallRef inRef, TCarbonEvent &event) {
//	fprintf( stderr, "[%08X] FPPalette Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus    result = eventNotHandledErr;

	switch ( event.GetClass() ) {
		case kEventClassWindow:
			switch ( event.GetKind() ) {
				case kEventWindowBoundsChanging:
					Magnetize(event);
					result = noErr;
					break;

				case kEventWindowClose:
					Hide();
					result = noErr;
					break;

			}
			break;
	}

	//
	// If the event wasn't handled here we
	// give it to TWindow.
	//
	if (result == eventNotHandledErr)
		result = TWindow::HandleEvent(inRef, event);

	return result;
}


void FPPalette::Magnetize( TCarbonEvent &event ) {
	OSStatus		err;
	WindowRef		window;
	Rect			bounds;
	UInt32			attributes = 0;

	err = event.GetParameter(kEventParamDirectObject, &window);
	require_noerr( err, Magnetize_CantGetWindow );

	err = event.GetParameter(kEventParamCurrentBounds, &bounds);
	require_noerr( err, Magnetize_CantGetCurrentBounds );

	err = event.GetParameter(kEventParamAttributes, &attributes);

	// If we don't have any attributes, or we have them, but we aren't being called
	// due to DragWindow/ResizeWindow, then let's do nothing. This would happen if,
	// say, we were called from SetWindowBounds or the like (which doesn't actually
	// happen as I write this, but it will in time).

	if ( (err == noErr) && (attributes & kWindowBoundsChangeUserDrag ) != 0 ) {
		UInt32 diff;

		Rect	newBounds = bounds;
		SInt16	height = bounds.bottom - bounds.top;
		SInt16	width = bounds.right - bounds.left;

		if (behavior & MAGNET_PALETTES) {
			//
			// Go through the palette list (visible, and not this)
			//
			FPPalette *palette, *topPalette=NULL, *bottomPalette=NULL, *leftPalette=NULL, *rightPalette=NULL;
			UInt32	nearestToRight = 100000, nearestToLeft = 100000,
					nearestToTop = 100000, nearestToBottom = 100000;

			for (FPPaletteIterator itr = paletteList.begin();
					itr != paletteList.end();
					itr++) {
				palette = *itr;
				if (palette != this && palette->IsVisible()) {
					Rect paletteRect = palette->Bounds();
					paletteRect.bottom += kTitleBarFudge;
					paletteRect.top -= kTitleBarFudge;

					// Within the range for left-right testing?
					if (bounds.top < paletteRect.bottom - 10 && bounds.bottom > paletteRect.top + 10) {
						// My Right to its Left
						diff = abs( paletteRect.left - 1 - bounds.right );
						if ( diff < kDockingDistance && diff < nearestToRight ) {
							nearestToRight = diff;
							rightPalette = palette;
							leftPalette = NULL;
							newBounds.right = paletteRect.left - 1;
							newBounds.left = newBounds.right - width;
						}

						// My Left to its Right
						diff = abs( paletteRect.right + 1 - bounds.left );
						if ( diff < kDockingDistance && diff < nearestToLeft ) {
							nearestToLeft = diff;
							leftPalette = palette;
							rightPalette = NULL;
							newBounds.left = paletteRect.right + 1;
							newBounds.right = newBounds.left + width;
						}
					}

					// Within the range for top-bottom testing?
					if (bounds.left < paletteRect.right - 10 && bounds.right > paletteRect.left + 10) {
						// My Bottom to its Top
						diff = abs( paletteRect.top - 1 - bounds.bottom );
						if ( diff < kDockingDistance && diff < nearestToBottom ) {
							nearestToBottom = diff;
							bottomPalette = palette;
							topPalette = NULL;
							newBounds.bottom = paletteRect.top - 1;
							newBounds.top = newBounds.bottom - height;
						}

						// My Top to its Bottom
						diff = abs( paletteRect.bottom + 1 - bounds.top );
						if ( diff < kDockingDistance && diff < nearestToTop ) {
							nearestToTop = diff;
							topPalette = palette;
							bottomPalette = NULL;
							newBounds.top = paletteRect.bottom + 1;
							newBounds.bottom = newBounds.top + height;
						}
					}

				}

			} // loop

			if (behavior & MAGNET_INNER) {
				if (topPalette || bottomPalette) {
					FPPalette *thePalette = topPalette ? topPalette : bottomPalette;
					Rect paletteRect = thePalette->Bounds();

					// My Left to its Left
					diff = abs( paletteRect.left - bounds.left );
					if ( diff < kDockingDistance && diff < nearestToLeft ) {
						nearestToLeft = diff;
						newBounds.left = paletteRect.left;
						newBounds.right = newBounds.left + width;
					}
					else {
						// My Right to its Right
						diff = abs( paletteRect.right - bounds.right );
						if ( diff < kDockingDistance && diff < nearestToRight ) {
							nearestToRight = diff;
							newBounds.right = paletteRect.right;
							newBounds.left = newBounds.right - width;
						}
					}
				} // top || bottom

				if (leftPalette || rightPalette) {
					FPPalette *thePalette = leftPalette ? leftPalette : rightPalette;
					Rect paletteRect = thePalette->Bounds();

					// My Top to its Top
					diff = abs( paletteRect.top - bounds.top );
					if ( diff < kDockingDistance && diff < nearestToTop ) {
						nearestToTop = diff;
						newBounds.top = paletteRect.top;
						newBounds.bottom = newBounds.top + height;
					}
					else {
						// My Bottom to its Bottom
						diff = abs( paletteRect.bottom - bounds.bottom );
						if ( diff < kDockingDistance && diff < nearestToBottom ) {
							nearestToBottom = diff;
							newBounds.bottom = paletteRect.bottom;
							newBounds.top = newBounds.bottom - height;
						}
					}
				} // left || right

			} // MAGNET_INNER

		} // MAGNET_PALETTES

		//
		// Outer edges take precedence
		// The fact we use the same original bounds
		// for all checking is significant.
		// Closeness is measured for each edge also
		// and when something not closer is proposed it is rejected
		//

		if (behavior & MAGNET_EDGES) {
			GDHandle theDevice = DMGetFirstScreenDevice(true);

			while (theDevice) {
				Rect displayRect;
				(void) GetAvailableWindowPositioningBounds(theDevice, &displayRect);
				displayRect.top += kTitleBarFudge;
				theDevice = DMGetNextScreenDevice(theDevice, true);

				// Is any part of me within the current rectangle?
				if ( RectsOverlap(bounds, displayRect) ) {
					// Left
					diff = abs( bounds.left - displayRect.left );
					if ( diff < kDockingDistance /* || bounds.left < displayRect.left */ ) {
						newBounds.left = displayRect.left + 1;
						newBounds.right = newBounds.left + width;
					}

					// Top
					diff = abs( bounds.top - displayRect.top );
					if ( diff < kDockingDistance /* || bounds.top < displayRect.top */ ) {
						newBounds.top = displayRect.top + 1;
						newBounds.bottom = newBounds.top + height;
					}
					// Right
					diff = abs( bounds.right - displayRect.right );
					if ( diff < kDockingDistance /* || bounds.right > displayRect.right */ ) {
						newBounds.right = displayRect.right - 1;
						newBounds.left = newBounds.right - width;
					}

					// Bottom
					diff = abs( bounds.bottom - displayRect.bottom );
					if ( diff < kDockingDistance /* || bounds.bottom > displayRect.bottom */ ) {
						newBounds.bottom = displayRect.bottom - 1;
						newBounds.top = newBounds.bottom - height;
					}

				} // rect overlap?
			}
		}

		event.SetParameter(kEventParamCurrentBounds, newBounds);
	}

Magnetize_CantGetCurrentBounds:
Magnetize_CantGetWindow:
	return;
}


