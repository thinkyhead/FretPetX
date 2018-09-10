/*
 *  FPChordPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2011 Scott Lahteine. All rights reserved.
 *
 */

#include "FPApplication.h"
#include "FPChordPalette.h"
#include "FPGuitarPalette.h"
#include "FPPreferences.h"
#include "FPScalePalette.h"
#include "TString.h"
#include "FPUtilities.h"
#include "TCarbonEvent.h"



FPTonesControl::FPTonesControl(WindowRef wind) : TControl(wind, 'chrd', 100) {}


bool FPTonesControl::Draw(const Rect &bounds) {
	ForeColor(blackColor);
	PaintRect(&bounds);
	UInt16		mask = globalChord.tones;

	if (mask) {
		char	funcs[64], *piece[10];
		bool	rom = fretpet->romanMode;

		if (rom) {
			strcpy(funcs, globalChord.ChordToneFunctions());

			int i = 0, p = 0;
			char c;
			bool st = true;
			while((c = funcs[i])) {
				if (c == ' ') {
					funcs[i++] = 0;
					st = true;
					continue;
				}

				if (st) {
					piece[p++] = &funcs[i];
					st = false;
				}
				i++;
			}
		}

		FontInfo	font;
		SetFont(&font, (StringPtr)"\pOptima", 14, normal);

		short		x = bounds.left + 15, xx = x;
		short		y = bounds.top + font.ascent + 3;
		Str15		name;
		UInt16		root = globalChord.Root(), tone = root;
		UInt16		key = globalChord.Key();
		int			p = 0;

		BackColor(blackColor);
		for (int i=0; i<OCTAVE; i++) {
			if (mask & BIT(tone)) {
				CopyCStringToPascal(rom ? piece[p++] : scalePalette->NameOfNote(key, tone), name);
				MoveTo(x, y);

				bool inScale = scalePalette->ScaleHasTone(key, tone);
				RGBForeColor(tone == root ? (inScale ? &rgbYellow : &rgbBrown) : (inScale ? &rgbWhite : &rgbGray8));
				DrawString(name);
				x += 34;
				if (x > bounds.right - 10) {
					y += font.ascent + font.descent;
					x = xx;
				}
			}
			INC_WRAP(tone, OCTAVE);
		}
	}

	return true;
}


#pragma mark -
FPChordNameControl::FPChordNameControl(WindowRef wind) : TControl(wind, 'chrd', 101) {
}


bool FPChordNameControl::Draw(const Rect &bounds) {

    CGrafPtr oldPort = TWindow().Focus();

	CGContextRef	gc;
	CGRect			cgBounds = CGBounds();	// (CGPoint)origin, (CGSize)size
	OSStatus		err;
	FPChord&		chord = globalChord;

	// Clear the window background, setting the pixels opaque
	// (No EraseRect substitute yet known!)
//	Rect outer = bounds;
//	InsetRect(&outer, -1, -1);
//	EraseRect(&outer);

	gc = FocusContext();

	CGContextSetLineWidth(gc, 2);
	CGContextSetShouldAntialias(gc, false);
	CGContextSetAlpha(gc, IsEnabled() && IsActive() ? 1.0 : 0.25);
//	CGMakeRoundedRectPath(gc, cgBounds, kCornerSize);
//	CGContextFillPath(gc);
	CGContextSetRGBFillColor(gc, 1.0, 1.0, 1.0, 1.0);
	CGContextFillRect(gc, cgBounds);
	CGContextSetRGBStrokeColor(gc, 0.0, 0.0, 0.0, 1.0);
	CGContextStrokeRectWithWidth(gc, cgBounds, 1.0);

	CGContextSetShouldAntialias(gc, true);

	SetFont(NULL, "\pLucida Grande", 14, normal, gc);

	TString	name, ext, missing;
	chord.ChordName(name, ext, missing);

	Rect bounds1 = { bounds.top, bounds.left + 10, bounds.bottom, bounds.right - 10 };

	Point	textSize;
	SInt16	baseline;
	err = GetThemeTextDimensions( name.GetCFStringRef(), kThemeCurrentPortFont, GetDrawState(), false, &textSize, &baseline );

	OffsetRect( &bounds1, 0, (bounds.bottom - bounds.top - textSize.v) / 2 );

	//
	//	Color red if the chord root isn't in key
	//	Draw dimmed if the chord has no root
	//
	if (scalePalette->ScaleHasTone(chord.key, chord.root))
		CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, chord.HasTone(chord.root) ? 1.0 : 0.5);
	else
		CGContextSetRGBFillColor(gc, 1.0, 0.0, 0.0, chord.HasTone(chord.root) ? 1.0 : 0.5);

	//
	//	Color red if the chord has no root
	//	Draw dimmed if the chord root isn't in key
	//
