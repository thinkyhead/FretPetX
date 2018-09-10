/*
 *  FPMusicPlayer.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPMUSICPLAYER_H
#define FPMUSICPLAYER_H

#define USE_DEPRECATED_TIMER 1
#define	BASE_VELOCITY	90
#define	BASE_SUSTAIN	15

#include <CoreMIDI/MIDIServices.h>
#include <AudioUnit/AudioUnit.h>
#include <AudioToolbox/AudioToolbox.h>
#include <QuickTime/QuickTimeMusic.h>

#include "FPChord.h"
#include "FPApplication.h"

#include "TQTOutput.h"

class TSynthOutput;
class TMidiOutput;
class FPMusicPlayer;
class FPDocument;
class FPDocWindow;

#define MidiOutForPart(p) CH[(IsMidiOutputSplit() ? (p) : 0)].midiOutput

//
// QueueTone
//
typedef struct {
	UInt64			when;
	PartIndex		part;
	UInt16			note;
	UInt16			string;
	UInt16			fret;
} QueueTone;

#include <deque>
typedef std::deque<QueueTone>	ToneList;
typedef ToneList::iterator		ToneIterator;


//
// ChannelInfo
//	Keeps track of a single channel
//
typedef struct {
#if QUICKTIME_SUPPORT
	TQTOutput		*quickTimeOutput;			// QuickTime as a Music Output - wow
	bool			qtOutEnabled;				// QuickTime out enabled
#else
	TSynthOutput	*synthOutput;				// Even newer newfangled output port
	bool			synthOutEnabled;			// Music Device out enabled
#endif

	bool			midiOutEnabled;				// MIDI out enabled
	TMidiOutput		*midiOutput;				// Newfangled output port

	long			trueGMNumber;				// MIDI instrument number
	UInt16			outChannel;					// MIDI output
	UInt16			velocity;					// The part's velocity
	UInt16			sustain;					// The part's sustain
	bool			transformFlag;				// Transform this voice in multi-mode
} ChannelInfo;


//
// FPPlayerExtTask
//	Extended task structure is a block beginning with a task
//	Which also corresponds to a Queue Element (QElem).
//

#define FPART(x) (((x)==kCurrentChannel)?recentPart:(x))

typedef struct	{
	TMTask			task;
	FPMusicPlayer	*player;
} FPPlayerExtTask;

#pragma mark -
class FPMusicPlayer {
private:
	ToneList		ToneQueue;				//!< Tone Queue - notes played outside a document

#ifdef USE_DEPRECATED_TIMER
	FPPlayerExtTask		playerTask;			//!< Time Manager Task for the player
#else
	// Can't use CFRunLoopTimer because opening the menu interrupts it - WTF!
	CFRunLoopTimerRef	playerTimer;		//!< CFRunLoopTimer for the player
#endif

	EventLoopTimerUPP	animationUPP;		//!< These support animation and periodic events
	EventLoopTimerRef	animationLoop;

	// Quicktime Support
#if QUICKTIME_SUPPORT
	bool			usingQuicktimeOut;		//!< flag to use Quicktime output
#else
	// Audio Device Support! (to replace QuickTime)
	bool			usingAUSynthOut;		//!< flag to use Audio Device output
	AUGraph			graph;					//!< The synth routing graph
#endif

	ChannelInfo		CH[TOTAL_PARTS];		//!< Each part's synths and states

	// CoreMIDI Support!
	bool			usingMidiOut;			//!< flag to use MIDI output
	MIDIClientRef	midiClient;				//!< our MIDI client
	bool			splitMidiOut;			//!< flag to split MIDI output

	PartIndex		recentPart;				//!< most recent part number that was selected
											// used when no document is open

	//
	// Play Progress
	//
	FPDocument		*playDoc;				//!< The document playing
	FPDocWindow		*playWindow;			//!< and its window
	bool			playFlag;				//!< playing the sequence?
	ChordIndex		playFirst;				//!< first chord in to play
	ChordIndex		playLast;				//!< last chord to play
	UInt16			stringToPlay;			//!< tones are subtly arpeggiated
	UInt16			beatToPlay;				//!< the beat to play
	ChordIndex		chordToPlay;			//!< the chord we hear
	UInt16			repeatToPlay;			//!< the count of repeats

	SInt16			prevBeat;				//!< remember the last dots drawn for later erasure

	bool			abortPlay;				//!< flag from the player that it aborted

	bool			justHearFlag;			//!< when set the hear button strums?
	UInt64			lastTime;				//!< document player timing
	UInt64			playInterim;			//!< interim

public:
	FPMusicPlayer();
	~FPMusicPlayer();

	void			SavePreferences();

	inline bool		IsPlaying(FPDocument *doc)					{ return playFlag && (playDoc == doc); }
	inline bool		IsPlaying()									{ return playFlag; }
	inline FPDocument*	PlayingDocument()						{ return playDoc; }
	inline FPDocWindow*	PlayingDocWindow()						{ return playWindow; }

	inline void		SetPlayRange(ChordIndex start, ChordIndex end) {
						if (playFlag && (chordToPlay < start || chordToPlay > end))
							chordToPlay = start;
					
						playFirst = start; playLast = end;
					}

	void			SetPlayRangeFromSelection();
	inline bool		LineInPlayRange(ChordIndex line)			{ return (playFlag && playFirst != playLast && line >= playFirst && (playLast < 0 || line <= playLast)); }

	ChordIndex		AdjustForInsertion(ChordIndex x, ChordIndex insert, ChordIndex count);
	ChordIndex		AdjustForDeletion(ChordIndex c, ChordIndex newSize, ChordIndex startDel, ChordIndex delSize);

	void			AddNoteToQueue(UInt16 tone, UInt64 when, UInt16 string=100, UInt16 fret=100);
	inline void		PlayArpeggio(const FPChord &chord)			{ PlayArpeggio(chord.tones, MIN_NOTE(chord.root, chord.key), true); }
	void			PlayArpeggio(UInt16 toneMask, UInt16 startTone, bool fast=false, UInt16 count=0);
	void			PlayNote(PartIndex part, UInt16 tone, bool novisual=false);
	void			StrumChord(const FPChord &chord, bool reverse=false);
	void			StrikeChord(const FPChord &chord);
	void			PickString(UInt16 string);

	// Part data accessors
	inline PartIndex	CurrentPart()							{ return recentPart; }

#if QUICKTIME_SUPPORT
	const inline CFStringRef	GetInstrumentName(PartIndex part=kCurrentChannel) const { return CH[FPART(part)].quickTimeOutput->InstrumentName(); }
#else
	const CFStringRef	GetInstrumentName(PartIndex part=kCurrentChannel) const;
#endif

	inline UInt16		GetInstrumentNumber(PartIndex part=kCurrentChannel) { return CH[FPART(part)].trueGMNumber; }

	UInt16				GetInstrumentGroup(PartIndex part=kCurrentChannel);

	inline UInt16	GetVelocity(PartIndex part=kCurrentChannel)				{ return CH[FPART(part)].velocity; }
	inline UInt16	GetSustain(PartIndex part=kCurrentChannel)				{ return CH[FPART(part)].sustain; }
	inline bool		GetMidiOutEnabled(PartIndex part=kCurrentChannel)		{ return CH[FPART(part)].midiOutEnabled; }
	inline UInt16	GetMidiChannel(PartIndex part=kCurrentChannel)			{ return CH[FPART(part)].outChannel; }
	inline UInt16	GetTransformFlag(PartIndex part=kCurrentChannel)		{ return CH[FPART(part)].transformFlag; }

	inline ChordIndex	PlayingChord()										{ return chordToPlay; }
	void			SetPlayingChord(ChordIndex index);

	inline UInt16	PlayingBeat()											{ return beatToPlay; }
	inline void		PlayRange(ChordIndex &start, ChordIndex &end)			{ start = playFirst; end = playLast; }

	// Part data setters
	inline void		SetTransformFlag(PartIndex part, bool ena)	{ CH[FPART(part)].transformFlag = ena; }
	inline bool		ToggleTransformFlag(PartIndex part)			{ PartIndex p = FPART(part); return (CH[p].transformFlag = !CH[p].transformFlag); }

	// Setting the Instrument
	void			SetInstrument(PartIndex part, long inTrueGM);
	void			FinishSustainingNotes();

	inline void		SetCurrentPart(PartIndex part)				{ recentPart = part; }
	UInt16			PickInstrument(PartIndex part);

	// Channel parameters that apply to all devices
	inline void		SetMidiChannel(PartIndex part, UInt16 c)	{ CH[FPART(part)].outChannel = c; }
	inline void		SetVelocity(PartIndex part, UInt16 newVel)	{ CH[FPART(part)].velocity = newVel; }
	inline void		SetSustain(PartIndex part, UInt16 newSus)	{ CH[FPART(part)].sustain = newSus; }

#if QUICKTIME_SUPPORT

	// QuickTime
	inline bool		GetQuicktimeOutEnabled(PartIndex part=kCurrentChannel)			{ return CH[FPART(part)].qtOutEnabled; }
	void			InitQuickTime();
	void			DisposeQuicktime();
	void			DisposeNoteChannel(PartIndex part);
	inline bool		IsQuicktimeEnabled()						{ return usingQuicktimeOut; }
	void			SetQuicktimeEnabled(bool ena)				{ usingQuicktimeOut = ena; }
	bool			ToggleQuicktime(PartIndex part=kAllChannels) {
						bool qout;
						if (part == kAllChannels) {
							qout = !usingQuicktimeOut;
							SetQuicktimeEnabled(qout);
						}
						else {
							qout = !GetQuicktimeOutEnabled(part);
							SetQuicktimeOutEnabled(part, qout);
						}
						return qout;
					}

	inline void		SetQuicktimeOutEnabled(PartIndex part, bool ena)	{ CH[FPART(part)].qtOutEnabled = ena; }

	void			SetQuickTimeInstrument(SInt16 part, long inTrueGM);
	void			QuicktimeNoteOn(PartIndex part, long tone, long velocity);
	void			QuicktimeNoteOff(PartIndex part, long tone);

#else

	// AU Synth
	void			InitAUSynth();
	void			DisposeAUSynth();
	inline bool		IsAUSynthEnabled()						{ return usingAUSynthOut; }
	inline bool		GetAUSynthOutEnabled(PartIndex part=kCurrentChannel)		{ return CH[FPART(part)].synthOutEnabled; }
	void			SetAUSynthEnabled(bool enable)			{ usingAUSynthOut = enable; }
	inline void		SetAUSynthOutEnabled(PartIndex part, bool ena)	{ CH[FPART(part)].synthOutEnabled = ena; }
	void			SetAUSynthInstrument(SInt16 part, long inTrueGM);
	bool			ToggleAUSynth(PartIndex part=kAllChannels) {
						bool mout;
						if (part == kAllChannels) {
							mout = !usingAUSynthOut;
							SetAUSynthEnabled(mout);
						}
						else {
							mout = !GetAUSynthOutEnabled(part);
							SetAUSynthOutEnabled(part, mout);
						}
						return mout;
					}

#endif

	// CoreMIDI
	void			InitCoreMIDI();
	void			DisposeMidiOutputs(bool notZero=false);
	void			DisposeCoreMIDI();
	inline bool		IsMidiEnabled()								{ return usingMidiOut; }
	inline bool		IsMidiOutputSplit()							{ return splitMidiOut; }

	void			SetMidiEnabled(bool enable);

	bool			ToggleMidi(PartIndex part=kAllChannels);

	void			ToggleSplitMidiOut();

	void			SetMidiOut(PartIndex part, bool ena);

	void			SetMidiInstrument(SInt16 part, long inTrueGM);

	void			StartAnimationTimer();
	void			DisposeAnimationTimer();
	static void		AnimationTimerProc( EventLoopTimerRef timer, void *player );
	void			DoAnimationTimer();

	// Player callback or event loop
#ifdef USE_DEPRECATED_TIMER
	static pascal void PlayerTimerTaskProc(TMTaskPtr taskPtr);
#else
	static void		PlayerTimerCallback(CFRunLoopTimerRef timer, void *info);
#endif
	void			DoPlayerTimer();
	void			StartPlaybackTimer();
	void			DisposePlaybackTimer();

	void			GetRangeToPlay(ChordIndex &start, ChordIndex &end);
	void			PlayDocument();
	void			Stop();

};

extern FPMusicPlayer *player;

#endif
