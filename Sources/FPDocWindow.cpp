/*
 *  FPDocWindow.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPDocWindow.h"

#include "FPScalePalette.h"
#include "FPGuitarPalette.h"
#include "FPPreferences.h"
#include "FPBankControl.h"
#include "FPDocControls.h"
#include "FPDocument.h"
#include "FPHistory.h"
#include "FPCloneSheet.h"

#include "TControl.h"
#include "TCarbonEvent.h"
#include "TError.h"

#include "CGHelper.h"

#define BASE_CHORD_NUMBER 1

extern long systemVersion;

EventLoopTimerUPP FPDocWindow::scrollUPP	= NULL;
EventLoopTimerRef FPDocWindow::scrollLoop	= NULL;

bool			FPDocWindow::closingAllDocuments	= false;
SInt32			FPDocWindow::untitledCount	= 0;
FPDocWindowList	FPDocWindow::docWindowList;
FPDocWindow*	FPDocWindow::activeWindow	= NULL;
FPDocWindow*	FPDocWindow::frontWindow	= NULL;

GWorldPtr		FPDocWindow::chordCursGW	= NULL;
GWorldPtr		FPDocWindow::chordSelGW		= NULL;
GWorldPtr		FPDocWindow::chordGrayGW	= NULL;
GWorldPtr		FPDocWindow::overlayCursGW	= NULL;
GWorldPtr		FPDocWindow::overlaySelGW	= NULL;
GWorldPtr		FPDocWindow::overlayGrayGW	= NULL;
GWorldPtr		FPDocWindow::itemDrawGW		= NULL;
GWorldPtr		FPDocWindow::headingGW		= NULL;
GWorldPtr		FPDocWindow::headDrawGW		= NULL;

GWorldPtr		FPDocWindow::repStretchGW1	= NULL;
GWorldPtr		FPDocWindow::repTopGW1		= NULL;
GWorldPtr		FPDocWindow::repStretchGW2	= NULL;
GWorldPtr		FPDocWindow::repTopGW2		= NULL;

Rect			FPDocWindow::tabOnRect		= { 0, 0, WTABV, WTABH };
Rect			FPDocWindow::itemRect		= { 0, 0, kLineHeight, CHORDW };
Rect			FPDocWindow::headRect		= { 0, 0, kTabHeight, kDocWidth };
Rect			FPDocWindow::numRect		= { kLineHeight / 2 - 9, 8, kLineHeight / 2 + 9, 8 + 24 };

// Sequencer parts
CGRect			FPDocWindow::circRect		= { { CIRCX, CIRCY }, { CIRC, CIRC } };
Rect			FPDocWindow::repeatRect		= { REPTY, REPTX, REPTY+REPTV, REPTX+REPTH };
Rect			FPDocWindow::lockRect		= { LOCKY, LOCKX, LOCKY+LOCKV, LOCKX+LOCKH };
Rect			FPDocWindow::patternRect	= { SEQY, SEQX, SEQY+SEQVV, SEQX+SEQHH };
Rect			FPDocWindow::lengthRect		= { SEQY, SEQX, SEQY+SEQVV, SEQX+SEQH };
Rect			FPDocWindow::overRect1		= { 0, 0, OVERV, OVERH };
Rect			FPDocWindow::overRect2		= { OVERY, OVERX, OVERY+OVERV, OVERX+OVERH };

ScrollInfo	scrollInfo;
int			entered = 0;

#pragma mark -
FPDocWindow::FPDocWindow(CFStringRef windowTitle) : FPWindow(CFSTR("Document")) {
	Init();

	document = new FPDocument(this);
	document->SetDocumentInfo(Window(), CFSTR(".fret"), windowTitle, CREATOR_CODE, BANK_FILETYPE);
	InitWindowAssets();
}


FPDocWindow::FPDocWindow(const FSSpec &fileSpec) : FPWindow(CFSTR("Document")) {
	Init();

	document = new FPDocument(this, fileSpec);
	document->SetDocumentInfo(Window(), CFSTR(".fret"), document->BaseName(), CREATOR_CODE, BANK_FILETYPE);
	InitWindowAssets();
}


FPDocWindow::FPDocWindow(const FSRef &fileRef) : FPWindow(CFSTR("Document")) {
	Init();
	(void) InitFromFile(fileRef);
}


FPDocWindow::~FPDocWindow() {
	delete sequencerControl;
	delete tabsControl;
	delete docPopControl;
	delete tempoControl;
	delete velocityControl;
	delete sustainControl;
	delete doubleTempoButton;
	delete scrollControl;
	delete tempoRollover;
	delete velocityRollover;
	delete sustainRollover;
	delete blockedRollover;
	delete repeatRollover;
	delete sequencerRollover;
	delete lengthRollover;

	DisposeControlActionUPP(liveScrollUPP);

	if (activeWindow == this)
		activeWindow = NULL;

	if (frontWindow == this)
		frontWindow = NULL;

	if (document)
		delete document;

	docWindowList.remove(this);

	delete history;
}


FPDocWindow* FPDocWindow::NewUntitled() {
	TString titleString = CFSTR("untitled");
	titleString.Localize();

	if (untitledCount > 0) {
		titleString += CFSTR(" ");
		titleString += untitledCount;
	}
	untitledCount++;

	bool isFirst = FPDocWindow::docWindowList.empty();
	FPDocWindow *newWindow = new FPDocWindow(titleString.GetCFStringRef());

	if (isFirst)
		newWindow->LoadPreferences();

	newWindow->Show();

	return newWindow;
}


//-----------------------------------------------
//
//	InitFromFile
//
//	After a hidden document window has been created
//	call this method to initialize it with file info.
//	If this fails the window should be destroyed.
//
OSStatus FPDocWindow::InitFromFile(const FSRef &fileRef) {
	document = new FPDocument(this);
	OSStatus err = document->InitFromFile(fileRef);
	nrequire(err, FailGetFile);

	document->SetDocumentInfo(Window(), CFSTR(".fret"), document->BaseName(), CREATOR_CODE, BANK_FILETYPE);
	InitWindowAssets();

FailGetFile:
	return err;
}


void FPDocWindow::Init() {
	didFirstResize	= systemVersion < 0x1070 || systemVersion >= 0x1080;
	lastBeatDrawn	= -1;
	lastChordDrawn	= -1;
	document		= NULL;
	currentSize		= GetContentSize();
	isClosing		= false;
	cloneSheet		= NULL;
	SetModified(false);

	history			= new FPHistory(this);

	docWindowList.push_back(this);

//	EnableDragReceive();
//	EnableDragTracking();

//	SetAutomaticControlDragTrackingEnabled(true);
}


void FPDocWindow::InitWindowAssets() {
	//
	// Set the title from the document
	//
	SetTitle(document->DisplayName());
	document->UpdateProxyIcon();

	OSErr err;

	//
	// Register to receive essential events
	//
	const EventTypeSpec	docEvents[] = {
		{ kEventClassWindow, kEventWindowBoundsChanging },
		{ kEventClassWindow, kEventWindowGetIdealSize },
		{ kEventClassWindow, kEventWindowClickProxyIconRgn },

		{ kEventClassCommand, kEventCommandUpdateStatus },
		{ kEventClassCommand, kEventCommandProcess },

		{ kEventClassKeyboard, kEventRawKeyDown },
		{ kEventClassKeyboard, kEventRawKeyRepeat },
		{ kEventClassKeyboard, kEventRawKeyModifiersChanged }
	};

	RegisterForEvents( GetEventTypeCount( docEvents ), docEvents );

	//
	// Create offscreen graphics for later
	//
	if (chordCursGW == NULL) {
		repStretchGW1	= MakeGWorldWithPictResource(kPictStretch1);
		repTopGW1		= MakeGWorldWithPictResource(kPictTop1);
		repStretchGW2	= MakeGWorldWithPictResource(kPictStretch2);
		repTopGW2		= MakeGWorldWithPictResource(kPictTop2);

		chordGrayGW		= MakeGWorldWithPictResource(kPictChordGray, 32);
		chordSelGW		= MakeGWorldWithPictResource(kPictChordSel, 32);
		chordCursGW		= MakeGWorldWithPictResource(kPictChordCurs, 32);
		headingGW		= MakeGWorldWithPictResource(kPictDocHeading, 32);

		overlayGrayGW	= MakeGWorldWithPictResource(kPictOverlayGray, 32);
		overlaySelGW	= MakeGWorldWithPictResource(kPictOverlaySel, 32);
		overlayCursGW	= MakeGWorldWithPictResource(kPictOverlayCurs, 32);

		err = QTNewGWorld(&itemDrawGW, 32, &itemRect, NULL, NULL, 0);
		verify_action(err==noErr, throw TError(err, CFSTR("Unable to create a GWorld.")) );
		LockPixels(GetGWorldPixMap(itemDrawGW));

		err = QTNewGWorld(&headDrawGW, 16, &headRect, NULL, NULL, 0);
		verify_action(err==noErr, throw TError(err, CFSTR("Unable to create a GWorld.")) );
		LockPixels(GetGWorldPixMap(headDrawGW));

		//
		// Draw tab controls in the current theme
		//
		OSStatus	err;
		FontInfo	font;
		GWorldPtr	saveGWorld;
		GDHandle	saveGDevice;
		GetGWorld(&saveGWorld, &saveGDevice);
		SetGWorld(headingGW, saveGDevice);
		LockPixels(GetGWorldPixMap(headingGW));
		ForeColor(blackColor);
		SetFont(&font, (StringPtr)"\pLucida Grande", 10, bold);

		for (int i=DOC_PARTS; i--;) {
			unsigned char	numstr[] = { 1, '1' + i };
			short			x = WTABX + WTABD * i;
			Rect			rRect = tabOnRect;
			OffsetRect(&rRect, x, 2);

			err = DrawThemeTab(&rRect, kThemeTabFrontUnavailable, kThemeTabNorth, NULL, 0);

			MoveTo(x + WTABH / 2 - StringWidth(numstr) / 2, 15);
			DrawString(numstr);
		}

		SetGWorld(saveGWorld, saveGDevice);
		UnlockPixels(GetGWorldPixMap(headingGW));
	}

	//
	// Sequencer Control covers the whole document
	//
	sequencerControl = new FPBankControl(Window());

	//
	// Tabs control stretches across the top of the window
	//
	tabsControl = new FPTabsControl(Window());

	//
	// Instrument popup
	//
	docPopControl = new FPDocPopup(Window());

	//
	// Tempo, Velocity, and Sustain sliders
	//
	tempoControl	= new FPTempoSlider(Window());
	velocityControl	= new FPVelocitySlider(Window());
	sustainControl	= new FPSustainSlider(Window());

	//
	// Tracking regions for rollover effects
	//
	blockedRollover		= NewTrackingRegion(itemRect);

	Rect bounds = tempoControl->Bounds();
	tempoRollover		= NewTrackingRegion(bounds);

	bounds = velocityControl->Bounds();
	velocityRollover	= NewTrackingRegion(bounds);

	bounds = sustainControl->Bounds();
	sustainRollover		= NewTrackingRegion(bounds);

	repeatRollover		= NewTrackingRegion(repeatRect);
	sequencerRollover	= NewTrackingRegion(patternRect);
	lengthRollover		= NewTrackingRegion(lengthRect);

	//
	// This button needs to be prettier
	//
	doubleTempoButton = new FPTempoDoubler(Window());

	//
	// Prepare the scrollbar
	//
	scrollControl = new TControl(Window(), 'vbar', 0);
	scrollbar = scrollControl->Control();
	SetControlReference(scrollbar, (SInt32)this);
	liveScrollUPP = NewControlActionUPP(LiveScrollProc);
	SetControlAction(scrollbar, liveScrollUPP);

	//
	// Update the scrollbar for the new window
	//
	UpdateScrollState();

	//
	// Initialize the control values
	//
	RefreshControls();

	//
	// Position the window
	//
	LoadPreferences(false);
	Cascade(kDocCascadeX, kDocCascadeY);
}


void FPDocWindow::SetScale(UInt16 newMode) {
	document->SetScale(newMode);
	InvalidateWindow();
}


void FPDocWindow::Cascade(SInt16 h, SInt16 v) {
	if (activeWindow) {
		Rect bounds = activeWindow->Bounds();
		Move( bounds.left + h, bounds.top + v );
	}
}


#pragma mark -
void FPDocWindow::TryToClose(bool inCloseAll) {

	if (inCloseAll) closingAllDocuments = true;

	TerminateNavDialogs();

	//
	// If the window is dirty then put up a save sheet
	// and do Save/SaveAs, Discard, or Cancel
	//
	bool closeNow = false;
	isClosing = true;
	if (!fretpet->discardingAll && IsDirty()) {
#if KAGI_SUPPORT || DEMO_ONLY
		if (fretpet->AuthorizerIsAuthorized()) {
#endif

#if !DEMO_ONLY
			if (noErr != document->AskSaveChanges(fretpet->isQuitting))
				isClosing = false;
#else
		if (fretpet->AuthorizerRunDiscardDialog())
			closeNow = true;
		else
			NavCancel();
#endif

#if KAGI_SUPPORT || DEMO_ONLY
		}
		else if (fretpet->AuthorizerRunDiscardDialog())
			closeNow = true;
		else
			NavCancel();
#endif
	}
	else
		closeNow = true;

	if (closeNow) {
		Close();

		if (docWindowList.empty())
			fretpet->discardingAll = closingAllDocuments = false;
	}
}


void FPDocWindow::Close() {
	if (player->IsPlaying(document))
		fretpet->DoStop();

	SavePreferences();

	FPWindow::Close();

	if (closingAllDocuments) {
		FPDocWindow *wind = frontWindow;
		if (wind)
			wind->TryToClose();
		else if (fretpet->isQuitting)
			fretpet->DoQuitNow();
	}
}


//-----------------------------------------------
//
// DoCloseAll
//
// TODO: Move this to the application class and
//	have all windows initiate close simultaneously.
//	This will emulate the behavior of TextEdit.
//
void FPDocWindow::DoCloseAll() {
	fretpet->TerminateOpenFileDialog();
	CancelSheets();

	NavUserAction action;
	fretpet->AskReviewChangesDialog(&action, false);

	switch (action) {
		case kNavUserActionReviewDocuments:
			TryToClose(true);
			break;

		case kNavUserActionDiscardDocuments:
			fretpet->discardingAll = true;
			TryToClose(true);
			break;
	}
}

bool FPDocWindow::RunDialogsModal() {
	return preferences.GetBoolean(kPrefModalDialogs, TRUE);
}

void FPDocWindow::NavCancel() {
	isClosing =
	closingAllDocuments =
	fretpet->isQuitting =
	fretpet->discardingAll = false;
}

void FPDocWindow::NavDiscardChanges() {
	document->DeleteAll();
	document->InitFromFile();
	SetModified(false);
	history->Clear();
	fretpet->DoWindowActivated(this);
	InvalidateWindow();
	UpdateScrollState();
	RefreshControls();
}


#if !DEMO_ONLY

//-----------------------------------------------
//
// DoSaveAs
//
// This shows a save dialog in the document window.
// The dialog will call the save function itself if the
// user clicks "Save." If the window was closing it will
// be closed automatically.
//
OSStatus FPDocWindow::DoSaveAs() {
	return document->SaveAs();
}

void FPDocWindow::NavDidSave() {
	if (isClosing)
		Close();
	else {
		SetModified(false);
		history->Clear();
		SetTitle(document->DisplayName());
		document->UpdateProxyIcon();
		fretpet->AddToRecentItems((TFile&)*document);
	}
}

#endif

void FPDocWindow::HandleNavError(OSStatus err) {
	if (err != kErrorHandled) {
		if (err == fBsyErr)
			AlertError(err, CFSTR("The file could not be overwritten because it is in use."));
		else
			AlertError(err, CFSTR("The file could not be saved due to an error."));
	}
}


void FPDocWindow::TerminateNavDialogs() {
	document->TerminateNavDialogs();
}

bool FPDocWindow::AskTuningChange() {
	DialogItemIndex item = kStdOkItemIndex;
	if (preferences.GetBoolean(kPrefFingerWarning, TRUE) && document->HasFingering()) {
		 item = RunPrompt(
			CFSTR("Tuning Change"),
			CFSTR("Changing the tuning will cause all chord fingerings in the document to be recalculated. Are you sure you want to proceed?"),
			CFSTR("Change Tuning"),
			CFSTR("Cancel"),
			NULL
		);
	}
	return (item == kStdOkItemIndex);
}


#pragma mark -
void FPDocWindow::DoCopy() {
	document->CopySelectionToClipboard();
}


void FPDocWindow::DoCut() {
	DoCopy();

	FPHistoryEvent *event = fretpet->StartUndo(UN_CUT, CFSTR("Cut Chords"));
	event->SaveSelectionBefore();

	DeleteSelection();

	event->Commit();
}


void FPDocWindow::DoPaste() {
	FPClipboard	&clip = fretpet->clipboard;
	if (clip.Size()) {
		FPHistoryEvent *event = fretpet->StartUndo(UN_PASTE, CFSTR("Paste Chords"));

		if (HasSelection())
			event->SaveSelectionBefore();

		event->SaveClipboardAfter();

		DoReplace(clip.Contents());

		event->Commit();
	}
}


void FPDocWindow::DoReplace(FPChordGroupArray &groupArray, bool undoable, bool replace) {
	ChordIndex startDel, endDel;
	(void)document->GetSelection(&startDel, &endDel);
	if ( (replace || document->HasSelection()) && startDel <= endDel ) {
		bool atEnd = (endDel == document->Size() - 1);
		DeleteSelection(undoable, true);
		ChordIndex curs = GetCursor();
		if (curs > -1 && !atEnd) curs--;
		document->SetCursorLine(curs);
	}

	Insert(groupArray);
}


//-----------------------------------------------
//
// DoPasteTones
//
//	Paste the tones from the clipboard into the
//	selected chords, repeating if necessary
//
void FPDocWindow::DoPasteTones() {
	if (fretpet->clipboard.Size() > 0) {
		UInt16 clipPart = fretpet->clipboard.Part();
		FPHistoryEvent	*event = history->UndoStart(UN_PASTE_TONES, CFSTR("Paste Tones"), kAllChannelsMask);
		event->SaveSelectionBefore();
		event->SaveClipboardAfter();
		event->SaveDataBefore(CFSTR("clipPart"), clipPart);

		DoPasteTones(fretpet->clipboard.Contents(), fretpet->clipboard.Part());

		event->Commit();
	}
}


//-----------------------------------------------
//
// DoPasteTones
//
//	Paste the tones, fretboard, and scale info from a
//	chord array into the selected chords, repeating as
//	necessary
//
void FPDocWindow::DoPasteTones(FPChordGroupArray &clipSource, UInt16 part) {
	ChordIndex	start, end;
	if (!clipSource.empty() && document->GetSelection(&start, &end)) {
		ChordIndex srcIndex = 0;
		for (ChordIndex dstIndex = start; dstIndex <= end; dstIndex++) {
			FPChord &src = clipSource[srcIndex][part];
			FPChord	&dst = document->Chord(dstIndex);

			dst.Set(src.tones, src.key, src.root);

			dst.rootLock		= src.rootLock;
			dst.rootModifier	= src.rootModifier;
			dst.rootScaleStep	= src.rootScaleStep;
			dst.bracketFlag		= src.bracketFlag;
			dst.brakLow			= src.brakLow;
			dst.brakHi			= src.brakHi;

			bcopy(src.fretHeld, dst.fretHeld, sizeof(src.fretHeld));

			if (++srcIndex >= (ChordIndex)clipSource.size())
				srcIndex = 0;
		}

		UpdateGlobalChord();
		fretpet->UpdatePlayTools();
		DrawVisibleLines();
	}
}


//-----------------------------------------------
//
// ReplaceSelection
//
//	Copy entire chords from a source array over
//	the selected chords (in one or more parts)
//
//	This is ONLY used by Undo/Redo,
//	so it never dirties the document
//
void FPDocWindow::ReplaceSelection(FPChordGroupArray &src, UInt16 partMask) {
	ChordIndex	start, end;
	document->GetSelection(&start, &end);
	ReplaceRange(src, start, end, partMask);
}


//-----------------------------------------------
//
// ReplaceAll
//
//	Copy entire chords from a source array over
//	all chords (in one or more parts)
//
//	This is ONLY used by Undo/Redo,
//	so it never dirties the document
//
void FPDocWindow::ReplaceAll(FPChordGroupArray &src, UInt16 partMask) {
	ReplaceRange(src, 0, document->Size() - 1, partMask);
}


//-----------------------------------------------
//
// ReplaceRange
//
//	Copy entire chords from a source array over
//	a range of chords (in one or more parts)
//
//	This is ONLY used by Undo/Redo,
//	so it never dirties the document
//
void FPDocWindow::ReplaceRange(FPChordGroupArray &src, ChordIndex start, ChordIndex end, UInt16 partMask) {
	if (!src.empty() && end >= start) {
		ChordIndex srcIndex = 0;
		for (ChordIndex dstIndex = start; dstIndex <= end; dstIndex++) {
			for (PartIndex part=DOC_PARTS; part--;)
				if ((partMask & BIT(part)) != 0)
					document->Chord(dstIndex, part) = src[srcIndex][part];

			if (++srcIndex >= (ChordIndex)src.size())
				srcIndex = 0;
		}

		UpdateGlobalChord();
		fretpet->UpdatePlayTools();
		DrawVisibleLines();
	}
}


//-----------------------------------------------
//
// DoPastePattern
//
//	Paste the sequence from the clipboard into the
//	selected chords, repeating if necessary
//
void FPDocWindow::DoPastePattern() {
	if (fretpet->clipboard.Size() > 0) {
		UInt16 clipPart = fretpet->clipboard.Part();
		FPHistoryEvent *event = history->UndoStart(UN_PASTE_PATT, CFSTR("Paste Pattern"), kAllChannelsMask);
		event->SaveSelectionBefore();
		event->SaveClipboardAfter();
		event->SaveDataBefore(CFSTR("clipPart"), clipPart);

		DoPastePattern(fretpet->clipboard.Contents(), clipPart);

		event->Commit();
	}
}


//-----------------------------------------------
//
// DoPastePattern
//
//	Paste the sequence from an array into the
//	selected chords, repeating if necessary
//
void FPDocWindow::DoPastePattern(FPChordGroupArray &clipSource, UInt16 part) {
	ChordIndex	start, end;
	if (!clipSource.empty() && document->GetSelection(&start, &end)) {
		ChordIndex srcIndex = 0;
		for (ChordIndex dstIndex = start; dstIndex <= end; dstIndex++) {
			FPChordGroup	&srcGrp = clipSource[srcIndex];
			FPChordGroup	&dstGrp = document->ChordGroup(dstIndex);
			FPChord &src = srcGrp[part];
			FPChord &dst = dstGrp[CurrentPart()];

			dstGrp.SetPatternSize(srcGrp.PatternSize());
			dstGrp.SetRepeat(srcGrp.Repeat());
			dst.SetPattern(src);

			if (++srcIndex >= (ChordIndex)clipSource.size())
				srcIndex = 0;
		}

		UpdateGlobalChord();
		fretpet->UpdatePlayTools();
		DrawVisibleLines();
	}
}


void FPDocWindow::DoToggleTempoMultiplier(bool undoable) {
	bool dbl = !(document->TempoMultiplier() == 2);

	if (undoable) {
		FPHistoryEvent *event = fretpet->StartUndo(UN_TEMPO_X, dbl ? CFSTR("Set Tempo 2X") : CFSTR("Set Tempo 1X"));
		event->SetPartAgnostic();
		event->Commit();
	}

	document->SetTempoMultiplier(dbl);
	doubleTempoButton->SetState(dbl);
//	tempoControl->SetSnap(dbl ? 40 : 20);
	tempoControl->DrawNow();
}


#pragma mark -
#pragma mark Scrolling
pascal void FPDocWindow::LiveScrollProc(ControlHandle control, SInt16 part) {
	FPDocWindow *myDocument = (FPDocWindow*)GetControlReference(control);
	myDocument->LiveScroll(control, part);
}


void FPDocWindow::LiveScroll(ControlHandle control, SInt16 part) {
	SInt32	min, max, delta;
	SInt32	currentValue = GetControl32BitValue(control);
	UInt16	visi = VisibleLines();

	min = GetControl32BitMinimum(control);
	max = GetControl32BitMaximum(control);
	delta = 0;

	switch (part) {
		case kControlUpButtonPart:
			if (currentValue > min)
				delta = (currentValue - min < 1) ? -(currentValue - min) : -1;
			break;

		case kControlDownButtonPart:
			if (currentValue < max)
				delta = (max - currentValue > 1) ? 1 : max - currentValue;
			break;

		case kControlPageUpPart:
			if (currentValue > min)
				delta = (currentValue - min > visi - 1) ? -(visi - 1) : -(currentValue - min);
			break;

		case kControlPageDownPart:
			if (currentValue < max)
				delta = (visi - 1 < max - currentValue) ? visi - 1 : max - currentValue;
			break;

		case kControlIndicatorPart:
			delta = currentValue - document->TopLine();
			currentValue -= delta;
			break;
	}

	if (delta) {
		currentValue += delta;
		SetControl32BitValue(control, currentValue);
		document->SetTopLine(currentValue);
		ScrollLines(0, -delta, false);
		UpdateScrollState();
	}
}


void FPDocWindow::UpdateScrollState() {
	ChordIndex	topline = document->TopLine();
	ChordIndex	botmost = DocumentSize() - VisibleLines();		// Will be 0 even if lines above the top
	if (botmost <= 0 || botmost < topline)
		botmost = topline;

	scrollControl->SetMinimum(0);
	scrollControl->SetMaximum(botmost);
	scrollControl->SetViewSize(VisibleLines());
	scrollControl->SetValue(topline);

	UpdateTrackingRegions();
}


bool FPDocWindow::ScrollToReveal(ChordIndex line) {
	ChordIndex	topLine = TopLine();
	ChordIndex	botLine = BottomLine();
	ChordIndex	out = 0;

	if (line < topLine)
		out = line - topLine;
	else if (line > botLine && line < DocumentSize())
		out = line - botLine;

	if (out) {
		topLine += out;
		if (topLine < 0) topLine = 0;
		document->SetTopLine(topLine);

		if (topLine != BottomLine())
			ScrollLines(0, -out, false);

		UpdateScrollState();
	}

	return (out != 0);
}


//-----------------------------------------------
//
//	ScrollLines
//
//	Scroll the lines in a document
//
//	If distance is negative it scrolls the given line up.
//	If distance is positive it scrolls the given line down.
//
void FPDocWindow::ScrollLines(ChordIndex inTop, ChordIndex distance, bool bFix) {
	ChordIndex	botLine = BottomLine();

	if (distance > 0 && inTop > botLine)
		return;

	ChordIndex	topLine = TopLine();
	ChordIndex	count = DocumentSize();

	ChordIndex newTop = inTop;
	if (distance < 0)
		newTop += distance;

	if (newTop < topLine)
		newTop = topLine;

    Focus();

	/*
	 *	This breaks on Lion and Mountain Lion for some scrolls
	 *	Items are drawn with white bands on the left and right,
	 *	giving the impression that WindowScroll method is
	 *	not the right way to go.
	 *
	 */
	RgnHandle oldClip = NewRgn(); GetClip(oldClip);
	
	Rect theRect = GetContentSize();
	theRect.right -= kScrollbarSize;
	theRect.top = kTabHeight + (newTop - topLine) * kLineHeight;
	ClipRect(&theRect);

	if (false && systemVersion >= 0x1070) {
		// So for 10.7 and up on your fancy hardware
		// we just lazily redraw everything
		DrawVisibleLines();
	}
	else
	{
		(void)ScrollWindowRect(Window(), &theRect, 0, distance * kLineHeight, kScrollWindowInvalidate, NULL );

		//  CGContextRef gc = FocusContext();
		//	CGRect cgRect = CGBounds(), origRect = cgRect;
		//	cgRect.size.width -= kScrollbarSize;
		//	cgRect.origin.y = kTabHeight + (newTop - topLine) * kLineHeight;
		//	CGContextClipToRect(gc, cgRect);
		//      // SCROLL THE WINDOW CONTENTS HERE
		//	CGContextClipToRect(gc, origRect);	// presume no clip
		
		//
		// Renumber scrolled visible items
		//
		if (bFix && count) {
			ChordIndex drawTop = inTop + distance;
			if (drawTop < topLine) drawTop = topLine;
			
			ChordIndex drawBot = botLine;
			if (drawBot > count-1) drawBot = count-1;
			
			if (drawTop < count && drawTop <= drawBot) {
				for (ChordIndex i=drawTop; i<=drawBot; i++)
					if (!DrawLineNumber(i))
						break;
			}
		}

		//
		// Draw the exposed area
		//
		ChordIndex topExp, botExp;
		if (distance < 0) {
			topExp = botLine + distance + 1;
			botExp = botLine;
		}
		else {
			if (inTop < topLine)
				inTop = topLine;
			
			topExp = inTop;
			botExp = inTop + distance - 1;
		}

		if (topExp < topLine)	topExp = topLine;
		if (botExp > botLine)	botExp = botLine;

		if (botExp >= topExp) {
			ChordIndex line = topExp;
			//		BeginUpdate(Window());
			while (line <= botExp && DrawLine(line)) { line++; }
			while (line <= botExp && EraseLine(line)) { line++; }
			//		EndUpdate(Window());
		}

		//	EndContext();
	}

	SetClip(oldClip);
	DisposeRgn(oldClip);

