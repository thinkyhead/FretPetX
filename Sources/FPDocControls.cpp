/*
 *  FPDocControls.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPDocControls.h"
#include "FPDocWindow.h"
#include "FPHistory.h"
#include "FPApplication.h"
#include "CGHelper.h"
#include "TPictureControl.h"

#pragma mark -
#pragma mark Document Controls
#pragma mark -
//-----------------------------------------------
//
// FPTabsControl
//
FPTabsControl::FPTabsControl(WindowRef wind) : TCustomControl(wind, 'docu', 103) {
}


bool FPTabsControl::Track(MouseTrackingResult eventType, Point where) {
	SInt16 newPart = where.h / (kDocWidth / 4);
	CONSTRAIN(newPart, 0, 3);
	fretpet->DoSelectPart(newPart);
	return true;
}


#pragma mark -
//-----------------------------------------------
//
//	FPDocPopup
//
FPDocPopup::FPDocPopup(WindowRef w) : TControl(w, 'docu', 101) {
	numberMode = false;

	MenuRef			menu, subMenu, seqMenu, instSub;
	MenuItemIndex	instItem, subItem, pickItem, newPickItem;
	if ( ! GetIndMenuItemWithCommandID(NULL, kFPCommandInstrumentMenu, 1, &seqMenu, &instItem) ) {
		if ( ! GetMenuItemHierarchicalMenu(seqMenu, instItem, &instSub) ) {
			OSStatus err = DuplicateMenu(instSub, &instrumentMenu);
			if (!err) {
				if ( ! GetIndMenuItemWithCommandID(NULL, kFPCommandPickInstrument, 1, &seqMenu, &pickItem) ) {
					AppendMenuItemTextWithCFString(instrumentMenu, CFSTR("sep"), kMenuItemAttrSeparator, 0, NULL);
					AppendMenuItemTextWithCFString(instrumentMenu, CFSTR("hey"), kMenuItemAttrUpdateSingleItem, 0, &newPickItem);
					MenuItemDataRec ioData;
					ioData.whichData =
						kMenuItemDataCmdVirtualKey |
						kMenuItemDataCFString |
						kMenuItemDataAttributes |
						kMenuItemDataCommandID |
						kMenuItemDataEnabled |
						kMenuItemDataCmdKey |
						kMenuItemDataCmdKeyGlyph |
						kMenuItemDataCmdKeyModifiers;
					err = CopyMenuItemData(seqMenu, pickItem, false, &ioData);
					err = SetMenuItemData(instrumentMenu, newPickItem, false, &ioData);
				}
				(void)SetMenuFont(instrumentMenu, 0, 10);
				UInt16 numItems = CountMenuItems(instrumentMenu);
				for (int i=1; i<=numItems; i++) {
					(void)GetMenuItemHierarchicalMenu(instrumentMenu, i, &subMenu);
					if (subMenu) (void)SetMenuFont(subMenu, 0, 10);
				}

			}
		}
	}

	const EventTypeSpec	popEvents[] = {
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlHit },
		{ kEventClassMouse, kEventMouseWheelMoved }
	};

	RegisterForEvents( GetEventTypeCount( popEvents ), popEvents );
}


bool FPDocPopup::Draw(const Rect &bounds) {
	int				state = IsActive() ? (IsPressed() ? kThemeStatePressed : kThemeStateActive) : kThemeStateInactive;

	ForeColor(blackColor);
	EraseRect(&bounds);
	ThemeButtonDrawInfo	tinfo = { state, kThemeButtonOff, kThemeAdornmentNone };
	OSStatus err = DrawThemeButton(
		&bounds,			// our bounds
		kThemeSmallBevelButton,	// a popup button
		&tinfo,				// state to draw
		NULL,				// no previous info
		NULL,				// erase for me
		NULL,				// don't draw the label
		(UInt32)this		// might as well pass this
		);

	Rect arrowRect = { bounds.bottom - 7, bounds.right - 10, bounds.bottom, bounds.right };
	err = DrawThemePopupArrow(
		&arrowRect,			// rect
		kThemeArrowDown,	// orientation
		kThemeArrow7pt,		// size
		state,				// current state
		NULL,				// no erase proc
		0					// no erase data
		);

	//
	//  DRAW BUTTON TEXT
	//
	CGContextRef gc = FocusContext();
	FPDocWindow *wind = (FPDocWindow*)GetTWindow();
	Rect bRect = { bounds.top + 1, bounds.left + 2, bounds.bottom, bounds.right - 3 };

	if (numberMode) {
		TString	string;

		SInt16	trueGM = wind->document->CurrentInstrument();
		string = (trueGM <= kLastGMInstrument) ? "GM" : ( (trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit) ? "KIT" : "GS" );

		SetFont(NULL, (StringPtr)"\pMonaco", 20, bold);
		CGContextSetRGBFillColor(gc, CG_RGBCOLOR(&rgbGray12), IsActive() ? 1.0 : 0.5);
		string.Draw( gc, bRect, kThemeCurrentPortFont, GetDrawState(), teFlushLeft );

		string = trueGM;
		CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, IsActive() ? 1.0 : 0.5);
		Point textSize = string.GetDimensions(kThemeSmallEmphasizedSystemFont);
		bRect.top += (bRect.bottom - bRect.top - textSize.v) / 2;
		string.Draw( gc, bRect, kThemeSmallEmphasizedSystemFont, GetDrawState(), teCenter );
	}
	else {
		TString string;
		StringPtr iname = wind->document->CurrentInstrumentName();
		string.SetFromPascalString(iname);
		delete [] iname;

		CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, IsActive() ? 1.0 : 0.5);
		Point textSize = string.GetDimensions(kThemeMiniSystemFont, GetDrawState(), bRect.right - bRect.left);
		bRect.top += (bRect.bottom - bRect.top - textSize.v) / 2;
		string.DrawWrapped( gc, bRect, kThemeMiniSystemFont, GetDrawState(), teCenter );
	}

	EndContext();

	return true;
}


ControlPartCode FPDocPopup::HitTest(Point where) {
	Rect bounds = Bounds();
	where.h += bounds.left;
	where.v += bounds.top;
	return PtInRect(where, &bounds) ? kControlMenuPart : kControlNoPart;
}


bool FPDocPopup::ControlHit(Point where) {
	if (IS_COMMAND(GetCurrentEventKeyModifiers()))
		ToggleNumberMode();
	else {
		Rect		bounds = Bounds();
		Point		globPoint = { bounds.top, bounds.left };
		FPDocWindow *wind = (FPDocWindow*)GetTWindow();
		wind->Focus();
		LocalToGlobal(&globPoint);
		MenuItemIndex item = player->GetInstrumentGroup();
		(void) PopUpMenuSelect(instrumentMenu, globPoint.v, globPoint.h, item);
	}

	return true;
}


bool FPDocPopup::MouseWheelMoved(SInt16 deltaY, SInt16 deltaX) {
	if (fretpet->swipeIsReversed) deltaX *= -1, deltaY *= -1;
	fretpet->AddToCurrentInstrument((ABS(deltaX) > ABS(deltaY)) ? -deltaX : deltaY);
	return true;
}


#pragma mark -
//-----------------------------------------------
//
// FPDocSlider
//
FPDocSlider::FPDocSlider( WindowRef wind, OSType sig, UInt32 cid ) : TCustomControl(wind, sig, cid) {
	const EventTypeSpec	wheelEvents[] = {
			{ kEventClassMouse, kEventMouseWheelMoved }
		};
	RegisterForEvents(GetEventTypeCount(wheelEvents), wheelEvents);

	ispressed	= false;
	snap		= 0;
}


bool FPDocSlider::Draw(const Rect &bounds) {
	CGContextRef	gc = FocusContext();

	SInt32 val = Value(), min = Minimum(), max = Maximum();

	Rect	windSize = GetTWindow()->GetContentSize();
	CGRect sliderRect = { { bounds.left, windSize.bottom - bounds.bottom }, { bounds.right - bounds.left, bounds.bottom - bounds.top } };
	CGRect innerRect = CGRectInset(sliderRect, 1, 1);
	float	w = CGRectGetWidth(innerRect),
			l = CGRectGetMinX(innerRect),
			r = l + w,
			b = CGRectGetMinY(innerRect),
			t = b + CGRectGetHeight(innerRect),
			span = (max - min + 1),
			newSize = w * (val - min + 1) / span,
			tickSpace = w / (span / snap);

//	printf("width:%.2f  min:%ld  max:%ld  span:%.2f  space:%.2f\n", w, min, max, span, tickSpace);
	
	// Draw outer rect as an inset
	CGDraw3DBox(gc, sliderRect, true, &rgbPalette);

	// Draw tick marks
	float lx = l + tickSpace + 1.0;
	CGContextSetLineWidth(gc, 1);
	CGContextSetShouldAntialias(gc, false);
	CGContextSetRGBStrokeColor(gc, CG_RGBCOLOR(&rgbGray12), 1);
	CGPoint	points[2];
	do {
		points[0].x = points[1].x = lx;
		points[0].y = b;
		points[1].y = t;
		CGContextBeginPath(gc);
		CGContextAddLines(gc, points, 2);
		CGContextStrokePath(gc);
		lx += tickSpace;
	} while (lx <= r);

	if (newSize > 0.0) {
		innerRect.size.width = newSize;
		CGDraw3DBox(gc, innerRect, false, ispressed ? &rgbLtBlue2 : &rgbLtBlue);
	}

	TString	*valString = ValueString();
	if (!valString)
		valString = TString::NewWithFormat( CFSTR("%d"), Value() );

	SetFont(NULL, (StringPtr)"\pLucida Grande", 9, bold);

	Point	textSize;
	SInt16	baseline;

	textSize = valString->GetDimensions(kThemeCurrentPortFont, GetDrawState(), 0, &baseline);

	Rect	textBounds = bounds;
	bool	doLeft = LeftAlign();

	if (doLeft) {
		short x = (short)(textBounds.left + newSize - textSize.h - 2.0);
		if (x < textBounds.left + 1)
			x = textBounds.left + 1;

		textBounds.left = x;
	}
	else {
		float x = newSize;
		if (x < 45.0) x = 45.0;
		textBounds.right = textBounds.left + (short)x;
	}

	CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, 1.0);
	CGContextSetShouldAntialias(gc, true);
	valString->Draw(gc, textBounds, kThemeCurrentPortFont, GetDrawState(), doLeft ? teFlushLeft : teFlushRight );

	delete valString;

	EndContext();

	return true;
}


bool FPDocSlider::Track(MouseTrackingResult eventType, Point where) {
	static UInt32 clickTime;
	static UInt16 dragCount;

	switch (eventType) {
		case kMouseTrackingMouseDown:
			clickTime = TickCount();
			ispressed = true;
			dragCount = 0;
//			HiliteControl(ControlRef(), kControlIndicatorPart);
			DoMouseDown();

		case kMouseTrackingMouseDragged: {
			if (eventType == kMouseTrackingMouseDragged && dragCount++ > 2)
				clickTime = 0;

			Rect	bounds = Bounds();
			SInt32	size = bounds.right - bounds.left;
			SInt32	range = Maximum() - Minimum();
			SInt32	pos = where.h;
			CONSTRAIN(pos, 0, size);

			if (size > range) {
				UInt32	prop = (size + range / 2) / range;
				SetValue(Minimum() + pos / prop, true);
			}
			else {
				UInt32	prop = (range + size / 2) / size;
				SetValue(Minimum() + pos * prop, true);
			}

			break;
		}

		case kMouseTrackingMouseUp:
			ispressed = false;
			if (snap && (TickCount() - clickTime < GetDblTime())) {
				UInt32 val = Value() + (snap / 2);
				SetValue(val - (val % snap), true);
			}
			DrawNow();
			DoMouseUp();
			break;
	}

	return true;
}


bool FPDocSlider::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	UInt32 mySnap = snap / 2; 
	SInt32 oldVal = Value();
	if (fretpet->swipeIsReversed) delta *= -1, deltaX *= -1;
	SInt32 newVal = oldVal + (delta - deltaX) * mySnap;
	newVal += (newVal >  oldVal) ? 0 : (mySnap - 1);
	newVal -= (newVal % mySnap);
	SetValue(newVal);
	if (oldVal != Value())
		ValueChanged();
	return true;
}


#pragma mark -
//-----------------------------------------------
//
// FPTempoDoubler
//
FPTempoDoubler::FPTempoDoubler(WindowRef w) : TPictureControl(w, kFPDoubleTempo, kPictDoubleButton) {
}


UInt16 FPTempoDoubler::GetFrameForState() {
//	fprintf(stderr, "A:%d E:%d P:%d\n", IsActive(), IsEnabled(), IsPressed());
	return (IsPressed() && IsActive() ? (state ? kFrameSetPressed : kFramePressed) : (state ? kFrameSet : kFrameNormal));
}


#pragma mark -
TString* FPTempoSlider::ValueString() {
	FPDocWindow *wind = (FPDocWindow*)GetTWindow();
	UInt32	tempo = Value() * wind->document->TempoMultiplier();

	UInt16 temp = tempo / 4;
	UInt16 rema = tempo % 4;

	TString *number = new TString(rema ? "\305" : "   ");
	*number += temp;
	*number += CFSTR(" bpm");

	return number;
}


bool FPTempoSlider::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	SInt32 oldVal = Value();

	(void)FPDocSlider::MouseWheelMoved(delta, deltaX);

	if (oldVal != Value()) {
		event = fretpet->StartUndo(UN_TEMPO, CFSTR("Tempo Change"));
		event->SetPartAgnostic();
		event->SaveDataBefore(CFSTR("tempo"), oldVal);
		event->SetMergeUnderInterim(30);
		event->SaveDataAfter(CFSTR("tempo"), Value());
		event->Commit();
	}

	return true;
}


void FPTempoSlider::ValueChanged() {
	fretpet->DoSetTempo(Value(), false);
}


void FPTempoSlider::DoMouseDown() {
	event = fretpet->StartUndo(UN_TEMPO, CFSTR("Tempo Change"));
	event->SetPartAgnostic();
	event->SaveDataBefore(CFSTR("tempo"), Value());
}


void FPTempoSlider::DoMouseUp() {
	if ( Value() != event->GetDataBefore(CFSTR("tempo")) ) {
		event->SaveDataAfter(CFSTR("tempo"), Value());
		event->Commit();
	}
}


#pragma mark -
bool FPVelocitySlider::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	SInt32 oldVal = Value();

	(void)FPDocSlider::MouseWheelMoved(delta, deltaX);

	if (oldVal != Value()) {
		event = fretpet->StartUndo(UN_VELOCITY, CFSTR("Velocity Change"));
		event->SetPartAgnostic();
		event->SaveDataBefore(CFSTR("part"), player->CurrentPart());
		event->SaveDataBefore(CFSTR("velocity"), oldVal);
		event->SetMergeUnderInterim(30);
		event->SaveDataAfter(CFSTR("velocity"), Value());
		event->Commit();
	}

	return true;
}


void FPVelocitySlider::ValueChanged() {
	fretpet->DoSetVelocity(Value(), kCurrentChannel, false, true);
}


void FPVelocitySlider::DoMouseDown() {
	event = fretpet->StartUndo(UN_VELOCITY, CFSTR("Velocity Change"));
	event->SetPartAgnostic();
	event->SaveDataBefore(CFSTR("part"), player->CurrentPart());
	event->SaveDataBefore(CFSTR("velocity"), Value());
}


void FPVelocitySlider::DoMouseUp() {
	if ( Value() != event->GetDataBefore(CFSTR("velocity")) ) {
		event->SaveDataAfter(CFSTR("velocity"), Value());
		event->Commit();
	}
}


#pragma mark -
bool FPSustainSlider::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	SInt32 oldVal = Value();

	(void)FPDocSlider::MouseWheelMoved(delta, deltaX);

	if (oldVal != Value()) {
		event = fretpet->StartUndo(UN_SUSTAIN, CFSTR("Sustain Change"));
		event->SetPartAgnostic();
		event->SaveDataBefore(CFSTR("part"), player->CurrentPart());
		event->SaveDataBefore(CFSTR("sustain"), oldVal);
		event->SetMergeUnderInterim(30);
		event->SaveDataAfter(CFSTR("sustain"), Value());
		event->Commit();
	}

	return true;
}


void FPSustainSlider::ValueChanged() {
	fretpet->DoSetSustain(Value(), kCurrentChannel, false, true);
}


void FPSustainSlider::DoMouseDown() {
	event = fretpet->StartUndo(UN_SUSTAIN, CFSTR("Sustain Change"));
	event->SetPartAgnostic();
	event->SaveDataBefore(CFSTR("part"), player->CurrentPart());
	event->SaveDataBefore(CFSTR("sustain"), Value());
}


void FPSustainSlider::DoMouseUp() {
	if ( Value() != event->GetDataBefore(CFSTR("sustain")) ) {
		event->SaveDataAfter(CFSTR("sustain"), Value());
		event->Commit();
	}
}


