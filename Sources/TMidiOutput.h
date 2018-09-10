/*
 *  TMidiOutput.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/28/08.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TMIDIOUTPUT_H
#define TMIDIOUTPUT_H

#include "TMusicOutput.h"
#include <AudioToolbox/AUMIDIController.h>

#define TEST_MIDI_PORTS 0

//! A rough MIDI output port wrapper.
class TMidiOutput : public TMusicOutput {
private:
	bool			ownMidiPort;			//!< The object manages its own CoreMIDI output
	MIDIEndpointRef	midiOutPort;			//!< The CoreMIDI output port

public:

	/*! Default Constructor.
	 */
	TMidiOutput() { midiOutPort = 0; };

	/*! Constructor.
		Multiple outputs can share a single CoreMIDI output port
		If no port is supplied the output creates its own.
		@param ref the client that will own this output
		@param index the number of the output - used in naming it
		@param inMidiPort an optional output port
	*/
	TMidiOutput(MIDIClientRef ref, UInt16 index=0, MIDIEndpointRef inMidiPort=NULL);

	~TMidiOutput();

	/*! Accessor for the CoreMIDI output port
		@result the CoreMIDI output port
	*/
	MIDIEndpointRef	GetMIDIPort() { return midiOutPort; }

	/*! Send any number of bytes to the MIDI output.
	 @param event the buffer to send
	 @param length the number of bytes to send
	 */
	virtual void SendMidiEvent(Byte *event, UInt16 length);
	
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
	void ControlChange(Byte channel, Byte control, Byte value);


	/*! Send a Bank Select event.
		@param channel the channel on which to send the event
		@param bank the bank to select
	*/
	void BankSelect(Byte channel, UInt16 bank);

	/*! Send an Aftertouch event.
		@param channel the channel to modify
		@param note the note to aftertouch
		@param velocity the velocity of the aftertouch
	*/
	void Aftertouch(Byte channel, Byte note, Byte velocity);

	/*! Send a Program Change event.
		@param channel the channel to modify
		@param program the program number to set
	*/
	void Program(Byte channel, Byte program);

	/*! Send a Meta event header.
		@param event the event code
		@param length the event length
	*/
	void MetaEvent(Byte event, Byte length);

	/*! Pseudo-event to set midi bank + program
	 @param channel the channel to modify
	 @param inTrueGM the instrument number to set
	 */
	void Instrument(Byte channel, UInt16 inTrueGM);

#if TEST_MIDI_PORTS
	/*! Test function to learn how to use MIDI outputs
	 */
	void ScanForOutputs();
#endif

};

#endif