//	DRAW(B_DOCHELP);
}


#pragma mark -

void FPDocWindow::StartScrollTimer(int distance) {
	scrollInfo.distance = distance;

	if (scrollLoop == NULL) {
		scrollInfo.target = this;
		scrollInfo.counter = 100;
		scrollInfo.keepSelection = !(fretpet->IsFollowViewEnabled() && player->IsPlaying());

		scrollUPP = NewEventLoopTimerUPP(FPDocWindow::ScrollTimerProc);

		(void)InstallEventLoopTimer(
			GetMainEventLoop(),
			0,
			1.0 / 10.0,
			scrollUPP,
			&scrollInfo,
			&scrollLoop
			);
	}
}


void FPDocWindow::DisposeScrollTimer() {
	if (scrollLoop != NULL) {
		(void)RemoveEventLoopTimer(scrollLoop);
		DisposeEventLoopTimerUPP(scrollUPP);
		scrollLoop = NULL;
		scrollUPP = NULL;
	}
}


void FPDocWindow::ScrollTimerProc( EventLoopTimerRef timer, void *info ) {
	((ScrollInfo*)info)->target->DoScrollTimer((ScrollInfo*)info);
}


void FPDocWindow::DoScrollTimer(ScrollInfo *info) {
	info->counter += ABS(info->distance);
	if (info->counter > 50) {
		info->counter = 0;

		ChordIndex oldLine = GetCursor();
		ChordIndex line = (info->distance < 0) ? MAX(0, TopLine() - 1) : MIN(DocumentSize() - 1, BottomLine() + 1);

		if (line != oldLine)
			MoveCursor(line, info->keepSelection);
	}
}