//	if (chord.HasTone(chord.root))
//		CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, scalePalette->ScaleHasTone(chord.root) ? 1.0 : 0.5);
//	else
//		CGContextSetRGBFillColor(gc, 1.0, 0.0, 0.0, scalePalette->ScaleHasTone(chord.root) ? 1.0 : 0.5);

	DrawText(bounds1, name.GetCFStringRef(), kThemeCurrentPortFont, teFlushLeft, gc);

	if (ext.Length()) {
		bounds1.left += 30;
		DrawText(bounds1, ext.GetCFStringRef(), kThemeCurrentPortFont, teFlushLeft, gc);
	}

	if (missing.Length())
		DrawText(bounds1, missing.GetCFStringRef(), kThemeCurrentPortFont, teFlushRight, gc);

	EndContext();

    SetPort(oldPort);

	return true;
}


#pragma mark -
FPMoreNamesControl::FPMoreNamesControl(WindowRef wind) : TCustomControl(wind, 'chrd', 102) {
	const EventTypeSpec	moreEvents[] = {
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlHit }
		};

	RegisterForEvents(GetEventTypeCount(moreEvents), moreEvents);
}


bool FPMoreNamesControl::Draw(const Rect &bounds) {

	CGContextRef gc = FocusContext();
	CGRect cgBounds = CGBounds();

	CGContextSetAlpha(gc, IsEnabled() && IsActive() ? 1.0 : 0.25);
	CGContextSetRGBFillColor(gc, 1.0, 1.0, 1.0, 1.0);
	CGContextFillRect(gc, cgBounds);

	if (globalChord.HasTones()) {
		for (int line=OCTAVE-1; line--;)
			DrawMoreLine(line, false, gc);
	}

	CGContextSetShouldAntialias(gc, false);
	CGContextSetAlpha(gc, IsEnabled() && IsActive() ? 1.0 : 0.25);
	CGContextSetRGBStrokeColor(gc, 0.0, 0.0, 0.0, 1.0);
	CGContextStrokeRectWithWidth(gc, cgBounds, 1.0);

	EndContext();

	return true;
}


