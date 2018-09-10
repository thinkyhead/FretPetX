/**
 *  FPMusicPlayer.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

/*
 TODO: External Sync
 
 External Sync
 A parameter found in many keyboards, synthesizers, and drum machines that have
 built in sequencers. It allows the user to synchronize the sequencer in the
 device to some external sequencer or clock source. The other choice is obviously
 internal sync, where the device uses its own clock and tempo information for
 timing. When set to external sync (sometimes called MIDI sync) it essentially
 becomes a slave and waits for MIDI Clock <1> signals to come in over MIDI, which
 it then follows. For example, one could program a drum part in a drum machine,
 but do all of the keyboard parts in the sequencer of a keyboard while the sequencer
 is slaved to the drum machine's MIDI clock. When the user starts the drum machine,
 MIDI clock signals are transmitted to the sequencer in the keyboard, which makes
 it follow in time. Tempo settings need not be set in the slave device because it
 will simply follow the master. This is all distinctly different that synchronizing
 over MIDI using MIDI Time Code (MTC) <2> where the only thing transmitted and
 received is a raw timing signal that has no relationship with the music. In that
 case tempo and song start time must be set in the slave device.
 
 1. MIDI Clock
 A MIDI timing reference signal used to synchronize pieces of equipment together.
 MIDI clock runs at a rate of 24 ppqn (pulses per quarter note). This means that
 the actual speed of the MIDI clock varies with the tempo of the clock generator
 (as contrasted with time code, which runs at a constant rate). Also note that
 MIDI clock does not carry any location information - the receiving device
 does not know what measure or beat it should be playing at any given time,
 just how fast it should be going.
 
 2. MIDI Time Code (MTC)
 A form of time code representing real time in Hours: Minutes: Seconds: Frames:
 Subframes, and transmitted over MIDI. MTC can also be described as a way of
 sending SMPTE time code over MIDI cables. Like all forms of time code, MTC is
 designed to allow various pieces (in this case MIDI-equipped) of equipment to
 synchronize together.
 
 */

#include "FPApplication.h"
#include "FPGuitarPalette.h"
#include "FPCirclePalette.h"
#include "FPMusicPlayer.h"
#include "FPMidiHelper.h"
#include "FPPreferences.h"
#include "FPDocWindow.h"
#include "TSynthOutput.h"
#include "TMidiOutput.h"
#include "TError.h"


#define	LOWEST_C		36
#define SUSTAIN_FACTOR	(MICROSECOND/60)
#define SLOW_ARPEGGIO	(MICROSECOND/6)
#define FAST_ARPEGGIO	(MICROSECOND/12)
#define STRUM_ARPEGGIO	(MICROSECOND/20)

#define TIMER_DELAY			1			// 1/1000th of a second

#define kPrefSplitOutputs	CFSTR("splitOutputs")

/*!
 *  FPMusicPlayer				* CONSTRUCTOR *
 */
FPMusicPlayer::FPMusicPlayer() {

#if QUICKTIME_SUPPORT
	usingQuicktimeOut = true;
#else
	usingAUSynthOut	= true;
#endif
	
	animationLoop	= NULL;
	usingMidiOut	= true;
	splitMidiOut	= preferences.GetBoolean(kPrefSplitOutputs, FALSE);
	midiClient		= NULL;
	
	playDoc			= NULL;
	playWindow		= NULL;
	playFlag		= false;
	playFirst		= 0;
	playLast		= -1;
	stringToPlay	= 0;
	beatToPlay		= 0;
	chordToPlay		= 0;
	repeatToPlay	= 0;
	
	prevBeat		= 0;
	abortPlay		= false;
	justHearFlag	= false;
	lastTime		= 0;
	
	for (PartIndex p=0; p < TOTAL_PARTS; p++) {
		
		static short init[] = { 26, 1, 17, 33 };
		ChannelInfo	&ch = CH[p];
		
		ch.outChannel		= p;
		ch.trueGMNumber		= (p < DOC_PARTS) ? init[p] : p * 16 + 1;
		ch.sustain			= BASE_SUSTAIN;
		ch.velocity			= BASE_VELOCITY;
		ch.transformFlag	= true;
		
		ch.midiOutput		= NULL;
		ch.midiOutEnabled	= true;
		
#if QUICKTIME_SUPPORT
		ch.quickTimeOutput	= NULL;
		ch.qtOutEnabled		= true;
#else
		ch.synthOutput		= NULL;
		ch.synthOutEnabled	= true;
#endif
	}
	
#if QUICKTIME_SUPPORT
	InitQuickTime();
#else
	InitAUSynth();
#endif
	
	InitCoreMIDI();
	
	SetCurrentPart(0);
}