#pragma mark -
#pragma mark Drawing
void FPDocWindow::RefreshControls() {
	UpdateDocPopup();
	UpdateTempoSlider();
	UpdateVelocitySlider();
	UpdateSustainSlider();
	UpdateDoubleTempoButton();
}

void FPDocWindow::UpdateDocPopup()			{ docPopControl->DrawNow(); }
void FPDocWindow::UpdateTempoSlider()		{ tempoControl->SetValue(document->Tempo()); }
void FPDocWindow::UpdateDoubleTempoButton()	{ doubleTempoButton->SetState(document->TempoMultiplier() == 2); }
void FPDocWindow::UpdateVelocitySlider()	{ velocityControl->SetValue(document->Velocity()); }
void FPDocWindow::UpdateSustainSlider()		{ sustainControl->SetValue(document->Sustain()); }


//-----------------------------------------------
//
// UpdateAll
//	Class method to update all documents
//
void FPDocWindow::UpdateAll() {
	for (FPDocumentIterator itr = docWindowList.begin(); itr != docWindowList.end(); itr++)
		(*itr)->InvalidateWindow();
}


//-----------------------------------------------
//
// Draw
//
// This is called any time the window resizes or a
// part of the window is invalidated.
//
void FPDocWindow::Draw() {
	DrawHeading();
	DrawVisibleLines();
}


//-----------------------------------------------
//
// DrawVisibleLines
//
// Refresh usually for first time display
//
void FPDocWindow::DrawVisibleLines() {

	Rect size = GetContentSize();

	//	printf("DrawVisibleLines() with size (%d x %d)\n", size.right, size.bottom);

	long line = TopLine(), bot = BottomLine();
	
	if (DocumentSize())
		while (line <= bot && DrawLine(line)) { line++; }

	while (line <= bot && EraseLine(line)) { line++; }
}


