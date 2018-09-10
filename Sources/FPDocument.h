/*!
 *	@file	FPDocument.h
 *
 *	@brief	Interface for the FPDocument class
 *
 *	@section COPYRIGHT
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPDOCUMENT_H
#define FPDOCUMENT_H

#include "FPChord.h"
#include "TFile.h"

#include "FPMusicPlayer.h"
#include "FPTuningInfo.h"
#include <QuickTime/QuickTime.h>

class	FPDocWindow;

#if !DEMO_ONLY
class	FPMovieFile;
class	FPMidiFile;
class	FPSunvoxFile;
#endif

//
//	File Format IDs
//
#define	FILE_FORMAT_B3			'FPB3'
#define	FILE_FORMAT_14			'FP14'
#define	FILE_FORMAT_21			'FP21'
#define	FILE_FORMAT_X1			'FPX1'
#define	FILE_FORMAT_QQ			'FPQQ'
#define	FILE_FORMAT_XML_COMPACT	CFSTR("Compact")
#define	FILE_FORMAT_XML_LOOSE	CFSTR("Loose")

/*!
 *	Structure describing a Part, which is a single voice
 */
typedef struct {
	UInt16		instrument;				//!< 2 The instrument in terms of the synth
	UInt16		velocity;				//!< 2 The velocity
	UInt16		sustain;				//!< 2 The sustain
#if QUICKTIME_SUPPORT
	Boolean		outQT;					//!< 1 Quicktime output?
#else
	Boolean		outSynth;				//!< 1 Synth output?
#endif
	Boolean		outMIDI;				//!< 1 MIDI output?
	UInt16		outChannel;				//!< 2 The MIDI channel to play on
	Boolean		transformFlag;			//!< 1 Is this channel transformed in multi-mode
	Boolean		unused;					//!< 1 placeholder
} partInfo;								//  12 bytes total

#pragma mark -

#define DPART(x) (((x)==kCurrentChannel)?CurrentPart():(x))

/*!
 *	@brief A class encapsulating a FretPet document
 *
 *	A document has its own settings plus a list of chords.
 *	The chord list is represented
 *	here with methods for applying selection actions.
 *	Some actions are handled by the chord objects, others
 *	like Scramble operate on groups of chords.
 *
 *	The chord array is an array of pointers to chord groups
 *	The smallest unit is a chord group, plus an array entry
 *	for the pointer. When chords are deleted they are all
 *	first deallocated then the pointers are disposed.
 *	then the buffer size is adjusted.
 * 
 */
class FPDocument : public TFile {
	friend class FPMidiFile;

	private:
		FPChordGroupArray	chordGroupArray;	//!< A smart array class to handle our chord data
#if !DEMO_ONLY
		FPMovieFile			*movieExporter;		//!< Helper to export as a movie
		FPMidiFile			*midiExporter;		//!< Helper to export as MIDI
		FPSunvoxFile		*sunvoxExporter;	//!< Helper to export as Sunvox
#endif
		FPDocWindow			*window;			//!< The document's window
		ChordIndex			cursor;				//!< The cursor position
		ChordIndex			selectionEnd;		//!< The selection end (0 = before Chord 0)
		ChordIndex			topLine;			//!< Top line in the document view
		UInt16				tempo;				//!< Tempo
		UInt16				tempoX;				//!< Tempo Multiplier
		UInt64				interim;			//!< The tempo converted into milliseconds
		FPTuningInfo		tuning;				//!< The tuning of this document, as last synched

		TString instrumentName[DOC_PARTS];		//!< Instrument names for all parts

		UInt16				scaleMode;			//!< Scale
		UInt16				enharmonic;			//!< Enharmonic index

		PartIndex			partNum;			//!< Part Number
		partInfo			part[DOC_PARTS];	//!< Metadata for all parts

		bool				soloQT;				//!< Solo for QuickTime (unused)
		bool				soloMIDI;			//!< Solo for MIDI (unused)
		bool				soloTransform;		//!< Solo for Transform (unused)

	public:
							FPDocument(FPDocWindow *wind);
							FPDocument(FPDocWindow *wind, const FSSpec &fspec);
							FPDocument(FPDocWindow *wind, const FSRef &fref);
							~FPDocument();

		void				Init(FPDocWindow *wind);
		void				SetWindow(FPDocWindow *wind);
		WindowRef			Window();
		inline FPDocWindow*	DocWindow() const					{ return window; }

		inline ChordIndex	Size() const						{ return chordGroupArray.size(); }

		inline FPChordGroupArray&	ChordGroupArray()			{ return chordGroupArray; }

		inline FPChordGroup&	ChordGroup(ChordIndex index)	{ return chordGroupArray[index]; }
		inline FPChordGroup&	ChordGroup()					{ return ChordGroup(cursor); }

