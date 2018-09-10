/*
 *  FPGuitarPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#define kUseAlphaChannels	true	// 1 bit (false) or 8 bit mask (true)

#include "FPGuitarPalette.h"
#include "FPGuitar.h"
#include "FPSpriteLayer.h"

#include "FPApplication.h"
#include "FPCustomTuning.h"
#include "FPDiagramPalette.h"
#include "FPHistory.h"
#include "FPPreferences.h"
#include "FPScalePalette.h"
#include "CGHelper.h"
#include "TError.h"
#include "TCarbonEvent.h"


FPGuitarPalette	*guitarPalette = NULL;


int init_stringx[] = { 30, 41, 52, 63, 74, 85 };

int init_stringy[] = {	 34,  60,  92, 123, 153,
						181, 207, 233, 255, 277,
						298, 318, 337, 355, 371,
						387, 402, 415, 429, 443 }; // last number for measurement

int init_frettop[] = {	24,  43,  76, 106, 137,
						166, 193, 219, 243, 266,
						287, 308, 327, 345, 362,
						378, 394, 408, 422, 436 };

#pragma mark -
class FPDotSprite : public FPSprite {
	public:
		FPDotSprite(FPContext *c, CFURLRef url) : FPSprite(c) {
			LoadImageMatrix(url, 12);
			Hide();
		}

		FPDotSprite(const FPDotSprite &src) : FPSprite(src) { }

		void Animate() {
			UInt32	time = (TickCount() % 60) / 30;
			if (frameIndex < kDotIndexX)
				frameIndex = (frameIndex % kDotColorCount) + (time ? kDotColorCount : 0);
			else {
				long group = (frameIndex - kDotIndexNumWhite) / kDotNumCount;
				frameIndex = (group * kDotNumCount + kDotIndexNumWhite) + (frameIndex % kDotColorCount) + (time ? kDotColorCount : 0);
			}
		}
};



#pragma mark -
class FPEffectSprite : public FPSprite {
	public:
		FPEffectSprite(FPContext *c, CFURLRef url) : FPSprite(c) {
			LoadImageSquares(url);
		}

		FPEffectSprite(const FPEffectSprite &src) : FPSprite(src) { }

		void Animate() {
			int fi = frameIndex;
			FPSprite::Animate();
			if (fi != 0 && frameIndex == 0)
				MarkForDisposal();
		}
};



#pragma mark -

enum {
	kBracketNoPart = 0,
	kBracketPartTop,
	kBracketPartMiddle,
	kBracketPartBottom
};



FPBracketControl::FPBracketControl(WindowRef wind, int controlID, const GuitarMetrics &inMetrics) : TCustomControl(wind, 'guit', controlID), metrics(inMetrics) {
	bracketEvent = NULL;
	dragPart = kBracketNoPart;
	origLow = globalChord.brakLow;
	origHigh = globalChord.brakHi;
}


bool FPBracketControl::Draw(const Rect &bounds) {
	CGContextRef gc = FocusContext();

//		UInt16	bott = GetTWindow()->Height();

	CGContextBeginPath(gc);
	CGContextAddRect(gc, CGBounds());
	CGContextClosePath(gc);

	CGContextSetRGBFillColor(gc, 1.0, 1.0, 1.0, 1.0);
	CGContextFillPath(gc);

	EndContext();

	return true;
}


//-----------------------------------------------
//
//	Track
//
//	Theoretically, the bracket should simply report
//	tracking events to the palette, and the palette
//	class should act as the controller.
//
bool FPBracketControl::Track(MouseTrackingResult eventType, Point where) {
	FPGuitarPalette			*pal = guitarPalette;

	static int				lastLow, lastHigh;

	SInt16					y;
	SInt16					fret;

	// Get metrics for the bracket
	int	capHeight1, capHeight2, capOffset1, capOffset2;
	pal->GetBracketCapInfo(capHeight1, capHeight2, capOffset1, capOffset2);

	// Get the current bracket position from the global chord
	SInt16	low = globalChord.brakLow,
			high = globalChord.brakHi;

	y = OrientedPosition(where);

	switch (eventType) {
		case kMouseTrackingMouseDown: {
			bracketEvent = fretpet->StartUndo(UN_EDIT, CFSTR("Bracket Change"));
			bracketEvent->SaveCurrentBefore();

			int	cap_y_top = (int) (metrics.frettop[low] + capOffset1),
				cap_y_bot = (int) (metrics.fretbottom[high] + capOffset2);

			// Test for click in top, bottom, or middle
			if (y >= cap_y_top && y <= cap_y_top + capHeight1) {
				dragPart = kBracketPartTop;
			}
			else if (y >= cap_y_bot && y <= cap_y_bot + capHeight2) {
				dragPart = (high == metrics.numberOfFrets && low == metrics.numberOfFrets)
							? kBracketPartMiddle : kBracketPartBottom;
			}
			else {
				fret = GetFretFromPosition(y);
				CONSTRAIN(fret, 0, metrics.numberOfFrets);

				if (fret >= low && fret <= high) {
					dragPart = kBracketPartMiddle;
				}
				else if (fret < low - 1 || fret > high + 1) {
					dragPart = kBracketPartMiddle;
					SInt16 size = high - low;
					low = fret - (size / 2);
					CONSTRAIN(low, 0, metrics.numberOfFrets);
					high = low + size;
					CONSTRAIN(high, 0, metrics.numberOfFrets);
					low = high - size;
					fretpet->DoMoveFretBracket(low, high);
				}
				else
					dragPart = kBracketNoPart;
			}

			if (dragPart != kBracketNoPart) {
				pal->BracketDragStart(dragPart, y);
				origLow = lastLow = low;
				origHigh = lastHigh = high;
			}
		}

		case kMouseTrackingMouseDragged:

			if (dragPart != kBracketNoPart) {
				pal->DragBracket(y);

				switch(dragPart) {
					case kBracketPartTop:
						low = GetFretFromPosition(pal->DragTop(), 1);
						CONSTRAIN(low, 0, high);
						break;

					case kBracketPartBottom:
						high = GetFretFromPosition(pal->DragBottom(), -1);
						CONSTRAIN(high, low, metrics.numberOfFrets);
						break;

					case kBracketPartMiddle: {
						low = GetFretFromPosition(pal->DragTop(), 1);
						high = GetFretFromPosition(pal->DragBottom(), -1);
						CONSTRAIN(low, 0, high);								// better safe than sorry here
						CONSTRAIN(high, low, metrics.numberOfFrets);
						break;
					}
				}
			}
			break;

		case kMouseTrackingMouseUp:
			FinishDrag();
			break;
	}

	if (low != lastLow || high != lastHigh) {
		fretpet->DoMoveFretBracket(low, high);
		lastLow = low;
		lastHigh = high;
	}

	return true;
}


void FPBracketControl::FinishDrag() {
	if (dragPart != kBracketNoPart) {
		dragPart = kBracketNoPart;
		guitarPalette->BracketDragStop();

		if (globalChord.brakLow != origLow || globalChord.brakHi != origHigh) {
			bracketEvent->SaveCurrentAfter();
			bracketEvent->Commit();
		}
	}
}


ControlPartCode FPBracketControl::HitTest( Point where ) {
	return globalChord.bracketFlag ? kControlIndicatorPart : kControlNoPart;
}


int FPBracketControl::GetFretFromPosition(int y, int middleAdjustment) {
	int fret = 0;

	if (y >= metrics.frettop[0]) {
		if (y > metrics.fretbottom[metrics.numberOfFrets]) {
			fret = metrics.numberOfFrets;
		}
		else {
			for (int f=metrics.numberOfFrets+1; f--;) {
				if ( y >= metrics.frettop[f] && y <= metrics.fretbottom[f] ) {
					fret = f;
					if (middleAdjustment != 0) {
						int thresh = (metrics.frettop[f] + metrics.fretbottom[f]) / 2;
						if (middleAdjustment > 0) {
							if (y >= thresh /* && fret < metrics.numberOfFrets - 1 */ )
								fret++;
						}
						else {
							if (y <= thresh /* && fret > 0 */ )
								fret--;
						}
					}
					break;
				}
			}
		}
	}

	return fret;
}