//-----------------------------------------------
//
//	DrawHeading
//	Draw the document heading.
//
void FPDocWindow::DrawHeading() {
	OSStatus err;

//	GrafPtr	oldPort; GetPort(&oldPort);

	SetGWorld(headDrawGW, NULL);

	PixMapHandle headPixMap = GetGWorldPixMap(headDrawGW);
	LockPixels(headPixMap);

	CopyBits(	GetPortBitMapForCopyBits(headingGW),
				GetPortBitMapForCopyBits(headDrawGW),
				&headRect, &headRect, srcCopy, NULL);

	//
	// Draw the active tab
	//
	Rect rRect = tabOnRect;
	short x = WTABX + WTABD * CurrentPart();
	OffsetRect(&rRect, x, 1);

	err = DrawThemeTab(&rRect, IsActive() ? kThemeTabFront : kThemeTabFrontInactive, kThemeTabNorth, NULL, 0);

	FontInfo font;
	unsigned char numstr[] = { 1, '1' + CurrentPart() };
	SetFont(&font, (StringPtr)"\pLucida Grande", 10, bold);
	MoveTo(x + WTABH / 2 - StringWidth(numstr) / 2, 14);
	ForeColor(blackColor);
	DrawString(numstr);


	//
	// Copy to the window
	//
	Focus();

	CopyBits(	GetPortBitMapForCopyBits(headDrawGW),
				GetPortBitMapForCopyBits(Port()),
				&headRect, &headRect, srcCopy, NULL);

	UnlockPixels(headPixMap);
//	SetPort(oldPort);

	DrawControls(Window());
}


void FPDocWindow::UpdateSelection() {
	ChordIndex min, max;
	document->GetSelection(&min, &max);
	DrawLines(min, max);
}


void FPDocWindow::DrawLines(ChordIndex min, ChordIndex max) {
	if (Size()) {
		ChordIndex top = TopLine(), bot = BottomLine();

		if (min < top) min = top;
		if (max > bot) max = bot;
		if (max >= min)
			for (ChordIndex line=min; line<=max; line++)
				DrawLine(line);
	}
}


void FPDocWindow::UpdateLine(ChordIndex line) {
	if (DocumentSize()) {
		if (line == -1)
			line = document->GetCursor();

		(void)DrawLine(line);
	}
}


//-----------------------------------------------
//
//	DrawLine
//
//	Draw a Line in a document Window.
//	- Returns false if nothing was drawn.
//
bool FPDocWindow::DrawLine(ChordIndex line) {
	SInt16			x, y, yy, ty, ly, i, xm;

	yy = kTabHeight + (line - document->TopLine()) * kLineHeight;

	// IN RANGE?
	if (yy < 0 || yy > currentSize.bottom || line >= DocumentSize())
		return false;

	//
	// Focus on window first
	//
//	GrafPtr oldPort; GetPort(&oldPort);
	Focus();

	//
	// Save GWorld  of window and switch to offscreen GWorld
	//
	GWorldPtr		saveGWorld;
	GDHandle		saveGDevice;
	GetGWorld( &saveGWorld, &saveGDevice );
	SetGWorld(itemDrawGW, saveGDevice);

	// Get a copy of the chord
	FPChordGroup	&group = document->ChordGroup(line);
	FPChord			&chord(group[CurrentPart()]);

	bool		hilited, liteGreen = false;
	ChordIndex	curs = document->GetCursor();
	ChordIndex	t = document->SelectionEnd();
	if (t >= 0) {
		hilited = (t < curs) ? (line >= t && line <= curs) : (line >= curs && line <= t);
		liteGreen = hilited && !(line == curs);
	}
	else
		hilited = (line == curs);

	//
	// Get the appropriate starting image
	//
	GWorldPtr		imageGW = hilited ? (line == curs ? chordCursGW : chordSelGW) : chordGrayGW;
	PixMapHandle	itemPixMap = GetGWorldPixMap(itemDrawGW);
	LockPixels(itemPixMap);

	CopyBits(	GetPortBitMapForCopyBits(imageGW),
				GetPortBitMapForCopyBits(itemDrawGW),
				&itemRect, &itemRect, srcCopy, NULL);

	//
	// CHORD NUMBER
	//

//	bool		bInRange = player->LineInPlayRange(line);
	Str15		number;
	FontInfo	font;
	NumToString(line + BASE_CHORD_NUMBER, number);
	SetFont(&font, (StringPtr)"\pLucida Grande", 10, /* bInRange ? bold : */ normal);
	MoveTo(21 - (StringWidth(number) / 2), (font.ascent + kLineHeight) / 2);
	ForeColor( /* bInRange ? whiteColor : */ blackColor);
	DrawString(number);

	//
	// DRAW REPEAT SLIDER & VALUE
	//
	NumToString(chord.Repeat(), number);
	SetFont(NULL, (StringPtr)"\pGeneva", 9, normal);
	MoveTo(REPTX + (REPTH/2) - StringWidth(number) / 2, REPTY + REPTV + 14);
	ForeColor(blackColor);
	DrawString(number);

	Rect rRect = { 0, 0, 16, 9 };
	Rect aRect = repeatRect;				// destination rectangle
	aRect.top = REPTY + REPTV - chord.Repeat() * 2;

	GWorldPtr		theGW = (line == curs) ? repStretchGW2 : repStretchGW1;
	PixMapHandle	pix = GetGWorldPixMap(theGW);
	LockPixels(pix);
	CopyBits(	GetPortBitMapForCopyBits(theGW),
				GetPortBitMapForCopyBits(itemDrawGW),
				&rRect, &aRect, srcCopy, NULL);
	UnlockPixels(pix);


	//
	// DRAW CHORD AS A CIRCLE
	//
	if (chord.tones) {
        CGContextRef gc;
        (void)QDBeginCGContext(itemDrawGW, &gc);
		DrawCircle(gc, chord.tones, chord.root, chord.key);
    	QDEndCGContext(itemDrawGW, &gc);
    }


	//
	// Get the Chord Name
	//
	Str255 aChord, aExt, aMiss;
	chord.ChordName(aChord, aExt, aMiss);

	//
	// Prepare drawing elements
	//
	ty = 16;
	SetFont(&font, (StringPtr)"\pLucida Grande", 11, bold);

	//
	// Draw the key if it's roman mode
	//
	if (fretpet->romanMode) {
		ForeColor(blackColor);

		// Draw an overlay
		GWorldPtr	srcGW = hilited ? (liteGreen ? overlaySelGW : overlayCursGW) : overlayGrayGW;
		pix = GetGWorldPixMap(srcGW);
		LockPixels(pix);
		CopyBits(	GetPortBitMapForCopyBits(srcGW),
					GetPortBitMapForCopyBits(itemDrawGW),
					&overRect1, &overRect2, srcCopy, NULL);
		UnlockPixels(pix);

		// Draw the key name in blue
		MoveTo(CORDL + 5, ty);
		RGBForeColor(&rgbGray4);
		c2pstrncpy(number, scalePalette->NameOfNote(chord.key, chord.key), 3);
		DrawString(number);

		x = OVERX + OVERH + 2;
	}
	else
		x = CORDL;

	//
	// Draw Chord Name
	//
	ForeColor(blackColor);
	RGBBackColor(hilited ? &rgbChord1 : &rgbChord2);

	MoveTo(x + 5, ty);
	DrawString(aChord);

	if (aExt[0]) {
		MoveTo(x + 28, ty);
		DrawString(aExt);
	}

	if (aMiss[0]) {
		MoveTo(CORDR - 14 - StringWidth(aMiss), ty);
		DrawString(aMiss);
	}


	//
	// DRAW LOCK IF CHORD IS LOCKED
	//
	if (chord.rootLock) {
		SetHVRect(&aRect, LOCKX, LOCKY + 4, 8, 6);		// Lock body
		PaintRect(&aRect);
		MoveTo(LOCKX + 1, LOCKY + 3);					// Latch Left
		LineTo(LOCKX + 1, LOCKY + 1);
		MoveTo(LOCKX + 2, LOCKY);						// Latch Top
		LineTo(LOCKX + 5, LOCKY);
		MoveTo(LOCKX + 6, LOCKY + 1);					// Latch Right
		LineTo(LOCKX + 6, LOCKY + 3);
	}

	BackColor(whiteColor);

	//
	// DRAW PICKING PATTERN
	//
 	xm = SEQX + 7 + chord.PatternSize() * SEQH;

	ForeColor(blackColor);
	MoveTo(xm, SEQTOP);
	LineTo(xm, SEQBOT);
	MoveTo(xm-1, SEQTOP);
	LineTo(xm-1, SEQBOT);

	//
	// Vertical Tick Marks
	//
	for (i=0; i<chord.PatternSize(); i++) {
		if (i % 4 < 2)
			RGBForeColor((i & 3) ? (hilited ? &rgbTickGreen1 : &rgbTickGray1) : (hilited ? &rgbTickGreen2 : &rgbTickGray2));
		MoveTo(CORDL+1 + SEQH * (i+1), SEQTOP);
		LineTo(CORDL+1 + SEQH * (i+1), SEQBOT);
	}

	y = SEQY + 4;
	ly = 11;
	SetFont(&font, (StringPtr)"\pGeneva", 9, normal);
	long color;
	bool rev = fretpet->IsTablatureReversed();
	for (int str=0; str<NUM_STRINGS; str++) {
		int string = rev ? str : NUM_STRINGS - str - 1;

		//
		// Horizontal Line
		//
		ForeColor(blackColor);
		MoveTo(CORDL + 1, y);
		LineTo(xm - 1, y);

		//
		// Fret Number
		//
		int fret = chord.fretHeld[string];
		if (fret >= 0 && chord.bracketFlag) {
			color = blueColor;
			NumToString(fret, number);
		} else {
			color = redColor;
			pstrcpy(number, (StringPtr)"\px");
		}

		MoveTo(GTAB - (StringWidth(number)/2), ly);
		ForeColor(color);
		DrawString(number);

		//
		// Picking Pattern Dots
		//
		UInt16 sbits = chord.GetPatternMask(string);
		for (int step=0; step<chord.PatternSize(); step++) {
			if (sbits & BIT(step)) {
				x = CORDL + SEQH * (step + 1);
				SetHVRect(&aRect, x - 1, y - 2, 5, 5);
				PaintOval(&aRect);
			}
		}

		ly += 9;
		y += SEQV;
	}

	//
	//	Copy the final image to the window port
	//
	ForeColor(blackColor);

	aRect = itemRect;
	OffsetRectY(aRect, yy);

	SetGWorld(saveGWorld, saveGDevice);

	CopyBits(	GetPortBitMapForCopyBits(itemDrawGW),
				GetPortBitMapForCopyBits(Port()),
				&itemRect, &aRect, srcCopy, NULL);

/*
	UInt16 beat = player->PlayingBeat();
	if (player->IsPlaying() && line == player->PlayingChord() && beat < document->Chord(line, 0).beats)
		DrawOneBeat(line, beat, true);
*/

	if (player->PlayingDocWindow() == this && player->IsPlaying() && line == player->PlayingChord())
		DrawPlayDots(true);

	UnlockPixels(itemPixMap);

	return true;
}


