/*
 *  FPPianoPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPPianoPalette.h"

#include "FPScalePalette.h"

#include "FPSprite.h"
#include "FPApplication.h"
#include "FPHistory.h"
#include "FPPreferences.h"
#include "FPMusicPlayer.h"
#include "FPMacros.h"


FPPianoPalette *pianoPalette = NULL;


#define kWhiteKeyCount 31

typedef struct {
	const char *tag;
	int whiteWidth,	whiteHeight, whiteDotY,
		blackWidth,	blackHeight, blackDotY,
		dotSize;
} FPPianoInfo;

FPPianoInfo pianoSize[] = {
	{ "",
		11, 55, 40,
		9, 33, 20,
		11 },
	{ "Medium",
		15, 63, 48,
		10, 36, 23,
		15 },
	{ "Large",
		21, 88, 67,
		15, 54, 34,
		15 }
};



#pragma mark -
FPPianoControl::FPPianoControl(WindowRef wind) : TCustomControl(wind, 'pian', 1) { }


bool FPPianoControl::Track(MouseTrackingResult eventType, Point where) {
	static SInt16			lastKey;
	SInt16					newKey = GetKeyFromPoint(where);
	static UInt16			totalClicks;
	static FPHistoryEvent	*event;
	static FPChord			chordBefore;
	bool					transforming = fretpet->IsTransformModeEnabled();

	bool noTone = false;
	switch (eventType) {
		case kMouseTrackingMouseDown:

			totalClicks = CountClicks(newKey==lastKey, transforming ? 1 : 3);
			lastKey = -1;

			switch(totalClicks) {
				case 1: {
					if (transforming) {
						chordBefore = globalChord;
						event = fretpet->StartUndo(UN_EDIT, CFSTR("Direct Transform"));
						event->SaveCurrentBefore();
					}
					break;
				}

				case 2: {
					event = fretpet->StartUndo(UN_EDIT, CFSTR("Toggle Tone"));
					event->SaveCurrentBefore();

					fretpet->DoToggleTone(newKey - OCTAVE);

					event->SaveCurrentAfter();
					event->Commit();

					noTone = true;
					break;
				}

				case 3: {
					event = fretpet->StartUndo(UN_EDIT, CFSTR("Add Triad"));
					event->SetMerge();
					event->SaveCurrentBefore();

					if (!globalChord.HasTones())
						globalChord.SetRoot(newKey);

					noTone = fretpet->DoAddTriadForTone(newKey - OCTAVE);

					if (!noTone)
						SysBeep(1);

					event->SaveCurrentAfter();
					event->Commit();
					break;
				}
			}

			break;

		case kMouseTrackingMouseUp: {
			if (transforming) {
				if (chordBefore != globalChord) {
					event->SaveCurrentAfter();
					event->Commit();
				}
				fretpet->EndTransformOnce();
			}
			break;
		}
	}

	if (lastKey != newKey) {
		if (transforming) {
			fretpet->DoDirectHarmonize(newKey);
		}
		else if
			(	!noTone
				&&	newKey >= 0
				&&	newKey < kPianoKeys
				&&	(!HAS_COMMAND(GetCurrentEventKeyModifiers()) || scalePalette->ScaleHasTone(newKey))
				&&	(!HAS_OPTION(GetCurrentEventKeyModifiers()) || globalChord.HasTone(newKey))
			)
				player->PlayNote(kCurrentChannel, newKey);

		lastKey = newKey;
	}

	return true;
}


UInt16 FPPianoControl::GetKeyFromPoint(Point where) {
	UInt16	size = pianoPalette->PianoSize();

	const UInt16 space = pianoSize[size].whiteWidth + 1;

	UInt16	whiteKey = where.h / space;
	UInt16	octave = whiteKey / NUM_STEPS;
	UInt16	tone = octave * OCTAVE + theCScale[whiteKey % NUM_STEPS];

	UInt16	blackAdjust = pianoSize[size].blackWidth / 2,
			blackInset = space - blackAdjust;

	if (where.v < pianoSize[size].blackHeight) {
		int inkey = where.h % space,
			modtone = whiteKey % NUM_STEPS;

		if (inkey > blackInset) {
			if (modtone != 2 && modtone != 6) {
				tone++;
			}
		}
		else if (inkey < blackAdjust) {
			if (modtone != 3 && modtone != 0) {
				tone--;
			}
		}
	}


//	printf("Where: %03d  Tone: %d:%d  White: %d  Black: %s (%d %d)\r", where.h, octave, tone, whiteKey, black ? "YES" : "NO", blackKey, blackRem);

	return tone;
}


#pragma mark -
enum {
	kPianoLayerWhite1 = 0,
	kPianoLayerWhite2,
	kPianoLayerWhite3,
	kPianoLayerBlack1,
	kPianoLayerBlack2,
	kPianoLayerBlack3,
	kPianoLayerDots1,
	kPianoLayerDots2,
	kPianoLayerDots3,
	kPianoLayerCount
};



FPPianoPalette::FPPianoPalette() : FPPaletteGL(CFSTR("Keyboard"), kPianoLayerCount) {
	const EventTypeSpec	pianoEvents[] = {
		{ kEventClassWindow, kEventWindowZoom }
	};

	RegisterForEvents( GetEventTypeCount( pianoEvents ), pianoEvents );

	for (int size=0; size < kPianoSizes; size++) {
		keyData[size].whiteLayer = Layer(kPianoLayerWhite1 + size);
		keyData[size].blackLayer = Layer(kPianoLayerBlack1 + size);
		keyData[size].dotsLayer = Layer(kPianoLayerDots1 + size);
		keyData[size].whiteLayer->Hide();
		keyData[size].blackLayer->Hide();
		keyData[size].dotsLayer->Hide();
	}

	pianoControl = new FPPianoControl(Window());
	float color[] = { 0, 0, 0, 0 };
	SetClearColor(color);
	InitSprites();

	keyboardSize = 999;
	SetKeyboardSize(preferences.GetInteger(kPrefKeyboardSize, 0));
}


FPPianoPalette::~FPPianoPalette() {
	for (int i=0; i<kPianoSizes; i++) {
		delete keyData[i].whiteMaster;
		delete keyData[i].blackMaster;
		delete keyData[i].dotsMaster;
	}
}


void FPPianoPalette::InitSprites() {
	FPSpritePtr	pSprite;
	CFURLRef	url;

	TString		imagePath;

	for (int size=0; size < kPianoSizes; size++) {
		FPPianoKeyData	&kd = keyData[size];
		FPPianoInfo		&ps = pianoSize[size];

		//
		// Key Graphics
		//
		imagePath.SetWithFormat(CFSTR("piano/whitekeys%s.png"), ps.tag);
		url = fretpet->GetImageURL(imagePath.GetCFStringRef());
		kd.whiteMaster = NewSprite();
		kd.whiteMaster->LoadImageGrid(url, ps.whiteWidth, ps.whiteHeight);
		kd.whiteMaster->SetHandle(0, 0);
		CFRELEASE(url);

		imagePath.SetWithFormat(CFSTR("piano/blackkeys%s.png"), ps.tag);
		url = fretpet->GetImageURL(imagePath.GetCFStringRef());
		kd.blackMaster = NewSprite();
		kd.blackMaster->LoadImageGrid(url, ps.blackWidth, ps.blackHeight);
		kd.blackMaster->SetHandle(0, 0);
		CFRELEASE(url);

		//
		// Key Sprites
		//
		float	xpos, pos = 0;
		for (int key=0; key<kPianoKeys; key++) {
			int		k = key % OCTAVE;
			bool	black = isBlack[k];

			xpos = pos;
			float x = xpos * (ps.whiteWidth + 1);

			if (black) {
				x -= (ps.blackWidth + 1) / 2;
				pSprite = NewSprite(kd.blackMaster, kPianoLayerBlack1 + size);
				kd.key[key].x = x + (ps.blackWidth + 1) / 2;
				kd.key[key].y = ps.blackDotY;
			}
			else {
				pSprite = NewSprite(kd.whiteMaster, kPianoLayerWhite1 + size);
				kd.key[key].x = x + (ps.whiteWidth + 2) / 2;
				kd.key[key].y = ps.whiteDotY;
				pos++;
			}

			pSprite->Move(x, 0);
			kd.key[key].sprite = pSprite;
		}

		//
		// Dot sprites
		//
		imagePath.SetWithFormat(CFSTR("piano/dots%s.png"), ps.tag);
		url = fretpet->GetImageURL(imagePath.GetCFStringRef());
		kd.dotsMaster = NewSprite();
		kd.dotsMaster->LoadImageGrid(url, ps.dotSize);
		kd.dotsMaster->Hide();
		CFRELEASE(url);

		for (int dot=OCTAVE; dot--;) {
			kd.dotSprite[dot] = pSprite = NewSprite(kd.dotsMaster);
			AppendSprite(pSprite, kPianoLayerDots1 + size);
		}
	}

	UpdatePianoKeys();
	StartSpriteTimer();
}


bool FPPianoPalette::Zoom() {
	ToggleKeyboardSize();
	return true;
}


void FPPianoPalette::ToggleKeyboardSize() {
	SetKeyboardSize(keyboardSize + 1);
}


void FPPianoPalette::SetKeyboardSize(UInt16 size) {
	size %= kPianoSizes;
	if (keyboardSize != size) {
		keyboardSize = size;
		int	w = kWhiteKeyCount * (pianoSize[keyboardSize].whiteWidth + 1) - 1,
			h = pianoSize[keyboardSize].whiteHeight + 1;
		SetContentSize(w, h);
		SetDefaultSize(w, h);
		pianoControl->SetBounds(GetContentSize());
		for (int i=0; i < kPianoSizes; i++) {
			bool vis = (keyboardSize == i);
			keyData[i].whiteLayer->SetVisible(vis);
			keyData[i].blackLayer->SetVisible(vis);
			keyData[i].dotsLayer->SetVisible(vis);
		}

		PositionPianoDots();
	}
}


void FPPianoPalette::PositionPianoDots() {
	FPPianoKeyData	&kd = keyData[keyboardSize];

	if (fretpet->IsBracketEnabled()) {
		for (int dot=OCTAVE; dot--;) {
			FPSpritePtr sprite = kd.dotSprite[dot];
			int key = (dot < NUM_STRINGS) ? fretpet->StringTone(dot) : -1;
			if (key >= 0) {
				sprite->Move(kd.key[key].x - 1, kd.key[key].y);
				sprite->SetAnimationFrameIndex(fretpet->DotColor(key));
			}
			sprite->SetVisible(key >= 0);
		}
	}
	else {
		UInt16	root = globalChord.root;
		UInt16	tone = root + OCTAVE;
		if (root < globalChord.key) tone += OCTAVE;

		for (int dot=OCTAVE; dot--;) {
			FPSpritePtr sprite = kd.dotSprite[dot];
			if (globalChord.HasTone(tone)) {
				sprite->Move(kd.key[tone].x - 1, kd.key[tone].y);
				sprite->SetAnimationFrameIndex(fretpet->DotColor(tone));
				sprite->Show();
			}
			else
				sprite->Hide();

			tone++;
		}
	}
}


void FPPianoPalette::SetKeyPlaying(SInt16 k, bool isPlaying) {
	if (k >= 0 && k < kPianoKeys) {
		UInt16 frame = isPlaying ? 2 : (scalePalette->ScaleHasTone(globalChord.Key(), k % OCTAVE) ? 1 : 0);
		for (int i=0; i < kPianoSizes; i++)
			keyData[i].key[k].sprite->SetAnimationFrameIndex(frame);
	}
}


void FPPianoPalette::UpdatePianoKeys() {
	for (int size=0; size<kPianoSizes; size++) {
		for (int k=0; k<kPianoKeys; k++) {
			FPSpritePtr sprite = keyData[size].key[k].sprite;
			if (sprite->AnimationFrameIndex() != 2)
				sprite->SetAnimationFrameIndex(scalePalette->ScaleHasTone(globalChord.Key(), k % OCTAVE) ? 1 : 0);
		}
	}
}


void FPPianoPalette::SavePreferences() {
	preferences.SetInteger(kPrefKeyboardSize, keyboardSize);
	FPPalette::SavePreferences();
}


void FPPianoPalette::ProcessAndAnimateSprites() {
	FPPaletteGL::ProcessAndAnimateSprites();
//	SetNextFireTime(1.0 / 10.0);
}