/*!
 *  FPMusicPlayer				* DESTRUCTOR *
 */
FPMusicPlayer::~FPMusicPlayer() {
	DisposeAnimationTimer();
	DisposePlaybackTimer();
	DisposeCoreMIDI();
	
#if QUICKTIME_SUPPORT
	DisposeQuicktime();
#else
	DisposeAUSynth();
#endif
	
}

/*!
 *	SavePreferences
 */
void FPMusicPlayer::SavePreferences() {
	preferences.SetBoolean(kPrefSplitOutputs, false /* IsMidiOutputSplit() */ );
	preferences.Synchronize();
}


/*!
 *	PickInstrument
 */
UInt16 FPMusicPlayer::PickInstrument(PartIndex part) {
	part = FPART(part);
	UInt16	trueGM = CH[part].trueGMNumber;
	
	do {
		ComponentResult err = noErr;
		NoteAllocator	na = NULL;
		NoteChannel		nc = 0L;
		NoteRequest		nr;

		err = TMusicOutput::InitializeQuickTimeChannel(na, nr, nc);
		if (err == noErr) {
			TString pickerMsg;
			pickerMsg.SetLocalized(CFSTR("Choose Instrument"));
			StringPtr pMsg = pickerMsg.GetPascalString();

			err = NAPickInstrument(na, NULL, pMsg, &nr.tone, kPickSameSynth, 0, 0, 0);
			if (!err) {
#if defined(__BIG_ENDIAN__)
			trueGM = nr.tone.gmNumber ? nr.tone.gmNumber : nr.tone.instrumentNumber;
#else
			trueGM = nr.tone.gmNumber.bigEndianValue ? EndianS32_BtoN(nr.tone.gmNumber.bigEndianValue) : EndianS32_BtoN(nr.tone.instrumentNumber.bigEndianValue);
#endif

			}
			delete [] pMsg;
		}

		if (nc) NADisposeNoteChannel(na, nc);
		if (na) CloseComponent(na);
		
	} while (trueGM <= 0);
	
	return trueGM;
}

#pragma mark -

#if QUICKTIME_SUPPORT

/*!
 *	InitQuickTime
 */
void FPMusicPlayer::InitQuickTime() {
	for (PartIndex part=0; part<TOTAL_PARTS; part++) {
		CH[part].quickTimeOutput = new TQTOutput(part);
	}
}

/*!
 *	DisposeQuicktime
 */
void FPMusicPlayer::DisposeQuicktime() {
	for (PartIndex part=0; part<TOTAL_PARTS; part++) {
		TQTOutput *c = CH[part].quickTimeOutput;
		if (c) delete c, CH[part].quickTimeOutput = NULL;
	}
}

/*!
 *	SetQuickTimeInstrument
 */
void FPMusicPlayer::SetQuickTimeInstrument(SInt16 part, long inTrueGM) {
	CH[part].quickTimeOutput->Instrument(part, inTrueGM);
}



#else // !QUICKTIME_SUPPORT


#pragma mark -
/*!
 *	InitAUSynth
 */