		inline FPChord&	Chord(ChordIndex index, PartIndex part)	{ return chordGroupArray[index][part]; }
		inline FPChord&	Chord(ChordIndex index)					{ return Chord(index, CurrentPart()); }
		inline FPChord&	CurrentChord()							{ return Chord(cursor); }

		inline const FPChordGroup& ChordGroup(ChordIndex index) const		{ return chordGroupArray[index]; }
		inline const FPChordGroup& ChordGroup() const						{ return ChordGroup(cursor); }

		inline const FPChord& Chord(ChordIndex index, PartIndex part) const	{ return chordGroupArray[index][part]; }
		inline const FPChord& Chord(ChordIndex index) const					{ return Chord(index, CurrentPart()); }
		inline const FPChord& CurrentChord() const							{ return chordGroupArray[cursor][CurrentPart()]; }

		inline void		SetTopLine(ChordIndex top)				{ topLine = top; }
		void			SetCurrentChord(const FPChord &srcChord);

		void			Insert(const FPChord &chord);
		void			Insert(const FPChordGroup &group);
		bool			Insert(const FPChordGroupArray &srcArray);
		bool			Insert(const FPChordGroup *srcChord, ChordIndex count, bool bResetSeq);

		void			Delete();
		void			Delete(ChordIndex start, ChordIndex end);
		inline void		DeleteAll()								{ if (Size()) { Delete(0, Size()-1); } }
		void			DeleteSelection();

		void			SetSelection(ChordIndex newSel);
		void			SetSelectionRaw(ChordIndex newSel);

		void			CopySelectionToClipboard();

		// View and interface accessors
		inline void			SetCursorLine(ChordIndex curs)					{ cursor = curs; }
		inline ChordIndex	GetCursor() const								{ return cursor; }
		inline PartIndex	CurrentPart() const								{ return partNum; }
		inline ChordIndex	TopLine() const									{ return topLine; }
		inline ChordIndex	SelectionEnd() const							{ return selectionEnd; }
		inline bool			HasSelection() const							{ return (selectionEnd >= 0); }
		ChordIndex			GetSelection(ChordIndex *start, ChordIndex *end) const;
		inline ChordIndex	SelectionSize() const							{ ChordIndex st, en; return GetSelection(&st, &en); }

		// Document settings accessors
		inline bool		HasFile() const										{ return Exists();	/* refNumber != 0; */ }
		inline UInt16	GetInstrument(PartIndex p) const					{ return part[p].instrument; }
		inline UInt16	CurrentInstrument() const							{ return GetInstrument(CurrentPart()); }
		inline StringPtr CurrentInstrumentName() const						{ return instrumentName[CurrentPart()].GetPascalString(); }
		inline UInt16	ScaleMode() const									{ return scaleMode; }
		inline UInt16	Enharmonic() const									{ return enharmonic; }
		inline const	FPTuningInfo& Tuning() const						{ return tuning; }
		inline UInt16	OpenTone(UInt16 string) const						{ return Tuning().tone[string]; }
		inline UInt16	Tempo() const										{ return tempo; }
		inline UInt16	TempoMultiplier() const								{ return tempoX; }
		inline UInt16	PlayTempo() const									{ return tempoX * tempo; }
		inline UInt64	Interim() const										{ return interim; }
		inline UInt16	Velocity(PartIndex p=kCurrentChannel) const			{ return part[DPART(p)].velocity; }
		inline UInt16	Sustain(PartIndex p=kCurrentChannel) const			{ return part[DPART(p)].sustain; }

#if QUICKTIME_SUPPORT
		inline UInt16	GetQuicktimeOutEnabled(PartIndex p=kCurrentChannel) const { return part[DPART(p)].outQT; }
		inline void		SetQuicktimeOutEnabled(PartIndex p, bool b)			{ part[DPART(p)].outQT = b; }
#else
		inline UInt16	GetSynthOutEnabled(PartIndex p=kCurrentChannel) const	{ return part[DPART(p)].outSynth; }
		inline void		SetSynthOutEnabled(PartIndex p, bool b)				{ part[DPART(p)].outSynth = b; }
#endif

		inline UInt16	MidiOut(PartIndex p=kCurrentChannel) const			{ return part[DPART(p)].outMIDI; }
		inline UInt16	MidiChannel(PartIndex p=kCurrentChannel) const		{ return part[DPART(p)].outChannel; }
		inline UInt16	TransformFlag(PartIndex p=kCurrentChannel) const	{ return part[DPART(p)].transformFlag; }