int FPBracketControl::OrientedPosition(Point where) {
	return	guitarPalette->IsHorizontal()
			?	(
				guitarPalette->IsLeftHanded()
				?	Width() - where.h
				:	where.h
				)
			: where.v;
}


#pragma mark -
//-----------------------------------------------
//
//	FPTuningControl
//
//	By registering for hit test by default it
//	will send a command id when clicked.
//
FPTuningControl::FPTuningControl(WindowRef wind) : TCustomControl(wind, 'guit', 3) {
	tuningMenu = GetMiniMenu(CFSTR("TuningPop"));
	customTuningList.AttachMenu(tuningMenu);

	const EventTypeSpec	moreEvents[] = {
		{ kEventClassControl, kEventControlHitTest },
		{ kEventClassControl, kEventControlContextualMenuClick }
		};

	RegisterForEvents(GetEventTypeCount(moreEvents), moreEvents);
}


void FPTuningControl::ContextClick() {
	Rect			bounds = Bounds();
	Rect			winpos = TWindow::GetTWindow(Window())->Bounds();
	MenuItemIndex	firstIndex = customTuningList.FindMatchingMenuItem(tuningMenu, guitarPalette->CurrentTuning());

	if (firstIndex == kInvalidItemIndex)
		(void)GetIndMenuItemWithCommandID(tuningMenu, kFPCommandGuitarTuning, 1, NULL, &firstIndex);

	(void)PopUpMenuSelect(tuningMenu, bounds.top + winpos.top, bounds.left + winpos.left, firstIndex);
}


#pragma mark -
FPGuitarControl::FPGuitarControl(WindowRef wind, const GuitarMetrics &inMetrics) : TCustomControl(wind, 'guit', 1), metrics(inMetrics) {
	overrideUPP = NewEventLoopTimerUPP(FPGuitarControl::OverrideTimerProc);

	(void)InstallEventLoopTimer(
		GetMainEventLoop(),
		kEventDurationForever,
		kEventDurationForever,
		overrideUPP,
		this,
		&overrideLoop
		);
}


FPGuitarControl::~FPGuitarControl() {
	if (overrideLoop != NULL) {
		(void)RemoveEventLoopTimer(overrideLoop);
		DisposeEventLoopTimerUPP(overrideUPP);
	}
}


bool FPGuitarControl::Track(MouseTrackingResult eventType, Point where) {
	SInt16					fret, string;
	int						newBox, clicks;
	bool					justClicked = false;
	static int				lastBox = 0;
	static bool				override = false;
	static FPHistoryEvent	*event;
	static bool				transforming;
	static FPChord			oldChord;

	GetPositionFromPoint(where, &string, &fret);

	newBox = string * metrics.numberOfFrets + fret;

	switch (eventType) {
		case kMouseTrackingMouseDown: {
			transforming = fretpet->IsTransformModeEnabled();
			clicks = CountClicks(newBox==lastBox, (override || transforming) ? 1 : 3);
			switch(clicks) {
				case 1:
					event = NULL;
					oldChord = globalChord;
					override = !transforming && IS_OPTION(GetCurrentEventKeyModifiers());

					if (override || transforming) {
						event = fretpet->StartUndo(UN_EDIT, transforming ? CFSTR("Direct Transform") : CFSTR("Change Fingering"));
						event->SaveCurrentBefore();
					}

					justClicked = true;
					break;

				case 2:
					fretpet->DoToggleGuitarTone();	// Always Creates an Undo Event
					break;

				case 3:
					event = fretpet->StartUndo(UN_EDIT, CFSTR("Add Guitar Triad"));
					event->SetMerge();
					fretpet->DoAddGuitarTriad();
					break;
			}

//			lastBox = -1;
			break;
		}

		case kMouseTrackingMouseUp:

			SetEventLoopTimerNextFireTime(overrideLoop, kEventDurationForever);

			if ( (event != NULL) && (oldChord != globalChord) ) {
				event->SaveCurrentAfter();
				event->Commit();
			}

			fretpet->EndTransformOnce();
			break;
	}

	if (!override && !transforming && (lastBox != newBox || justClicked))
		SetEventLoopTimerNextFireTime(overrideLoop, 1.0 / 2.0);

	if (lastBox != newBox)
		fretpet->DoMoveFretCursor(string, fret);

	if (override && (lastBox != newBox || justClicked))
		fretpet->DoOverrideFingering();

	lastBox = newBox;
	return true;
}


void FPGuitarControl::GetPositionFromPoint(Point where, SInt16 *string, SInt16 *fret) {
	SInt16	newFret = 0;
	Rect	bounds = Bounds();
	UInt16	hh, yy, w = bounds.right - bounds.left, h = bounds.bottom - bounds.top;
	SInt16	x, y;

	if (guitarPalette->IsHorizontal()) {
		hh = w;
		x = guitarPalette->IsReverseStrung() ? where.v : h - where.v;

		if (guitarPalette->IsLeftHanded()) {
//			x = h - x;
			y = w - where.h;
			Rect r = guitarPalette->GetContentSize();
			yy = y + (r.right - bounds.right);
		}
		else {
			y = where.h;
			yy = y + bounds.left;
		}
	}
	else {
		hh = h;
		x = guitarPalette->IsReverseStrung() ? w - where.h : where.h;
		y = where.v;
		yy = y + bounds.top;
	}

	// The control is sized just right so the math is easy
	*string = (x / metrics.stringSpacing);
	CONSTRAIN(*string, 0, metrics.numberOfStrings - 1);

	// Fret needs to be looked up
	if (y < 0) {
		newFret = 0;
	}
	else if (y > hh) {
		newFret = metrics.numberOfFrets;
	}
	else {
		for (int f=metrics.numberOfFrets+1; f--;) {
			if ( yy >= metrics.frettop[f] && yy <= metrics.fretbottom[f] ) {
				newFret = f;
				break;
			}
		}

		CONSTRAIN(newFret, 0, metrics.numberOfFrets);
	}

	*fret = newFret;
}


void FPGuitarControl::OverrideTimerProc( EventLoopTimerRef timer, void *ignoreme ) {
	fretpet->DoOverrideFingering();
}


#pragma mark -

enum {
	kBackgroundLayer = 0,
	kTuningLayer,
	kDotsLayer,
	kCursorLayer,
	kBracketLayer,
	kDebugLayer,
	kGuitarLayerCount
};