void FPMusicPlayer::InitAUSynth() {
	OSStatus result;
	//create the nodes of the graph
	AUNode synthNode, limiterNode, outNode;
	int part;
	
	AudioComponentDescription cd;
	cd.componentManufacturer = kAudioUnitManufacturer_Apple;
	cd.componentFlags = 0;
	cd.componentFlagsMask = 0;
	
	require_noerr(result = NewAUGraph(&graph), home);
	
	cd.componentType = kAudioUnitType_MusicDevice;
	cd.componentSubType = kAudioUnitSubType_DLSSynth;
	
	require_noerr(result = AUGraphAddNode(graph, &cd, &synthNode), home);
	
	cd.componentType = kAudioUnitType_Effect;
	cd.componentSubType = kAudioUnitSubType_PeakLimiter;
	
	require_noerr(result = AUGraphAddNode(graph, &cd, &limiterNode), home);
	
	cd.componentType = kAudioUnitType_Output;
	cd.componentSubType = kAudioUnitSubType_DefaultOutput;
	
	require_noerr(result = AUGraphAddNode(graph, &cd, &outNode), home);
	
	require_noerr(result = AUGraphOpen(graph), home);
	
	require_noerr(result = AUGraphConnectNodeInput(graph, synthNode, 0, limiterNode, 0), home);
	require_noerr(result = AUGraphConnectNodeInput(graph, limiterNode, 0, outNode, 0), home);
	
	// ok we're good to go - get all the Synth objects...
	for (part=0; part<TOTAL_PARTS; part++) {
		CH[part].synthOutput = new TSynthOutput(part, graph, synthNode);
	}
	
	require_noerr(result = AUGraphInitialize(graph), home);
	
	for (part=0; part<TOTAL_PARTS; part++) {
		ChannelInfo &ch = CH[part];
		ch.synthOutput->ControlChange(part, 0x00, 0);
		ch.synthOutput->Program(part, ch.trueGMNumber);
	}
	require_noerr(result = AUGraphStart(graph), home);
	
home:
	verify_action(result==noErr, throw TError(result, CFSTR("Unable to initialize AU Synth.")) );
}

/*!
 *	DisposeAUSynth
 */
void FPMusicPlayer::DisposeAUSynth() {
	if (graph) {
		// AUGraphStop(graph); // stop is optional here
		DisposeAUGraph(graph);
		graph = NULL;
		for (PartIndex part=0; part<TOTAL_PARTS; part++) {
			TSynthOutput *synth = CH[part].synthOutput;
			if (synth) {
				delete synth;
				CH[part].synthOutput = NULL;
			}
		}
	}
}

/*!
 *	SetAUSynthTrueInstrument
 */
void FPMusicPlayer::SetAUSynthInstrument(SInt16 part, long inTrueGM) {
	if (usingAUSynthOut && GetAUSynthOutEnabled(part))
		CH[part].synthOutput->Instrument(part, inTrueGM);
}

/*!
 *	GetInstrumentName
 */
const CFStringRef FPMusicPlayer::GetInstrumentName(PartIndex part) const {
	TString name;
	PartIndex truePart = FPART(part);

	// Get the name from the synthesizer output foremost
	// the synth should get this from the synth driver if
	// it can. If that fails, return the name from the
	// master list (the instrument popup)

	// I am about to grab a big list from DLS...!

	// The synth can be asked if it does GM, GS, or DLS...
	// Try to find all installed SoundFonts, instruments, and banks
	// in future versions.

	return CH[truePart].synthOutput->InstrumentName(truePart);
}

UInt16 FPMusicPlayer::GetInstrumentGroup(PartIndex part) {
	UInt16 group, index;
	FPMidiHelper::GroupAndIndexForInstrument(GetInstrumentNumber(part), group, index);
	return group;
}

#endif


#pragma mark -
/*!
 *	InitCoreMIDI
 */
void FPMusicPlayer::InitCoreMIDI() {
	// TODO: Move output dependencies to their classes as statics
	OSStatus	err = noErr;
	MIDIEndpointRef out;
	
	if (midiClient == NULL)
		err = MIDIClientCreate(CFSTR("FretPet"), NULL, NULL, &midiClient);
	
	if (err == noErr) {
		// if this were to re-init (it doesn't in fp)
		DisposeMidiOutputs(true);
		
		// Always create one real output
		if (CH[0].midiOutput == NULL)
			CH[0].midiOutput = new TMidiOutput( midiClient );
		
		// Create the rest as either real or virtual outputs
		out = IsMidiOutputSplit() ? NULL : CH[0].midiOutput->GetMIDIPort();
		for (PartIndex p=1; p<TOTAL_PARTS; p++)
			CH[p].midiOutput = new TMidiOutput(midiClient, p, out);
	}
	
	verify_action(err==noErr, throw TError(err, CFSTR("Unable to initialize CoreMIDI.")) );
}

