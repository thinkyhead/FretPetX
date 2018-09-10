/*
 *  TControls.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TControls.h"
#include "FPUtilities.h"
#include "FPApplication.h"

#include "FPSprite.h"
#include "TWindow.h"
#include "TCarbonEvent.h"

//-----------------------------------------------
//
//	TSlider
//

TSlider::TSlider(WindowRef wind, UInt32 cmd) : TControl(wind, cmd) {
	sliderAction = NewControlActionUPP(LiveSliderProc);
	SetControlAction(Control(), sliderAction);
}

TSlider::~TSlider() {
	if (sliderAction)
		DisposeControlActionUPP(sliderAction);
}

pascal void TSlider::LiveSliderProc(ControlRef control, ControlPartCode part) {
	if ( part == kControlIndicatorPart ) {
		TSlider *slider = (TSlider*)GetControlReference(control);
	
		if ( slider != NULL )
			slider->IndicatorMoved();
	}
}


#pragma mark -
OSStatus TPopupMenu::ClearAllItems() {
	return ClearItems(1, CountItems());
}


OSStatus TPopupMenu::ClearItems(MenuItemIndex first, ItemCount count) {
	OSStatus err = DeleteMenuItems(GetMenu(), first, count);
	SetMaximum(0);
	return err;
}


UInt16 TPopupMenu::CountItems() {
	return CountMenuItems(GetMenu());
}


OSStatus TPopupMenu::AddItem(CFStringRef itemText, MenuCommand cid) {
	OSStatus err = AppendMenuItemTextWithCFString(GetMenu(), itemText, 0, cid, NULL);
	SetMaximum(CountItems());
	return err;
}


OSStatus TPopupMenu::InsertItem(CFStringRef itemText, MenuItemIndex after, MenuCommand cid) {
	OSStatus err = InsertMenuItemTextWithCFString(GetMenu(), itemText, after, 0, cid);
	SetMaximum(CountItems());
	return err;
}


bool TPopupMenu::ControlHit(Point where) {
	Rect			wBounds = GetTWindow()->Bounds();
	Rect			cBounds = Bounds();
	(void) PopUpMenuSelect(GetMenu(), cBounds.top + wBounds.top, cBounds.left + wBounds.left, Value());
	return true;
}

