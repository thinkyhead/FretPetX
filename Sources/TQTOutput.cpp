/*
 *  TQTOutput.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/25/11.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TQTOutput.h"
#include "TError.h"
#include <QuickTime/QuickTime.h>

TQTOutput::TQTOutput(UInt16 index) {
	err = noErr;
	na = NULL;
	nc = 0;
	NoteRequestInfo	&info = nr.info;
	ToneDescription	&tone = nr.tone;
	
	// Create a channel
	err = InitializeQuickTimeChannel(na, nr, nc);
	if (err != noErr) {
		DisposeNoteChannel();
		throw TError(err, CFSTR("Unable to initialize Quicktime."));
	}
}

TQTOutput::~TQTOutput() {
	StopAllNotes();
	DisposeNoteChannel();
}

void TQTOutput::DisposeNoteChannel() {
	if (nc) NADisposeNoteChannel(na, nc), nc = 0;
	if (na) CloseComponent(na), na = 0;
}

void TQTOutput::NoteOn(Byte channel, Byte tone, UInt16 velocity) {
	#pragma unused(channel)
	NAPlayNote(na, nc, tone, velocity);
}

void TQTOutput::NoteOff(Byte channel, Byte tone) {
	#pragma unused(channel)
	NAPlayNote(na, nc, tone, 0);
}

void TQTOutput::Instrument(Byte channel, UInt16 program) {
	#pragma unused(channel)
	TMusicOutput::Instrument(channel, program);
	if ( ! (err = NASetInstrumentNumber(na, nc, program)) ) {
		err = NAStuffToneDescription(na, program, &nr.tone);
	}
}

CFStringRef TQTOutput::InstrumentName(Byte channel) {
	#pragma unused(channel)
	return CFStringCreateWithPascalString(
	  kCFAllocatorDefault,
	  nr.tone.instrumentName,
	  kCFStringEncodingMacRoman
	);
}