/*!
 *	DisposeMIDIOutputs
 */
void FPMusicPlayer::DisposeMidiOutputs(bool notZero) {
	for (PartIndex p=(notZero?1:0); p < TOTAL_PARTS; p++) {
		TMidiOutput *port = CH[p].midiOutput;
		if (port != NULL) {
			CH[p].midiOutput = NULL;
			delete port;
		}
	}
}

/*!
 *	DisposeCoreMIDI
 */
void FPMusicPlayer::DisposeCoreMIDI() {
	DisposeMidiOutputs();
	
	if (midiClient) {
		MIDIClientDispose( midiClient );
		midiClient = NULL;
	}
}

/*!
 *	SetMidiInstrument
 */
void FPMusicPlayer::SetMidiInstrument(SInt16 part, long inTrueGM) {
	if (usingMidiOut && GetMidiOutEnabled(part))
		MidiOutForPart(part)->Instrument(CH[part].outChannel, inTrueGM);
}

void FPMusicPlayer::SetMidiEnabled(bool enable) {
	usingMidiOut = enable;
	
	if (enable) {
		for (PartIndex part=DOC_PARTS; part--;) {
			if (GetMidiOutEnabled(part))
				MidiOutForPart(part)->Instrument(GetMidiChannel(part), GetInstrumentNumber(part));
		}
	}
}

bool FPMusicPlayer::ToggleMidi(PartIndex part) {
	bool mout;
	if (part == kAllChannels) {
		mout = !usingMidiOut;
		SetMidiEnabled(mout);
	}
	else {
		mout = !GetMidiOutEnabled(part);
		SetMidiOut(part, mout);
	}
	return mout;
}

void FPMusicPlayer::ToggleSplitMidiOut() {
	splitMidiOut = !splitMidiOut;
	InitCoreMIDI();
}

void FPMusicPlayer::SetMidiOut(PartIndex part, bool ena) {
	PartIndex truePart = FPART(part);
	bool changed = CH[truePart].midiOutEnabled != ena;
	CH[truePart].midiOutEnabled = ena;
	
	if (ena && changed)
		MidiOutForPart(truePart)->Instrument(GetMidiChannel(truePart), GetInstrumentNumber(truePart));
}


#pragma mark -
/*!
 *	SetInstrument
 */
void FPMusicPlayer::SetInstrument(SInt16 part, long inTrueGM) {

	if (part < 0) part = recentPart;

#if QUICKTIME_SUPPORT
	SetQuickTimeInstrument(part, inTrueGM);
#else
	SetAUSynthInstrument(part, inTrueGM);
#endif
	SetMidiInstrument(part, inTrueGM);
	CH[part].trueGMNumber = inTrueGM;
}

#pragma mark -
#ifdef USE_DEPRECATED_TIMER
//-----------------------------------------------
//
//  StartPlaybackTimer
//
void FPMusicPlayer::StartPlaybackTimer() {
	playerTask.task.tmAddr = NewTimerUPP(FPMusicPlayer::PlayerTimerTaskProc);
	playerTask.task.tmWakeUp = 0;
	playerTask.task.tmReserved = 0;
	
	OSErr	err = InstallXTimeTask((QElemPtr) &playerTask);
	err = PrimeTimeTask((QElemPtr) &playerTask, TIMER_DELAY);
}

//-----------------------------------------------
//
//  DisposePlaybackTimer
//
void FPMusicPlayer::DisposePlaybackTimer() {
	(void) RemoveTimeTask((QElemPtr)&playerTask);
	DisposeTimerUPP(playerTask.task.tmAddr);
}

