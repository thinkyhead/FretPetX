/*!
	@file TMusicOutput.h

	@brief An abstract base class for a music output device.

	Music Devices are objects that receive tones and play them
	back in real-time. A music device may do anything with the
	notes it receives - not necessarily playing them back. The
	MidiOutput sub-class, for example, only sends the tones to
	the application's output port. The SynthOutput sub-class
	plays tones on the built-in DLS synthesizer.

	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
 */

#ifndef TMUSICOUTPUT_H
#define TMUSICOUTPUT_H

#define SYNTH_OCTAVES	12

#include "TString.h"
#include <QuickTime/QuickTime.h>

//! The state of a note in the MIDI channel.
typedef struct {
	UInt16			channel;		//!< The channel the note was played on
	SInt64			noteEnd;		//!< time of note end, in microseconds
	bool			isPlaying;		//!< a flag that the tone is playing
} NoteState;

//! A rough MIDI output port wrapper.
class TMusicOutput {
private:
	static TString nameCache[kLastFauxGS];

public:
	NoteState state[SYNTH_OCTAVES * OCTAVE];			//!< The state of all notes in the output

protected:
	static NoteAllocator helper_na;
	static NoteChannel helper_nc;

	TString		name;									//!< A readable name for this output
	Byte		outChannel;								//!< The (MIDI) channel assignment
	UInt16		instrument;								//!< The last instrument that was set

public:

	/*! Default Constructor.
	 */
	TMusicOutput();

	/*! Constructor.
		Multiple outputs can share a single CoreMIDI output port
		If no port is supplied the output creates its own.
		@param ref the client that will own this output
		@param index the number of the output - used in naming it
		@param inName a name for the output
	*/
	TMusicOutput(CFStringRef inName);

	virtual ~TMusicOutput();

	virtual void Init(CFStringRef inName);
	
	/*! Setter for the name property
	 */
	inline void SetName(CFStringRef inName) { name = inName; }
	inline void SetName(TString &inName) { name = inName; }
	inline void SetName(TString *inName) { name = *inName; }

	/*! Get the name of the current instrument
	 */
	virtual CFStringRef InstrumentName(Byte channel=0);

	/*! A utility function to prep a QuickTime channel
	 */
	static ComponentResult InitializeQuickTimeChannel(NoteAllocator &na, NoteRequest &nr, NoteChannel &nc);

	/*! Start a MIDI note that will play for some duration.
		To stop notes played with this method, periodically
		call StopExpiredNotes.
		@param channel the channel for the note start event
		@param tone the tone for the note start event
		@param velocity the velocity for the note start event
		@param stopTime the microsecond to stop at
	 */
	virtual void NoteStart(Byte channel, Byte tone, UInt16 velocity, SInt64 stopTime);


	/*! Stop a note played through NoteStart.
		@param tone the tone to stop
	*/
	virtual void NoteStop(Byte tone);


	/*! Stop all expired notes played through NoteStart.
		@param stoplist a place to flag which notes ended
	*/
	virtual void StopExpiredNotes(Byte stoplist[]);


	//! Stop all notes played through NoteStart.
	virtual void StopAllNotes();


	/*! Send a Note On event right away.
		@param channel the channel on which to play the note
		@param tone the note to play
		@param velocity the velocity at which to play the note
	*/
	virtual void NoteOn(Byte channel, Byte tone, UInt16 velocity) = 0;


	/*! Send a Note Off event right away.
		@param channel the channel on which to stop the note
		@param tone the note to stop
	*/
	virtual void NoteOff(Byte channel, Byte tone) = 0;


	/*! Send a Control Change event.
		@param channel the channel on which to send the event
		@param control the control to change
		@param value the value to set
	*/
	virtual void ControlChange(Byte channel, Byte control, Byte value) = 0;


	/*! Send a Bank Select event.
		@param channel the channel on which to send the event
		@param bank the bank to select
	*/
	virtual void BankSelect(Byte channel, UInt16 bank) = 0;

	/*! Send an Aftertouch event.
		@param channel the channel to modify
		@param note the note to aftertouch
		@param velocity the velocity of the aftertouch
	*/
	virtual void Aftertouch(Byte channel, Byte note, Byte velocity) = 0;

	/*! Send a Program Change event.
		@param channel the channel to modify
		@param program the program number to set
	*/
	virtual void Program(Byte channel, Byte program) = 0;

	/*! Set the instrument appropriately - handles GS and drums.
		@param channel the channel to modify
		@param program the program number to set
	*/
	virtual void Instrument(Byte channel, UInt16 program);

	/*! Send a Meta event header.
		@param event the event code
		@param length the event length
	*/
	virtual void MetaEvent(Byte event, Byte length) = 0;

};

#endif