FPGuitarPalette::FPGuitarPalette() : FPPaletteGL(CFSTR("Guitar"), kGuitarLayerCount) {
	Init();

	// An array of all installed guitar bundles
	guitarArray = new FPGuitarArray();

	// The guitar assets to use when a bundles lacks components
	currentGuitar = defaultGuitar = guitarArray->GetDefaultGuitar();

	// Use the array to populate the Guitar Image menu
	UpdateGuitarImageMenu();

	// Get the initial size of the guitar palette
	Rect currSize = GetContentSize();
	initWidth = currSize.right;
	initHeight = currSize.bottom;
	initRight = initWidth - 1;
	initBottom = initHeight - 1;

	if (isHorizontal)
		SetContentSize(initHeight, initWidth);

	InitSprites();

	// Create the controls with default metrics
	guitarControl = new FPGuitarControl(Window(), metrics);
	bracketControl = new FPBracketControl(Window(), 2, metrics);
	tuneNameControl = new FPTuningControl(Window());

	// Load a guitar and all its assets
	// This may completely alter the window metrics!
	LoadGuitar(guitarArray->SelectedGuitarIndex());

	//
	// Register to receive these events in addition
	// to the ones TWindow already handles.
	//
	const EventTypeSpec	guitEvents[] = {
		{ kEventClassWindow, kEventWindowBoundsChanging },
		{ kEventClassWindow, kEventWindowZoom },
		{ kEventClassMouse, kEventMouseWheelMoved }
	};

	RegisterForEvents( GetEventTypeCount( guitEvents ), guitEvents );

	// Resize the bracket sprites to match the new metrics
	UpdateBracket();
	MoveFretCursor(0, 0);

	if (isHorizontal)
		PositionControls();
}


void FPGuitarPalette::Init() {
	isHorizontal	= preferences.GetBoolean(kPrefHorizontal, FALSE);
	isLefty			= preferences.GetBoolean(kPrefLeftHanded, FALSE);
	reverseStrung	= preferences.GetBoolean(kPrefReverseStrung, FALSE);
	showRedDots		= preferences.GetBoolean(kPrefUnusedTones, FALSE);
	dotStyle		= preferences.GetInteger(kPrefDotStyle, 0);

	metrics.stringx = new float[NUM_STRINGS];
	metrics.fretbottom = &metrics.frettop[1];

	// these are initialized later from the loaded guitar
	// meanwhile this sets some sane defaults
	int i;
	for (i = 0; i < NUM_STRINGS; i++)
		metrics.stringx[i] = init_stringx[i];

	int initFretCount = COUNT(init_frettop) - 2;
	for (i = 0; i < initFretCount + 2; i++) {
		metrics.stringy[i] = init_stringy[i];
		metrics.frettop[i] = init_frettop[i];
	}

	if (initFretCount < MAX_FRETS) {
		for (i=0; i < MAX_FRETS - initFretCount; i++) {
			metrics.stringy[i] = init_stringy[initFretCount + 1] + i * 2;
			metrics.frettop[i] = init_frettop[initFretCount + 1] + i * 2;
		}
	}

	redoDots				= false;

	fingeredChord			= 0;
	currentString			= 0;
	currentFret				= 0;

	masterTwinkle			= NULL;
	backgroundSprite		= NULL;
	cursorSprite			= NULL;

	bracketTopSprite		= NULL;
	bracketMiddleSprite		= NULL;
	bracketStretchSprite1	= NULL;
	bracketStretchSprite2	= NULL;
	bracketBottomSprite		= NULL;
	bracketDragType			= kBracketNoPart;

	tuningInfoContext		= NULL;
	tuningInfoSprite		= NULL;

	bzero(dotSprite, sizeof(dotSprite));

	tuningViewName			= false;
}


void FPGuitarPalette::SavePreferences() {
	preferences.SetBoolean(kPrefHorizontal, isHorizontal);
	preferences.SetBoolean(kPrefLeftHanded, isLefty);
	preferences.SetBoolean(kPrefReverseStrung, reverseStrung);
	preferences.SetBoolean(kPrefUnusedTones, showRedDots);
	preferences.SetInteger(kPrefDotStyle, dotStyle);

	guitarArray->SavePreferences();

	FPPalette::SavePreferences();
}


void FPGuitarPalette::Reset() {
	if (isHorizontal)
		ToggleHorizontal();

	FPPalette::Reset();
}


void FPGuitarPalette::InitSprites() {
	// Create a context for the tuning info
	// that's large enough for any instrument
	tuningInfoContext = CGCreateContext(250, 100);

	// Create a sprite that will use the context
	tuningInfoSprite = NewSprite();
	tuningInfoSprite->SetHandle(0, 0);
	AppendSprite(tuningInfoSprite, kTuningLayer);
	tuningInfoSprite->LoadCGContext(tuningInfoContext);

	StartSpriteTimer();
}


void FPGuitarPalette::LoadCursorSprite() {
	if (cursorSprite)
		cursorSprite->MarkForDisposal();

	FPGuitar *cursorSource = currentGuitar->CursorFile() ? currentGuitar : defaultGuitar;

	cursorSprite = NewSprite();
	cursorSprite->LoadImageSquares(cursorSource->CursorFile());
	cursorSprite->SetAnimationInterval(1000 / cursorSource->CursorSpeed());
	AppendSprite(cursorSprite, kCursorLayer);

	MoveFretCursor(currentString, currentFret);
}


void FPGuitarPalette::LoadDotSprites() {
	static CFURLRef lastURL = NULL;

	FPGuitar *guit = currentGuitar->DotsFile() ? currentGuitar : defaultGuitar;

	CFURLRef url = guit->DotsFile();

	if (lastURL != url) {
		lastURL = url;
		FPDotSprite *pSprite1 = new FPDotSprite(context, url);

		for (int string=0; string<NUM_STRINGS; string++) {
			for (int fret=0; fret<MAX_FRETS; fret++) {
				if (dotSprite[string][fret] != NULL)
					dotSprite[string][fret]->MarkForDisposal();

				if (fret <= metrics.numberOfFrets && string < metrics.numberOfStrings) {
					if (fret || string)
						pSprite1 = new FPDotSprite(*pSprite1);

					dotSprite[string][fret] = pSprite1;
					AppendSprite(pSprite1, kDotsLayer);
				}
				else
					dotSprite[string][fret] = NULL;
			}
		}
	}

	PositionDotSprites();
	ArrangeDots();
}


void FPGuitarPalette::LoadEffectSprites() {
	FPSpritePtr oldMaster = masterTwinkle;
	masterTwinkle = NULL;
	if (oldMaster) delete oldMaster;

	// Master Effect Sprite
	FPGuitar *guit = currentGuitar;
	CFURLRef url = guit->EffectFile();
	if ( url == NULL ) {
		guit = defaultGuitar;
		url = guit->EffectFile();
	}

	masterTwinkle = new FPEffectSprite(context, url);
	masterTwinkle->SetAnimationInterval(1000.0 / guit->EffectSpeed());
}