//-----------------------------------------------
//
//	PlayerTimerTaskProc
//
pascal void FPMusicPlayer::PlayerTimerTaskProc(TMTaskPtr taskPtr) {
	player->DoPlayerTimer();
	(void) PrimeTimeTask((QElemPtr)taskPtr, TIMER_DELAY);
}
#else
//-----------------------------------------------
//
//  StartPlaybackTimer
//
void FPMusicPlayer::StartPlaybackTimer() {
	static CFRunLoopTimerContext ctx = { 0, this, NULL, NULL, NULL };
	playerTimer = CFRunLoopTimerCreate(NULL, CFAbsoluteTimeGetCurrent(), TIMER_DELAY / 1000.0, 0, 0, FPMusicPlayer::PlayerTimerCallback, &ctx);
	CFRunLoopAddTimer(CFRunLoopGetCurrent(), playerTimer, kCFRunLoopDefaultMode);
}

//-----------------------------------------------
//
//  DisposePlaybackTimer
//
void FPMusicPlayer::DisposePlaybackTimer() {
	CFRunLoopTimerInvalidate(playerTimer);
	CFRELEASE(playerTimer);
}

//-----------------------------------------------
//
//	PlayerTimerCallback
//
void FPMusicPlayer::PlayerTimerCallback(CFRunLoopTimerRef timer, void *info) {
	((FPMusicPlayer*)info)->DoPlayerTimer();
}
#endif


/*!
 * DoPlayerTimer
 */
void FPMusicPlayer::DoPlayerTimer() {
	UInt64			playTime = Micro64();
	QueueTone		tone;
	
	FinishSustainingNotes();
	
	if (ToneQueue.size()) {
		for (ToneIterator itr = ToneQueue.begin(); itr != ToneQueue.end();) {
			tone = (*itr);
			if (U64Compare(tone.when, playTime) <= 0) {
				PlayNote(tone.part, tone.note);
				
				if (tone.string < 100)
					guitarPalette->TwinkleTone(tone.string, tone.fret);
				
				itr = ToneQueue.erase(itr);
			}
			else
				itr++;
		}
	}
	
	
	//
	// Is the player turned on?
	//
	if (abortPlay || !playFlag /*|| playDoc == NULL*/)
		return;
	
	bool bPlayedOne = false;
	
	ChordIndex total = playDoc->Size();
	if (total == 0) {
		abortPlay = true;
		return;
	}
	
	SInt16		fret, i, last;
	UInt16		beats;
	
	//
	// Sanity check the chord number
	//	to prevent memory faults
	//
	if (chordToPlay >= total)
		chordToPlay = playFirst;
	
	//
	// If beats changed prior to the player
	//	wrap it around
	FPChordGroup	&group = playDoc->ChordGroup(chordToPlay);
	beats = group.PatternSize();
	if (beatToPlay >= beats) {
		beatToPlay %= 4;
		if (beatToPlay >= beats)
			beatToPlay = 0;
	}
	
	//
	// Play either one or all available notes
	//
	while (!bPlayedOne && stringToPlay <= 5) {
		i = stringToPlay++;
		
		for (PartIndex p=0; p<DOC_PARTS; p++) {
			FPChord &chord = group[p];
			
			if (!fretpet->IsSoloModeEnabled() || p == recentPart) {
				if (chord.GetPatternDot(i, beatToPlay)) {
					if ((fret = chord.fretHeld[i]) >= 0) {
						if (p == recentPart)
							guitarPalette->TwinkleTone(i, fret);
						
						PlayNote(p, guitarPalette->Tone(i, fret));
						if (playDoc->TempoMultiplier() == 1)
							bPlayedOne = true;
					}
				}
			}
		}
	}
	
	//
	// If enough time has elapsed then set up for the next note
	//
	
	if (U64Compare(playTime, U64Add(lastTime, playInterim)) >= 0) {		// if ((pt - lt) >= pi)...  if (pt >= pi + lt)))
		lastTime = U64Add(lastTime, playInterim);
		
		stringToPlay = 0;
		
		if (++beatToPlay >= group.PatternSize()) {
			beatToPlay = 0;
			if (playDoc->Interim() != playInterim)
				playInterim = playDoc->Interim();
			
			if (justHearFlag) {
				abortPlay = true;
				return;
			}
			
			if (++repeatToPlay > group.Repeat()) {
				repeatToPlay = 1;
				
				last = playLast;					// when last<0 it means play to the end
				if (last < 0 || last >= total)
					last = total - 1;
				
				UInt16 ch = chordToPlay + 1;
				if (ch >= total || ch > last || ch < playFirst) {
					ch = playFirst;
					if (!fretpet->IsLoopingEnabled() || ch >= total) {
						abortPlay = true;
						return;
					}
				}
				
				chordToPlay = ch;
			}
		}
	}
}