void FPMoreNamesControl::DrawMoreLine(UInt16 line, bool hilite, CGContextRef inContext) {
	if (globalChord.HasTones()) {
		CGContextRef gc = inContext ? inContext : FocusContext();

		UInt16	bott = TWindow().Height();
		Rect	bounds = Bounds(), lineRect = bounds;
		InsetRect(&lineRect, 1, 1);

		float		linehi = (float)(lineRect.bottom - lineRect.top) / 11.0;

		lineRect.top += (int)(line * linehi);
		lineRect.bottom = (int)(lineRect.top + linehi);
//		if (lineRect.bottom >= bounds.bottom)
//			lineRect.bottom = bounds.bottom - 1;

		if (line & 1)
			CGContextSetRGBFillColor(gc, 1.0, 1.0, 1.0, 1.0);
		else
			CGContextSetRGBFillColor(gc, 0.93, 0.93, 1.0, 1.0);

		CGContextBeginPath(gc);
		CGRect cgLineRect = { { lineRect.left, bott - lineRect.bottom  }, { lineRect.right - lineRect.left, linehi } };
		CGContextAddRect(gc, cgLineRect);
		CGContextClosePath(gc);
		CGContextFillPath(gc);

		//
		// Draw the hilite, if any
		//
		RGBColor rgbHilite;
		if (hilite) {
			cgLineRect = CGRectInset(cgLineRect, 1.0, 1.0);
			(void)GetPortHiliteColor(WindowPort(), &rgbHilite);

			float arc_radius = cgLineRect.size.height / 2;
			float x1 = cgLineRect.origin.x + arc_radius;
			float x2 = cgLineRect.origin.x + cgLineRect.size.width - arc_radius;
			float y1 = cgLineRect.origin.y;
			float y2 = y1 + cgLineRect.size.height;
			float color[] = { (float)rgbHilite.red / 65535.0, (float)rgbHilite.green / 65535.0, (float)rgbHilite.blue / 65535.0, 1.0 };

			CGContextBeginPath(gc);
			CGContextSetFillColor(gc, color);
			CGContextMoveToPoint(gc, x1, y1);
			CGContextAddLineToPoint(gc, x2, y1);
			CGContextAddArc(gc, x2, y1 + arc_radius, arc_radius, RAD(270), RAD(90), false);
			CGContextAddLineToPoint(gc, x1, y2);
			CGContextAddArc(gc, x1, y1 + arc_radius, arc_radius, RAD(90), RAD(270), false);
			CGContextClosePath(gc);
			CGContextFillPath(gc);

			CGContextSetLineWidth(gc, 1);
			CGContextSetShouldAntialias(gc, true);
			CGContextBeginPath(gc);
			color[0] /= 4.0; color[1] /= 4.0; color[2] /= 4.0;
			CGContextSetStrokeColor(gc, color);
			CGContextMoveToPoint(gc, x1, y1);
			CGContextAddLineToPoint(gc, x2, y1);
			CGContextAddArc(gc, x2, y1 + arc_radius, arc_radius, RAD(270), RAD(90), false);
			CGContextAddLineToPoint(gc, x1, y2);
			CGContextAddArc(gc, x1, y1 + arc_radius, arc_radius, RAD(90), RAD(270), false);
			CGContextClosePath(gc);
			CGContextStrokePath(gc);
		}

		//
		// Get the Chord name components
		//
		Str255	name, ext, missing;
		UInt16	rootNote = NOTEMOD(globalChord.root + (KEY_FOR_INDEX(1)*(line+1)));
		globalChord.ChordName(rootNote, name, ext, missing);

		//
		// Draw the name using a theme text box.
		// The vertical alignment is kinda fudgy
		//
		Rect textRect = { lineRect.top - 1, lineRect.left + 9, lineRect.bottom - 1, lineRect.right - 10 };

		//
		//	Color red if the proposed root isn't in key
		//	Draw dimmed if the chord doesn't have the proposed root tone
		//
		if (scalePalette->ScaleHasTone(rootNote))
			CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, globalChord.HasTone(rootNote) ? 1.0 : 0.5);
		else
			CGContextSetRGBFillColor(gc, 1.0, 0.0, 0.0, globalChord.HasTone(rootNote) ? 1.0 : 0.5);

		//
		//	Color red if the chord doesn't have the proposed root tone
		//	Draw dimmed if the proposed root isn't in key
		//
//		if (globalChord.HasTone(rootNote))
//			CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, scalePalette->ScaleHasTone(rootNote) ? 1.0 : 0.5);
//		else
//			CGContextSetRGBFillColor(gc, 1.0, 0.0, 0.0, scalePalette->ScaleHasTone(rootNote) ? 1.0 : 0.5);


		DrawText(textRect, name, kThemeSmallSystemFont, teFlushLeft, gc);

		if (ext[0]) {
			textRect.left += 30;
			DrawText(textRect, ext, kThemeSmallSystemFont, teFlushLeft, gc);
		}

		if (missing[0])
			DrawText(textRect, missing, kThemeSmallSystemFont, teFlushRight, gc);

		if (inContext == NULL)
			EndContext();
	}
}


