/*!
	@file FPHistory.h

	@brief Classes supporting undo and redo.

	FPHistoryEvent stores a single event and supplies virtual
	methods to Undo or Redo that event.

	FPHistory stores an arbitrarily long list of events and
	provides the public interface to do things to the history.

	Many undoable events need to save off data before the operation
	begins. For this reason there are separate methods to save the
	before state.

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef FPHISTORY_H
#define FPHISTORY_H

#include "FPChord.h"
#include "TDictionary.h"
#include "TString.h"
#include "TObjectDeque.h"

class FPDocWindow;
class FPHistory;


//
// UNDO ACTIONS
//
enum {
	UN_CANT = 0,			// Empty history
	UN_EDIT,				// Tones changed in a single chord
	UN_MODIFY,				// Change the note modifier
	UN_INVERT,				// Tones were inverted
	UN_EDIT_SEQ,			// Edited the sequence
	UN_CLEAR,				// Chord cleared (specifically)
	UN_CLEAR_SEQ,			// Cleared the sequence
	UN_CLEAR_BOTH,			// Cleared both
	UN_PASTE_PATT,			// Pasted sequence dots
	UN_PASTE_TONES,			// Pasted tones
	UN_TRANSPOSE,			// Transposed the chord
	UN_CLONE_PART,			// Cloned a part (by option-dragging)
	UN_CUT,					// Cut a selection
	UN_PASTE,				// Pasted a selection
	UN_INSERT,				// Inserted chords
	UN_DELETE,				// Deleted chords
	UN_TEMPO,				// Edited the tempo
	UN_TEMPO_X,				// Toggled the tempo multiplier
	UN_VELOCITY,			// Edited the velocity
	UN_SUSTAIN,				// Edited the sustain
	UN_INSTRUMENT,			// Changed the instrument of a part
	UN_MIDI_CHANNEL,		// Changed the MIDI channel assignment of a part

	UN_LAST_DOCUMENT,		// Global chord came from the last document closed

	UN_TUNING_CHANGE,		// Changed the guitar tuning (all chords)

	UN_S_SCRAMBLE,			// Scrambled a range of chords
	UN_S_SPLAY,				// Expand to full breadth
	UN_S_CONSOLIDATE,		// Reduce to minimal breadth
	UN_S_DOUBLE,			// Double the duration
	UN_S_CLONE,				// Produce two or more clones
	UN_S_REVERSE,			// Reverse the chords
	UN_S_REVMEL,			// Reverse chords and sequences both
	UN_S_INVERT,			// Invert tones
	UN_S_CLEANUP,			// Cleanup tones
	UN_S_CLEAR,				// Clear tones
	UN_S_LOCK,				// Lock
	UN_S_UNLOCK,			// Unlock
	UN_S_CLEAR_SEQ,			// Clear sequences
	UN_S_HFLIP,				// H-flip sequences
	UN_S_VFLIP,				// V-Flip sequences
	UN_S_RANDOM1,			// Randomize sequences (rhythmic pattern)
	UN_S_RANDOM2,			// Randomize sequences (one string per beat)
	UN_S_TRANSTO,			// Transpose
	UN_S_TRANSBY,
	UN_S_HARMUP,			// Harmonize
	UN_S_HARMDOWN,
	UN_S_HARMTO,
	UN_S_HARMBY
};

/*! The state of the document at some point.
	That's why they call it a brief description.
*/
typedef struct {
	ChordIndex			cursor;					//!< the cursor
	ChordIndex			selEnd;					//!< the selection ending
	ChordIndex			playStart;				//!< the playback range start
	ChordIndex			playEnd;				//!< the playback range end
	TDictionary			moreData;				//!< other data related to the state
	FPChordGroupArray	groups;					//!< chords that were deleted (for example)
} HistoryState;



/*!	@class FPHistoryEvent

	@brief	An event that the user performed

	A history event contains the before and after state
	and is created in response to document and chord
	modifications.
*/
class FPHistoryEvent {
friend class FPHistory;

private:
	FPHistory			*history;				//!< the history manager this belongs to
	FPDocWindow			*docWindow;				//!< the parent window, if any
	UInt16				action;					//!< action identifier (from above)
	PartIndex			partNum;				//!< the active part
	PartMask			partMask;				//!< the mask of affected parts
	TString				nameString;				//!< string for the undo/redo menu items
	HistoryState		before,					//!< the state before committing
						after;					//!< the state after committing
	UInt32				eventTime;				//!< TickCount at the time of this event

public:

	/*!	Default Constructor
		Items that will be placed into stl containers must
		have a default constructor, but this one shouldn't be
		used!
	*/
	FPHistoryEvent()
	{ Init(NULL, 0, NULL); }

	/*!	Constructor
		@param hist the history that owns this event
		@param act the action identifier
		@param opName the name of the operation
		@param pm the parts to which the event applies
	*/
	FPHistoryEvent(FPHistory *hist, UInt16 act, CFStringRef opName, PartMask pm=kCurrentChannelMask)
	{ Init(hist, act, opName, pm); }

	virtual ~FPHistoryEvent() {}

	/*!	The instance Init method
		@param hist the history that owns this event
		@param act the action identifier
		@param opName the name of the operation
		@param pm the parts to which the event applies
	*/
	void Init(FPHistory *hist, UInt16 act, CFStringRef opName, PartMask pm=kCurrentChannelMask);


	//! @brief Store the clipboard contents in the After state
	void SaveClipboardAfter();


	//! @brief Store the current selection in the Before state
	void SaveSelectionBefore();


	//! @brief Store the current selection in the After state
	void SaveSelectionAfter();


	/*!	Store a chord in the Before state.
		@param chord the chord to store
	*/
	void SaveChordBefore(FPChord &chord);


	/*!	Store a chord in the After state.
		@param chord the chord to store
	*/
	void SaveChordAfter(FPChord &chord);


	//! @brief Store the current chord in the Before state
	void SaveCurrentBefore();


	//! @brief Store the current chord in the After state
	void SaveCurrentAfter();


	/*!	Store a selection of chord groups in the After state.
		@param start the selection start
		@param end the selection end
	*/
	void SaveGroupsBefore(ChordIndex start, ChordIndex end);


	/*!	Store an array of chord groups in the Before state.
		@param groupArray the groups to store
	*/
	void SaveGroupsBefore(FPChordGroupArray &groupArray);


	/*!	Store a selection of chord groups in the After state.
		@param start the selection start
		@param end the selection end
	*/
	void SaveGroupsAfter(ChordIndex start, ChordIndex end);


	/*!	Store an array of chord groups in the After state.
		@param groupArray the groups to store
	*/
	void SaveGroupsAfter(FPChordGroupArray &groupArray);


	/*!	Save a piece of Before data.
		@param key the storage key
		@param value the data to store
	*/
	void SaveDataBefore(CFStringRef key, SInt32 value);


	/*!	Get a Before data value.
		@result the Before data
	*/
	SInt32 GetDataBefore(CFStringRef key);


	/*!	Save a piece of After data.
		@param key the storage key
		@param value the data to store
	*/
	void SaveDataAfter(CFStringRef key, SInt32 value);


	/*!	Get an After data value.
		@result the After data
	*/
	SInt32 GetDataAfter(CFStringRef key);


	/*!	Save a piece of Before data.
		@param key the storage key
		@param value the data to store
	*/
	void SaveDataBefore(CFStringRef key, CFTypeRef value);


	/*!	Save a piece of After data.
		@param key the storage key
		@param value the data to store
	*/
	void SaveDataAfter(CFStringRef key, CFTypeRef value);


	/*!	Get a piece of Before data as a CFTypeRef.
		@result the Before data
	*/
	CFTypeRef GetCFDataBefore(CFStringRef key);


	/*!	Get a piece of After data as a CFTypeRef.
		@result the After data
	*/
	CFTypeRef GetCFDataAfter(CFStringRef key);


	/*!	The chord that was highlighted after this event.
		@result the chord or NULL if there is none
	*/
	FPChord* PrimaryChord();


	/*!	Merge another event with this one.
		@param event the event to merge
	*/
	void MergeEvent(FPHistoryEvent *event);


	//! @brief Store selection and play state and commit the event.
	void Commit();


	//! @brief Set the merge flag for the next Commit.
	void SetMerge();


	/*!	Sets merge if the previous event was very recent.

		This is intended to be used in circumstances when the
		mousewheel is used to alter a control value to prevent
		an unwieldy number of undo events being generated.

		@param interim the time threshold for merging events
	*/
	void SetMergeUnderInterim(UInt32 interim);


	/*!	Setter for the part index.
		@param part the part index
	*/
	inline void SetPart(PartIndex part) { partNum = part; }

	//! @brief Set the part index to be agnostic about the part
	inline void SetPartAgnostic() { partNum = kCurrentChannel; }


	//! @brief Restore this event's before state.
	virtual void Undo();


