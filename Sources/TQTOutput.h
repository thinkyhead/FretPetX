/*
 *  TQTOutput.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/25/11.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TQTOUTPUT_H
#define TQTOUTPUT_H

#include "TMusicOutput.h"
#include <QuickTime/QuickTime.h>

//! A wrapper for an Audio Toolbox plugin synthesizer.
class TQTOutput : public TMusicOutput {
private:
	ComponentResult	err;
	NoteAllocator	na;							// Note Allocator provided by Quicktime
	NoteChannel		nc;							// Note Channel provided by Quicktime
	NoteRequest		nr;							// Note Request provided by Quicktime
	bool			qtPlayed[NUM_OCTAVES*OCTAVE];
	SInt64			noteEnd[NUM_OCTAVES*OCTAVE];	// Storage for Note ends

public:

	/*! Constructor.
		Multiple outputs can share a single CoreMIDI output port
		If no port is supplied the output creates its own.
		@param index the number of the output - used in naming it
		@param outSynth - a var to store the synth
	*/
	TQTOutput(UInt16 index);
	~TQTOutput();

	void DisposeNoteChannel();

	/*! The name of the instrument, according to QuickTime
		@result A Pascal String - yecch!
	*/
	CFStringRef InstrumentName(Byte channel=0);

	/*! QuickTime Accessor
		@result A NoteAllocator
	*/
	inline NoteAllocator	GetNoteAllocator()	{ return na; }

	/*! QuickTime Accessor
		@result A NoteChannel
	*/
	inline NoteChannel		GetNoteChannel()	{ return nc; }

	/*! QuickTime Accessor
		@result A NoteRequest
	*/
	inline NoteRequest		GetNoteRequest()	{ return nr; }

	/*! Helper function to set up a note channel
	 @result The error status
	 */
	static ComponentResult InitializeChannel(NoteRequest &nr, NoteAllocator &na, NoteChannel &nc);

	/*! Send a Note On event right away.
		@param channel the channel on which to play the note
		@param tone the note to play
		@param velocity the velocity at which to play the note
	*/
	void NoteOn(Byte channel, Byte tone, UInt16 velocity);


	/*! Send a Note Off event right away.
		@param channel the channel on which to stop the note
		@param tone the note to stop
	*/
	void NoteOff(Byte channel, Byte tone);


	/*! Send a Control Change event.
		@param channel the channel on which to send the event
		@param control the control to change
		@param value the value to set
	*/
	void ControlChange(Byte channel, Byte control, Byte value) {}


	/*! Send a Bank Select event.
		@param channel the channel on which to send the event
		@param bank the bank to select
	*/
	void BankSelect(Byte channel, UInt16 bank) {}

	/*! Send an Aftertouch event.
		@param channel the channel to modify
		@param note the note to aftertouch
		@param velocity the velocity of the aftertouch
	*/
	void Aftertouch(Byte channel, Byte note, Byte velocity) {}

	/*! Send a Program Change event.
		@param channel the channel to modify
		@param program the program number to set
	*/
	void Program(Byte channel, Byte program) {}

	/*! Send a Meta event header.
		@param event the event code
		@param length the event length
	*/
	void MetaEvent(Byte event, Byte length) {}

	/*! Send any number of bytes to the MIDI output.
		@param event the buffer to send
		@param length the number of bytes to send
	*/
	void SendMidiEvent(Byte *event, UInt16 length) {}

	/*! Set the instrument appropriately - handles GS and drums.
	 @param channel the channel to modify
	 @param program the program number to set
	 */
	void Instrument(Byte channel, UInt16 program);

};

#endif