ControlPartCode FPMoreNamesControl::HitTest(Point where) {
	return globalChord.HasTones() ? kControlIndicatorPart : kControlNoPart;
}


bool FPMoreNamesControl::Track(MouseTrackingResult eventType, Point where) {
			int line = GetLineFromPoint(where);
	static	int	lastLine, firstLine;

	switch (eventType) {
		case kMouseTrackingMouseDown:
			lastLine = -1;
			firstLine = line;
			break;

		case kMouseTrackingMouseUp:
			if (line == firstLine)
				fretpet->DoChooseName(line);

			line = -1;
			break;

		case kMouseTrackingMouseExited:
			line = -1;
			break;

		case kMouseTrackingMouseEntered:
		case kMouseTrackingMouseDragged:
			break;
	}

	if (line != lastLine) {

		if (lastLine >= 0)
			DrawMoreLine(lastLine, false);

		if (line == firstLine)
			DrawMoreLine(line, true);

		lastLine = line;
	}

	return true;
}


SInt16 FPMoreNamesControl::GetLineFromPoint(Point where) {
	Rect	bounds = Bounds();
	SInt16	line = (int)(where.v / ((float)(bounds.bottom - bounds.top) / 11.0));

	return (line < 0 || line > 10 || where.h < 0 || where.h > bounds.right-bounds.left) ? -1 : line;
}


#pragma mark -
TTriangleControl::TTriangleControl(WindowRef wind) : TTriangle(wind, 'Cmor') { }


#pragma mark -
FPChordPalette::FPChordPalette() : FPPalette(CFSTR("Chord")) {
	WindowRef wind = Window();

	tonesControl	= new FPTonesControl(wind);
	nameControl		= new FPChordNameControl(wind);
	moreControl		= new FPMoreNamesControl(wind);
	lockButton		= new TPictureControl(wind, kFPCommandLock, kPictNewLock1);

	bool isOpen = preferences.GetBoolean(kPrefMoreNames, FALSE);

	triangleControl	= new TTriangleControl(wind);
	triangleControl->SetOpenState(isOpen);

	Rect	currSize = GetContentSize();
	SetContentSize(currSize.right, isOpen ? kChordOpenHeight : kChordClosedHeight);
}


FPChordPalette::~FPChordPalette() {
	delete tonesControl;
	delete nameControl;
	delete moreControl;
	delete lockButton;
	delete triangleControl;
}


void FPChordPalette::SavePreferences() {
	preferences.SetBoolean(kPrefMoreNames, triangleControl->GetOpenState());
	FPPalette::SavePreferences();
}


void FPChordPalette::Draw() {
//	Rect bounds = GetContentSize();
//	RGBForeColor(&rgbPalette);
//	PaintRect(&bounds);
//	ForeColor(blackColor);

	DrawControls(Window());
}


void FPChordPalette::UpdateNow() {
	tonesControl->DrawNow();
	nameControl->DrawNow();
	moreControl->DrawNow();
}


void FPChordPalette::Reset() {
	if (triangleControl->Value()) {
		triangleControl->SetOpenState(false);
		UpdateExtendedView();
	}

	FPPalette::Reset();
}


//-----------------------------------------------
//
//	HandleEvent
//
//	Handler for window events. As custom controls
//	are developed they will have their own event
//	handlers and this will go away.
//
OSStatus FPChordPalette::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
//	fprintf( stderr, "[%08X] Chord Palette Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	return FPPalette::HandleEvent(inRef, event);
}


void FPChordPalette::UpdateExtendedView() {
	Rect	currSize = GetContentSize();
	SetContentSize(currSize.right, triangleControl->GetOpenState() ? kChordOpenHeight : kChordClosedHeight);
//	TransitionSize(currSize.right, triangleControl->GetOpenState() ? kChordOpenHeight : kChordClosedHeight);
}


void FPChordPalette::UpdateLockButton() {
	lockButton->SetState(globalChord.IsLocked());
}