bool FPDocWindow::DrawLineNumber(ChordIndex line) {
	SInt16		yy;

	if (line < TopLine() || line > BottomLine() || line >= DocumentSize())
		return false;

	yy = kTabHeight + (line - TopLine()) * kLineHeight;

	//
	// Focus on window first then
	// save GWorld of window and switch to offscreen GWorld
	//
//	GrafPtr oldPort; GetPort(&oldPort);
	Focus();

	GWorldPtr		saveGWorld;
	GDHandle		saveGDevice;
	GetGWorld( &saveGWorld, &saveGDevice );

	SetGWorld(itemDrawGW, saveGDevice);

	// Get the proper chord
	bool		hilited, liteGreen = false;
	ChordIndex	t = document->SelectionEnd(),
				y = document->GetCursor();
	if (t >= 0) {
		hilited = (t < y) ? (line >= t && line <= y) : (line >= y && line <= t);
		liteGreen = hilited && !(line == y);
	}
	else
		hilited = (line == y);

	//
	// Get the appropriate starting image
	//
	GWorldPtr		imageGW = hilited ? (line == y ? chordCursGW : chordSelGW) : chordGrayGW;
	PixMapHandle	itemPixMap = GetGWorldPixMap(itemDrawGW);
	LockPixels(itemPixMap);
	CopyBits(	GetPortBitMapForCopyBits(imageGW),
				GetPortBitMapForCopyBits(itemDrawGW),
				&numRect, &numRect, srcCopy, NULL);

	//
	// Chord Number
	//
//	bool		bInRange = player->LineInPlayRange(line);
	Str15		number;
	FontInfo	font;
	NumToString(line + BASE_CHORD_NUMBER, number);
	SetFont(&font, (StringPtr)"\pMonaco", 10, normal);
	MoveTo(21 - (StringWidth(number) / 2), (font.ascent + kLineHeight) / 2);
	ForeColor(blackColor);
	DrawString(number);

	//
	// Refocus on the window port
	//
	SetGWorld(saveGWorld, saveGDevice);

	Rect aRect = numRect;
	OffsetRectY(aRect, yy);
	CopyBits(	GetPortBitMapForCopyBits(itemDrawGW),
				GetPortBitMapForCopyBits(Port()),
				&numRect, &aRect, srcCopy, NULL);

	UnlockPixels(itemPixMap);
//	SetPort(oldPort);

	return true;
}


//-----------------------------------------------
//
//	DrawCircle(context, tones, root, key)
//
//	tones	-	the notes to highlight
//	root	-	the root of the chord (in yellow)
//	key		-	the key to show (in blue)
//
void FPDocWindow::DrawCircle(CGContextRef gc, UInt16 tones, UInt16 root, UInt16 key) {

	UInt16 mask = scalePalette->MaskForMode(document->ScaleMode(), key);
	float arc_radius = circRect.size.height / 2;
	float x1 = circRect.origin.x + arc_radius;
	float y1 = circRect.origin.y + arc_radius;

	for (int i=0; i<OCTAVE; i++) {	// 0..11
		float color[4] = { 0.0, 0.0, 0.0, 1.0 };
		UInt16 tone = KEY_FOR_INDEX(i);
		UInt16 bit = BIT(tone);

		if (bit & mask || bit & tones) {
			if (bit & tones) {
				if (tone == root)
					color[0] = color[1] = 1.0;
				else
					color[0] = color[1] = color[2] = 1.0;

				if (!(bit & mask))
					color[3] /= 2;
			}
			else {
				color[0] = color[1] = 0.0;
				color[2] = 0.8;
			}

			float rad = RAD(360 - (i * 30 + 15 - 90));
			CGContextSetRGBFillColor(gc, color[0], color[1], color[2], color[3]);
			CGContextBeginPath(gc);
			CGContextMoveToPoint(gc, x1, y1);
			CGContextAddArc(gc, x1, y1, arc_radius, rad, rad + RAD(29), false);
			CGContextAddLineToPoint(gc, x1, y1);
			CGContextClosePath(gc);
			CGContextFillPath(gc);
		}
	}
}


bool FPDocWindow::EraseLine(ChordIndex line) {
	Rect		aRect;
	SInt16		yy;

	yy = kTabHeight + (line - document->TopLine()) * kLineHeight;
	if (yy < 0 || yy > currentSize.bottom)
		return false;

//	GrafPtr oldPort; GetPort(&oldPort);
	Focus();

	aRect = itemRect;
	OffsetRectY(aRect, yy);
	EraseRect(&aRect);

//	SetPort(oldPort);
	return true;
}


#pragma mark -
#pragma mark Music Player
void FPDocWindow::PlayStarted() {
	UpdateTrackingRegions();
}


void FPDocWindow::PlayStopped() {
	DrawPlayDots();
	UpdateTrackingRegions();
}


//-----------------------------------------------
//
//	FollowPlayer
//
//	If the player is active:
//		Move the document cursor to the chord
//		Draw the playing dots
//
void FPDocWindow::FollowPlayer() {
	if (player->IsPlaying()) {
		if (fretpet->IsFollowViewEnabled() && !fretpet->IsEditModeEnabled()) {
			ChordIndex chord = player->PlayingChord();
			if ((document->GetCursor() != chord) && (chord < DocumentSize())) {
				guitarPalette->FinishBracketDrag();
				MoveCursor(chord);
			}
		}

		DrawPlayDots();
	}
}


void FPDocWindow::DrawPlayDots(bool force) {
	if (player->PlayingDocWindow() != this)
		return;

	bool	playing = player->IsPlaying();
	SInt16	beatNum = player->PlayingBeat(),
			chordNum = player->PlayingChord();

	// make sure there is a document open
	//	AND either the play has stopped or the play has moved forward
	if (!playing || lastBeatDrawn != beatNum || lastChordDrawn != chordNum || force) {
		SInt16	pick = 0, i;

		FPChordGroup &group = document->ChordGroup(chordNum);

		// only test for an empty beat when still playing
		// and play hasn't moved to a new chord
		if (playing && lastChordDrawn == chordNum) {
			// determine if there are any dots on the current beat
			for (i=NUM_STRINGS; i--;)
				pick |= group[CurrentPart()].GetPatternMask(i) & BIT(beatNum);

			// if it's empty then leave the previous dots lighted
			if (!pick) return;
		}

        GrafPtr	oldPort = Focus();
        
		// if the old dots are in an existing chord and beat then erase them
		if (lastChordDrawn >= 0 &&	lastChordDrawn < DocumentSize() && lastBeatDrawn >= 0 && lastBeatDrawn < document->ChordGroup(lastChordDrawn).PatternSize())
			DrawOneBeat(lastChordDrawn, lastBeatDrawn, false);


		// if the document is still playing then draw the new beat
		if (playing && beatNum < group.PatternSize())
			DrawOneBeat(chordNum, beatNum, true);

		ForeColor(blackColor);

		// keep track of the last item drawn
		lastBeatDrawn = beatNum;
		lastChordDrawn = chordNum;

		SetPort(oldPort);
	}

}


void FPDocWindow::DrawOneBeat(ChordIndex chord, SInt16 beat, bool light) {
	SInt16		x, y, f;
	Rect		aRect;
	bool		reverse = fretpet->IsTablatureReversed();

	ForeColor(light ? whiteColor : blueColor);

	x = CORDL + SEQH * (beat + 1);
	y = chord - TopLine();

	if (y >= 0 && y < VisibleLines()) {
		y = y * kLineHeight + kTabHeight + 24;

		for (UInt16 str=0; str<NUM_STRINGS; str++) {
			UInt16 string = reverse ? str : NUM_STRINGS - str - 1;
			f = document->Chord(chord, CurrentPart()).fretHeld[string];

			if ( f >= 0 && (document->Chord(chord, CurrentPart()).GetPatternMask(string) & BIT(beat)) ) {
				SetHVRect(&aRect, x - 1, y - 2, 5, 5);
				PaintOval(&aRect);
			}

			y += SEQV;
		}
	}
}


bool FPDocWindow::IsInteractive() {
	return !( player->IsPlaying(document) && fretpet->IsFollowViewEnabled() && !fretpet->IsEditModeEnabled() );		// can click or select
}


bool FPDocWindow::IsRangePlaying() {
	return ( player->IsPlaying(document) && !fretpet->IsFollowViewEnabled() && fretpet->IsEditModeEnabled() );		// play range affected by selection
}


#pragma mark -
#pragma mark Cursor Movement
void FPDocWindow::SetCurrentPart(PartIndex part) {
	if (CurrentPart() != part) {
		document->SetCurrentPart(part);
		Draw();
		UpdateTempoSlider();
		UpdateVelocitySlider();
		UpdateSustainSlider();
		UpdateGlobalChord();
	}
}


PartMask FPDocWindow::GetTransformMask() {
	PartMask partMask = 0;

	for (int i=DOC_PARTS; i--;)
		if (document->TransformFlag(i))
			partMask |= BIT(i);

	return partMask;
}


PartMask FPDocWindow::GetTransformMaskForMenuItem(MenuRef inMenu, SInt16 inItem) {
	UInt32		partSpec;
	(void)GetMenuItemRefCon(inMenu, inItem, &partSpec);

	switch (partSpec) {
		case kFilterAll:		return kAllChannelsMask;
		case kFilterEnabled:	return GetTransformMask();
		case kFilterCurrent:	return BIT(CurrentPart());
		default :				return kAllChannelsMask;
	}
}


void FPDocWindow::PageUp() {
	ChordIndex top = TopLine();
	if (top > 0) {
		ChordIndex	newtop = top - (VisibleLines() - 1);
		if (newtop < 0) newtop = 0;
		ScrollToReveal(newtop);
	}
}


void FPDocWindow::PageDown() {
	ChordIndex	bot = BottomLine();
	ChordIndex	last = DocumentSize() - 1;
	if (bot < last) {
		ChordIndex	newbot = BottomLine() + (VisibleLines() - 1);
		if (newbot > last) newbot = last;
		ScrollToReveal(newbot);
	}
}


void FPDocWindow::Home() {
	if (TopLine() > 0)
		ScrollToReveal(0);
}


void FPDocWindow::End() {
	ChordIndex	size = DocumentSize();
	if (size > 0)
		ScrollToReveal(size - 1);
}