	//! @brief Restore this event's after state.
	virtual void Redo();


	/*!	Get the name of the action
		@result the name of the action
	*/
	inline CFStringRef ActionName() const { return nameString.GetCFStringRef(); }
};


#pragma mark -

//! A place to store history events
typedef TObjectDeque<FPHistoryEvent> FPHistoryArray;


#pragma mark -
/*! A manager for history events related to a document
 *
 *	This is a very specialized class that retains a
 *	growing list of chord and document events.
 *	It always associates the event with a document window
 *	and its document.
 */
class FPHistory {
friend class FPHistoryEvent;

private:
	FPDocWindow		*docWindow;				//!< the window of each undo item created
	UInt16			undoPosition;			//!< position in history for undo/redo
	FPHistoryArray	history;				//!< A growing array of history events
	FPHistoryEvent	*tempUndoEvent;			//!< A temporary event for use in undo creation
	bool			mergeFlag;				//!< Set if the next event should merge

public:

	//! @brief The constructor takes an optional document window
	FPHistory(FPDocWindow *homeWindow=NULL);


	virtual	~FPHistory() {
		// needed because not every event is committed
		if (tempUndoEvent != NULL)
			delete tempUndoEvent;
	}


	//! @brief The class Init method
	void Init() {
		mergeFlag		= false;
		undoPosition	= 0;
		docWindow		= NULL;
		tempUndoEvent	= NULL;
	}


	//! @brief Accessor for the history size
	inline ChordIndex Size() { return history.size(); }


	//! @brief Clear the history
	inline void Clear() { history.clear(); undoPosition = 0; }


	/*!	@brief The last chord that was stored in the history
		@result the last chord stored
	*/
	FPChord* PreviousChord();


	/*!	@brief Saves data for a complete undoable/redoable event.
		The event becomes the last event in the history.
		@param event the event to store
	*/
	void RememberEvent(FPHistoryEvent *event);


	//! @brief Take back the most recent event without undoing it.
	inline void Recant() { if (undoPosition > 0) undoPosition--; }


	/*!	Set the merge flag

		The merge flag indicates that the next event should
		be merged with the most recent one. This changes the
		behavior of the commit operation so that it uses the
		'before' info of the previous event.
	*/
	inline void SetUndoMerge() { mergeFlag = true; }


	/*!	Sets merge if the previous event was very recent.

		This is intended to be used in circumstances when the
		mousewheel is used to alter a control value to prevent
		an unwieldy number of undo events being generated.

		@param interim the time threshold for merging events
	*/
	void SetMergeUnderInterim(UInt32 interim);


	/*!	Create a new history event to be populated and committed.

		@result the new event
	*/
	FPHistoryEvent* UndoStart(UInt16 act, CFStringRef opName, PartMask partMask) {
		// needed because not every event is committed
		if (tempUndoEvent != NULL)
			delete tempUndoEvent;

		tempUndoEvent = new FPHistoryEvent(this, act, opName, partMask );

		return tempUndoEvent;
	}


	//! @brief Undo the previous event and decrement undoPosition
	void DoUndo();


	/*!	The name of the current Undo event

		@result the name of the event
	*/
	inline CFStringRef UndoName() const {
		return CanUndo() ? history[undoPosition - 1]->ActionName() : NULL;
	}


	/*!	Whether there is anything to Undo

		@result TRUE if there is anything to Undo
	*/
	inline bool CanUndo() const {
		return (undoPosition > 0);
	}

	//! @brief Redo the last Undone event and increment undoPosition
	void DoRedo();


	/*!	The name of the current Redo event.

		@result the name of the event
	*/
	inline CFStringRef RedoName() const {
		return CanRedo() ? history[undoPosition]->ActionName() : NULL;
	}

	/*!	Whether there is anything to Redo

		@result TRUE if there is anything to Redo
	*/
	inline bool CanRedo() const {
		return (undoPosition < history.size());
	}

private:

	/*!	Commit the event begun with UndoStart

		The committed event becomes the last in the history.

		If merging, this applies the "after" state
		to the last item in the history.

		@result TRUE if there is anything to Redo
	*/
	void UndoCommit();


	/*!	Set the document window as modified or not

		The history is ultimately responsible for setting
		the document window as modified. Certain operations
		- such as chord changes - won't affect the document
		when it's empty, whereas others always do.

		The test is slightly crude, but generally okay.

		@param action The action that was performed
		@param modified The modified state to set
	*/
	void SetDocumentModified(UInt16 action, bool modified);

};

#endif
