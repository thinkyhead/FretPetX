/*
 *  FPDiagramPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPDiagramPalette.h"

#include "FPApplication.h"
#include "FPGuitarPalette.h"
#include "FPScalePalette.h"
#include "FPUtilities.h"
#include "TCarbonEvent.h"
#include "TString.h"


FPDiagramPalette *diagramPalette = NULL;


#define	GAPH	19
#define	GAPV	24
#define	DIAX	26
#define	DIAY	2
#define	DIAV	158

#define kDotRadius		8
#define	kDiagramFrets	7
#define kDiagramMinY	5

#pragma mark -
FPDiagramControl::FPDiagramControl(WindowRef wind) : TCustomControl(wind, 'diag', 103) {
}


bool FPDiagramControl::Draw(const Rect &bounds) {
	FPDiagramPalette* diag = (FPDiagramPalette*)GetTWindow();
	diag->DrawDiagram(bounds);

	return true;
}


ControlPartCode FPDiagramControl::HitTest( Point where ) {
	if (globalChord.IsBracketEnabled()) {
		if ( (where.v >= kDiagramMinY)
			&& (where.v < kDiagramMinY + GAPV * kDiagramFrets)
			&& (where.h > DIAX - kDotRadius)
			&& (where.h < DIAX + GAPH * NUM_STRINGS + kDotRadius) )
			return kControlIndicatorPart;
	}

	return kControlNoPart;
}


bool FPDiagramControl::Track( MouseTrackingResult eventType, Point where ) {
	SInt16			fret, string;
	static int		lastPos = -1;
	int				newPos;

	GetPositionFromPoint(where, &string, &fret);

	if (fret >= 0) {
		UInt16 theTone = guitarPalette->Tone(string, fret);

		newPos = string * MAX_FRETS + fret;

		switch (eventType) {
			case kMouseTrackingMouseDown:

				switch(CountClicks(newPos==lastPos, 3)) {
					case 1:
						break;

					case 2:
						fretpet->DoToggleGuitarTone(string, fret);
						break;

					case 3:
						fretpet->DoAddTriadForTone(theTone);
						break;
				}
				break;

			case kMouseTrackingMouseUp:
				lastPos = -1;
				break;

			case kMouseTrackingMouseEntered:
			case kMouseTrackingMouseDragged:
			case kMouseTrackingMouseExited:
				break;
		}

		if (lastPos != newPos) {
			fretpet->DoMoveFretCursor(string, fret);
			lastPos = newPos;
		}
	}

	return true;
}


void FPDiagramControl::GetPositionFromPoint( Point where, SInt16 *string, SInt16 *fret ) {
	SInt16 newString = (where.h - (DIAX - kDotRadius)) / GAPH;
	CONSTRAIN(newString, 0, NUM_STRINGS - 1);
	SInt16 newFret = (where.v - kDiagramMinY) / GAPV;
	CONSTRAIN(newFret, 0, kDiagramFrets - 1);

	*string	= newString;
	*fret = globalChord.brakLow + newFret;
}


#pragma mark -
FPDiagramPalette::FPDiagramPalette() : FPPalette(CFSTR("Diagram")) {
	SetContentSize(135, Height());
	SetDefaultBounds();
	diagramControl	= new FPDiagramControl(Window());
}


void FPDiagramPalette::Draw() {
	Rect		bounds = GetContentSize();
	Rect		dRect = diagramControl->Bounds();

	bounds.bottom = dRect.top - 1;

/*
	RGBForeColor(&rgbPalette);
	PaintRect(&bounds);
	ForeColor(blackColor);
*/

//	EraseRect(&bounds);

//	MoveTo(0, bounds.bottom);
//	LineTo(bounds.right, bounds.bottom);

	DrawControls(Window());
}