void FPGuitarPalette::LoadBracketSprites() {
	layerArray[kBracketLayer]->clear();

	CFURLRef top, stretch, middle, bottom;
	FPGuitar *guit = currentGuitar;

	top = guit->BracketTopFile(isHorizontal);
	if (!top) {
		guit = defaultGuitar;
		top = guit->BracketTopFile(isHorizontal);
	}
	stretch = guit->BracketStretchFile(isHorizontal);
	middle = guit->BracketMiddleFile(isHorizontal);
	bottom = guit->BracketBottomFile(isHorizontal);

	// Top
	bracketTopSprite = NewSprite(top, kBracketLayer);
	bracketTopSprite->SetHandle(0, 0);

	// Stretch 1
	bracketStretchSprite1 = NewSprite(stretch, kBracketLayer);
	bracketStretchSprite1->SetHandle(0, 0);

	// Middle
	bracketMiddleSprite = NewSprite(middle, kBracketLayer);
	bracketMiddleSprite->SetHandle(0, 0);

	// Stretch 2
	bracketStretchSprite2 = NewSprite(bracketStretchSprite1, kBracketLayer);
	bracketStretchSprite2->SetHandle(0, 0);

	// Bottom
	bracketBottomSprite = NewSprite(bottom, kBracketLayer);
	bracketBottomSprite->SetHandle(0, 0);

	UpdateBracket();
}


void FPGuitarPalette::PositionDotSprites() {
	for (int string=0; string<metrics.numberOfStrings; string++) {
		for (int fret=0; fret<=metrics.numberOfFrets; fret++) {
			int x, y;
			DotPosition(string, fret, &x, &y);
			dotSprite[string][fret]->Move(x, y);
		}
	}
}


void FPGuitarPalette::DotPosition(UInt16 string, UInt16 fret, int *x, int *y) {
	int	nx, ny;

	nx = metrics.stringx[string];
	ny = metrics.stringy[fret];

	if (isHorizontal) {
		if (!reverseStrung)
			nx = initRight - nx;

		if (isLefty)
			ny = initBottom - ny;

		int nt = nx; nx = ny; ny = nt;

		--ny;
	}
	else if (reverseStrung)
		nx = initRight - nx;

	*x = nx;
	*y = ny;
}


void FPGuitarPalette::PositionControls() {
	if (isHorizontal) {
		UInt16	gt, gl, gb, gr, bt, bl, bb, br, tt, tl, tb, tr;

		if (isLefty) {
			gt = guitarBounds.left;
			gl = initBottom - guitarBounds.bottom;
			gb = guitarBounds.right;
			gr = initBottom - guitarBounds.top;

			bl = initBottom - bracketBounds.bottom;
			br = initBottom - bracketBounds.top;

			tt = tuneNameBounds.left;
			tl = initBottom - tuneNameBounds.bottom;
			tb = tuneNameBounds.right;
			tr = initBottom - tuneNameBounds.top;
		}
		else {
			gt = initRight - guitarBounds.right;
			gl = guitarBounds.top;
			gb = initRight - guitarBounds.left;
			gr = guitarBounds.bottom;

			bl = bracketBounds.top;
			br = bracketBounds.bottom;

			tt = initRight - tuneNameBounds.right;
			tl = tuneNameBounds.top;
			tb = initRight - tuneNameBounds.left;
			tr = tuneNameBounds.bottom;

		}

		bt = initRight - bracketBounds.right;
		bb = initRight - bracketBounds.left;

		Rect gBounds = { gt, gl, gb, gr };
		Rect bBounds = { bt, bl, bb, br };
		Rect tBounds = { tt, tl, tb, tr };

		guitarControl->SetBounds(&gBounds);
		bracketControl->SetBounds(&bBounds);
		tuneNameControl->SetBounds(&tBounds);
	}
	else {
		guitarControl->SetBounds(&guitarBounds);
		bracketControl->SetBounds(&bracketBounds);
		tuneNameControl->SetBounds(&tuneNameBounds);
	}
}


void FPGuitarPalette::DisposeSpriteAssets() {
	FPPaletteGL::DisposeSpriteAssets();

	backgroundSprite		= NULL;
	cursorSprite			= NULL;
	bracketTopSprite		= NULL;
	bracketMiddleSprite		= NULL;
	bracketStretchSprite1	= NULL;
	bracketStretchSprite2	= NULL;
	bracketBottomSprite		= NULL;
	bracketBottomSprite		= NULL;
	tuningInfoSprite		= NULL;

	for (int string=metrics.numberOfStrings; string--;)
		for (int fret=metrics.numberOfFrets; fret--;)
			dotSprite[string][fret] = NULL;
}


void FPGuitarPalette::MoveFretCursor(UInt16 string, UInt16 fret) {
	int x, y;
	DotPosition(string, fret, &x, &y);
	cursorSprite->Move(x, y);
	currentString = string;
	currentFret = fret;
}


OSStatus FPGuitarPalette::HandleEvent(EventHandlerCallRef inRef, TCarbonEvent &event) {
	OSStatus    result = eventNotHandledErr;

	switch ( event.GetClass() ) {
		case kEventClassWindow:
			switch ( event.GetKind() ) {
				case kEventWindowBoundsChanging:
					Magnetize(event);
					result = noErr;
					break;

				case kEventWindowDrawContent:
					// Not needed for Sprite windows
					break;

				case kEventWindowUpdate:
					// Not needed for Sprite windows
					break;
			}
			break;
	}

	if (result == eventNotHandledErr)
		result = FPPaletteGL::HandleEvent(inRef, event);

	return result;
}


bool FPGuitarPalette::Zoom() {
	ToggleHorizontal();
	return true;
}


bool FPGuitarPalette::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {

	if (fretpet->swipeIsReversed) delta *= -1, deltaX *= -1;

	SInt16	low = globalChord.brakLow,
			high = globalChord.brakHi,
			oldLow = low,
			size = high - low;

	// do something with the delta
	low += IsHorizontal() ? (IsLeftHanded() ? deltaX-delta : -deltaX-delta) : -delta;

	if (low < 0)
		low = 0;
	else if (low + size > metrics.numberOfFrets)
		low = metrics.numberOfFrets - size;

	high = low + size;

	if (low != oldLow)
		fretpet->DoMoveFretBracket(low, high);

	return true;
}


#pragma mark -
void FPGuitarPalette::UpdateGuitarImageMenu() {
	OSStatus		err;
	MenuRef			theMenu;
	MenuItemIndex	theIndex;

	// Get the guitar image submenu
	err = GetIndMenuItemWithCommandID(NULL, kFPCommandImageSubmenu, 1, &theMenu, &theIndex);
	err = GetMenuItemHierarchicalMenu(theMenu, theIndex, &theMenu);

	guitarArray->PopulateMenu(theMenu, kFPCommandGuitarImage);
}


bool FPGuitarPalette::LoadGuitar(CFStringRef shortName) {
	return LoadGuitar( guitarArray->IndexOfGuitar(shortName) );
}


