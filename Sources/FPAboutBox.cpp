/**
 *  FPAboutBox.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TCarbonEvent.h"
#include "FPAboutBox.h"
#include "FPMacros.h"
#include "TString.h"
#include "CGHelper.h"
#include "FPApplication.h"

WindowDefSpec	aboutWindowSpec;
Rect			aboutBounds = { 0, 0, 300, 300 };

bool FPAboutBox::boxIsActive = false;

FPAboutBox::FPAboutBox() : TCustomWindow(&aboutWindowSpec, kMovableModalWindowClass, kWindowNoAttributes, aboutBounds) {

	boxIsActive = true;

	const EventTypeSpec	boxEvents[] = {
		{ kEventClassWindow, kEventWindowGetRegion },
		{ kEventClassMouse, kEventMouseUp },
		{ kEventClassKeyboard, kEventRawKeyDown }
		};

	RegisterForEvents( GetEventTypeCount( boxEvents ), boxEvents );

	// Disable Opacity
	SetOpaque(false);

	//
	// The splash image and mask!
	// Used by Draw() to show the image.
	//
	CFBundleRef thisBundle = CFBundleGetMainBundle();
	CFArrayRef locArray = CFBundleCopyBundleLocalizations(thisBundle);
	if (locArray) {
		CFArrayRef preferredLoc = CFBundleCopyPreferredLocalizationsFromArray(locArray);
		if (preferredLoc) {
			CFStringRef	curLocalization	= (CFStringRef)CFArrayGetValueAtIndex(preferredLoc, (CFIndex) 0);
			if (curLocalization) {
				CFURLRef url = CFBundleCopyResourceURLForLocalization(thisBundle, CFSTR("about"), CFSTR("png"), NULL, curLocalization);
				if (url) {
					aboutImage = CGLoadImage(url);
					CFRELEASE(url);
				}
			}
			CFRELEASE(preferredLoc);
		}
		CFRELEASE(locArray);
	}

	SetContentSize(CGImageGetWidth(aboutImage), CGImageGetHeight(aboutImage));

	Center(-30);
	Show();
}



FPAboutBox::~FPAboutBox() {
	Hide();
	WindowRef w = FrontNonFloatingWindow();
	if (w) ActivateWindow(w, true);
	CGImageRelease(aboutImage);
	boxIsActive = false;
}



void FPAboutBox::Draw() {
	TString		version, registered;

	CGContextRef gc = FocusContext();
	CGRect bounds = CGBounds();
	CGContextClearRect( gc, bounds );
	CGContextDrawImage(gc, bounds, aboutImage);

	Rect	inner = GetContentSize();

	inner.top += 172;
	CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, 1.0);
	version.SetWithFormatLocalized(CFSTR("Version %@"), CFSTR(FRETPET_VERSION));
	version.Draw(gc, inner, kThemeSystemFont, kThemeStateActive, teCenter);

#if KAGI_SUPPORT
	CFStringRef name = fretpet->AuthorizerName();
	if (name != NULL)
		registered.SetWithFormatLocalized(CFSTR("Registered to %@"), name);
	else {
		CGContextSetRGBFillColor(gc, 0.75, 0.0, 0.0, 1.0);
		registered.SetLocalized(CFSTR("*** UNREGISTERED ***"));
	}
#elif DEMO_ONLY
	CGContextSetRGBFillColor(gc, 0.2, 0.7, 0.0, 1.0);
	registered.SetLocalized(CFSTR("DEMO VERSION. Find FretPet on the App Store!"));
#elif APPSTORE_SUPPORT
	CGContextSetRGBFillColor(gc, 0.2, 0.7, 0.0, 1.0);
	registered.SetLocalized(CFSTR("* Licensed via the Mac App Store *"));
#endif

	inner.top += 85;
	registered.Draw(gc, inner, kThemeSystemFont, kThemeStateActive, teCenter);

//	EndContext();
}



void FPAboutBox::RegisterCustomClass(ToolboxObjectClassRef *customRef, EventHandlerUPP *customHandler) {
	const EventTypeSpec	boxEvents[] = {
		{ kEventClassWindow, kEventWindowHitTest }
	};

	TCustomWindow::RegisterCustomClass(
		CFSTR("com.thinkyhead.fretpet.about"),
		&aboutWindowSpec,
		boxEvents, GetEventTypeCount(boxEvents),
		customRef, customHandler);
}



OSStatus FPAboutBox::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
	OSStatus			result = eventNotHandledErr;

	switch ( event.GetClass() ) {
		case kEventClassWindow:
			switch ( event.GetKind() ) {
				case kEventWindowHitTest: {
					WindowDefPartCode where = wInContent;
					event.SetParameter(kEventParamWindowDefPart, typeWindowDefPartCode, where);
					result = noErr;
 					break;
				}

				case kEventWindowGetRegion: {
					WindowRegionCode	code;
					RgnHandle			rgn;

					event.GetParameter(kEventParamWindowRegionCode, typeWindowRegionCode, &code);
					event.GetParameter(kEventParamRgnHandle, &rgn);
					if (code == kWindowOpaqueRgn) {
						SetEmptyRgn(rgn);
						result = noErr;
					}
					break;
				}
			}
			break;

		case kEventClassMouse:
			switch ( event.GetKind() ) {
				case kEventMouseUp: {
					QuitModal();
//					Close();
					result = noErr;
					break;
				}
			}
			break;

		case kEventClassKeyboard:
			switch ( event.GetKind() ) {
				case kEventRawKeyDown:
					QuitModal();
					result = noErr;
					break;
			}
			break;
	}

	if (result == eventNotHandledErr)
		result = TWindow::HandleEvent(inRef, event);

	return result;
}