#pragma mark -
/*!
 *  StartAnimationTimer
 */
void FPMusicPlayer::StartAnimationTimer() {
	if (animationLoop == NULL) {
		animationUPP = NewEventLoopTimerUPP(FPMusicPlayer::AnimationTimerProc);
		
		(void)InstallEventLoopTimer(
									GetMainEventLoop(),
									0.0, 1.0 / 30.0,
									animationUPP,
									this,
									&animationLoop
									);
	}
}


/*!
 *  DisposeAnimationTimer
 */
void FPMusicPlayer::DisposeAnimationTimer() {
	if (animationLoop != NULL) {
		(void)RemoveEventLoopTimer(animationLoop);
		DisposeEventLoopTimerUPP(animationUPP);
		animationLoop = NULL;
		animationUPP = NULL;
	}
}


/*!
 *  AnimationTimerProc
 */
void FPMusicPlayer::AnimationTimerProc( EventLoopTimerRef timer, void *player ) {
	((FPMusicPlayer*)player)->DoAnimationTimer();
}


/*!
 *  DoAnimationTimer
 */
void FPMusicPlayer::DoAnimationTimer() {
	if (playFlag) {
		playWindow->FollowPlayer();
		
		if (abortPlay) {
			abortPlay = false;
			fretpet->DoStop();
		}
	}
}


#pragma mark -
/*!
 *	AddNoteToQueue
 */
void FPMusicPlayer::AddNoteToQueue(UInt16 tone, UInt64 when, UInt16 string, UInt16 fret) {
	QueueTone newTone = { when, recentPart, tone, string, fret };
	ToneQueue.push_back(newTone);
}


/*!
 *	PlayArpeggio
 */
void FPMusicPlayer::PlayArpeggio(UInt16 toneMask, UInt16 startTone, bool fast, UInt16 count) {
	UInt64			playTime = Micro64();
	UInt16			sofar = 0;
	UInt64			period = U64SetU(fast ? FAST_ARPEGGIO : SLOW_ARPEGGIO);
	
	UInt16	tone = startTone + OCTAVE;
	for (int i=0; i<OCTAVE; i++) {
		if (toneMask & BIT(tone % OCTAVE)) {
			AddNoteToQueue(tone, playTime);
			playTime = U64Add(playTime, period);
			sofar++;
		}
		tone++;
	}
	
	while (sofar < count) {
		if (toneMask & BIT(tone % OCTAVE)) {
			AddNoteToQueue(tone, playTime);
			playTime = U64Add(playTime, period);
			sofar++;
		}
		tone++;
	}
}


/*!
 *	StrumChord
 */
void FPMusicPlayer::StrumChord(const FPChord &chord, bool reverse) {
	UInt64			playTime = Micro64();
	SInt16			fret;
	UInt64			period = 20000000 / fretpet->CurrentTempo();
	
	for (int i=0; i<NUM_STRINGS; i++) {
		UInt16 string = reverse ? NUM_STRINGS - 1 - i : i;
		fret = chord.FretHeld(string);
		if (fret >= 0) {
			AddNoteToQueue(guitarPalette->Tone(string, fret), playTime, string, fret);
			playTime = U64Add(playTime, period);
		}
	}
}


/*!
 *	StrikeChord
 */
