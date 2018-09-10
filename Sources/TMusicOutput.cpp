/*
 *  TMusicOutput.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/28/08.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPApplication.h"
#include "FPMidiHelper.h"
#include "TMusicOutput.h"
#include "TString.h"
#include <QuickTime/QuickTime.h>

NoteAllocator TMusicOutput::helper_na = NULL;
NoteChannel TMusicOutput::helper_nc = 0;
TString TMusicOutput::nameCache[kLastFauxGS];

TMusicOutput::TMusicOutput() {
	Init(CFSTR("Music Output"));
}

TMusicOutput::TMusicOutput(CFStringRef inName) {
	Init(inName);
}

void TMusicOutput::Init(CFStringRef inName) {
	instrument = 1;
	name = inName;
	bzero(state, sizeof(state));
}

TMusicOutput::~TMusicOutput() {
	if (helper_nc) NADisposeNoteChannel(helper_na, helper_nc);
	if (helper_na) CloseComponent(helper_na);
}

#pragma mark -

void TMusicOutput::NoteStart(Byte channel, Byte tone, UInt16 velocity, SInt64 stopTime) {
	NoteStop(tone);
	NoteOn(channel, tone, velocity);
	state[tone].channel = channel;
	state[tone].isPlaying = true;
	state[tone].noteEnd = stopTime;
}


void TMusicOutput::NoteStop(Byte tone) {
	if (state[tone].isPlaying) {
		state[tone].isPlaying = false;
		NoteOff(state[tone].channel, tone);
	}
}

void TMusicOutput::Instrument(Byte channel, UInt16 inTrueGM) {

	instrument = inTrueGM;

	if (inTrueGM >= kFirstDrumkit && inTrueGM <= kLastDrumkit) {
		BankSelect(channel, 1);
	}
	else if (inTrueGM >= kFirstGSInstrument && inTrueGM <= kLastGSInstrument) {
		UInt16 bank = ((inTrueGM - 1) >> 7) << 8;
		BankSelect(channel, bank);
	}
	else
		BankSelect(channel, 0);

	Program(channel, (inTrueGM - 1) & 0x7F);
}

CFStringRef TMusicOutput::InstrumentName(Byte channel) {
	// use quicktime if possible
	static ComponentResult quickTimeError = noErr;
	NoteRequest nr;

	TString iname(CFSTR("(err)"));
	int didSetName = 0;

	if (instrument > 0) {

		UInt16	fauxGM = FPMidiHelper::TrueGMToFauxGM(instrument),
				cacheIndex = fauxGM - 1;

		// get from our local static cache first
		if (nameCache[cacheIndex].Length()) {
			iname = nameCache[cacheIndex];
			didSetName = 2;
		}
		else if (!quickTimeError) {
			// The first time this is used initialize from QuickTime

			if (!helper_na) {
				quickTimeError = InitializeQuickTimeChannel(helper_na, nr, helper_nc);
			}

			if (helper_na) {
				if ( !NASetInstrumentNumber(helper_na, helper_nc, instrument) && !NAStuffToneDescription(helper_na, instrument, &nr.tone) ) {
					iname.SetFromPascalString(nr.tone.instrumentName);
					didSetName = 1;
				}
			}
		}

		if (!didSetName && false) {
			// Get the instrument name from the menus, if possible
			MenuRef theMenu;
			MenuItemIndex theIndex;
			UInt16 grpnum, index;

			FPMidiHelper::GroupAndIndexForInstrument(instrument, grpnum, index);

			OSStatus err = GetIndMenuItemWithCommandID(NULL, kFPCommandInstGroup, grpnum + 1, &theMenu, &theIndex);
			if (err == noErr) {
				err = GetMenuItemHierarchicalMenu(theMenu, theIndex, &theMenu);
				if (err == noErr) {
					CFStringRef itemName = NULL;
					err = CopyMenuItemTextAsCFString(theMenu, index, &itemName);
					if (itemName) {
						if (err == noErr) {
							iname = itemName;
							didSetName = 1;
						}
						CFRELEASE(itemName);
					}
				}
			}
		}

		// Finally fall back to a non-localized list
		if (!didSetName) {
			if (fauxGM >= kFirstFauxGM && fauxGM <= kLastFauxGM) {
				static const char *names[] = {"Acoustic Grand Piano", "Bright Acoustic Piano", "Electric Grand Piano", "Honkytonk Piano", "Electric Piano", "Chorused Piano", "Harpsichord", "Clavi", "Celesta", "Glockenspiel", "Music Box", "Vibraphone", "Marimba", "Xylophone", "Tubular bells", "Dulcimer", "Drawbar Organ", "Percussive Organ", "Rock Organ", "Church Organ", "Reed Organ", "Accordion", "Harmonica", "Tango Accordion", "Acoustic Nylon Guitar", "Acoustic Steel Guitar", "Electric Jazz Guitar", "Electric Clean Guitar", "Electric Guitar Muted", "Overdriven Guitar", "Distortion Guitar", "Guitar Harmonics", "Acoustic Fretless Bass", "Electric Bass Fingered", "Electric Bass Picked", "Fretless Bass", "Slap Bass 1", "Slap Bass 2", "Synth Bass 1", "Synth Bass 2", "Violin", "Viola", "Cello", "Contrabass", "Tremolo Strings", "Pizzicato Strings", "Orchestral Harp", "Timpani", "Acoustic String Ensemble", "Acoustic String Ensemble 2 ", "SynthStrings 1", "SynthStrings 2", "Aah Choir", "Ooh Choir", "SynthVox", "Orchestra Hit", "Trumpet", "Trombone", "Tuba", "Muted Trumpet", "French Horn", "Brass Section", "Synth Brass 1", "Synth Brass 2", "Soprano Sax", "Alto Sax", "Tenor Sax", "Baritone Sax", "Oboe", "English Horn", "Bassoon", "Clarinet", "Piccolo", "Flute", "Recorder", "Pan Flute", "Bottle Blow", "Shakuhachi", "Whistle", "Ocarina", "Square Wave", "Saw Wave", "Calliope", "Chiffer", "Charang", "Solo Vox", "5th Saw Wave", "Bass & Lead", "Fantasy", "Warm", "Polysynth", "Choir", "Bowed", "Metal", "Halo", "Sweep", "Ice Rain", "Sound Tracks", "Crystal", "Atmosphere", "Brightness", "Goblins", "Echoes", "Space", "Sitar", "Banjo", "Shamisen", "Koto", "Kalimba", "Bag Pipe", "Fiddle", "Shannai", "Tinkle Bell", "Agogo", "Steel Drums", "Woodblock", "Taiko Drum", "Melodic Tom", "Synth Drum", "Reverse Cymbal", "Guitar Fret Noise", "Breath Noise", "Seashore", "Bird Tweet", "Telephone Ring", "Helicopter", "Applause", "Gunshot"};
				iname = names[fauxGM - kFirstFauxGM];
			}
			else if (fauxGM <= kLastFauxDrum) {
				static const char *drum_names[] = {"Standard Kit", "Room Kit", "Power Kit", "Electronic Kit", "Analog Kit", "Jazz Kit", "Brush Kit", "Orchestra Kit", "SFX Kit"};
				iname = drum_names[fauxGM - kFirstFauxDrum];
			}
			else if (fauxGM >= kFirstFauxGS) {
				static const char *gs_backup[] = {"GS SynthBass101", "GS Trombone 2", "GS Fr.Horn 2", "GS Square", "GS Saw", "GS Syn Mallet", "GS Echo Bell", "GS Sitar 2", "GS Gt.Cut Noise", "GS Fl.Key Click", "GS Rain", "GS Dog", "GS Telephone 2", "GS Car-Engine", "GS Laughing", "GS Machine Gun", "GS Echo Pan", "GS String Slap", "GS Thunder", "GS Horse-Gallop", "GS DoorCreaking", "GS Car-Stop", "GS Screaming", "GS Lasergun", "GS Wind", "GS Bird 2", "GS Door", "GS Car-Pass", "GS Punch", "GS Explosion", "GS Stream", "GS Scratch", "GS Car-Crash", "GS Heart Beat", "GS Bubble", "GS Wind Chimes", "GS Siren", "GS Footsteps", "GS Train", "GS Jetplane", "GS Piano 1", "GS Piano 2", "GS Piano 3", "GS Honky-tonk", "GS Detuned EP 1", "GS Detuned EP 2", "GS Coupled Hps.", "GS Vibraphone", "GS Marimba", "GS Church Bell", "GS Detuned Or.1", "GS Detuned Or.2", "GS Church Org.2", "GS Accordion It", "GS Ukulele", "GS 12-str.Gt", "GS Hawaiian Gt.", "GS Chorus Gt.", "GS Funk Gt.", "GS Feedback Gt.", "GS Gt. Feedback", "GS Synth Bass 3", "GS Synth Bass 4", "GS Slow Violin", "GS Orchestra", "GS Syn.Strings3", "GS Brass 2", "GS Synth Brass3", "GS Synth Brass4", "GS Sine Wave", "GS Doctor Solo", "GS Taisho Koto", "GS Castanets", "GS Concert BD", "GS Melo. Tom 2", "GS 808 Tom", "GS Starship", "GS Carillon", "GS Elec Perc.", "GS Burst Noise", "GS Piano 1d", "GS E.Piano 1v", "GS E.Piano 2v", "GS Harpsichord", "GS 60's Organ 1", "GS Church Org.3", "GS Nylon Gt.o", "GS Mandolin", "GS Funk Gt.2", "GS Rubber Bass", "GS AnalogBrass1", "GS AnalogBrass2", "GS 60's E.Piano", "GS Harpsi.o", "GS Organ 4", "GS Organ 5", "GS Nylon Gt.2", "GS Choir Aahs 2"};
					if (fauxGM <= kLastFauxGS)
						iname = gs_backup[fauxGM - kFirstFauxGS];
					else
						iname.SetWithFormat(CFSTR("GS %d"), instrument);
			}
			didSetName = 1;
		}

		if (didSetName == 1) nameCache[cacheIndex] = iname;
	}

	return iname.GetCFString();
}

ComponentResult TMusicOutput::InitializeQuickTimeChannel(NoteAllocator &na, NoteRequest &nr, NoteChannel &nc) {
	ComponentResult err = noErr;
	na = NULL;
	nc = 0;
	NoteRequestInfo	&info = nr.info;
	ToneDescription	&tone = nr.tone;
	
	// Get an instance of the "Note Allocator" component
	na = OpenDefaultComponent(kNoteAllocatorComponentType, 0);
	if (na) {
		/**
		 *	Prepare the note info fields
		 *
		 *	UInt8					flags;
		 *	NoteRequestMIDIChannel	midiChannelAssignment;
		 *	BigEndianShort			polyphony;
		 *	BigEndianFixed			typicalPolyphony;
		 *
		 */
		info.flags = 0;
		info.midiChannelAssignment = kNoteRequestSpecifyMIDIChannel | 1;
		
