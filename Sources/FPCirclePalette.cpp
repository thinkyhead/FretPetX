/*
 *  FPCirclePalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPUtilities.h"
#include "FPApplication.h"
#include "FPHistory.h"
#include "FPMusicPlayer.h"
#include "FPScalePalette.h"

#include "FPCirclePalette.h"

#include "FPSprite.h"

#include "CGHelper.h"

#include "TCarbonEvent.h"
#include "TError.h"


FPCirclePalette	*circlePalette = NULL;


#define	kBlobSize	22

#define H_SIZE		78

#define C_SIZE		113
#define C_SIZ2		((int)C_SIZE/2)
#define C_LABEL		45
#define CIRCLEX		0
#define CIRCLEY		0


Point blobCoords[] = { {14,167}, {26,192}, {52,198}, {74,181}, {74,154}, {52,137}, {26,143} };

#pragma mark -
//-----------------------------------------------
//
//	FPWedgeSprite
//
class FPWedgeSprite : public FPSprite {
	public:
		FPWedgeSprite(FPContext *c) : FPSprite(c) {
			CFURLRef url = fretpet->GetImageURL(CFSTR("circle/twinklewedge.png"));
			LoadImageGrid(url, 25, 39);
			CFRELEASE(url);

			SetHandle(12.5, 55);
			SetAnimationInterval(1000/15);
			SetAnimationFrameIncrement(1);
			Move(66, 75);
		}

		FPWedgeSprite(const FPWedgeSprite &src) : FPSprite(src) { }

		void Animate() {
			int fi = frameIndex;
			FPSprite::Animate();
			if (fi != 0 && frameIndex == 0)
				MarkForDisposal();
		}
};

#pragma mark -
//-----------------------------------------------
//
// FPCircleControl				CONSTRUCTOR
//
FPCircleControl::FPCircleControl(WindowRef wind) : TCustomControl(wind, 'circ', 1) {
	const EventTypeSpec	moreEvents[] = {
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlHit }
		};

	RegisterForEvents(GetEventTypeCount(moreEvents), moreEvents);
}


//-----------------------------------------------
//
// HitTest
//
ControlPartCode FPCircleControl::HitTest( Point where ) {
	SInt16 segment = CircleSegment(where, C_SIZ2, 10.0, C_SIZ2, OCTAVE);

	if (segment >= 0)
		return kControlIndicatorPart;
	else
		return kControlNoPart;
}


//-----------------------------------------------
//
// Track
//
bool FPCircleControl::Track(MouseTrackingResult eventType, Point where) {
	static SInt16			oldSegment;
	static SInt16			firstTone;
	static bool				bOff;
	static FPHistoryEvent	*event;
	static FPChord			oldChord;
	static bool				transforming;
	static int				dragMode;

	enum {
		kDragToggle,
		kDragHear,
		kDragRotate,
		kDragTransform
	};

	SInt16 segment = CircleSegment(where, C_SIZ2, 16.0, 0.0, OCTAVE);

	if (segment >= 0) {
		UInt16 tone = NOTEMOD(circlePalette->circleTop + KEY_FOR_INDEX(segment));

		switch (eventType) {
			case kMouseTrackingMouseDown: {
				UInt32 mods = GetCurrentEventKeyModifiers();
				if (IS_COMMAND(mods)) {
					dragMode = kDragRotate;
					oldSegment = segment;
				}
				else if (IS_OPTION(mods)) {
					dragMode = kDragHear;
					oldSegment = -1;
				}
				else {
					if (fretpet->IsTransformModeEnabled())
						dragMode = kDragTransform;
					else {
						bOff = globalChord.HasTone(tone);
						dragMode = kDragToggle;
					}

					oldChord = globalChord;
					event = fretpet->StartUndo(UN_EDIT, transforming ? CFSTR("Direct Transform") : CFSTR("Edit Chord"));
					event->SaveCurrentBefore();

					oldSegment = -1;
				}

				firstTone = tone;
			}
			// pass-through

			case kMouseTrackingMouseEntered:
			case kMouseTrackingMouseExited:
			case kMouseTrackingMouseDragged: {
				if (segment != oldSegment) {
					switch(dragMode) {
						case kDragHear:
							if (tone < firstTone) tone += OCTAVE;
							player->PlayNote(kCurrentChannel, tone);
							break;

						case kDragRotate: {
							FPCirclePalette *wind = (FPCirclePalette*)GetTWindow();
							UInt16 diff = INDEX_FOR_KEY(NOTEMOD(oldSegment - segment));
							wind->RotateCircle(diff);
							break;
						}

						case kDragTransform:
							fretpet->DoDirectHarmonize(tone);
							break;

						case kDragToggle:
							if (bOff)
								fretpet->DoSubtractTone(tone);
							else
								fretpet->DoAddTone(tone, firstTone);
							break;
					}

					oldSegment = segment;
				}
				break;
			}

			case kMouseTrackingMouseUp: {
				if ( (dragMode != kDragRotate) && (dragMode != kDragHear) && (oldChord != globalChord) ) {
					event->SaveCurrentAfter();
					event->Commit();
					fretpet->EndTransformOnce();
				}
				break;
			}
		}
	}

	return true;
}


#pragma mark -
//-----------------------------------------------
//
// FPHarmonyControl				CONSTRUCTOR
//
FPHarmonyControl::FPHarmonyControl(WindowRef wind) : TCustomControl(wind, 'circ', 2) {
}


//-----------------------------------------------
//
// Track
//
bool FPHarmonyControl::Track(MouseTrackingResult eventType, Point where) {
	static SInt16 oldFunction;

	SInt16 function = CircleSegment(where, H_SIZE/2, H_SIZE/2-18, 0.0, NUM_STEPS);

	if (function >= 0) {
		switch (eventType) {
			case kMouseTrackingMouseDown: {
				oldFunction = globalChord.RootScaleStep();

//				fprintf(stderr, "oldFunction=%d\n", oldFunction);

				if (function == oldFunction)
					fretpet->DoHearChord( GetCurrentKeyModifiers() );
			}

			case kMouseTrackingMouseEntered:
			case kMouseTrackingMouseExited:
			case kMouseTrackingMouseDragged:
				if (function != oldFunction) {
					fretpet->DoHarmonizeBy(function - oldFunction);
					oldFunction = function;
//					fprintf(stderr, "function=%d\n", function);
				}
				break;
		}
	}

	return true;
}


#pragma mark -
enum {
	kCircleLayerBkgd = 0,
	kCircleLayerVisual,
	kCircleLayerOverlay,
	kCircleLayerCount
};

//-----------------------------------------------
//
//  FPCirclePalette				* CONSTRUCTOR *
//
FPCirclePalette::FPCirclePalette() : FPPaletteGL(CFSTR("Circle"), kCircleLayerCount) {
	circlePalette = this;

	circleTop	= 0;
	spinLock	= false;
	visualPlay	= false;

	InitSprites();

	circleControl	= new FPCircleControl(Window());
	harmonyControl	= new FPHarmonyControl(Window());
	leftControl		= new TSpriteControl(Window(), kFPCommandHarmonizeDown, harmLeftSprite);
	rightControl	= new TSpriteControl(Window(), kFPCommandHarmonizeUp, harmRightSprite);
	upControl		= new TSpriteControl(Window(), kFPCommandTransposeNorth, transUpSprite);
	downControl		= new TSpriteControl(Window(), kFPCommandTransposeSouth, transDownSprite);
	holdControl		= new TSpriteControl(Window(), kFPCommandHold, holdSprite);
	eyeControl		= new TSpriteControl(Window(), kFPCommandCircleEye, eyeSprite);
//	majMinControl	= new TSpriteControl(Window(), kFPCommandRelativeMinor, majorMinorSprite);

	//
	// Register to receive these events in addition
	// to the ones TWindow already handles.
	//
	const EventTypeSpec	circEvents[] = { 
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
	};

	RegisterForEvents( GetEventTypeCount( circEvents ), circEvents );

	Rect bounds = circleControl->Bounds();
	circleRollover = NewTrackingRegion(bounds);
	circleRollover->SetEnabled(true);

//	circleControl->DrawNow();
//	UpdateHarmonyDots();
	ToggleHold();
	ToggleEye();
//	Show();
//	Draw();
}


//-----------------------------------------------
//
//  FPCirclePalette				* DESTRUCTOR *
//
FPCirclePalette::~FPCirclePalette() {
	delete circleRollover;
	delete circleControl;
	delete harmonyControl;
	delete leftControl;
	delete rightControl;
	delete upControl;
	delete downControl;
	delete holdControl;
	delete eyeControl;
}


//-----------------------------------------------
//
//  InitSprites
//
void FPCirclePalette::InitSprites() {
	CFURLRef url = fretpet->GetImageURL(CFSTR("circle/background.png"));
	backSprite = NewSprite(url, kCircleLayerBkgd);
	backSprite->SetHandle(0, 0);
	CFRELEASE(url);

	//
	// Little Eye (visual playback)
	//
	url = fretpet->GetImageURL(CFSTR("circle/littleeye.png"));
	eyeSprite = NewSprite();
	eyeSprite->LoadImageGrid(url, 20, 16);
	AppendSprite(eyeSprite, kCircleLayerOverlay);
	eyeSprite->Move(17, 12);
	CFRELEASE(url);

	//
	// red light (hold)
	//
	url = fretpet->GetImageURL(CFSTR("circle/triangle.png"));
	holdSprite = NewSprite();
	holdSprite->LoadImageGrid(url, 19, 10);
	AppendSprite(holdSprite, kCircleLayerOverlay);
	holdSprite->Move(65, 10);
	CFRELEASE(url);

	//
	// major/minor
	//
/*
	url = fretpet->GetImageURL(CFSTR("circle/majorminor.png"));
	majorMinorSprite = NewSprite();
	majorMinorSprite->LoadImageSquares(url);
	CFRELEASE(url);
	AppendSprite(majorMinorSprite, kCircleLayerOverlay);
	majorMinorSprite->Move(188, 124);
	majorMinorSprite->SetAnimationFrameIndex(0);
*/
	//
	// Harmony dots
	//
	url = fretpet->GetImageURL(CFSTR("circle/blobs.png"));
	masterBlob = NewSprite();
	masterBlob->LoadImageSquares(url);
	CFRELEASE(url);

	verify_action(masterBlob != NULL, throw TError(1000, CFSTR("Can't load harmony dots")));

	for (int i=0; i<=6; i++) {
		FPSpritePtr s = blob[i] = NewSprite(masterBlob);
		s->Move(blobCoords[i].h, blobCoords[i].v);
		AppendSprite(s, kCircleLayerOverlay);
	}

	//
	// Left Harmony Arrow
	//
	url = fretpet->GetImageURL(CFSTR("circle/harmleft.png"));
	harmLeftSprite = NewSprite();
	harmLeftSprite->LoadImageGrid(url, 23, 20);
	AppendSprite(harmLeftSprite, kCircleLayerOverlay);
	harmLeftSprite->Move(143, 113);
	CFRELEASE(url);

	//
	// Right Harmony Arrow
	//
	url = fretpet->GetImageURL(CFSTR("circle/harmright.png"));
	harmRightSprite = NewSprite();
	harmRightSprite->LoadImageGrid(url, 23, 20);
	AppendSprite(harmRightSprite, kCircleLayerOverlay);
	harmRightSprite->Move(191, 113);
	CFRELEASE(url);

	//
	// Transpose Up Arrow
	//
	url = fretpet->GetImageURL(CFSTR("circle/transup.png"));
	transUpSprite = NewSprite();
	transUpSprite->LoadImageGrid(url, 20, 23);
	AppendSprite(transUpSprite, kCircleLayerOverlay);
	transUpSprite->Move(168, 95);
	CFRELEASE(url);

	//
	// Transpose Down Arrow
	//
	url = fretpet->GetImageURL(CFSTR("circle/transdown.png"));
	transDownSprite = NewSprite();
	transDownSprite->LoadImageGrid(url, 20, 23);
	AppendSprite(transDownSprite, kCircleLayerOverlay);
	transDownSprite->Move(168, 130);
	CFRELEASE(url);

	//
	// Circle Overlay
	//
	url = fretpet->GetImageURL(CFSTR("circle/wheel.png"));
	overlaySprite = NewSprite(url, kCircleLayerOverlay);
	overlaySprite->Move(65, 74);
	CFRELEASE(url);

	//
	// Circle Twinkle
	//
	masterWedge = new FPWedgeSprite(context);

	//
	// The Circle Itself
	//
	circleContext = CGCreateContext(113, 113);
	CGContextSetShouldAntialias(circleContext, true);
	circleSprite = NewSprite();
	circleSprite->Move(65, 74);
	AppendSprite(circleSprite, kCircleLayerBkgd);
	UpdateCircle();

	//
	// The Function Name
	//
	functionContext = CGCreateContext(40, 20);
	CGContextSetShouldAntialias(functionContext, true);
	functionSprite = NewSprite();
	functionSprite->Move(167, 46);
	AppendSprite(functionSprite, kCircleLayerBkgd);
	UpdateHarmonyDots();

	StartSpriteTimer();
}