//-----------------------------------------------
//
//	MoveCursor
//
//	Move the document cursor elsewhere.
//	Flag to indicate just erasing the old.
//
void FPDocWindow::MoveCursor(ChordIndex newLine, bool keepSel, bool justOld) {
	ChordIndex	docSize = DocumentSize();
	ChordIndex	oldLine = GetCursor();
	ChordIndex	oldTop, oldBot;

	(void)document->GetSelection(&oldTop, &oldBot);
	bool	hadSelection = document->HasSelection();

	if (keepSel) {
		if (!hadSelection)
			document->SetSelectionRaw(oldBot);
	}
	else if (hadSelection)
		SetSelection(-1, false);

	document->SetCursorLine(newLine);
	if (newLine < 0) justOld = true;

	// for a new position erase the old
	if (docSize && (oldLine != newLine || docSize == 1)) {
		if (!justOld)
			(void)ScrollToReveal(newLine);

		ChordIndex	topLine = TopLine(),
					botLine = BottomLine(),
					drawTop, drawBot;

		//
		// Draw all visible lines affected by a selection change
		// A selection can change in a few different ways:
		//
		//	- newTop  < oldTop && newBot  < oldBot		Draw newTop .. oldBot
		//	- newTop  < oldTop && newBot  > oldBot		** INVALID **
		//	- newTop  < oldTop && newBot == oldBot		Draw newTop .. oldTop
		//
		//	- newTop  > oldTop && newBot  < oldBot 		** INVALID **
		//	- newTop  > oldTop && newBot  > oldBot 		Draw oldTop .. newBot
		//	- newTop  > oldTop && newBot == oldBot		Draw oldTop .. newTop
		//
		//	- newTop == oldTop && newBot  < oldBot		Draw newBot .. oldBot
		//	- newTop == oldTop && newBot  > oldBot		Draw oldBot .. newBot
		//	- newTop == oldTop && newBot == oldBot		Draw oldTop .. oldBot
		//
		(void)document->GetSelection(&drawTop, &drawBot);

		if (drawTop < oldTop) {
			if (drawBot == oldBot)
				drawBot = oldTop;
			else if (drawBot < oldBot)
				drawBot = oldBot;
		}
		else if (drawTop > oldTop) {
			if (drawBot == oldBot) {
				drawBot = drawTop;
				drawTop = oldTop;
			}
			else if (drawBot > oldBot)
				drawTop = oldTop;
			else {
				drawTop = oldTop;
				drawBot = oldBot;
			}
		}
		else {
			if (drawBot < oldBot) {
				drawTop = drawBot;
				drawBot = oldBot;
			}
			else if (drawBot > oldBot)
				drawTop = oldBot;
		}

		if (drawTop < topLine)	drawTop = topLine;
		if (drawBot > botLine)	drawBot = botLine;

		for (ChordIndex i=drawTop; i<=drawBot; i++)
			DrawLine(i);

		// Draw the old line if showing
		// >1 lines or if "just erasing"
		if (justOld || topLine != botLine) {
			// Don't draw if already drawn
			// TODO: don't draw if not visible
			if (oldLine < drawTop || oldLine > drawBot)
				DrawLine(oldLine);
		}

		// if "just erasing" don't draw the new line
		if ( !justOld ) {
			if ( newLine < drawTop || newLine > drawBot )
				DrawLine(newLine);
		}

		// Copy the chord to the global chord
		if ( oldLine != newLine && newLine >= 0 && newLine < DocumentSize() )
			UpdateGlobalChord();
	}

	if (player->IsPlaying(document) && fretpet->IsEditModeEnabled()) {	//  && !fretpet->IsFollowViewEnabled())
		player->SetPlayRangeFromSelection();

		if (!keepSel)
			player->SetPlayingChord(newLine);
	}

	if ( ! (player->IsPlaying(document) && fretpet->IsFollowViewEnabled() && !fretpet->IsEditModeEnabled()) )
		UpdateTrackingRegions();
}


#pragma mark -
#pragma mark Add
//-----------------------------------------------
//
// AddChord
//
//	This adds the global chord to this document (giving it
//	a default picking sequence the first time it's used).
//
//	TODO: In the original FretPet you could add a chord
//	with an empty sequence by Option-clicking the Add
//	button.
//
//	TODO: It will make the code cleaner to remove any and
//	all references to the "fretpet" object from the document
//	implementation. It should be self-contained. Those methods
//	that need outside data should receive it from parameters.
//
bool FPDocWindow::AddChord(bool bResetSeq) {
	if (DocumentSize() == 0) {
		FPChordGroup srcGroup(globalChord);				// Create a group from the global chord
		srcGroup.ResetPattern();						// Clear the pattern on the group
		srcGroup[CurrentPart()].SetDefaultPattern();	// Set the default pattern on the current part
		globalChord.SetDefaultPattern();				// Set the default pattern on the global chord too
		return Insert(&srcGroup, 1, true);				// Finally, insert the group into the document
	}
	else {
		FPChordGroup srcGroup(document->ChordGroup());

		if (document->HasSelection())
			SetSelection(-1, false);

		return Insert(&srcGroup, 1, bResetSeq);
	}
}


bool FPDocWindow::Insert(FPChordGroup *srcGroup, ChordIndex count, bool bResetSeq) {
	ChordIndex insert = DocumentSize() ? document->GetCursor() + 1 : 0;

	if (document->Insert(srcGroup, count, bResetSeq)) {
		Focus();
		ScrollLines(insert, count, true);		// Scrolls and renumbers

		ChordIndex x = insert + count - 1;
		(void) player->AdjustForInsertion(x, insert, count);

		MoveCursor(x);
		UpdateScrollState();

		return true;
	}
	else
		return false;
}


bool FPDocWindow::Insert(FPChordGroupArray &arrayRef) {
	ChordIndex insert = DocumentSize() ? document->GetCursor() + 1 : 0;
	ChordIndex count = arrayRef.size();

	if (document->Insert(arrayRef)) {
		Focus();
		ScrollLines(insert, count, true);		// Scrolls and renumbers

		ChordIndex x = insert + count - 1;
		(void)player->AdjustForInsertion(x, insert, count);

		MoveCursor(x);
		UpdateScrollState();

		return true;
	}
	else
		return false;
}


#pragma mark -
//-----------------------------------------------
//
//	SetSelection
//
//	Sets the selection and redraws the lines that differ
//
void FPDocWindow::SetSelection(ChordIndex inSel, bool bScroll) {
	ChordIndex	curs = GetCursor();

	ChordIndex	oldStart, oldEnd;
	document->GetSelection(&oldStart, &oldEnd);

	document->SetSelection(inSel);
	ChordIndex newSel = SelectionEnd();

	ChordIndex oldTop = TopLine();
	ChordIndex oldBot = BottomLine();

	// Scroll for things like drag-select
	if (bScroll) ScrollToReveal((newSel >= 0) ? newSel : curs);

	ChordIndex newStart, newEnd;
	document->GetSelection(&newStart, &newEnd);

	ChordIndex topLine = TopLine();
	ChordIndex botLine = BottomLine();
	ChordIndex drawTop, drawBot;

	if (newSel >= 0) {
		if (newStart == curs) {
			if (oldStart == curs) {
				if (newEnd > oldEnd)
					drawTop = oldEnd + 1, drawBot = newEnd;
				else
					drawTop = newEnd + 1, drawBot = oldEnd;
			}
			else
				drawTop = oldStart, drawBot = newEnd;

		} else {

			if (oldEnd == curs) {
				if (newStart < oldStart)
					drawTop = newStart, drawBot = oldStart - 1;
				else
					drawTop = oldStart, drawBot = newStart - 1;
			}
			else
				drawTop = newStart, drawBot = oldEnd;
		}
	}
	else
		drawTop = oldStart, drawBot = oldEnd;

	if (drawTop < oldTop)	drawTop = oldTop;
	if (drawBot > oldBot)	drawBot = oldBot;
	if (drawTop < topLine)	drawTop = topLine;
	if (drawBot > botLine)	drawBot = botLine;
	if (drawTop == curs)	drawTop++;
	if (drawBot == curs)	drawBot--;

	if (drawBot >= drawTop) {
		for (ChordIndex i=drawTop; i<=drawBot; i++)
			if (i != curs)
				DrawLine(i);
	}

	//
	// If getting a selection and normal Free Edit mode is on tell the player
	//
	//	TODO: Don't move play position for "select all" during playback with free edit
	//
	if (inSel >= 0 && IsRangePlaying())
		player->SetPlayRangeFromSelection();

	fretpet->UpdatePlayTools();
}


void FPDocWindow::SelectAll() {
	if (document->SelectionSize() < DocumentSize()) {
		MoveCursor(DocumentSize() - 1, false);
		SetSelection(0, false);
	}
}


void FPDocWindow::SelectNone() {
	if (HasSelection())
		SetSelection(-1, false);
}


void FPDocWindow::DeleteSelection(bool undoable, bool inserting) {
	ChordIndex	startDel, endDel, delSize = document->GetSelection(&startDel, &endDel);
	if (delSize) {
		document->DeleteSelection();

//		if (undoable)
//		{
			// Scroll the necessary lines
			ScrollLines(endDel + 1, -delSize, true);

			// hilite the cursor line
			if (!inserting)
				UpdateLine(GetCursor());

			if (!inserting && !ScrollToReveal(GetCursor()))
				UpdateScrollState();
//		}

		if (!inserting) {
			UpdateGlobalChord();
			fretpet->UpdatePlayTools();

//			if (!undoable)
//				DrawVisibleLines();
		}
	}
}


void FPDocWindow::TransformSelection(MenuCommand cid, MenuItemIndex index, UInt16 partMask, bool undoable) {
	if (DocumentSize()) {
		document->TransformSelection(cid, index, partMask, undoable);

//		if (undoable)
			DrawAfterTransform();
	}
	else
		SysBeep(1);
}


void FPDocWindow::CloneSelection(ChordIndex count, UInt16 clonePartMask, UInt16 cloneTranspose, UInt16 cloneHarmonize, bool undoable) {
	if (DocumentSize()) {
		document->CloneSelection(count, clonePartMask, cloneTranspose, cloneHarmonize, undoable);

//		if (undoable)
			DrawAfterTransform();
	}
	else
		SysBeep(1);
}


void FPDocWindow::DrawAfterTransform() {
	if (!ScrollToReveal(GetCursor()))
		UpdateScrollState();

	UpdateGlobalChord();
	fretpet->UpdatePlayTools();

	Draw();
}


#pragma mark -
void FPDocWindow::RunCloneDialog() {
	cloneSheet = new FPCloneSheet(this);
	cloneSheet->Run();
}


void FPDocWindow::KillCloneDialog() {
	if (cloneSheet != NULL) {
		cloneSheet->Close();
		cloneSheet = NULL;
	}
}


#pragma mark -
void FPDocWindow::CancelSheets() {
	for (FPDocumentIterator itr = docWindowList.begin(); itr != docWindowList.end(); itr++) {
		(*itr)->KillCloneDialog();
		(*itr)->TerminateNavDialogs();
	}
}


int FPDocWindow::CountDirtyDocuments() {
	int dirt = 0;

	for (FPDocumentIterator itr = docWindowList.begin(); itr != docWindowList.end(); itr++)
		if ( (*itr)->IsDirty() )
			dirt++;

	return dirt;
}