#if defined(__BIG_ENDIAN__)
		info.polyphony = THE_POLY;
		info.typicalPolyphony = (TYPICAL_POLY << 16);
#else
		info.polyphony.bigEndianValue = (short)EndianS16_NtoB(THE_POLY);
		info.typicalPolyphony.bigEndianValue = (Fixed)EndianS32_NtoB(TYPICAL_POLY << 16);
#endif
		/**
		 *	Prepare the tone descriptor - filling in these values:
		 *
		 *	BigEndianOSType     synthesizerType;
		 *	Str31               synthesizerName;
		 *	Str31               instrumentName;
		 *	BigEndianLong       instrumentNumber;
		 *	BigEndianLong       gmNumber;
		 *
		 */
		err = NAStuffToneDescription(na, 1, &tone);
		if (!err) {
			err = NANewNoteChannel(na, &nr, &nc);
			if (err || !nc) {
				SInt16	poly;
				SInt32	tpoly, inst;
				UInt32	stype, gmnum;
				#if defined(__BIG_ENDIAN__)
					poly = info.polyphony;
					tpoly = info.typicalPolyphony;
					stype = tone.synthesizerType;
					inst = tone.instrumentNumber;
					gmnum = tone.gmNumber;
				#else
					poly = EndianS16_BtoN(info.polyphony.bigEndianValue);
					tpoly = EndianS32_BtoN(info.typicalPolyphony.bigEndianValue);
					stype = EndianU32_BtoN(tone.synthesizerType.bigEndianValue);
					inst = EndianS32_BtoN(tone.instrumentNumber.bigEndianValue);
					gmnum = EndianU32_BtoN(tone.gmNumber.bigEndianValue);
				#endif
				fprintf(stderr, "NANewNoteChannel Error %d | NoteChannel %ld\r\n", (int)err, nc);
				fprintf(stderr, "NoteRequest info: %02X | %d | %d | %08lX\r\n", info.flags, info.midiChannelAssignment, poly, tpoly);
				fprintf(stderr, "NoteRequest tone: %08lX | %s | %s | %ld | %ld\r\n", stype, &tone.synthesizerName[1], &tone.instrumentName[1], inst, gmnum);
			}
			// provide an error if nc is 0
			if (!err && !nc) err = kFPErrorNoteChannel;
		}
		else {
			fprintf(stderr, "NAStuffToneDescription Error %d | instr=%ld\r\n", (int)err, 1L);
		}
		
	}
	return err;
}

#pragma mark -

void TMusicOutput::StopExpiredNotes(Byte stoplist[]) {
	UInt64			time64 = Micro64();
	SInt64			ot;

	for (int tone=0; tone < SYNTH_OCTAVES * OCTAVE; tone++) {
		if (state[tone].isPlaying) {
			ot = state[tone].noteEnd;
			if (S64Compare(time64,ot)>=0) {
				NoteStop(tone);
				stoplist[tone] = 1;
			}
		}
	}
}


void TMusicOutput::StopAllNotes() {
	for (int tone=0; tone < SYNTH_OCTAVES * OCTAVE; tone++)
		NoteStop(tone);
}