//-----------------------------------------------
//
//  UpdateCircle
//
#define LABELX(n)			( C_SIZ2 + C_LABEL * sin((18-n) * M_PI / 6) )
#define LABELY(n)			( C_SIZ2 + C_LABEL * -cos((18-n) * M_PI / 6) + 3 )
#define NOTE_FOR_POS(pos)	NOTEMOD(circleTop + KEY_FOR_INDEX(pos))
#define POS_FOR_NOTE(note)	NOTEMOD(INDEX_FOR_KEY(NOTEMOD(note)) - INDEX_FOR_KEY(circleTop))

void FPCirclePalette::UpdateCircle() {
	RGBColor	*tcolor;
	UInt16		tones = globalChord.tones;
	UInt16		root = globalChord.Root();
	UInt16		chordKey = globalChord.Key();

	//
	// Set the 12-o-clock note to the key center
	//	unless it's been locked.
	//
	if (!spinLock)
		circleTop = chordKey;

	UInt16	mask = scalePalette->MaskForKey(chordKey);

	CGContextRef gc = circleContext;
	CGContextSaveGState(gc);
	CGContextTranslateCTM(gc, 113.0/2.0, 113.0/2.0);
	CGContextScaleCTM(gc, 113, 113);
	CGContextClearRect(gc, CGRectMake(-0.5, -0.5, 1, 1));

	for (int i=0; i<OCTAVE; i++) {	// 0..11
		UInt16	tone = NOTE_FOR_POS(i);
		UInt16	bit = BIT(tone);
		bool	inKey = (bit & mask) != 0;

		float color[4] = { 0.0, 0.0, 0.0, 1.0 };
		if (bit & tones) {
			if (tone == root) {
				color[0] = color[1] = 1.0;
				tcolor = inKey ? &rgbBlack : &rgbYellow;
			}
			else {
				color[0] = color[1] = color[2] = 1.0;
				tcolor = inKey ? &rgbBlack : &rgbWhite;
			}

			if (!inKey)
				color[3] /= 2;
		}
		else if (inKey) {
			color[0] = color[1] = 0.0;
			color[2] = 0.8;
			tcolor = &rgbWhite;
		}
		else
			tcolor = &rgbRed;

		CGContextBeginPath(gc);
		CGContextMoveToPoint(gc, 0, 0);
		CGContextAddArc(gc, 0, 0, 0.5, RAD(90-15), RAD(90+15), false);
		CGContextClosePath(gc);
		CGContextSetRGBFillColor(gc, color[0], color[1], color[2], color[3]);
		CGContextFillPath(gc);

		CGContextRotateCTM(gc, -RAD(30));
	}

	CGContextRestoreGState(gc);

	FocusContext(); // for the font dimensions of the current window

	SetFont(NULL, (StringPtr)"\pLucida Grande", 10, normal, gc);

	TString itemString;
	for (int i=0; i<OCTAVE; i++) {	// 0..11
		UInt16	tone = NOTE_FOR_POS(i);
		UInt16	bit = BIT(tone);
		bool	inKey = (bit & mask) != 0;

		if (bit & tones) {
			if (tone == root)
				tcolor = inKey ? &rgbBlack : &rgbYellow;
			else
				tcolor = inKey ? &rgbBlack : &rgbWhite;
		}
		else if (inKey)
			tcolor = &rgbWhite;
		else
			tcolor = &rgbRed;

		if (fretpet->romanMode)
			itemString = NFPrimary[NOTEMOD(tone - globalChord.Root())];
		else {
			itemString = scalePalette->NameOfNote(chordKey, tone);
			itemString.Trim();
		}

		Point textSize = itemString.GetDimensions(kThemeCurrentPortFont);
		CGContextSetRGBFillColor(gc, CG_RGBCOLOR(tcolor), 1.0);
		itemString.Draw( gc, LABELX(i)-textSize.h/2, LABELY(i)-textSize.v/2 );
	}

	EndContext();

	circleSprite->LoadCGContext(gc);
}