void FPMusicPlayer::StrikeChord(const FPChord &chord) {
	UInt64 playTime = Micro64();
	
	for (int string=NUM_STRINGS; string--;) {
		SInt16 fret = chord.FretHeld(string);
		if (fret >= 0)
			AddNoteToQueue(guitarPalette->Tone(string, fret), playTime, string, fret);
	}
}


/*!
 *	PickString
 */
void FPMusicPlayer::PickString(UInt16 string) {
	SInt16 fret = globalChord.FretHeld(string);
	if (fret >= 0)
		AddNoteToQueue(guitarPalette->Tone(string, fret), 0, string, fret);
}


/*!
 *	PlayNote
 */
void FPMusicPlayer::PlayNote(PartIndex part, UInt16 tone, bool novisual) {
	if (part == kCurrentChannel)
		part = recentPart;
	
	if (part == kAllChannels) {
		for (PartIndex p=DOC_PARTS; p--;)
			PlayNote(p, tone, (p != 0));
	}
	else {
		tone += LOWEST_C;
		
		ChannelInfo &ch = CH[part];
		
		UInt16	velocity = ch.velocity;
		UInt64	sustain = U64Set(ch.sustain * SUSTAIN_FACTOR);	// 60ths * 1M / 60
		UInt64	stopTime = U64Add(Micro64(), sustain);
		
		// Send a NoteStart to the MIDI output
		if (usingMidiOut && GetMidiOutEnabled(part))
			MidiOutForPart(part)->NoteStart(ch.outChannel, tone, velocity, stopTime);
		
#if QUICKTIME_SUPPORT
		// Start the note on QuickTime if enabled
		if (usingQuicktimeOut && GetQuicktimeOutEnabled(part))
			ch.quickTimeOutput->NoteStart(part, tone, velocity, stopTime);
#else
		// Send a NoteStart to the Audio Device
		if (usingAUSynthOut && GetAUSynthOutEnabled(part))
			ch.synthOutput->NoteStart(part, tone, velocity, stopTime);
		
#endif
		if ( part == recentPart && !novisual && (usingMidiOut
#if QUICKTIME_SUPPORT
												 || usingQuicktimeOut
#else
												 || usingAUSynthOut
#endif
												 ) ) {
			fretpet->SetPianoKeyPlaying(tone - LOWEST_C, true);
			circlePalette->TwinkleTone(tone);
		}
	}
}


/*!
 *	FinishSustainingNotes
 */
void FPMusicPlayer::FinishSustainingNotes() {
	static int		part, channel;
	static Byte stoplist[NUM_OCTAVES * OCTAVE];
	
	// Do a single part each pass instead of all 4
	// This means a 1/250th second delay will occur
	// between supposedly synched-up cutoffs. I hope
	// no one notices.
	part = ((part + 1) % TOTAL_PARTS);
	channel = ((channel + 1) % TOTAL_CHANNELS);
	
	// A list to keep track of stopped notes
	bzero(stoplist, sizeof(stoplist));
	
	// Stop notes in MIDI world
	MidiOutForPart(part)->StopExpiredNotes(stoplist);
	
#if QUICKTIME_SUPPORT
	// Stop QuickTime notes
	CH[part].quickTimeOutput->StopExpiredNotes(stoplist);
#else
	// Stop Audio Device notes
	CH[part].synthOutput->StopExpiredNotes(stoplist);
#endif
	
	for (int tone = 0; tone < NUM_OCTAVES * OCTAVE; tone++) {
		if (stoplist[tone])
			fretpet->SetPianoKeyPlaying(tone-LOWEST_C, false);
	}
	
}


/*!
 *	SetPlayingChord
 */
void FPMusicPlayer::SetPlayingChord(ChordIndex index) {
	UInt16 beats = playDoc->ChordGroup(chordToPlay).PatternSize();
	
	if (index > playDoc->Size() - 1)
		index = playDoc->Size() - 1;
	
	chordToPlay = index;
	
	if (beatToPlay >= beats) {
		beatToPlay = beats % 4;
		if (beatToPlay >= beats)
			beatToPlay = 0;
	}
}


#pragma mark -
/*!
 *	AdjustForInsertion
 */