#pragma mark -
//-----------------------------------------------
//
// HandleEvent
//
// Document windows handle some menu events and of course
// window events. In addition to intercepting CommandStatus
// events that would be passed up the handler chain to the app
// class we also intercept events that might pass up the
// inheritance chain. The former case is automatically screened
// by the Carbon Event Manager. The latter case is accomplished
// by simply calling the inherited handler for unhandled events.
//
OSStatus FPDocWindow::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
	OSStatus    err, result = eventNotHandledErr;
	char		key;

	switch ( event.GetClass() ) {
		//
		// Handle Command Events
		//
		case kEventClassWindow:
			switch ( event.GetKind() ) {
				case kEventWindowClickProxyIconRgn:
					if (!document->HasFile())
						result = noErr;
					break;

				case kEventWindowBoundsChanging:
					ConstrainSize( event );
					result = noErr;
					break;

				case kEventWindowClose:
					//
					// This is a hack to get consistent behavior
					// between Close All and Option-Close. Option-Close
					// sends a close command to every window
					//
					if (HAS_OPTION(GetCurrentEventKeyModifiers())) {
						if (!closingAllDocuments)
							DoCloseAll();
					}
					else {
						TryToClose();
					}
					result = noErr;
					break;
			}
			break;

		case kEventClassKeyboard: {
			switch ( event.GetKind() ) {
				case kEventRawKeyDown:
				case kEventRawKeyRepeat: {
					UInt16	mods = GetCurrentEventKeyModifiers();
					if (HAS_COMMAND(mods)) break;
					err = event.GetParameter(kEventParamKeyMacCharCodes, &key);

					switch( key ) {
						//
						// Backspace deletes the selection
						//	but not in non-interactive mode
						//
						case kBackspaceCharCode:
							if (IS_NOMOD(mods)) {
								if ( IsInteractive() ) {
									FPHistoryEvent *event = fretpet->StartUndo(UN_DELETE, CFSTR("Delete Chords"));
									event->SaveSelectionBefore();
									DeleteSelection();
									event->Commit();
								}

								result = noErr;
							}
							break;

						//
						// Left and Right Arrows change the part
						//
						case kLeftArrowCharCode:
							if (IS_NOMOD(mods)) {
								if (CurrentPart() > 0)
									fretpet->DoSelectPart(CurrentPart() - 1);

								result = noErr;
							}
							break;

						case kRightArrowCharCode:
							if (IS_NOMOD(mods)) {
								if (CurrentPart() < DOC_PARTS-1)
									fretpet->DoSelectPart(CurrentPart() + 1);

								result = noErr;
							}
							break;

						//
						// Up and down arrows move the cursor
						//
						case kUpArrowCharCode: {
							bool shift = IS_SHIFT(mods);
							if (shift || IS_NOMOD(mods)) {
								if (IsInteractive() && GetCursor() > 0)
									MoveCursor(GetCursor() - 1, shift);

								result = noErr;
							}

							if (IS_CONTROL(mods) || IS_CONTROL_SHIFT(mods)) {
								if (IsInteractive() && GetCursor() > 0) {
									shift = HAS_SHIFT(mods);
									ChordIndex line = GetCursor() - VisibleLines() + 1;
									if (line < 0) line = 0;
									MoveCursor(line, shift);
								}

								result = noErr;
							}

							break;
						}

						case kDownArrowCharCode: {
							bool shift = IS_SHIFT(mods);
							if (shift || IS_NOMOD(mods)) {
								if (IsInteractive() && GetCursor() < DocumentSize() - 1)
									MoveCursor(GetCursor() + 1, shift);

								result = noErr;
							}

							// Control-arrows move the cursor
							if (IS_CONTROL(mods) || IS_CONTROL_SHIFT(mods)) {
								if (IsInteractive() && GetCursor() < DocumentSize() - 1) {
									shift = HAS_SHIFT(mods);
									ChordIndex line = GetCursor() + VisibleLines() - 1;
									if (line >= DocumentSize()) line = DocumentSize() - 1;
									MoveCursor(line, shift);
								}

								result = noErr;
							}

							break;
						}

						//
						// PageUp / PageDown / Home / End
						//	Change the view without moving the cursor
						//
						case kPageUpCharCode: {
							PageUp();
							result = noErr;
							break;
						}

						case kPageDownCharCode: {
							PageDown();
							result = noErr;
							break;
						}

						case kHomeCharCode: {
							Home();
							result = noErr;
							break;
						}

						case kEndCharCode: {
							End();
							result = noErr;
							break;
						}

						default:
							CallNextEventHandler( inRef, event.GetEventRef() );
							break;
					}
					break;
				}
			}

//			fprintf(stderr, "FPDocWindow key event result: %d\n", result);
			break;
		}
	}

	//
	// If the event wasn't handled then FPWindow handles it.
	//
	if (result == eventNotHandledErr)
		result = FPWindow::HandleEvent(inRef, event);

	return result;
}


void FPDocWindow::UpdateTrackingRegions(bool noExit) {
	if (!noExit)
		ExitPreviousRegion();

	bool	enable = true, doset = true;

	if (IsInteractive()) {
		ChordIndex	line = document->GetCursor();
		SInt16		yy = kTabHeight + (line - document->TopLine()) * kLineHeight;

		blockedRollover->Disable();

		if (yy >= 0 && yy < currentSize.bottom && line < DocumentSize()) {
			FPChord &chord = document->CurrentChord();
			short x = chord.PatternSize() * SEQH;

			Rect newRect = lengthRect;
			OffsetRect(&newRect, x, yy);
			lengthRollover->Change(newRect);

			newRect = patternRect;
			newRect.right = newRect.left + x;
			OffsetRect(&newRect, 0, yy);
			sequencerRollover->Change(newRect);

			newRect = repeatRect;
			OffsetRect(&newRect, 0, yy);
			repeatRollover->Change(newRect);

			if (noExit)
				doset = false;
		}
		else
			enable = false;
	}
	else {
		enable = false;
		Rect currentSize = GetContentSize();
		Rect rect1 = { kTabHeight, currentSize.left, currentSize.bottom, currentSize.right - kScrollbarSize };
		blockedRollover->Enable();
		blockedRollover->Change(rect1);
	}

	if (doset) {
		repeatRollover->SetEnabled(enable);
		sequencerRollover->SetEnabled(enable);
		lengthRollover->SetEnabled(enable);
	}

	EnterRegionUnderMouse();
}


void FPDocWindow::HandleTrackingRegion(TTrackingRegion *region, TCarbonEvent &event) {
	int curs = 0;

	UInt32 mods;
	(void) event.GetParameter(kEventParamKeyModifiers, &mods);

	switch ( event.GetClass() ) {
		case kEventClassMouse:

			switch ( event.GetKind() ) {
				case kEventMouseEntered: {
					if (region == sequencerRollover) {
						curs =	HAS_SHIFT(mods) ?
								HAS_OPTION(mods) ? kCursDupe : kCursMove :
								HAS_COMMAND(mods) ?
								/* HAS_OPTION(mods) ? kCursCLine : */ kCursLine :
								HAS_OPTION(mods) ? kCursColumn : kCursAdd;
					}
					else if (region == repeatRollover)
						curs = kCursRepeat;
					else if (region == lengthRollover)
						curs = kCursBar;
					else if (region == blockedRollover)
						curs = kCursNoSmoking;
					else
						curs = kCursBar;

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

					if (region == sequencerRollover) {
						curs =	HAS_SHIFT(mods) ?
								HAS_OPTION(mods) ? kCursDupe : kCursMove :
								HAS_COMMAND(mods) ?
								/* HAS_OPTION(mods) ? kCursCLine : */ kCursLine :
								HAS_OPTION(mods) ? kCursColumn : kCursAdd;

						fretpet->SetMouseCursor(curs);
					}
					break;
			}
			break;
	}
}


#pragma mark -
Point FPDocWindow::GetIdealSize() {
	Rect		avail, bounds = Bounds();
	(void) GetAvailableWindowPositioningBounds(GetMainDevice(), &avail);

	short	maxlines = (avail.bottom - avail.top - kTabHeight - bounds.top) / kLineHeight;
	short	lines = DocumentSize();
	if (lines > maxlines) lines = maxlines;
	if (lines < 1) lines = 1;

	Point size = { lines * kLineHeight + kTabHeight, kDocWidth };
	return size;
}


void FPDocWindow::ConstrainSize( TCarbonEvent &event ) {
	WindowRef		window;
	Rect			bounds;
	SInt16			height;
	UInt32			attributes = 0;
	OSStatus		err;

	//
	// Get window ref from the event
	//
	err = event.GetParameter(kEventParamDirectObject, &window);
	require_noerr( err, ConstrainSize_CantGetWindow );

	//
	// Get the current bounds from the event
	//
	err = event.GetParameter(kEventParamCurrentBounds, &bounds);
	require_noerr( err, ConstrainSize_CantGetCurrentBounds );

	//
	// Get the attributes from the event
	//
	err = event.GetParameter(kEventParamAttributes, &attributes);
	//
	// Keep the window constrained by chord sizes
	//
	if ( (err == noErr) && (attributes & kWindowBoundsChangeUserResize ) != 0 ) {
		height = bounds.bottom - bounds.top;
		height = ((height - kTabHeight + 30) / kLineHeight);
		if (height < 1) height = 1;

		bounds.right = bounds.left + kDocWidth;
		bounds.bottom = bounds.top + kTabHeight + height * kLineHeight;

		event.SetParameter(kEventParamCurrentBounds, bounds);

		//
		// Resize the scrollbar too
		//
//		Resized();
	}

ConstrainSize_CantGetCurrentBounds:
ConstrainSize_CantGetWindow:
	return;
}


//-----------------------------------------------
//
// Resized
//
// Recalculate the scrollbar and redraw content
// as necessary.
//
void FPDocWindow::Resized() {
	currentSize = GetContentSize();

	Rect rect1 = { kTabHeight, currentSize.left, currentSize.bottom, currentSize.right - kScrollbarSize };
	sequencerControl->SetBounds(&rect1);

	Rect rect2 = { kTabHeight, currentSize.right - kScrollbarSize, currentSize.bottom - kScrollbarSize, currentSize.right };
	SetControlBounds(scrollbar, &rect2);

	blockedRollover->Change(rect1);

	//
	// Check to see if scrolling needs to happen
	//
	ChordIndex top = document->TopLine();
 	if (top > 0) {
		ChordIndex	bottom = top + VisibleLines();		// Line that would be just below the bottom at the new size
		if (bottom > DocumentSize()) {
			ChordIndex newTop = DocumentSize() - VisibleLines();
			if (newTop < 0) newTop = 0;
			document->SetTopLine(newTop);
			ChordIndex dist = top - newTop;
			if (dist > 0)
				ScrollLines(0, dist, false);
		}
	}

	UpdateScrollState();

	if (!didFirstResize) {
		didFirstResize = true;
		//	InvalidateWindow();
		Hide(); Show();	// Hack for Lion to make lines past index 5 draw
	}
}


void FPDocWindow::Activated() {
	FPWindow::Activated();

	DrawHeading();

	fretpet->DoWindowActivated(this);

	if (player->IsPlaying() && !player->IsPlaying(document))
		fretpet->DoStop();
}


void FPDocWindow::Deactivated() {
	FPWindow::Deactivated();

	DrawHeading();

	if (activeWindow == this)
		activeWindow = NULL;
}


bool FPDocWindow::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	if (delta > 0)
		LiveScroll(scrollbar, kControlUpButtonPart);
	else if (delta < 0)
		LiveScroll(scrollbar, kControlDownButtonPart);

	return true;
}


