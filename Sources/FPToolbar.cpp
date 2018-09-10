/*
 *  FPToolbar.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPApplication.h"
#include "FPPreferences.h"
#include "FPDocWindow.h"
#include "FPToolbar.h"
#include "TControls.h"
#include "TCarbonEvent.h"


FPToolbar::FPToolbar() : FPPalette(CFSTR("Toolbar")) {
	WindowRef   wind = Window();
	transButton	= new TPictureControl(wind, kFPCommandTransformMode, kPictTrans1);
	clearButton	= new TPictureControl(wind, kFPCommandClearTonesButton, kPictClearNew1);
//	undoButton	= new TPictureControl(wind, kHICommandUndo, kPictUndo1);
	plusButton	= new TPictureControl(wind, kFPCommandAddChord, kPictPlus1);
	hearButton	= new TPictureControl(wind, kFPCommandHearButton, kPictHear1);
	stopButton	= new TPictureControl(wind, kFPCommandStop, kPictStop1);
	playButton	= new TPictureControl(wind, kFPCommandPlay, kPictPlay1);
	loopButton	= new TPictureControl(wind, kFPCommandLoop, kPictLoop1);
	eyeButton	= new TPictureControl(wind, kFPCommandEye, kPictEye1);
	editButton	= new TPictureControl(wind, kFPCommandFree, kPictEdit1);
	soloButton	= new TPictureControl(wind, kFPCommandSolo, kPictSolo1);
	romanButton	= new TPictureControl(wind, kFPCommandRoman, kPictNewRoman1);
	fifthButton	= new TSegmentedPictureButton(wind, kFPCommandFifths, kPictFifth1);
	halfButton	= new TSegmentedPictureButton(wind, kFPCommandHalves, kPictHalf1);
	rootButton	= new TSegmentedPictureButton(wind, kFPCommandNumRoot, kPictRoot1);
	keyButton	= new TSegmentedPictureButton(wind, kFPCommandNumKey, kPictKey1);

//	undoButton->Disable();
	hearButton->Disable();
	stopButton->Disable();
	playButton->Disable();

	triadButton	= new TPictureControl(wind, kFPCommandAddTriad, kPictTriad1);
	scaleButton	= new TPictureControl(wind, kFPCommandAddScale, kPictScale1);

	Init();

	SetAnchored(TRUE);
	UpdateButtonStates();
}


FPToolbar::~FPToolbar() {
	delete transButton;
	delete clearButton;
//	delete undoButton;
	delete plusButton;
	delete hearButton;
	delete stopButton;
	delete playButton;
	delete loopButton;
	delete eyeButton;
	delete editButton;
	delete soloButton;
	delete romanButton;

	delete triadButton;
	delete scaleButton;
}


void FPToolbar::Init() {
	loopButton->SetState(preferences.GetBoolean(kPrefLoop, TRUE));
	eyeButton->SetState(preferences.GetBoolean(kPrefFollow, FALSE));
	editButton->SetState(preferences.GetBoolean(kPrefFreeEdit, FALSE));

	DrawKeyOrder();
	DrawRootType();
}


bool FPToolbar::LoadPreferences(bool showing) {
	FPWindow::LoadPreferences(showing);

	if (IsVisible())
		SetAnchored(preferences.GetBoolean(kPrefAnchor, FALSE));

	return true;
}


void FPToolbar::SavePreferences() {
	preferences.SetBoolean(kPrefLoop, loopButton->State());
	preferences.SetBoolean(kPrefFollow, eyeButton->State());
	preferences.SetBoolean(kPrefFreeEdit, editButton->State());
	FPPalette::SavePreferences();
}

void FPToolbar::DrawKeyOrder() {
	fifthButton->SetState(!fretpet->keyOrderHalves);
	fifthButton->SetEnabled(fretpet->keyOrderHalves);

	halfButton->SetState(fretpet->keyOrderHalves);
	halfButton->SetEnabled(!fretpet->keyOrderHalves);
}


bool FPToolbar::IsAnchored() {
	Rect	displayRect, bounds = Bounds(), toolPos = StructureBounds();
	int		titlebarSize = bounds.top - toolPos.top;
	UInt16	maximumTop = 0;

	GDHandle theDevice = DMGetFirstScreenDevice(true);
	while (theDevice) {
		(void) GetAvailableWindowPositioningBounds(theDevice, &displayRect);
		theDevice = DMGetNextScreenDevice(theDevice, true);

		if (RectsOverlap(bounds, displayRect) && displayRect.top > maximumTop)
			maximumTop = displayRect.top;
	}

	return (bounds.top - maximumTop < titlebarSize);
}


void FPToolbar::SetAnchored(bool anchor) {
	anchor = FALSE;

	UInt16	maximumTop = 0;
	Rect	displayRect, bounds = Bounds(), toolPos = StructureBounds();
	int		titlebarSize = bounds.top - toolPos.top;

	GDHandle theDevice = DMGetFirstScreenDevice(true);
	while (theDevice) {
		(void) GetAvailableWindowPositioningBounds(theDevice, &displayRect);
		theDevice = DMGetNextScreenDevice(theDevice, true);

		if (RectsOverlap(bounds, displayRect) && displayRect.top > maximumTop)
			maximumTop = displayRect.top;
	}

	anchor
		? Move(bounds.left, maximumTop)
		: Move(bounds.left, maximumTop + titlebarSize + 1);

	Select();

	preferences.SetBoolean(kPrefAnchor, anchor);
	preferences.Synchronize();
}


void FPToolbar::UpdateButtonStates() {
	bool hasTones = globalChord.HasTones();

//	transButton->SetEnabled(hasTones);
	clearButton->SetEnabled(hasTones);
	hearButton->SetEnabled(hasTones);
	romanButton->SetState(fretpet->romanMode);

//	undoButton->SetEnabled(fretpet->activeDocument->undoBuffer.Size());

	UpdatePlayTools();

	InvalidateWindow();
}


void FPToolbar::UpdatePlayTools() {
	FPDocWindow *wind = FPDocWindow::activeWindow;
	bool play = player->IsPlaying();
	if (wind && (wind->ReadyToPlay() || play))
		playButton->Enable();
	else {
		playButton->Disable();
		stopButton->Disable();
	}

	playButton->SetState(play);
	stopButton->SetEnabled(play);
}


void FPToolbar::DrawRootType() {
	rootButton->SetState(!fretpet->chordRootType);
	rootButton->SetEnabled(fretpet->chordRootType);

	keyButton->SetState(fretpet->chordRootType);
	keyButton->SetEnabled(!fretpet->chordRootType);
}


