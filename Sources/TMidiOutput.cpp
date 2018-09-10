/*
 *  TMidiOutput.cpp
 *  FretPetX
 *
 *  Created by Scott Lahteine on 6/28/08.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TMidiOutput.h"
#include "TError.h"

#include "FPMusicPlayer.h"

#include <CoreMIDI/MIDIServices.h>

#if TEST_MIDI_PORTS
static void GotMIDINotification(const MIDINotification *message, void *refCon) {
	switch (message->messageID) {
		case kMIDIMsgObjectAdded:
		case kMIDIMsgObjectRemoved:
		{
			MIDIObjectAddRemoveNotification *msg = (MIDIObjectAddRemoveNotification*)message;
			if (msg->childType == kMIDIObjectType_Destination) {
				// add or remove the endpoint from our list
				if (msg->parent == NULL) {
					// remove from the list
					// revert outputs set for this to CoreMIDI "FretPet 1" or -NONE-
					// -- also remember the last destinations selected
					//		so when reconnected it can be auto-selected
				}
				else {
					// add to the list
				}
			}
		}
			break;
		default:
			break;
	}
}
#endif

TMidiOutput::TMidiOutput(MIDIClientRef ref, UInt16 index, MIDIEndpointRef inMidiPort) {
	ownMidiPort = (inMidiPort == NULL);

	if (ownMidiPort) {
		OSStatus	err;
		name.SetWithFormat(CFSTR("FretPet %d"), index + 1);
		err = MIDISourceCreate( ref, name.GetCFStringRef(), &midiOutPort );
		verify_action(err==noErr, throw TError(err, CFSTR("Unable to create MIDI channel.")) );
	} else {
		midiOutPort = inMidiPort;
	}

#if TEST_MIDI_PORTS
	// TEST!
	this->ScanForOutputs();
#endif
}


TMidiOutput::~TMidiOutput() {
	StopAllNotes();

	if (ownMidiPort)
		MIDIEndpointDispose(midiOutPort);
}


#pragma mark -
void TMidiOutput::SendMidiEvent(Byte *event, UInt16 length) {
	static Byte packetBuffer[sizeof(MIDIPacketList) + sizeof(MIDIPacket) * 9];	// up to ten packets
	
	// MIDIPacketList is an open-ended struct
	MIDIPacketList	*pktlist = (MIDIPacketList*)packetBuffer;
	
	// Initialize the packet to empty
	MIDIPacket		*curPacket = MIDIPacketListInit(pktlist);
	
	// Insert the data and update the packet index
	curPacket = MIDIPacketListAdd(pktlist, sizeof(packetBuffer), curPacket, 0, length, event);
	
	// Send the packet out right away
	MIDIReceived( midiOutPort, pktlist );
}

void TMidiOutput::NoteOn(Byte channel, Byte tone, UInt16 velocity) {
	Byte noteOn[] = { 0x90 | channel, tone, velocity };
	SendMidiEvent(noteOn, sizeof(noteOn));
}


void TMidiOutput::NoteOff(Byte channel, Byte tone) {
	Byte noteOff[] = { 0x80 | channel, tone, 64 };
	SendMidiEvent(noteOff, sizeof(noteOff));
}


void TMidiOutput::ControlChange(Byte channel, Byte k, Byte value) {
	Byte controlChange[] = { 0xB0 | channel, k, value };
	SendMidiEvent(controlChange, sizeof(controlChange));
}


void TMidiOutput::BankSelect(Byte channel, UInt16 bank) {
	ControlChange(channel, 0x00, bank >> 8);
	ControlChange(channel, 0x20, bank & 0xFF);
}


void TMidiOutput::Aftertouch(Byte channel, Byte note, Byte velocity) {
	Byte aftertouch[] = { 0xA0 | channel, note, velocity };
	SendMidiEvent(aftertouch, sizeof(aftertouch));
}


void TMidiOutput::Program(Byte channel, Byte program) {
	Byte progChange[] = { 0xC0 | channel, program };
	SendMidiEvent(progChange, sizeof(progChange));
}


void TMidiOutput::MetaEvent(Byte event, Byte l) {
	Byte metaEvent[] = { 0xFF, event, l };
	SendMidiEvent(metaEvent, sizeof(metaEvent));
}


void TMidiOutput::Instrument(Byte channel, UInt16 inTrueGM) {
	
	TMusicOutput::Instrument(channel, inTrueGM);
	
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

/*
	stringref	kMIDIPropertyName
	stringref	kMIDIPropertyManufacturer
	stringref	kMIDIPropertyModel
	int			kMIDIPropertyUniqueID
	int			kMIDIPropertyDeviceID
	bitmap		kMIDIPropertyReceiveChannels
	bitmap		kMIDIPropertyTransmitChannels
	int			kMIDIPropertyMaxSysExSpeed
	int			kMIDIPropertyAdvanceScheduleTimeMuSec
	int(0/1)	kMIDIPropertyIsEmbeddedEntity
	int(0/1)	kMIDIPropertyIsBroadcast
	int			kMIDIPropertySingleRealtimeEntity
	int/data	kMIDIPropertyConnectionUniqueID
	int(0/1)	kMIDIPropertyOffline
	int(0/1)	kMIDIPropertyPrivate
	stringref	kMIDIPropertyDriverOwner
	data(alias)	kMIDIPropertyFactoryPatchNameFile		*** DEPRECATED, use kMIDIPropertyNameConfiguration
	data(alias)	kMIDIPropertyUserPatchNameFile			*** DEPRECATED, use kMIDIPropertyNameConfiguration
	dictionary	kMIDIPropertyNameConfiguration
	stringref	kMIDIPropertyImage						; posix path to the device image icon file
	int			kMIDIPropertyDriverVersion
	int(0/1)	kMIDIPropertySupportsGeneralMIDI
	int(0/1)	kMIDIPropertySupportsMMC
	int(0/1)	kMIDIPropertyCanRoute
	int(0/1)	kMIDIPropertyReceivesClock
	int(0/1)	kMIDIPropertyReceivesMTC
	int(0/1)	kMIDIPropertyReceivesNotes
	int(0/1)	kMIDIPropertyReceivesProgramChanges
	int(0/1)	kMIDIPropertyReceivesBankSelectMSB
	int(0/1)	kMIDIPropertyReceivesBankSelectLSB
	int(0/1)	kMIDIPropertyTransmitsClock
	int(0/1)	kMIDIPropertyTransmitsMTC
	int(0/1)	kMIDIPropertyTransmitsNotes
	int(0/1)	kMIDIPropertyTransmitsProgramChanges
	int(0/1)	kMIDIPropertyTransmitsBankSelectMSB
	int(0/1)	kMIDIPropertyTransmitsBankSelectLSB
	int(0/1)	kMIDIPropertyPanDisruptsStereo
	int(0/1)	kMIDIPropertyIsSampler
	int(0/1)	kMIDIPropertyIsDrumMachine
	int(0/1)	kMIDIPropertyIsMixer
	int(0/1)	kMIDIPropertyIsEffectUnit
	int(0/1)	kMIDIPropertyMaxReceiveChannels
	int(0/1)	kMIDIPropertyMaxTransmitChannels
	int(0/1)	kMIDIPropertyDriverDeviceEditorApp
	int(0/1)	kMIDIPropertySupportsShowControl
	stringref	kMIDIPropertyDisplayName					*** sometimes NULL?

 */