//
// UpdateHarmonyDots
//
void FPCirclePalette::UpdateHarmonyDots() {

	SInt16	step = globalChord.RootScaleStep();

	if (step >= 0) {
		RGBColor *rgb;
		switch(scalePalette->GetTriadType(step)) {
			case kTriadMajor:
				rgb = &rgbMaj;
				break;

			case kTriadMinor:
				rgb = &rgbMin;
				break;

			case kTriadDiminished:
				rgb = &rgbDim;
				break;

			default:
				rgb = &rgbOth;
		}

		TString funcName(scalePalette->RomanFunctionName(step));

		CGContextRef gc = functionContext;
        FontInfo font;
		SetFont(&font, (StringPtr)"\pHoefler Text", 13, normal, gc);

		Point textSize = funcName.GetDimensions(kThemeCurrentPortFont);

		CGContextSetRGBFillColor(gc, (float)0xdd/0xff, (float)0xdd/0xff, (float)0xdd/0xff, 1);
		CGContextFillRect(gc, CGRectMake(0, 0, 40, 20));

		CGContextSetRGBFillColor(gc, CG_RGBCOLOR(rgb), 1);
		funcName.Draw(gc, 20-textSize.h/2, 12-textSize.v/2);

		functionSprite->LoadCGContext(gc);
	}
	else
		fprintf(stderr, "Chord root function not yet established!\n");

	int frame;
	for (int i=0; i<NUM_STEPS; i++) {
		switch(scalePalette->GetTriadType(i)) {
			case kTriadMajor:
				frame = 2;
				break;

			case kTriadMinor:
				frame = 4;
				break;

			case kTriadDiminished:
				frame = 6;
				break;

			case kTriadOther:
			default:
				frame = 0;
				break;

		}

		if (i == step)
			frame++;

		blob[i]->SetAnimationFrameIndex(frame);
	}
}


