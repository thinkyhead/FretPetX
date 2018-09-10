/*
 *  TSynthOutput.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/28/08.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TSynthOutput.h"
#include "TError.h"
#include <AudioToolbox/AudioToolbox.h>
#include <AudioUnit/AudioUnitProperties.h>

TSynthOutput::TSynthOutput(UInt16 index, AUGraph graph, AUNode inSynthNode) {
	// create an AudioUnit for the synth node
	theNode = inSynthNode;
	OSStatus err = AUGraphNodeInfo(graph, inSynthNode, NULL, &synth);
	verify_action(err==noErr, throw TError(err, CFSTR("Unable to create AU Synth.")));
}


TSynthOutput::~TSynthOutput() {
	StopAllNotes();
}

void TSynthOutput::SendMidiEvent(Byte *event, UInt16 length) {
	switch (length) {
		case 1:
			(void)MusicDeviceMIDIEvent(synth, event[0], 0, 0, 0);
			break;
		case 2:
			(void)MusicDeviceMIDIEvent(synth, event[0], event[1], 0, 0);
			break;
		case 3:
			(void)MusicDeviceMIDIEvent(synth, event[0], event[1], event[2], 0);
			break;
		case 4:
			(void)MusicDeviceMIDIEvent(synth, event[0], event[1], event[2], event[3]);
			break;
		default: break;
	}
}

/*
 // So far the only way to get this is to go straight to QT
 // and then if QT fails to use the Instrument Popup Menu
 // NO FALLBACK TO A STRING LIST ELSEWHERE
CFStringRef TSynthOutput::InstrumentName(Byte channel) {
	CFStringRef myName;
	UInt32 len;
	OSStatus err = AudioUnitGetProperty(synth, kMusicDeviceProperty_BankName,
						 kAudioUnitScope_Global, channel + 1, (void*)&myName, &len);
	return myName;
}
*/


/*! The Synth Name comes from Audio Unit properties
 */
CFStringRef TSynthOutput::SynthName() {
	CFStringRef myName;
	UInt32 len;
	OSStatus err = AudioUnitGetProperty(synth,
		kMusicDeviceProperty_BankName, kAudioUnitScope_Global, 1,
		(void*)&myName, &len
	);
	return myName;
}