		// Document settings setters
		inline void		SetInstrument(PartIndex p, UInt16 inInstrument)		{ part[DPART(p)].instrument = inInstrument; }
		inline void		UpdateInstrumentName(PartIndex p)					{ instrumentName[DPART(p)] = player->GetInstrumentName(DPART(p)); }
		inline void		UpdateInterim()										{ interim = U64Divide(60 * 1000000, PlayTempo(), NULL); }
		inline void		SetCurrentPart(PartIndex p)							{ partNum = p; }
		inline void		SetScale(UInt16 newMode)							{ scaleMode = newMode; }
		inline void		SetEnharmonic(UInt16 enh)							{ enharmonic = enh; }
		inline void		SetTempo(UInt16 t)									{ tempo = t; UpdateInterim(); }
		inline void		SetTempoMultiplier(bool x2)							{ tempoX = (x2 ? 2 : 1); UpdateInterim(); }
		inline void		SetVelocity(PartIndex p, UInt16 v)					{ part[DPART(p)].velocity = v; }
		inline void		SetSustain(PartIndex p, UInt16 s)					{ part[DPART(p)].sustain = s; }
		inline void		SetMidiOut(PartIndex p, bool b)						{ part[DPART(p)].outMIDI = b; }
		inline void		SetMidiChannel(PartIndex p, UInt16 c)				{ part[DPART(p)].outChannel = c; }
		inline void		SetTransformFlag(PartIndex p, bool t)				{ part[DPART(p)].transformFlag = t; }
		void			SetTuning(const FPTuningInfo &t)					{ tuning = t; }
		void			UpdateFingerings();
		void			TransformSelection(MenuCommand cid, MenuItemIndex index, PartMask partMask, bool undoable);
		void			CloneSelection(ChordIndex count, PartMask clonePartMask, UInt16 cloneTranspose, UInt16 cloneHarmonize, bool undoable);
		void			FixSelectionAfterFilter(ChordIndex startSel, ChordIndex endSel, ChordIndex addedSize);

		// Sequence methods
		UInt32			TotalBeats() const;
		bool			HasPattern() const;
		bool			HasFingering() const;
		bool			PartsHavePattern(PartMask partMask=kAllChannelsMask, bool noChordRequired=false) const;
		bool			SelectionHasPattern(PartMask partMask=kAllChannelsMask, bool noChordRequired=false) const;
		bool			SelectionHasTones(PartMask partMask=kAllChannelsMask) const;
		bool			SelectionHasLock(PartMask partMask=kAllChannelsMask, bool unlocked=false) const;
		bool			SelectionCanSplay() const;
		bool			SelectionCanCompact();
		bool			SelectionCanReverse() const;
		bool			SelectionCanReverseMelody() const;

		bool			SelectionCanHFlip(PartMask partMask=kAllChannelsMask) const;
		bool			SelectionCanVFlip(PartMask partMask=kAllChannelsMask) const;
		bool			SelectionCanInvert(PartMask partMask=kAllChannelsMask) const;
		bool			SelectionCanCleanup(PartMask partMask=kAllChannelsMask) const;
		bool			ReadyToPlay() const;
		inline bool		GetChordPatternDot(UInt16 string, UInt16 step) const	{ return CurrentChord().GetPatternDot(string, step); }
		inline void		SetChordPatternDot(UInt16 string, UInt16 step)			{ CurrentChord().SetPatternDot(string, step); }
		inline void		ClearPatternDot(UInt16 string, UInt16 step)				{ CurrentChord().ClearPatternDot(string, step); }
		inline void		SetPatternStep(UInt16 step)								{ CurrentChord().SetPatternStep(step); }
		inline void		ClearPatternStep(UInt16 step)							{ CurrentChord().ClearPatternStep(step); }

		// File methods
		OSStatus		InitFromFile(const FSRef &fsref);
		OSStatus		InitFromFile();
		OSErr			InitFromXMLFile();
		OSStatus		InitFromClassicFile();

#if !DEMO_ONLY
		OSErr			WriteXMLFormat();

		OSErr			WriteFormat214();
		OSErr			WriteFormat214Chords();

		// Export Formats
		Handle		GetFormat0();
		Handle		GetFormat1();
		OSStatus	ExportMIDI();

		Handle		GetSunvoxFormat();
		OSStatus	ExportSunvox();

		UInt32*		GetTuneHeader();
		UInt32**	GetTuneSequence(long *duration);
		OSStatus	ExportMovie();


		// TFile Overrides:
		OSErr		OpenAndSave();
		OSErr		WriteData();
		void		NavCancel();
		void		NavDiscardChanges();
		void		NavDontSave();
		void		NavDidSave();
		void		NavCustomSize(Rect &customRect);
		void		NavCustomControls(NavDialogRef navDialog, DialogRef dialog);
		void		HandleCustomControl(ControlRef inControl, UInt16 controlIndex);
#endif

		void		TerminateNavDialogs();
		void		HandleNavError(OSStatus err);
};

#endif