//-----------------------------------------------
//
// HandleTrackingRegion
//
void FPCirclePalette::HandleTrackingRegion(TTrackingRegion *region, TCarbonEvent &event) {
	int curs = 0;

	UInt32 mods;
	(void) event.GetParameter(kEventParamKeyModifiers, &mods);

	switch ( event.GetClass() ) {
		case kEventClassMouse:

			switch ( event.GetKind() ) {
				case kEventMouseEntered: {
					if (region == circleRollover)
						curs = IS_COMMAND(mods) ? (spinLock ? kCursRotate : 0) : IS_OPTION(mods) ? kCursHear : 0;

					fretpet->SetMouseCursor(curs);
					break;
				}

				case kEventMouseExited:
					fretpet->ResetMouseCursor();
					break;
			}
			break;

		case kEventClassKeyboard:
			switch ( event.GetKind() ) {
				case kEventRawKeyModifiersChanged:

					if (region == circleRollover)
						fretpet->SetMouseCursor(IS_COMMAND(mods) ? (spinLock ? kCursRotate : 0) : IS_OPTION(mods) ? kCursHear : 0);

					break;
			}
			break;
	}
}

//-----------------------------------------------
//
//	TwinkleTone
//
void FPCirclePalette::TwinkleTone(UInt16 tone) {
	if (visualPlay && IsVisible()) {
		FPWedgeSprite *pSprite = new FPWedgeSprite(*(FPWedgeSprite*)masterWedge);
		if (pSprite != NULL) {
			int i = POS_FOR_NOTE(tone);
			pSprite->SetRotation(i * 30.0);
			AppendSprite(pSprite, kCircleLayerVisual);
		}
	}
}

void FPCirclePalette::ProcessAndAnimateSprites() {

	FPPaletteGL::ProcessAndAnimateSprites();

//	if (!visualPlay) {
//		SetNextFireTime(1.0 / 5.0);		
//	}
//	else if (!player->IsPlaying()) {
//		SetNextFireTime(1.0 / 10.0);
//	}
}