bool FPGuitarPalette::LoadGuitar(SInt16 index) {
	static bool firstLoad = true;

	if (index < 0)
		return false;

	if (firstLoad || guitarArray->SelectedGuitarIndex() != index) {
		firstLoad = false;

		layerArray[kBackgroundLayer]->clear();
		backgroundSprite = NULL;

		guitarArray->SetSelectedGuitar(index);
		currentGuitar = guitarArray->SelectedGuitar();

		CFURLRef theBackdrop = currentGuitar->BackgroundFile();

		if (theBackdrop == NULL)
			theBackdrop = defaultGuitar->BackgroundFile();

		verify_action(theBackdrop != NULL, throw TError(1000, CFSTR("Couldn't load guitar image.")));

		backgroundSprite = NewSprite(theBackdrop);
		backgroundSprite->SetHandle(0, 0);
		backgroundSprite->MakeFlippable();
		AppendSprite(backgroundSprite, kBackgroundLayer);

		if (initWidth != backgroundSprite->Width() || initHeight != backgroundSprite->Height()) {
			initWidth = backgroundSprite->Width();
			initHeight = backgroundSprite->Height();
			initRight = initWidth - 1;
			initBottom = initHeight - 1;

			if (isHorizontal)
				SetContentSize(initHeight, initWidth);
			else
				SetContentSize(initWidth, initHeight);

			// This is needed so window.Reset() works properly
			SetDefaultSize(initWidth, initHeight);
		}

		UpdateMetrics();

		SetRect(&guitarBounds,
			currentGuitar->GetStringLeft(0),
			currentGuitar->GetFretTop(0) + metrics.bracketTopOffset,
			currentGuitar->GetStringRight(currentGuitar->NumberOfStrings() - 1),
			currentGuitar->GetFretBottom(currentGuitar->NumberOfFrets()) );

		UInt16 rightSize = currentGuitar->BracketGripRight();

		SetRect(&bracketBounds,
			currentGuitar->BracketPosX(),
			0,				// align to the top
			currentGuitar->BracketPosX()	// grip size is ignored, but makes the bracket really wide
				+ (rightSize ? initRight - currentGuitar->BracketPosX() : currentGuitar->BracketGripLeft()),
			initHeight - 1	// align to the bottom
			);

		SetRect(&tuneNameBounds,
			0,
			0,
			initRight,
			currentGuitar->GetFretTop(0) + metrics.bracketTopOffset );

		LoadDotSprites();
		LoadCursorSprite();
		LoadEffectSprites();
		LoadBracketSprites();
		PositionControls();
	}

	UpdateBackground();
	UpdateTuningInfo();

	return true;
}


void FPGuitarPalette::UpdateMetrics() {
	metrics.bracketTopHeight		= currentGuitar->BracketTopHeight();
	metrics.bracketBottomHeight		= currentGuitar->BracketBottomHeight();
	metrics.bracketMiddleHeight		= currentGuitar->BracketMiddleHeight();
	metrics.bracketStretchHeight	= currentGuitar->BracketStretchHeight();
	metrics.bracketWidth			= currentGuitar->BracketWidth();
	metrics.bracketPosX				= currentGuitar->BracketPosX();
	metrics.bracketTopOffset		= currentGuitar->BracketOffsetTop();
	metrics.bracketBottomOffset		= currentGuitar->BracketOffsetBottom();

	metrics.pixelsWide				= initWidth;
	metrics.pixelsHigh				= initHeight;

	metrics.numberOfStrings = currentGuitar->NumberOfStrings();
	metrics.numberOfFrets = currentGuitar->NumberOfFrets();

	if (metrics.stringx) delete metrics.stringx, metrics.stringx = NULL;

	metrics.stringx = new float[metrics.numberOfStrings];

	for (int string=0; string < metrics.numberOfStrings; string++)
		metrics.stringx[string] = currentGuitar->GetStringX(string);

	metrics.stringSpacing = currentGuitar->GetStringSpacing();

	metrics.titleMargin = currentGuitar->GetTitleMargin();

	metrics.titleColor = currentGuitar->GetTitleColor();
	metrics.titleFont = currentGuitar->GetTitleFont();
	metrics.titleFontSize = currentGuitar->GetTitleFontSize();
	metrics.titleFontStyle = currentGuitar->GetTitleFontStyle();

	metrics.labelColor = currentGuitar->GetLabelColor();
	metrics.labelFont = currentGuitar->GetLabelFont();
	metrics.labelFontSize = currentGuitar->GetLabelFontSize();
	metrics.labelFontStyle = currentGuitar->GetLabelFontStyle();

	if ((metrics.usingPegs = currentGuitar->UsingPegs())) {
		for (int i=0; i < NUM_STRINGS; i++)
			metrics.labelPegPosition[i] = currentGuitar->GetLabelPegPosition(i);
	}

	for (int fret=0; fret < MAX_FRETS + 2; fret++) {
		metrics.frettop[fret] = currentGuitar->GetFretTop(fret);
		metrics.stringy[fret] = (currentGuitar->GetFretTop(fret) + currentGuitar->GetFretBottom(fret)) / 2;
	}
}


void FPGuitarPalette::UpdateBackground() {
	backgroundSprite->SetFlip(reverseStrung, isHorizontal && isLefty);
	backgroundSprite->SetRotation(isHorizontal ? -90 : 0);
	backgroundSprite->Move(0, isHorizontal ? backgroundSprite->Width() : 0);
}


#pragma mark -
void FPGuitarPalette::SetTuning(const FPTuningInfo &t, bool skipRedraw) {
	if (t != currentTuning) {
		currentTuning = t;
		UpdateTuningInfo();

		if (!skipRedraw) {
			DropFingering();
			ArrangeDots();
		}
	}
}


void FPGuitarPalette::ToggleTuningName() {
	tuningViewName ^= true;
	UpdateTuningInfo();
}


void FPGuitarPalette::UpdateTuningOrientation() {
	float rot;
	if (isHorizontal) {
		if (isLefty) {
			rot = 90;
			tuningInfoSprite->Move(initHeight, 0);
		} else {
			rot = -90;
			tuningInfoSprite->Move(0, initWidth);
		}
	} else {
		rot = 0;
		tuningInfoSprite->Move(0, 0);
	}
	tuningInfoSprite->SetHandle(0, 0);
	tuningInfoSprite->SetRotation(rot);
}


void FPGuitarPalette::UpdateTuningInfo() {
	CGContextRef gc = tuningInfoContext;
	CGContextClearRect(gc, CGRectMake(0, 0, tuningInfoSprite->Width(), tuningInfoSprite->Height()));

	TWindow::Focus();

	// if the value is negative then

	int y = tuningInfoSprite->Height() - (metrics.frettop[0] - metrics.titleMargin);

	if (tuningViewName) {
		FontInfo font;
		StringPtr pstring = metrics.titleFont.GetPascalString();
		SetFont(&font, pstring, metrics.titleFontSize, metrics.titleFontStyle, gc);
		delete pstring;

		y += font.ascent;

		TString name(currentTuning.name);
		name.Trim();
		Point	textSize = name.GetDimensions(kThemeCurrentPortFont);

		char *str = name.GetCString();
		CGContextSetRGBFillColor(gc, CG_RGBCOLOR(&metrics.titleColor), 1.0);
		CGContextShowTextAtPoint(gc, (initWidth-textSize.h)/2, y, str, strlen(str));
		delete [] str;

//		fprintf(stderr, "Text Size %d x %d\r", textSize.h, textSize.v);
//		Rect bigrect = { 0, 0, textSize.v, textSize.h };
//		OffsetRect(&bigrect, 10, 10);
//		name.Draw(gc, bigrect, kThemeCurrentPortFont, kThemeStateActive, teCenter);

	}
	else {
		FontInfo font;
		StringPtr pstring = metrics.labelFont.GetPascalString();
		SetFont(&font, pstring, metrics.labelFontSize, metrics.labelFontStyle, gc);
		delete pstring;

		y += font.ascent;

		CGContextSetShouldAntialias(gc, metrics.labelFontSize >= 11);

		unsigned char pstr[6];
		int oddAdjust = (metrics.stringx[1] - metrics.stringx[0] < 14) ? 2 : 0;

		for (int s=0; s<=metrics.numberOfStrings-1; s++) {
			UInt16	tone = NOTEMOD(OpenTone(s));
			const char *str = scalePalette->NameOfNote(tone, tone);
			int len = strlen(str);
			while (str[len-1] == ' ') len--;
			c2pstrncpy(pstr, str, len);

			float dx, dy;
			if (metrics.usingPegs) {
				dx = metrics.labelPegPosition[s].x;
				dy = tuningInfoSprite->Height() - metrics.labelPegPosition[s].y;
				CGContextSetRGBFillColor(gc, CG_RGBCOLOR(&metrics.labelColor), 1.0);
			}
			else {
				dx = metrics.stringx[s];

				if (oddAdjust && (s & 1)) {
					dy = y + oddAdjust;
					CGContextSetRGBFillColor(gc, CG_RGBCOLOR(&rgbGray8), 1.0);
				} else {
					dy = y;
					CGContextSetRGBFillColor(gc, CG_RGBCOLOR(&metrics.labelColor), 1.0);
				}
			}

			if ((!isHorizontal && reverseStrung) || (isHorizontal && (isLefty != reverseStrung)))
				dx = initRight - dx;

			CGContextShowTextAtPoint(gc, dx - StringWidth(pstr) / 2, dy, str, len);
		}

		CGContextSetShouldAntialias(gc, true);
	}

	tuningInfoSprite->LoadCGContext(gc);
	UpdateTuningOrientation();
}