bool FPDocWindow::UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	if (!IsActive())
		return false;

	bool enable = true;
	bool hasfile = document->HasFile();

	switch ( cid ) {
		//
		// FretPet Menu
		//
		case kHICommandQuit:
			enable = !fretpet->isQuitting;
			break;

		#pragma mark File Menu
		//
		// File Menu
		//
		case kHICommandClose:
			enable = true;
			break;

		case kFPCommandCloseAll:
			enable = !closingAllDocuments;
			break;

		case kHICommandSave:
			enable = 
#if KAGI_SUPPORT || DEMO_ONLY
				fretpet->AuthorizerIsAuthorized() &&
#endif
				(IsDirty() || !hasfile);
			break;

		case kHICommandSaveAs:
			enable =
#if KAGI_SUPPORT || DEMO_ONLY
			fretpet->AuthorizerIsAuthorized();
#else
			true;
#endif
			break;

		case kHICommandRevert:
			enable = IsDirty() && hasfile;
			break;

		case kFPCommandExportMovie:
			enable = 
#if KAGI_SUPPORT || DEMO_ONLY
				fretpet->AuthorizerIsAuthorized() &&
#endif
				(DocumentSize() > 0)
			&&	(!fretpet->IsDialogFront() && FPApplication::QuickTimeIsOkay());
			break;

		case kFPCommandExportMIDI:
		case kFPCommandExportSunvox:
			enable =
#if KAGI_SUPPORT || DEMO_ONLY
				fretpet->AuthorizerIsAuthorized() &&
#endif
				(DocumentSize() > 0);
			break;

		#pragma mark Edit Menu
		case kHICommandCut:
		case kHICommandCopy:
			enable = (DocumentSize() > 0);
			break;

		case kHICommandPaste:
//			(void)SetMenuItemTextWithCFString(menu, index, blocked ? CFSTR("Paste") : CFSTR("Paste Chord"));
			enable = fretpet->clipboard.HasChords();
			break;

		case kFPCommandPastePattern:
		case kFPCommandPasteTones:
			enable = fretpet->clipboard.HasChords() && DocumentSize() > 0;
			break;

		case kFPCommandClearPattern:
		case kFPCommandClearBoth:
			break;

		case kHICommandSelectAll:
			enable =	document->SelectionSize() < DocumentSize()
						&&	(	!player->IsPlaying(document)
							||	!fretpet->IsFollowViewEnabled()
//							||	fretpet->IsEditModeEnabled()
							);
			break;

		case kFPCommandSelectNone: {
			enable = DocumentSize() > 0 && document->SelectionSize() > 1;
			break;
		}

		#pragma mark Sequencer Menu
		case kFPCommandSetDefaultSequence: {
			enable = DocumentSize() > 0;
			break;
		}


		#pragma mark Filter Menu
		//
		// Selection Menu
		//
		case kFPCommandFilterSubmenuAll:
			break;

		case kFPCommandFilterSubmenuEnabled: {
			int count = 0;
			TString itemStr, numStr;
			for (PartIndex p=0; p<DOC_PARTS; p++) {
				if (document->TransformFlag(p)) {
					if (count++)
						numStr += ", ";

					numStr += (p+1);
				}
			}

			if (count)
				itemStr.SetWithFormatLocalized((count > 1) ? CFSTR("Filter Parts %@") : CFSTR("Filter Part %@"), numStr.GetCFStringRef());
			else {
				itemStr.SetLocalized(CFSTR("(No Parts)"));
				enable = false;
			}

			(void)SetMenuItemTextWithCFString(menu, index, itemStr.GetCFStringRef());
			break;
		}

		case kFPCommandFilterSubmenuCurrent: {
			TString itemStr;
			itemStr.SetWithFormatLocalized(CFSTR("Filter Part %d"), CurrentPart() + 1);
			(void)SetMenuItemTextWithCFString(menu, index, itemStr.GetCFStringRef());
			break;
		}

		case kFPCommandSelScramble:
			enable = IsInteractive() && document->SelectionSize() > 1;
			break;

		case kFPCommandSelReverseChords:
			enable = IsInteractive() && document->SelectionCanReverse();
			break;

		case kFPCommandSelReverseMelody:
			enable = IsInteractive() && document->SelectionCanReverseMelody();
			break;

		case kFPCommandSelClone:
		case kFPCommandSelDouble:
		case kFPCommandSelRandom1:
		case kFPCommandSelRandom2:
		case kFPCommandSelTransposeBy:
		case kFPCommandSelTransposeTo:
		case kFPCommandSelHarmonizeUp:
		case kFPCommandSelHarmonizeDown:
		case kFPCommandSelHarmonizeBy:
		case kFPCommandSelHarmonizeSub:
		case kFPCommandSelTransposeToSub:
		case kFPCommandSelTransposeBySub: {
			enable = IsInteractive() && DocumentSize() > 0;
			break;
		}


		// TODO: Test whether each of these commands would have an effect
		case kFPCommandSelClearPatterns:
			enable = IsInteractive() && document->SelectionHasPattern(GetTransformMaskForMenuItem(menu, index), true);
			break;

		case kFPCommandSelClearTones:
			enable = IsInteractive() && document->SelectionHasTones(GetTransformMaskForMenuItem(menu, index));
			break;

		case kFPCommandSelLockRoots:
		case kFPCommandSelUnlockRoots:
			enable = IsInteractive() && document->SelectionHasLock(GetTransformMaskForMenuItem(menu, index), cid == kFPCommandSelLockRoots);
			break;

		case kFPCommandSelHFlip:
			enable = IsInteractive() && document->SelectionCanHFlip(GetTransformMaskForMenuItem(menu, index));
			break;

		case kFPCommandSelVFlip:
			enable = IsInteractive() && document->SelectionCanVFlip(GetTransformMaskForMenuItem(menu, index));
			break;

		case kFPCommandSelInvertTones:
			enable = IsInteractive() && document->SelectionCanInvert(GetTransformMaskForMenuItem(menu, index));
			break;

		case kFPCommandSelCleanupTones:
			enable = IsInteractive() && document->SelectionCanCleanup(GetTransformMaskForMenuItem(menu, index));
			break;

		case kFPCommandSelSplay:
			enable = IsInteractive() && document->SelectionCanSplay();
			break;

		case kFPCommandSelCompact:
			enable = IsInteractive() && document->SelectionCanCompact();
			break;

		default:
			return false;
	}

	enable ? EnableMenuItem(menu, index) : DisableMenuItem(menu, index);

	return true;
}


bool FPDocWindow::HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	switch (cid) {
		#pragma mark File Menu
		//
		// File Menu
		//
		case kHICommandClose:
			TryToClose();
			break;

		case kFPCommandCloseAll:
			DoCloseAll();
			break;

#if !DEMO_ONLY

		case kHICommandSave:
			document->SaveChanges();
			break;

		case kHICommandSaveAs:
			(void)DoSaveAs();
			break;

		case kHICommandRevert:
			(void)document->ConfirmRevert();
			break;

		case kFPCommandExportMovie:
			(void)document->ExportMovie();
			break;

		case kFPCommandExportMIDI:
			(void)document->ExportMIDI();
			break;

		case kFPCommandExportSunvox:
			(void)document->ExportSunvox();
			break;
#endif
			
		#pragma mark Edit Menu
		//
		// Edit Menu
		//
		case kHICommandCut:
			DoCut();
			break;

		case kHICommandCopy:
			DoCopy();
			break;

		case kHICommandPaste:
			DoPaste();
			break;

		case kFPCommandPasteTones:
			DoPasteTones();
			break;

		case kFPCommandPastePattern:
			DoPastePattern();
			break;

		case kHICommandSelectAll:
			SelectAll();
			break;

		case kFPCommandSelectNone:
			SelectNone();
			break;


		#pragma mark Document Button
		//
		// Double Tempo Button
		//
		case kFPDoubleTempo:
			DoToggleTempoMultiplier();
			break;

		#pragma mark Filter Menu
		//
		// Sequencer Menu
		//
		case kFPCommandSetDefaultSequence: {
			DialogItemIndex item = RunPrompt(
				CFSTR("Set Default Pattern?"),
				CFSTR("Do you want to save the current picking pattern as the default from now on?"),
				CFSTR("Save As Default"),
				CFSTR("Cancel"),
				NULL
			);

			if (item == kAlertStdAlertOKButton)
				globalChord.SavePatternAsDefault();

			break;
		}

		#pragma mark Filter Menu
		//
		// Filter Menu
		//
		case kFPCommandSelClone:
			RunCloneDialog();
			break;

		case kFPCommandSelSplay:
		case kFPCommandSelCompact:
		case kFPCommandSelDouble:
		case kFPCommandSelScramble:
		case kFPCommandSelReverseChords:
		case kFPCommandSelReverseMelody:
			TransformSelection(cid, index, kAllChannelsMask);
			break;

		case kFPCommandSelClearPatterns:
		case kFPCommandSelHFlip:
		case kFPCommandSelVFlip:
		case kFPCommandSelRandom1:
		case kFPCommandSelRandom2:
		case kFPCommandSelClearTones:
		case kFPCommandSelInvertTones:
		case kFPCommandSelCleanupTones:
		case kFPCommandSelLockRoots:
		case kFPCommandSelUnlockRoots:
		case kFPCommandSelTransposeBy:
		case kFPCommandSelTransposeTo:
		case kFPCommandSelHarmonizeUp:
		case kFPCommandSelHarmonizeDown:
		case kFPCommandSelHarmonizeBy: {
			TransformSelection(cid, index, GetTransformMaskForMenuItem(menu, index));
			break;
		}

		default:
			return false;
	}

	return true;
}


OSErr FPDocWindow::DragReceive(DragRef theDrag) {
	printf("Item Dropped!\r");
	return noErr;
}


OSErr FPDocWindow::DragEnter(DragRef theDrag) {
	UInt16 count;
	(void) CountDragItems(theDrag, &count);
	printf("Drag Enter (%d items)\r", count);
	return dragNotAcceptedErr;
}


OSErr FPDocWindow::DragLeave(DragRef theDrag) {
	(void) HideDragHilite(theDrag);
	printf("Drag Leave\r");
	return dragNotAcceptedErr;
}


OSErr FPDocWindow::DragWithin(DragRef theDrag) {
	RgnHandle hiliteRgn = NewRgn();
	Rect bounds = sequencerControl->Bounds();
	RectRgn(hiliteRgn, &bounds);
	(void) ShowDragHilite(theDrag, hiliteRgn, true);
	return dragNotAcceptedErr;
}
