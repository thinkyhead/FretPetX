/*
 *  TSynthOutput.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/24/11.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TSYNTHOUTPUT_H
#define TSYNTHOUTPUT_H

#include "TMidiOutput.h"
#include <AudioToolbox/AUMIDIController.h>
#include <AudioToolbox/AUGraph.h>

//! A wrapper for an Audio Toolbox plugin synthesizer.
class TSynthOutput : public TMidiOutput {
private:
	AUNode theNode;
	AudioUnit synth;

public:

	/*! Constructor.
		Multiple outputs can share a single CoreMIDI output port
		If no port is supplied the output creates its own.
		@param index the number of the output - used in naming it
		@param outSynth - a var to store the synth
	*/
	TSynthOutput(UInt16 index, AUGraph graph, AUNode inSynthNode);

	~TSynthOutput();

	/*! Accessor for the CoreMIDI output port
		@result the CoreMIDI output port
	*/
	AudioUnit	GetSynth() { return synth; }

	/*! Utility to set the name property based on the synth name
	 */
	CFStringRef	SynthName();

//	/*! Get the name of the current instrument
//	 */
//	CFStringRef InstrumentName(Byte channel);

	/*! Send any number of bytes to the MIDI output.
		@param event the buffer to send
		@param length the number of bytes to send
	*/
	virtual void SendMidiEvent(Byte *event, UInt16 length);
};

#endif