#pragma mark -
void FPGuitarPalette::ToggleHorizontal() {
	isHorizontal ^= true;

	if (isHorizontal)
		SetContentSize(initHeight, initWidth);
	else
		SetContentSize(initWidth, initHeight);

	UpdateBackground();
	UpdateTuningOrientation();
	LoadBracketSprites();
	PositionControls();
	PositionDotSprites();
	UpdateBracket();
	UpdateCursor();
}


void FPGuitarPalette::ToggleLefty() {
	isLefty ^= true;
	UpdateBackground();
	UpdateTuningInfo();
	PositionControls();
	PositionDotSprites();
	UpdateBracket();
	UpdateCursor();
}


void FPGuitarPalette::ToggleReverseStrung() {
	reverseStrung ^= true;
	UpdateBackground();
	UpdateTuningInfo();
	PositionDotSprites();
	UpdateBracket();
	UpdateCursor();
}


void FPGuitarPalette::ToggleUnusedTones() {
	showRedDots ^= true;
	ArrangeDots();
}


void FPGuitarPalette::ToggleFancyDots() {
	static UInt16 oldStyle = kDotStyleDots;

	if (dotStyle == kDotStyleDots)
		SetDotStyle((oldStyle == kDotStyleDots) ? kDotStyleLetters : oldStyle);
	else {
		oldStyle = dotStyle;
		SetDotStyle(kDotStyleDots);
	}
}


void FPGuitarPalette::SetDotStyle(UInt16 style) {
	dotStyle = style;
	ArrangeDots();
}


#pragma mark Fret Bracket
void FPGuitarPalette::BracketDragStart(UInt32 type, int y) {
	bracketDragType = type;
	bracketDragStart = y;

	int	low = globalChord.brakLow,
		high = globalChord.brakHi;

	if (type == kBracketPartMiddle) {
		// Bracket span
		const int span = high - low;

		const float
			// Important bracket part positions
			top_at_fret_0 = metrics.frettop[0],
			bot_at_fret_0 = metrics.fretbottom[span],
			top_at_fret_1 = metrics.frettop[1],
			bot_at_fret_1 = metrics.fretbottom[span + 1],
			top_at_fret_max = metrics.frettop[metrics.numberOfFrets - span],
			bot_at_fret_max = metrics.fretbottom[metrics.numberOfFrets],

			top_diff_special = top_at_fret_1 - top_at_fret_0,
			bot_diff_special = bot_at_fret_1 - bot_at_fret_0,
			top_diff_normal = top_at_fret_max - top_at_fret_1,
			bot_diff_normal = bot_at_fret_max - bot_at_fret_1,

			// Relative proportions between the sizes
			size_current = metrics.fretbottom[high] - metrics.frettop[low],
			grip_base = y - metrics.frettop[low],
			grip_factor = grip_base / size_current;

		// Grip position at every stage of the way
		grip_at_fret_0 = ceil(top_at_fret_0 + (bot_at_fret_0 - top_at_fret_0) * grip_factor),
		grip_at_fret_1 = ceil(top_at_fret_1 + (bot_at_fret_1 - top_at_fret_1) * grip_factor),
		grip_at_fret_max = ceil(top_at_fret_max + (bot_at_fret_max - top_at_fret_max) * grip_factor);

		// Finally derive the relative motion factors
		const float
			grip_diff_special = grip_at_fret_1 - grip_at_fret_0,
			grip_diff_normal = grip_at_fret_max - grip_at_fret_1;

		top_mult_normal = top_diff_normal / grip_diff_normal;
		bot_mult_normal = bot_diff_normal / grip_diff_normal;
		top_mult_special = top_diff_special / grip_diff_special;
		bot_mult_special = bot_diff_special / grip_diff_special;

		bracketTopBase = top_at_fret_1;
		bracketBottomBase = bot_at_fret_1;
	}
	else {
		bracketTopBase = metrics.frettop[low];
		bracketBottomBase = metrics.fretbottom[high];
	}

}


void FPGuitarPalette::BracketDragStop() {
	bracketDragType = kBracketNoPart;
	UpdateBracket();
}


void FPGuitarPalette::DragBracket(SInt16 y) {
	if (true || bracketDragType != kBracketPartMiddle) {
		int y1 = bracketTopBase;
		int y2 = bracketBottomBase;

		switch (bracketDragType) {
			case kBracketPartTop:
				y1 += (y - bracketDragStart);
				CONSTRAIN(y1, metrics.frettop[0], metrics.frettop[globalChord.brakHi]);
				break;

			case kBracketPartBottom:
				y2 += (y - bracketDragStart);
				CONSTRAIN(y2, metrics.fretbottom[globalChord.brakLow], metrics.fretbottom[metrics.numberOfFrets]);
				break;

			case kBracketPartMiddle: {
				CONSTRAIN(y, grip_at_fret_0, grip_at_fret_max);

				float diff = y - grip_at_fret_1;

				if (ABS(y - bracketDragStart) < 2) {
					y1 = metrics.frettop[globalChord.brakLow];
					y2 = metrics.fretbottom[globalChord.brakHi];
				} else if (diff < 0) {
					y1 += ceil(diff * top_mult_special);
					y2 += ceil(diff * bot_mult_special);
				} else {
					y1 += ceil(diff * top_mult_normal);
					y2 += ceil(diff * bot_mult_normal);
				}
				break;
			}
		}

		bracketDragActiveTop = y1;
		bracketDragActiveBottom = y2;

		DrawBracket(y1, y2);
	}
}


void FPGuitarPalette::GetBracketCapInfo(int &capHeight1, int &capHeight2, int &capOffset1, int &capOffset2) {
	if (isHorizontal && isLefty) {
		capHeight1 = metrics.bracketBottomHeight;
		capHeight2 = metrics.bracketTopHeight;
		capOffset1 = -(capHeight1 + metrics.bracketBottomOffset);
		capOffset2 = -(capHeight2 + metrics.bracketTopOffset);
	} else {
		capHeight1 = metrics.bracketTopHeight;
		capHeight2 = metrics.bracketBottomHeight;
		capOffset1 = metrics.bracketTopOffset;
		capOffset2 = metrics.bracketBottomOffset;
	}
}


