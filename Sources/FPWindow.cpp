/*
 *  FPWindow.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPWindow.h"
#include "FPPreferences.h"

FPWindow::FPWindow(CFStringRef windowName) : TWindow(MAIN_NIB_NAME, windowName) {
	veryHidden		= false;
	positionKey		= NULL;
	sizeKey			= NULL;
	visibleKey		= NULL;

	SetDefaultBounds();
}


FPWindow::~FPWindow() {
	if (positionKey)	CFRELEASE(positionKey);
	if (sizeKey)		CFRELEASE(sizeKey);
	if (visibleKey)		CFRELEASE(visibleKey);
}


bool FPWindow::LoadPreferences(bool showing) {
	bool shown = false;
	if (PreferenceKey() != NULL) {
		// Position
		if (preferences.IsSet(PositionKey())) {
			Point newPos = preferences.GetPoint(PositionKey());
			Move(newPos.h, newPos.v);
		}

		// Size
/*
		if (preferences.IsSet(SizeKey())) {
			Point newSize = preferences.GetPoint(SizeKey());
			SetContentSize(newSize.h, newSize.v);
		}
*/
		// Visibility
		if (showing) {
			if (preferences.GetBoolean(VisibleKey(), TRUE)) {
				Show();
				shown = true;
			}
			else
				ReallyHide();
		}

	}

	return shown;
}


void FPWindow::SavePreferences() {
	if (PreferenceKey() != NULL) {
		Rect bounds = Bounds();
		CFStringRef key = PositionKey();
		preferences.SetPoint(key, bounds.left, bounds.top);

		key = SizeKey();
		preferences.SetPoint(key, bounds.right - bounds.left, bounds.bottom - bounds.top);

		key = VisibleKey();
		preferences.SetBoolean(key, !veryHidden);

		preferences.Synchronize();
	}
}


void FPWindow::ReallyHide() {
	TWindow::Hide();
	veryHidden = true;
}


void FPWindow::ShowIfShown() {
	if (!veryHidden) Show();
}


void FPWindow::Show() {
	TWindow::Show();
	veryHidden = false;
}


void FPWindow::Reset() {
	Rect avail;
	(void) GetAvailableWindowPositioningBounds(GetMainDevice(), &avail);

	Move(originalBounds.left + avail.left, originalBounds.top);
	SetContentSize( originalBounds.right - originalBounds.left, originalBounds.bottom - originalBounds.top );
}