#if TEST_MIDI_PORTS
void TMidiOutput::ScanForOutputs() {
	static Boolean didScan = false; if (didScan) return; didScan = true;
	CFStringRef device_name, entity_name, source_name, destination_name;

	OSStatus status = MIDIRestart();	// ask the system to rescan ports
	TString aString;

	// loop through all the devices
	ItemCount device_count = MIDIGetNumberOfDevices();
	for (ItemCount device_id=0; device_id<device_count; device_id++) {
		MIDIDeviceRef device = MIDIGetDevice(device_id);

		// Get the device properties and display them
		CFPropertyListRef device_props;
		status = MIDIObjectGetProperties(device, &device_props, false);		// all

		status = MIDIObjectGetStringProperty(device, kMIDIPropertyName, &device_name);
		aString = TString("Device ") + TString(device_id) + TString(": ") + TString(device_name) + TString("\n");
		aString.Print();

		// Devices have Entities, which are groups of Endpoints
		ItemCount entities_count = MIDIDeviceGetNumberOfEntities(device);
		for (ItemCount entity_id=0; entity_id<entities_count; entity_id++) {
			MIDIEntityRef entity = MIDIDeviceGetEntity(device, entity_id);

			// Get the entity properties and display them
			status = MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &entity_name);
			aString = TString("    Entity ") + TString(entity_id) + TString(": ") + TString(entity_name) + TString("\n");
			aString.Print();

			// Endpoints from which we can receive MIDI data
			ItemCount source_count = MIDIEntityGetNumberOfSources(entity);
			for (ItemCount source_id=0; source_id<source_count; source_id++) {
				MIDIEndpointRef source = MIDIEntityGetSource(entity, source_id);

				// Get the Endpoint properties and display them
				status = MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &source_name);
				aString = TString("              << ") + TString(source_id) + TString(": ") + TString(source_name) + TString("\n");
				aString.Print();
			}

			// Endpoints to which we can send MIDI data
			ItemCount dest_count = MIDIEntityGetNumberOfDestinations(entity);
			for (ItemCount dest_id=0; dest_id<source_count; dest_id++) {
				MIDIEndpointRef destination = MIDIEntityGetDestination(entity, dest_id);

				// Get the Endpoint properties and display them
				status = MIDIObjectGetStringProperty(entity, kMIDIPropertyName, &destination_name);
				aString = TString("              >> ") + TString(dest_id) + TString(": ") + TString(destination_name) + TString("\n");
				aString.Print();
			}
		}
	}

	// External devices can be selected separately
	
	// loop through all the external devices
	ItemCount ext_device_count = MIDIGetNumberOfExternalDevices();
	for (ItemCount device_id=0; device_id<ext_device_count; device_id++) {
		MIDIDeviceRef device = MIDIGetExternalDevice(device_id);
		// Get the device properties and display them
		
		// Devices have Entities, which are groups of Endpoints
		ItemCount entities_count = MIDIDeviceGetNumberOfEntities(device);
		for (ItemCount entity_id=0; entity_id<entities_count; entity_id++) {
			MIDIEntityRef entity = MIDIDeviceGetEntity(device, entity_id);
			// Get the entity properties and display them
			
			// Endpoints from which we can receive MIDI data
			ItemCount source_count = MIDIEntityGetNumberOfSources(entity);
			for (ItemCount source_id=0; source_id<source_count; source_id++) {
				MIDIEndpointRef source = MIDIEntityGetSource(entity, source_id);
				// Get the Endpoint properties and display them
			}
			
			// Endpoints to which we can send MIDI data
			ItemCount dest_count = MIDIEntityGetNumberOfDestinations(entity);
			for (ItemCount dest_id=0; dest_id<source_count; dest_id++) {
				MIDIEndpointRef destination = MIDIEntityGetDestination(entity, dest_id);
				// Get the Endpoint properties and display them
			}
		}
	}
	
	// It's also possible to work backwards from the Endpoints

	// Endpoints from which we can receive MIDI data
	ItemCount source_count = MIDIGetNumberOfSources();
	for (ItemCount source_id=0; source_id<source_count; source_id++) {
		MIDIEndpointRef source = MIDIGetSource(source_id);

		// Get the Endpoint properties and display them
		status = MIDIObjectGetStringProperty(source, kMIDIPropertyName, &entity_name);
		aString = TString(entity_name);

		// Its Entity
		MIDIEntityRef entity;
		OSStatus status = MIDIEndpointGetEntity(source, &entity);
		// Get the entity properties and display them

		// The Entity's Device
		MIDIDeviceRef device;
		status = MIDIEntityGetDevice(entity, &device);

		if (device != NULL) {
			// Get the device properties and display them
			status = MIDIObjectGetStringProperty(device, kMIDIPropertyName, &device_name);
			TString devString = TString(device_name);
			
			if (devString != aString) {
				aString = devString + TString(" ") + aString;
			}
		}

		TString outString = TString("[Source] ") + aString + TString("\n");
		outString.Print();
	}
	
	// Endpoints to which we can send MIDI data
	ItemCount dest_count = MIDIGetNumberOfDestinations();
	for (ItemCount dest_id=0; dest_id<dest_count; dest_id++) {
		MIDIEndpointRef destination = MIDIGetDestination(dest_id);

		// Get the Endpoint properties and display them
		status = MIDIObjectGetStringProperty(destination, kMIDIPropertyName, &entity_name);
		aString = TString(entity_name);

		// Its Entity
		MIDIEntityRef entity;
		OSStatus status = MIDIEndpointGetEntity(destination, &entity);
		// Get the entity properties and display them

		// The Entity's Device
		MIDIDeviceRef device;
		status = MIDIEntityGetDevice(entity, &device);

		if (device != NULL) {
			// Get the device properties and display them
			status = MIDIObjectGetStringProperty(device, kMIDIPropertyName, &device_name);
			TString devString = TString(device_name);
			
			if (devString != aString) {
				aString = devString + TString(" ") + aString;
			}
		}
		
		TString outString = TString("[Destination] ") + aString + TString("\n");
		outString.Print();
	}
	
}
#endif

