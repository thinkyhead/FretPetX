/*!
	@file FPHistory.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPApplication.h"
#include "FPHistory.h"
#include "FPDocWindow.h"
#include "FPDocument.h"


void FPHistoryEvent::Init(FPHistory *hist, UInt16 inAction, CFStringRef opName, PartMask inMask) {
	history		= hist;
	docWindow	= history->docWindow;
	action		= inAction;
	nameString	= CFCopyLocalizedString(opName, "Undoable event name");
	eventTime	= TickCount();

	if (docWindow) {
		partNum			= docWindow->CurrentPart();
		partMask		= (inMask == kCurrentChannelMask) ? docWindow->CurrentPartMask() : inMask;
		before.cursor	= docWindow->DocumentSize() ? docWindow->GetCursor() : 0;
		before.selEnd	= docWindow->SelectionEnd();

		player->IsPlaying(docWindow->document)
			? player->PlayRange(before.playStart, before.playEnd)
			: player->GetRangeToPlay(before.playStart, before.playEnd);
	}
	else {
		partNum = kChannel1;
		before.selEnd = -1;
		before.cursor = 0;
		partMask = BIT(partNum);
	}
}


void FPHistoryEvent::SaveClipboardAfter() {
	SaveGroupsAfter(fretpet->clipboard.Contents());
}


//-----------------------------------------------
//
// SaveCurrentBefore / SaveCurrentAfter
//
void FPHistoryEvent::SaveCurrentBefore() {
	if (docWindow && docWindow->DocumentSize() ) {
		ChordIndex curs = docWindow->document->GetCursor();
		SaveGroupsBefore(curs, curs);
	}
	else
		SaveChordBefore(globalChord);
}


void FPHistoryEvent::SaveCurrentAfter() {
	if (docWindow && docWindow->DocumentSize() ) {
		ChordIndex curs = docWindow->document->GetCursor();
		SaveGroupsAfter(curs, curs);
	}
	else
		SaveChordAfter(globalChord);
}


//-----------------------------------------------
//
// SaveChordBefore / SaveChordAfter
//
void FPHistoryEvent::SaveChordBefore(FPChord &chord) {
	FPChordGroupArray	temp;
	temp.InsertCopyBefore(0, chord);
	SaveGroupsBefore(temp);
}


void FPHistoryEvent::SaveChordAfter(FPChord &chord) {
	FPChordGroupArray	temp;
	temp.InsertCopyBefore(0, chord);
	SaveGroupsAfter(temp);
}


//-----------------------------------------------
//
// SaveSelectionBefore / SaveSelectionAfter
//
void FPHistoryEvent::SaveSelectionBefore() {
	ChordIndex start, end;
	(void)docWindow->document->GetSelection(&start, &end);
	SaveGroupsBefore(start, end);
}


void FPHistoryEvent::SaveSelectionAfter() {
	ChordIndex start, end;
	(void)docWindow->document->GetSelection(&start, &end);
	SaveGroupsAfter(start, end);
}


//-----------------------------------------------
//
// SaveGroupsBefore / SaveGroupsAfter
//
void FPHistoryEvent::SaveGroupsBefore(ChordIndex start, ChordIndex end) {
	if (end >= start) {
		before.groups.clear();
		before.groups.append_copies(docWindow->document->ChordGroupArray(), start, end);
	}
}


void FPHistoryEvent::SaveGroupsBefore(FPChordGroupArray &groupArray) {
	before.groups.clear();
	before.groups.append_copies(groupArray);
}


void FPHistoryEvent::SaveGroupsAfter(ChordIndex start, ChordIndex end) {
	if (end >= start) {
		after.groups.clear();
		after.groups.append_copies(docWindow->document->ChordGroupArray(), start, end);
	}
}


void FPHistoryEvent::SaveGroupsAfter(FPChordGroupArray &groupArray) {
	after.groups.clear();
	after.groups.append_copies(groupArray);
}


//-----------------------------------------------
//
// SaveDataBefore / GetDataBefore / GetCFDataBefore
// SaveDataAfter / GetDataAfter / GetCFDataAfter
//
void FPHistoryEvent::SaveDataBefore(CFStringRef key, SInt32 value) {
	before.moreData.SetInteger(key, value);
}


SInt32 FPHistoryEvent::GetDataBefore(CFStringRef key) {
	return before.moreData.GetInteger(key);
}


void FPHistoryEvent::SaveDataAfter(CFStringRef key, SInt32 value) {
	after.moreData.SetInteger(key, value);
}


SInt32 FPHistoryEvent::GetDataAfter(CFStringRef key) {
	return after.moreData.GetInteger(key);
}


void FPHistoryEvent::SaveDataBefore(CFStringRef key, CFTypeRef value) {
	before.moreData.SetValue(key, value);
}


CFTypeRef FPHistoryEvent::GetCFDataBefore(CFStringRef key) {
	return before.moreData.GetValue(key);
}


void FPHistoryEvent::SaveDataAfter(CFStringRef key, CFTypeRef value) {
	after.moreData.SetValue(key, value);
}


CFTypeRef FPHistoryEvent::GetCFDataAfter(CFStringRef key) {
	return after.moreData.GetValue(key);
}


FPChord* FPHistoryEvent::PrimaryChord() {
	FPChord *chord = NULL;

	if (partNum >= 0 && partNum < DOC_PARTS && after.groups.size()) {
		ChordIndex i = (after.cursor <= after.selEnd) ? 0 : after.groups.size() - 1;
		chord = &after.groups[i][partNum];
 	}

	return chord;
}


//-----------------------------------------------
//
// Undo
//
//	For now the Undo method will be a big fat repository of knowledge.
//	Later on subclasses will encapsulate the
//	different styles of undo / redo action.
//
void FPHistoryEvent::Undo() {
	bool redraw = false, affectCursor = true;

	if ( partNum >= 0 && partNum < DOC_PARTS )
		fretpet->DoSelectPart( partNum );

	switch ( action ) {
		case UN_CUT:
		case UN_DELETE: {
			ChordIndex end = (before.selEnd == -1) ? before.cursor : MIN(before.cursor, before.selEnd);
			docWindow->SetCursorAndSelection( end - 1, -1 );
			break;
		}

		case UN_PASTE:
			docWindow->SetCursorAndSelection( after.cursor, after.cursor - after.groups.size() + 1 );
			break;

		case UN_INSTRUMENT:
		case UN_VELOCITY:
		case UN_SUSTAIN:
		case UN_TEMPO:
		case UN_TEMPO_X:
		case UN_MIDI_CHANNEL:
			// do nothing to the selection
			break;

		default:
			if ( docWindow )
				docWindow->SetCursorAndSelection( after.cursor, after.selEnd );
			break;
	}

	switch ( action ) {
		case UN_EDIT:
		case UN_CLEAR:
			fretpet->SetGlobalChord( before.groups[0][partNum] );
			break;

		case UN_INVERT:
			fretpet->DoInvertTones(false);
			affectCursor = false;
			break;

		case UN_MODIFY:
			fretpet->DoSetNoteModifier( GetDataBefore(CFSTR("modifier")), false );
			affectCursor = false;
			break;

		case UN_CLONE_PART:
			// TODO: Implement if there's any demand
			break;

		case UN_CUT:
		case UN_DELETE: {
//			if ( after.selEnd == -1 && docWindow->DocumentSize() )
//				docWindow->MoveCursor( after.cursor - 1, false, true );

			docWindow->DoReplace( before.groups, false, false );
			break;
		}

		case UN_PASTE:
			if (before.groups.size())
				docWindow->DoReplace( before.groups, false, true );
			else
				docWindow->DeleteSelection( false );
			redraw = true;
			break;

		case UN_PASTE_PATT:
			docWindow->DoPastePattern(before.groups, partNum);
			affectCursor = false;
			break;

		case UN_PASTE_TONES:
			docWindow->DoPasteTones(before.groups, partNum);
			affectCursor = false;
			break;

		case UN_INSERT:
			docWindow->DeleteSelection( false );
			redraw = true;
			break;

		case UN_TEMPO:
			fretpet->DoSetTempo( GetDataBefore(CFSTR("tempo")), true );
			affectCursor = false;
			break;

		case UN_TEMPO_X:
			docWindow->DoToggleTempoMultiplier( false );
			affectCursor = false;
			break;

		case UN_VELOCITY:
			fretpet->DoSetVelocity( GetDataBefore(CFSTR("velocity")), GetDataBefore(CFSTR("part")), true, true );
			affectCursor = false;
			break;

		case UN_SUSTAIN:
			fretpet->DoSetSustain( GetDataBefore(CFSTR("sustain")), GetDataBefore(CFSTR("part")), true, true );
			affectCursor = false;
			break;

		case UN_INSTRUMENT:
			fretpet->DoSetInstrument( GetDataBefore(CFSTR("part")), GetDataBefore(CFSTR("instrument")), false );
			affectCursor = false;
			break;

		case UN_MIDI_CHANNEL:
			fretpet->DoSetMidiChannel( GetDataBefore(CFSTR("part")), GetDataBefore(CFSTR("channel")), false );
			affectCursor = false;
			break;

		case UN_S_CLONE: {
			ChordIndex start, end;
			ChordIndex size = docWindow->GetSelection(&start, &end) * ( GetDataBefore(CFSTR("count")) - 1);
			docWindow->SetCursorAndSelection( end, end - size + 1 );
			docWindow->DeleteSelection( false );
			redraw = true;
			break;
		}

		//
		// Commands that are reversed by applying an action
		//
		case UN_S_REVERSE:
		case UN_S_REVMEL:
		case UN_S_INVERT:
		case UN_S_LOCK:
		case UN_S_UNLOCK:
		case UN_S_HFLIP:
		case UN_S_VFLIP:
			affectCursor = false;

		// TODO: Only affect the cursor in cases where the size actually changes
			docWindow->TransformSelection(GetDataBefore(CFSTR("undoCommand")), GetDataBefore(CFSTR("menuIndex")), partMask, false);
			break;

		//
		// Commands that change the number of chords
		//
		case UN_S_SPLAY:
		case UN_S_CONSOLIDATE:
		case UN_S_DOUBLE:
			docWindow->DoReplace( before.groups, false, true );
			break;

		//
		// Commands that transform chords in place
		//
		case UN_S_SCRAMBLE:
		case UN_S_CLEAR:
		case UN_S_CLEANUP:
		case UN_S_CLEAR_SEQ:
		case UN_S_HARMBY:
		case UN_S_HARMUP:
		case UN_S_HARMDOWN:
		case UN_S_TRANSTO:
		case UN_S_TRANSBY:
		case UN_S_RANDOM1:
		case UN_S_RANDOM2:
			affectCursor = false;
			docWindow->ReplaceSelection( before.groups );
			break;

		//
		// Commands that affect every chord
		//
		case UN_TUNING_CHANGE: {
			CFDictionaryRef dict = (CFDictionaryRef)GetCFDataBefore(CFSTR("tuning"));
			TDictionary		tdict(dict);
			FPTuningInfo	tuningInfo(&tdict);
			fretpet->DoSetGuitarTuning(tuningInfo, false);

			if (docWindow)
				docWindow->ReplaceAll( before.groups );
			else
				fretpet->SetGlobalChord( before.groups[0][partNum] );

			CFRELEASE(dict);
			affectCursor = false;
			break;
		}
	}

	if (affectCursor && docWindow) {
		bool playing = player->IsPlaying();
		bool follow = fretpet->IsFollowViewEnabled();

		if (!playing || !follow)
			docWindow->SetCursorAndSelection( before.cursor, before.selEnd );

		if (playing) {
			if ( !follow && fretpet->IsEditModeEnabled() )
				player->SetPlayRangeFromSelection();
			else
				player->SetPlayRange( before.playStart, before.playEnd );
		}

//		if (redraw)
//			docWindow->DrawVisibleLines();
	}
}


void FPHistoryEvent::Redo() {
	bool	redraw = false, affectCursor = true;

	if ( partNum >= 0 && partNum < DOC_PARTS )
		fretpet->DoSelectPart( partNum );

	switch (action) {
		case UN_INSTRUMENT:
		case UN_VELOCITY:
		case UN_SUSTAIN:
		case UN_MIDI_CHANNEL:
		case UN_TEMPO:
		case UN_TEMPO_X:
		case UN_TUNING_CHANGE:
			// do nothing to the selection
			affectCursor = false;
			break;

		default:
			if ( docWindow )
				docWindow->SetCursorAndSelection( before.cursor, before.selEnd );
			break;

	}

	switch ( action ) {
		case UN_EDIT:
		case UN_CLEAR:
			fretpet->SetGlobalChord( after.groups[0][partNum] );
//			fretpet->UpdateDocumentChord();
			break;

		case UN_INVERT:
			fretpet->DoInvertTones(false);
			affectCursor = false;
			break;

		case UN_MODIFY:
			fretpet->DoSetNoteModifier( GetDataAfter(CFSTR("modifier")), false );
			affectCursor = false;
			break;

		case UN_CLONE_PART:
			// TODO: Implement if there's any demand
			break;

		case UN_CUT:
		case UN_DELETE:
			docWindow->DeleteSelection( false );
			redraw = true;
			break;

		case UN_PASTE:
			docWindow->DoReplace( after.groups, false, false );
			break;

		case UN_PASTE_PATT:
			docWindow->DoPastePattern( after.groups, GetDataAfter(CFSTR("clipPart")) );
			affectCursor = false;
			break;

		case UN_PASTE_TONES:
			docWindow->DoPasteTones( after.groups, GetDataAfter(CFSTR("clipPart")) );
			affectCursor = false;
			break;

		case UN_INSERT:
			(void)docWindow->AddChord();
			break;

		case UN_TEMPO:
			fretpet->DoSetTempo( GetDataAfter(CFSTR("tempo")), true );
			break;

		case UN_TEMPO_X:
			docWindow->DoToggleTempoMultiplier( false );
			break;

		case UN_VELOCITY:
			fretpet->DoSetVelocity( GetDataAfter(CFSTR("velocity")), GetDataBefore(CFSTR("part")), true, true );
			break;

		case UN_SUSTAIN:
			fretpet->DoSetSustain( GetDataAfter(CFSTR("sustain")), GetDataBefore(CFSTR("part")), true, true );
			break;

		case UN_INSTRUMENT:
			fretpet->DoSetInstrument( GetDataBefore(CFSTR("part")), GetDataAfter(CFSTR("instrument")), false );
			break;

		case UN_MIDI_CHANNEL:
			fretpet->DoSetMidiChannel( GetDataBefore(CFSTR("part")), GetDataAfter(CFSTR("channel")), false );
			break;

		case UN_S_CLONE:
			docWindow->CloneSelection(GetDataBefore(CFSTR("count")), partMask, GetDataBefore(CFSTR("transpose")), GetDataBefore(CFSTR("harmonize")), false);
			break;

		case UN_S_SCRAMBLE:
		case UN_S_RANDOM1:
		case UN_S_RANDOM2:
			docWindow->ReplaceSelection( after.groups );
			affectCursor = false;
			break;

		// The following commands are redone by doing the same command again.
		// No data will have been saved during the original operation
		//	for these commands, saving a bit of RAM.
		case UN_S_REVERSE:
		case UN_S_REVMEL:
		case UN_S_CLEAR:
		case UN_S_INVERT:
		case UN_S_CLEANUP:
		case UN_S_CLEAR_SEQ:
		case UN_S_HFLIP:
		case UN_S_VFLIP:
		case UN_S_HARMBY:
		case UN_S_HARMUP:
		case UN_S_HARMDOWN:
		case UN_S_TRANSTO:
		case UN_S_TRANSBY:
			affectCursor = false;

		case UN_S_SPLAY:
		case UN_S_CONSOLIDATE:
		case UN_S_DOUBLE:
		case UN_S_LOCK:
		case UN_S_UNLOCK:
			docWindow->TransformSelection(GetDataBefore(CFSTR("redoCommand")), GetDataBefore(CFSTR("menuIndex")), partMask, false);
			break;

		case UN_TUNING_CHANGE: {
			CFDictionaryRef dict = (CFDictionaryRef)GetCFDataAfter(CFSTR("tuning"));
			TDictionary		tdict(dict);
			FPTuningInfo	tuningInfo(&tdict);
			fretpet->DoSetGuitarTuning(tuningInfo, false);
//			docWindow->document->ChordGroupArray().InitWithArray(after.groups);
			CFRELEASE(dict);
			break;
		}
	}

	if (affectCursor && docWindow) {
		bool playing = player->IsPlaying();
		bool follow = fretpet->IsFollowViewEnabled();

		if ( !playing || !follow )
			docWindow->SetCursorAndSelection( after.cursor, after.selEnd );

//		if (redraw)
//			docWindow->DrawVisibleLines();
	}
}


void FPHistoryEvent::MergeEvent(FPHistoryEvent *src) {
	nameString			= src->nameString;
	eventTime			= src->eventTime;
	after.playStart		= src->after.playStart;
	after.playEnd		= src->after.playEnd;
	after.groups		= src->after.groups;
	after.moreData		= src->after.moreData;
}


void FPHistoryEvent::Commit() {
	if (docWindow != NULL) {
		after.selEnd = docWindow->SelectionEnd();
		after.cursor = docWindow->GetCursor();
		player->PlayRange(after.playStart, after.playEnd);
	}

	history->UndoCommit();
}


void FPHistoryEvent::SetMerge() {
	history->SetUndoMerge();
}


void FPHistoryEvent::SetMergeUnderInterim(UInt32 interim) {
	history->SetMergeUnderInterim(interim);
}


#pragma mark -
FPHistory::FPHistory(FPDocWindow *wind) {
	Init();
	docWindow = wind;
}


//-----------------------------------------------
//
// RememberEvent
//
//	Undo sets the history back one item and
//	preserves all items in the history.
//
//	Adding a new action to the history drops
//	all (redo) items after the current undo index.
//
void FPHistory::RememberEvent(FPHistoryEvent *event) {
	if (CanRedo())
		history.resize(undoPosition);

	history.push_back( event );
	undoPosition = history.size();
}


FPChord* FPHistory::PreviousChord() {
	FPChord		*chord = NULL;
	ChordIndex	i = history.size();

	while (i--) {
		if ((chord = history[i]->PrimaryChord()))
			break;
 	}

	return chord;
}


void FPHistory::UndoCommit() {
	if (tempUndoEvent != NULL) {
		SetDocumentModified(tempUndoEvent->action, true);
		if (mergeFlag) {
			if (undoPosition > 0)
				history[undoPosition - 1]->MergeEvent(tempUndoEvent);

			delete tempUndoEvent;
		}
		else
			RememberEvent(tempUndoEvent);

		tempUndoEvent = NULL;
		mergeFlag = false;
	}
}


void FPHistory::DoUndo() {
	if (undoPosition > 0) {
		history[--undoPosition]->Undo();

		if (undoPosition == 0)
			SetDocumentModified(0, false);
	}
}


void FPHistory::DoRedo() {
	if (undoPosition < history.size()) {
		FPHistoryEvent *hist = history[undoPosition++];
		SetDocumentModified(hist->action, true);
		hist->Redo();
	}
}


void FPHistory::SetDocumentModified(UInt16 action, bool modified) {
	if (docWindow != NULL) {
		bool always = true;

		if (modified) {
			switch(action) {
				case UN_EDIT:
				case UN_MODIFY:
				case UN_INVERT:
				case UN_CLEAR:
				case UN_TRANSPOSE:
					always = false;
					break;
			}
		}

		docWindow->SetModified(modified, always);
	}
}


void FPHistory::SetMergeUnderInterim(UInt32 interim) {
	if (undoPosition > 0
		&& tempUndoEvent->action == history[undoPosition - 1]->action
		&& tempUndoEvent->eventTime - history[undoPosition - 1]->eventTime < interim)
			SetUndoMerge();

/*	if (undoPosition > 0) {
		FPHistoryEvent *hist = history[undoPosition - 1];
		if ( tempUndoEvent->action == hist->action
			&& tempUndoEvent->eventTime - hist->eventTime < interim) {
			SetUndoMerge();
		}
	}
*/
}