void FPGuitarPalette::DrawBracket(UInt16 ytop, UInt16 ybot) {
	int stretch1, stretch2, middle,
		capHeight1, capHeight2, capOffset1, capOffset2;

	GetBracketCapInfo(capHeight1, capHeight2, capOffset1, capOffset2);

	// The starting position is the bracket's top end
	int y = ytop + capOffset1;

	// Get the bottom of the bracket top
	int space_start = y + capHeight1;

	// And the top of the bracket bottom
	int space_end = ybot + capOffset2;

	// The space between them
	int space_between = space_end - space_start;

	// If there is any space, fill it with the thumb and stretch
	if (space_between) {
		middle = (space_between > metrics.bracketMiddleHeight) ? metrics.bracketMiddleHeight : 0;
		space_between -= middle;
		stretch1	= space_between / 2;
		stretch2	= space_between - stretch1;
	}
	else
		stretch1 = stretch2 = middle = 0;

	// Show all the parts that have some size
	bracketTopSprite->Show();
	bracketStretchSprite1->SetVisible(stretch1 > 0);
	bracketMiddleSprite->SetVisible(middle > 0);
	bracketStretchSprite2->SetVisible(stretch2 > 0);
	bracketBottomSprite->Show();

	if (isHorizontal) {
		int x = y;
		y = initRight - metrics.bracketPosX - metrics.bracketWidth + 2;		// +2 lower due to shadow

		// TODO: Add a new bracket offset value to guitars - to get rid of +2 above

		if (isLefty) {
			x = initBottom - x;

			//
			// Right
			//
			x -= metrics.bracketBottomHeight;
			bracketBottomSprite->Move(x, y);

			//
			// Stretch
			//
			if (stretch1) {
				x -= stretch1;
				bracketStretchSprite1->SetScaleX(stretch1 / bracketStretchSprite1->BaseWidth());
				bracketStretchSprite1->Move(x, y);
			}

			//
			// Middle
			//
			if (middle) {
				x -= metrics.bracketMiddleHeight;
				bracketMiddleSprite->Move(x, y);
			}

			//
			// Stretch
			//
			if (stretch2) {
				x -= stretch2;
				bracketStretchSprite2->SetScaleX(stretch2 / bracketStretchSprite2->BaseWidth());
				bracketStretchSprite2->Move(x, y);
			}

			//
			// Left
			//
			x -= metrics.bracketTopHeight;
			bracketTopSprite->Move(x, y);
		}
		else {
			//
			// Top
			//
			bracketTopSprite->Move(x, y);
			x += metrics.bracketTopHeight;

			//
			// Stretch
			//
			if (stretch1) {
				bracketStretchSprite1->SetScaleX(stretch1 / bracketStretchSprite1->BaseWidth());
				bracketStretchSprite1->Move(x, y);
				x += stretch1;
			}

			//
			// Middle
			//
			if (middle) {
				bracketMiddleSprite->Move(x, y);
				x += metrics.bracketMiddleHeight;
			}

			//
			// Stretch
			//
			if (stretch2) {
				bracketStretchSprite2->SetScaleX(stretch2 / bracketStretchSprite2->BaseWidth());
				bracketStretchSprite2->Move(x, y);
				x += stretch2;
			}

			//
			// Bottom
			//
			bracketBottomSprite->Move(x, y);
		}
	}
	else {
		int x = metrics.bracketPosX;

//		if (IsReverseStrung()) {
//			int original_gap = metrics.stringx[0] - x;
//			x = (InitWidth() - metrics.stringx[5]) - original_gap;
//		}

		//
		// Top
		//
		bracketTopSprite->Move(x, y);
		y += capHeight1;

		//
		// Stretch
		//
		if (stretch1) {
			bracketStretchSprite1->SetScaleY((float)stretch1 / metrics.bracketStretchHeight);
			bracketStretchSprite1->Move(x, y);
			y += stretch1;
		}

		//
		// Middle
		//
		if (middle) {
			bracketMiddleSprite->Move(x, y);
			y += middle;
		}

		//
		// Stretch
		//
		if (stretch2) {
			bracketStretchSprite2->SetScaleY((float)stretch2 / metrics.bracketStretchHeight);
			bracketStretchSprite2->Move(x, y);
			y += stretch2;
		}

		//
		// Bottom
		//
		bracketBottomSprite->Move(x, y);
		y+= capHeight2;
	}
}


void FPGuitarPalette::UpdateBracket() {
	if (bracketDragType != kBracketNoPart)
		return;

	if (globalChord.bracketFlag) {
		DrawBracket(metrics.frettop[globalChord.brakLow], metrics.fretbottom[globalChord.brakHi]);
	}
	else {
		bracketTopSprite->Hide();
		bracketStretchSprite1->Hide();
		bracketMiddleSprite->Hide();
		bracketStretchSprite2->Hide();
		bracketBottomSprite->Hide();
	}
}