void FPDiagramPalette::DrawDiagram(const Rect &bounds) {
	SInt16			i, j, y, mx, my, lowFret;
	FPChord&		chord = globalChord;
	short			diax = bounds.left + DIAX, diay = bounds.top + DIAY;
	UInt16			bott = Height();

	CGContextRef	gc = FocusContext();

	CGContextSetShouldAntialias(gc, false);

	CGContextSetRGBFillColor(gc, 0.95, 0.95, 1, 1);
	CGContextFillRect(gc, CGRectMake(bounds.left, bounds.top, bounds.right-bounds.left, bounds.bottom-bounds.top));

	CGPoint	points[2];
	CGContextSetRGBStrokeColor(gc, 0, 0, 0, 1);
	CGContextSetLineWidth(gc, 1);

	float xx = diax, basey = bott - diay;
	for (i=NUM_STRINGS; i--;) {
		points[0].x = points[1].x = xx;
		points[0].y = basey;
		points[1].y = basey - DIAV;

		CGContextBeginPath(gc);
		CGContextAddLines(gc, points, 2);
		CGContextClosePath(gc);
		CGContextStrokePath(gc);

		xx += GAPH;
	}

	lowFret = chord.bracketFlag ? chord.brakLow : 0;

	float yy = basey;
	for (i=0; i<kDiagramFrets; i++) {
		j = i+lowFret-1;
		if (j>=0) {
			points[0].y = points[1].y = yy;
			points[0].x = diax;
			points[1].x = diax + GAPH * 5;

			CGContextBeginPath(gc);
			CGContextAddLines(gc, points, 2);
			CGContextClosePath(gc);
			if (j==0) CGContextSetLineWidth(gc, 2);
			CGContextStrokePath(gc);
			if (j==0) CGContextSetLineWidth(gc, 1);
		}
		yy -= GAPV;
	}

	//
	// Draw Dots and Text for each string
	//
	if (chord.bracketFlag) {
		FatChord fatChord = chord.TwoOctaveChord();

		CGContextSetLineWidth(gc, 0);

		TString fretString(lowFret);
		SetFont(NULL, (StringPtr)"\pHelvetica", 16, normal, gc);
		Point textSize = fretString.GetDimensions(kThemeCurrentPortFont);
		CGContextSetRGBFillColor(gc, 0, 0, 1, 1);
		CGContextSetShouldAntialias(gc, true);
		fretString.Draw(gc, diax - kDotRadius - textSize.h, bott - (diay + (GAPV + 16) / 2));

		CGContextSetLineWidth(gc, 1);
		SetFont(NULL, (StringPtr)"\pGeneva", 10, normal, gc);
		TString numString;

		for (i=0; i<NUM_STRINGS; i++) {
			mx = diax + GAPH * (fretpet->IsTablatureReversed() ? NUM_STRINGS - i - 1 : i);
			y = chord.fretHeld[i];

			if (y < lowFret + kDiagramFrets) {
				if (y == -1) {
					my = diay + DIAV;
					CGContextSetRGBFillColor(gc, 1, 0, 0, 1);						// red
					numString = "X";
					CGContextSetShouldAntialias(gc, true);
				}
				else {
					UInt16 root = fretpet->chordRootType ? chord.key : chord.root;
					UInt16 tone = NOTEMOD(guitarPalette->Tone(i, y));

					if (y == 0 && lowFret > 0) {
						my = diay + DIAV;
						CGContextSetRGBFillColor(gc, 1, 0, 0, 1);					// red
						CGContextSetShouldAntialias(gc, true);
					}
					else {
						my = diay + (y - lowFret + 1) * GAPV - 18;

						CGContextSetShouldAntialias(gc, true);

						if (scalePalette->ScaleHasTone(tone))
							CGContextSetRGBFillColor(gc, 0.1, 0.1, 0.85, 1.0);		// Vivid Dark Blue
						else
							CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, 0.70);		// 70% Opaque Black

						CGContextBeginPath(gc);
						CGContextAddArc(gc, mx, bott - (my + 6), kDotRadius, 0, 2 * M_PI, 0);
						CGContextFillPath(gc);

						CGContextSetRGBStrokeColor(gc, 0, 0, 0, 1);
						CGContextBeginPath(gc);
						CGContextAddArc(gc, mx, bott - (my + 6), kDotRadius, 0, 2 * M_PI, 0);
						CGContextClosePath(gc);
						CGContextStrokePath(gc);

						CGContextSetShouldAntialias(gc, true);

						if (tone == chord.root)
							CGContextSetRGBFillColor(gc, 1, 1, 0, 1);				// Yellow
						else
							CGContextSetRGBFillColor(gc, 1, 1, 1, 1);				// White
					}

					UInt16 func = NOTEMOD(tone - root);
					if (!fretpet->chordRootType || (BIT(tone) & fatChord.mask))
						numString = NFPrimary[func];
					else
						numString = NFSecondary[func];
				}

				textSize = numString.GetDimensions(kThemeCurrentPortFont);
				numString.Draw(gc, mx - textSize.h / 2, bott - (my + 10));

			} // if (y < bottom)

		} // string loop

	} // if bracket

	EndContext();
}