#define moveup(qq)	do { if (qq >= insert) qq += insSize; } while (0)

ChordIndex FPMusicPlayer::AdjustForInsertion(ChordIndex x, ChordIndex insert, ChordIndex insSize) {
	if (IsPlaying()) {
		if (fretpet->IsEditModeEnabled()) {
			playFirst = playLast = chordToPlay = x;
		}
		else {
			moveup(playFirst);
			moveup(playLast);
			moveup(chordToPlay);
			moveup(playWindow->lastChordDrawn);
			moveup(x);
		}
	}
	
	return x;
}


/*!
 *	AdjustForDeletion
 *
 *	example:	playrange = 7-20	cursor = 7
 *				startDel = 7		delSize = 3
 *					
 *	desired:	playrange = 7-17	cursor = 7
 */
#define movedown(qq)	do {									\
if (qq >= startDel)					\
{									\
qq -= delSize;					\
if (qq < startDel)				\
qq = startDel;				\
if (qq >= playDoc->Size())		\
qq = playDoc->Size() - 1;	\
}									\
} while (0)

ChordIndex FPMusicPlayer::AdjustForDeletion(ChordIndex c, ChordIndex newSize, ChordIndex startDel, ChordIndex delSize) {
	if (IsPlaying()) {
		if (newSize) {
			movedown(playFirst);
			movedown(playLast);
			movedown(chordToPlay);
			movedown(playWindow->lastChordDrawn);
			movedown(c);
		}
		else
			fretpet->DoStop();
	}
	
	return c;
}


#pragma mark -
/*!
 *	PlayDocument
 *
 *	Play the active document according to the
 *	toolbar button settings
 */
void FPMusicPlayer::PlayDocument() {
	if (playFlag)
		Stop();
	
	FPDocWindow *wind = FPDocWindow::frontWindow;
	if (wind == NULL) return;
	
	FPDocument *doc = wind->document;
	
	//
	// reset the play flags
	//
	playFlag = justHearFlag = false;
	GetRangeToPlay(playFirst, playLast);
	
	//
	// disable selections when in eye mode
	//
	if (fretpet->IsFollowViewEnabled() && doc->HasSelection())
		wind->SetSelection(-1, false);
	
	//
	// Prepare the player
	//
	playDoc			= doc;
	playWindow		= wind;
	chordToPlay		= playFirst;
	playInterim		= doc->Interim();
	repeatToPlay	= 1;
	beatToPlay		= 0;
	prevBeat		= -1;
	stringToPlay	= 0;
	abortPlay		= false;
	
	lastTime = Micro64();
	
	//
	// Start the player!
	//
	playFlag		= true;
	
	wind->PlayStarted();
	
	//	SyncMetronome();
}


/*!
 *	GetRangeToPlay
 */
void FPMusicPlayer::GetRangeToPlay(ChordIndex &start, ChordIndex &end) {
	FPDocWindow *wind = FPDocWindow::frontWindow;
	if (wind == NULL)
		start = end = -1;
	else {
		FPDocument *doc = wind->document;
		ChordIndex	curs = doc->GetCursor();
		
		if (fretpet->IsFollowViewEnabled() && fretpet->IsEditModeEnabled())
			start = end = curs;
		else {
			if (doc->HasSelection() || fretpet->IsEditModeEnabled())
				(void)doc->GetSelection(&start, &end);
			else {
				start = 0;
				end = -1;
			}
		}
	}
}


/*!
 *	SetPlayRangeFromSelection
 */
void FPMusicPlayer::SetPlayRangeFromSelection() {
	if (playDoc) {
		ChordIndex first, last;
		(void)playDoc->GetSelection(&first, &last);
		SetPlayRange(first, last);
		
		if (chordToPlay < first || chordToPlay > last)
			chordToPlay = first;
	}
}

/*!
 *	Stop
 */
void FPMusicPlayer::Stop() {
	ToneQueue.clear();
	
	playFlag		= false;
	justHearFlag	= false;
	
	if (playWindow != NULL)
		playWindow->PlayStopped();
	
	playDoc			= NULL;
	playWindow		= NULL;
}