#pragma mark -
//-----------------------------------------------
//
//	ArrangeDots
//
//	1. Set dot visibility
//	2. Set dot frames based on chord function
//	3. Calculate fingering tones if the bracket is enabled
//	4. Simply don't draw dots beyond the guitar size
//
void FPGuitarPalette::ArrangeDots() {
	FPChord		chord(globalChord);
	FPSpritePtr	sprite = NULL;
	SInt16		tone, openTone, bit,
				bestFret, oldFret,
				color = 0,
				spriteFret = 0;
	bool		noTone, doDots,
				bShow = false,
				bBlink = false;
	UInt16		scaleNote = scalePalette->CurrentTone();
	UInt16		framePlus = (((TickCount() % 60) / 30) ? kDotColorCount : 0);
	FatChord	fatChord = chord.TwoOctaveChord();
	bool		rootType = fretpet->chordRootType;
	UInt16		accType = (FIFTHS_POSITION(chord.key) > 11 - scalePalette->Enharmonic()) ? 12 : 0;

	//
	//	First set up the colors for each note in the chord
	//
	UpdateDotColors();

	//
	//	Prepare an empty tone mask
	//
	fingeredChord = 0;

	//
	//	Loop through all the strings
	//
	for (int string=0; string<metrics.numberOfStrings; string++) {
		//	Get the open tone
		openTone = NOTEMOD(currentTuning.tone[string]);

		//	Get the current fretted tone
		oldFret = globalChord.fretHeld[string];
		if (oldFret == -1)
			bit = 0x8000;
		else
			bit = BIT((openTone + oldFret) % OCTAVE);

		//
		//	Calculate a new tone only if the fretting is:
		//		a. set to -1, indicating no tone
		//		b. on a tone that is no longer in the chord
		//
		//		... or if the palette has been flagged
		//
		doDots = redoDots || oldFret == -1 || !chord.HasTones(bit);

		//
		//	Set this string as having no tone yet
		//

		noTone = true;
		bestFret = -1;

		//
		//	Get the highest string tone to start
		//

		EQU_MOD(tone, openTone + metrics.numberOfFrets, OCTAVE);

		//
		//	Loop backwards through all the frets
		//
		for (int fret=metrics.numberOfFrets; fret>=-1; fret--) {
			//
			//	While we're still within the 'real' frets
			//	(fret -1 is purgatory for unused notes)
			//
			if (fret >= 0) {
				bit = BIT(tone);
				color = dotColor[tone];

				//
				//	Initially make all sprites invisible and unblinking
				//
				sprite = dotSprite[string][fret];
				spriteFret = fret;
				bShow = false;										// Assume it will be hidden
				bBlink = (tone == scaleNote);						// Scale tone blinks

				//
				//	Is the tone in the chord?
				//
				if (chord.HasTones(bit)) {
					//
					//	If the bracket is on we need to calculate fingered tones
					//
					if (chord.bracketFlag) {
						if (!doDots) {
							if (fret == oldFret) {
								noTone = false;
								fingeredChord |= bit;
							}
							else
								color = kDotColorRed;
						}

						//
						//	Are we within the bracketed range?
						//
						else if (fret >= chord.brakLow && fret <= chord.brakHi) {
							//
							//	Is no tone selected on this string yet?
							//
							if (noTone) {
								//
								//	If the tone has been used already then
								//	save it in case no other tones are good
								//
								if (fingeredChord & bit) {			// got this note already?
									bestFret = fret;				// remember the fret
									color = kDotColorRed;				// set color to non
								}

								//
								//	Unused tones get first dibs
								//
								else {
									noTone = false;					// got one...
									fingeredChord |= bit;			// add it in
									chord.fretHeld[string] = fret;	// store it
								}

							}
							else // (a tone has already been selected)
								color = kDotColorRed;
						}
						else // (outside the bracketed region)
							color = kDotColorRed;
					}
					else // (the bracket is turned off)
	 					chord.fretHeld[string] = -1;

					//
					//	Make the sprite visible if appropriate
					//
					bShow = (showRedDots || color != kDotColorRed);

				} // chord & bit

				DEC_WRAP(tone, OCTAVE);

			}

			//
			//	(...After looping thru all the frets)
			//	Was the bracket on but no tone selected?
			//
			else if (chord.bracketFlag && noTone) {
				//
				//	Try the open string as a last resort!
				//
				if (bestFret < 0) {
					bit = BIT(openTone);
					if (chord.HasTones(bit)) {
						fingeredChord |= bit;
						bestFret = 0;
					}
				}

				//
				//	Set some dot visible if possible
				//
				chord.fretHeld[string] = bestFret;

				if (bestFret >= 0) {
					tone = (openTone + bestFret) % OCTAVE;
					bShow = true;
					sprite = dotSprite[string][bestFret];
					spriteFret = bestFret;
					color = dotColor[tone];
					bBlink = (tone == scaleNote);
				}

				if (bestFret == -1) {
					bShow = true;
					sprite = dotSprite[string][0];
					color = kDotIndexX;
					bBlink = false;
				}

			} // bracket && noTone

			//
			//	Set up the sprite for the current tone
			//
			sprite->SetAnimationInterval(-1);
			sprite->SetVisible(bShow);

			if (bShow) {
				UInt16 style = (color == kDotIndexX) ? kDotStyleDots : dotStyle;

				switch (style) {
					case kDotStyleDots: {
						int f = color + (bBlink ? framePlus : 0);
						sprite->SetAnimationFrameIndex(f);

						if (bBlink)
							sprite->SetAnimationInterval(1000/15);

						sprite->Show();
						break;
					}

					case kDotStyleNumbers: {
						UInt16 myTone = NOTEMOD(openTone + spriteFret);
						UInt16 func = NOTEMOD(myTone - (rootType ? chord.key : chord.root));

						if (!rootType && !(BIT(myTone) & fatChord.mask))
							func += OCTAVE;

						sprite->SetAnimationFrameIndex(kDotIndexNumWhite + kDotNumCount * color + func);
						break;
					}

					case kDotStyleLetters: {
						UInt16 myTone = NOTEMOD(openTone + spriteFret);
						sprite->SetAnimationFrameIndex(kDotIndexToneWhite + kDotNumCount * color + accType + myTone);
						break;
					}
				}
			}

		} // fret loop

	} // string loop

	redoDots = false;

	for (int i=metrics.numberOfStrings; i--;)
		globalChord.fretHeld[i] = chord.fretHeld[i];
}


//-----------------------------------------------
//
//	NewFingering
//
//  Calculate a new fingering for a chord using
//	the current tuning of the palette.
//
void FPGuitarPalette::NewFingering(FPChord &pChord) {
	UInt16	tones = pChord.tones, fingeredChord = 0;
	SInt16	fret, string, tone, openTone, bestTone, bit;
	SInt16	hh, bestFret;
	bool	noTone;

	for (string=0; string<metrics.numberOfStrings; string++) {
		if (pChord.bracketFlag) {
			hh = fretpet->IsReverseStrung() ? metrics.numberOfStrings-1 - string : string;
			openTone = NOTEMOD(OpenTone(string));
			tone = NOTEMOD(openTone + metrics.numberOfFrets);

			noTone = true;
			bestFret = -1;

			for (fret = metrics.numberOfFrets; fret >= -1 ; fret--) {
				if (fret >= 0) {
					bit = BIT(tone);

					if (tones & bit) {
						if (noTone && fret >= pChord.brakLow && fret <= pChord.brakHi) {
							if (fingeredChord & bit) {
								bestFret = fret;
								bestTone = tone;
							}
							else {
								noTone = false;
								fingeredChord |= bit;
								pChord.fretHeld[string] = fret;
							}
						}

					} // tones & bit

					DEC_WRAP(tone, OCTAVE);

				}
				else if (noTone) {
					if (bestFret < 0) {
						bit = BIT(openTone);
						if (tones & bit) {
							fingeredChord |= bit;
							bestFret = 0;
							bestTone = openTone;
						}
					}

					pChord.fretHeld[string] = bestFret;

				} // noTone

			} // fret loop
		}
		else {
			pChord.fretHeld[string] = -1;
		}

	} // string loop
}


bool FPGuitarPalette::OverrideFingering(FPChord &pChord) {
	if ( pChord.bracketFlag && ShowingUnusedTones() && pChord.HasTone(CurrentTone()) ) {
		if (!pChord.HasTone(CurrentTone()))
			pChord.AddTone(CurrentTone());

		SInt16 oldFret = pChord.fretHeld[currentString];
		pChord.fretHeld[currentString] = ((currentFret == oldFret) ? -1 : currentFret);
		return true;
	}

	return false;
}


void FPGuitarPalette::UpdateDotColors() {
	int tone;
	for (int i=0; i<OCTAVE; i++) {
		EQU_LIMIT(tone, globalChord.Root() + i, OCTAVE);
		dotColor[tone] = (i > 0) ? (scalePalette->ScaleHasTone(globalChord.Key(), tone) ? kDotColorWhite : kDotColorGray) : kDotColorYellow;
	}
}


void FPGuitarPalette::TwinkleTone(SInt16 string, SInt16 fret) {
	if (IsVisible() && masterTwinkle) {
		FPSpritePtr		sprite = new FPEffectSprite(*(FPEffectSprite*)masterTwinkle);

		if (sprite) {
			int x, y;
			DotPosition(string, fret, &x, &y);
			sprite->Move(x, y);
			AppendSprite(sprite, kDotsLayer);
		}
	}
}

