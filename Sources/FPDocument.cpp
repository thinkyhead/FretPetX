/*!
 *  @file FPDocument.cpp
 *
 *	@brief Implementation of the FPDocument class
 *
 *	FretPet X
 *	Copyright © 2012 Scott Lahteine. All rights reserved.
 */

#include "FPApplication.h"
#include "FPMidiHelper.h"
#include "FPClipboard.h"
#include "FPDocWindow.h"
#include "FPGuitarPalette.h"
#include "FPPreferences.h"
#include "FPScalePalette.h"
#include "FPTuningInfo.h"
#include "FPExportFile.h"
#include "TString.h"
#include "TCarbonEvent.h"
#include "FPHistory.h"

#define DEBUG_MIDI		0
#define TICKS_PER_16TH	60
#define TICKS_PER_4TH	(TICKS_PER_16TH * 4)

//
// File data keys
//
#define kStoredVersion				CFSTR("version")

#define kStoredInfo					CFSTR("info")

#define kStoredLength				CFSTR("length")
#define kStoredTempo				CFSTR("tempo")
#define kStoredTempoX				CFSTR("tempoMultiplier")
#define kStoredScaleMode			CFSTR("scaleMode")
#define kStoredEnharmonic			CFSTR("enharmonic")
#define kStoredTopLine				CFSTR("topLine")
#define kStoredCursor				CFSTR("cursor")
#define kStoredSelection			CFSTR("selectionEnd")
#define kStoredCurrentPart			CFSTR("currentPart")
#define kStoredSoloQT				CFSTR("soloQuicktime")
#define kStoredSoloMIDI				CFSTR("soloMidi")
#define kStoredSoloTrans			CFSTR("soloTransform")
#define kStoredSoloTuneName			CFSTR("tuningName")
#define kStoredTuningTones			CFSTR("tuningTones")

#define kStoredPartsArray			CFSTR("partInfo")
#define kStored_GMNumber			CFSTR("gmNumber")
#define kStored_Instrument			CFSTR("instrument")
#define kStored_Velocity			CFSTR("velocity")
#define kStored_Sustain				CFSTR("sustain")
#define kStored_OutChannel			CFSTR("outChannel")

#if QUICKTIME_SUPPORT
#define kStored_OutQT			CFSTR("outQT")
#else
#define kStored_OutSynth		CFSTR("outQT")
#endif

#define kStored_OutMidi				CFSTR("outMIDI")
#define kStored_TransformFlag		CFSTR("transformFlag")

#define kStoredChordGroups			CFSTR("chordGroups")

#pragma pack(2)

/*!
 * Common fields in the header
 */
typedef struct {
	UInt32			fileType;			//  4 start of the writeable file
	Point			lastPoint;			//  4 the last position of the window
	UInt16			tempo;				//  2 speed of the tune
	UInt16			scaleMode;			//  2 current modal scale
	UInt16			enharmonic;			//  2 current flat-sharp naming
	UInt16			tuning;				//  2 guitar tuning
	UInt16			length;				//  2 how many chords?
	UInt16			topLine;			//  2 first bank chord in window
	SInt16			cursor;				//  2 cursor position
} FileHeadCommon;						// 22

/*!
 * File Format Beta 3
 */
typedef struct {
	SInt16			fauxGMNumber;		//   2 the instrument in FretPet terms
} FileHeadB3;							//   2

/*!
 * Format 14 added multiple voices
 */
typedef struct {
	SInt16			partNum;			//   2 current part number in view
	SInt16			fauxGMNumbers[OLD_NUM_PARTS];	//   8 instruments in FretPet terms
	SInt16			velocity[OLD_NUM_PARTS];		//   8 velocities
	SInt16			sustain[OLD_NUM_PARTS];			//   8 note held
	SInt16			reserved2[OLD_NUM_PARTS];		//   8 the notes of this tuning
} FileHead14;							//  34

/*!
 * Format 21 saves more state and remembers custom tunings
 */
typedef struct {
	SInt16		partNum;					//   2
	SInt16		fauxGMNumbers[OLD_NUM_PARTS];	//   8 four parts in FP 2
	SInt16		velocity[OLD_NUM_PARTS];	//   8
	SInt16		sustain[OLD_NUM_PARTS];		//   8
	
	SInt16		tempoX;					//   2 faster tempo than 600?
	SInt16		reserved2;				//   2
	SInt16		reserved3;				//   2
	SInt16		reserved4;				//   2
	
	Str31		tuningName;				//  32 Name of a Custom Tuning
	SInt16		lowNote[6];				//  12 The lowNotes of the tuning
} FileHead21;							//  78

/*!
 * Format X1 was the first Mac OS X header
 */
typedef struct {
	UInt16		tempoX;					//   2 Tempo multipler (1 or 2)
	SInt16		partNum;				//   2 The current part number of the document
	partInfo	part[OLD_NUM_PARTS];	//  48
	
#if QUICKTIME_SUPPORT
	Boolean		soloQT;					//	 1 Are we soloing on Quicktime?
#else
	Boolean		soloSynth;				//	 1 Are we soloing on the Synth?
#endif
	Boolean		soloMIDI;				//	 1 Are we soloing on MIDI?
	Boolean		soloTransform;			//	 1 Is transform soloing?
	Boolean		reserved;				//	 1 keep things aligned
	
	Str31		tuningName;				//  32 Name of a Custom Tuning
	SInt16		lowNote[6];				//  12 The lowNotes of the tuning
} FileHeadX1;							// 100

/*!
 * This part structure was used briefly in Format QQ
 */
typedef struct {
	UInt16		fauxGMNumber;			//  2 The instrument in FretPet terms
	UInt16		velocity;				//  2 The velocity
	UInt16		sustain;				//  2 The sustain
	Boolean		outQT;					//  1 Quicktime output?
	Boolean		outMIDI;				//  1 MIDI output?
	UInt16		outChannel;				//  2 The MIDI channel to play on
} interim_partInfo;						// 12

/*!
 * Format QQ was barely used
 */
typedef struct {
	UInt16		tempoX;					//  2 Tempo multipler (1 or 2)
	SInt16		partNum;				//  2 The current part number of the document
	interim_partInfo part[DOC_PARTS];	// 48
	Str31		tuningName;				// 32 Name of a Custom Tuning
	SInt16		lowNote[6];				// 12 The lowNotes of the tuning
} FileHeadQQ;							// 96

/*!
 * This file header was used for flat files on Mac OS 9
 * and in fact "Format 21" can still be saved.
 */
typedef struct {
	FileHeadCommon	common;				// 22
	union {
		FileHeadB3	dataB3;				// 2
		FileHead14	data14;				// 34
		FileHead21	data21;				// 78
		FileHeadQQ	dataQQ;				// 96
		FileHeadX1	dataX1;				// 100
	} u;
} FileHead;


#pragma pack()

#pragma mark - Binary File Macros

/*!
 *  FILE UTILITY MACROS
 *
 *	These macros and functions are intended to make
 *	it easier to insert binary data into a file buffer.
 *
 */

#define buff_Byte(z,v)		*(char*)(z)++ = (unsigned char)((v) & 0xFF)

// Big-Endian multi-byte representations
#define buff_WordBE(z,v)	{ buff_Byte(z,(v)>>8); buff_Byte(z,v); }
#define buff_TrioBE(z,v)	{ buff_Byte(z,(v)>>16); buff_Byte(z,(v)>>8); buff_Byte(z,v); }
#define buff_LongBE(z,v)	{ buff_Byte(z,(v)>>24); buff_Byte(z,(v)>>16); buff_Byte(z,(v)>>8); buff_Byte(z,v); }

// Little-Endian multi-byte representations
#define buff_WordLE(z,v)	{ buff_Byte(z,v); buff_Byte(z,(v)>>8); }
#define buff_TrioLE(z,v)	{ buff_Byte(z,v); buff_Byte(z,(v)>>8); buff_Byte(z,(v)>>16); }
#define buff_LongLE(z,v)	{ buff_Byte(z,v); buff_Byte(z,(v)>>8); buff_Byte(z,(v)>>16); buff_Byte(z,(v)>>24); }


#pragma mark - FPDocument

/*!
 *	FPDocument				* CONSTRUCTOR *
 */
FPDocument::FPDocument(FPDocWindow *wind) {
	Init(wind);
}

FPDocument::FPDocument(FPDocWindow *wind, const FSSpec &fspec) : TFile(fspec) {
	Init(wind);
	(void) InitFromFile();
}

FPDocument::FPDocument(FPDocWindow *wind, const FSRef &fsref) : TFile(fsref) {
	Init(wind);
	(void) InitFromFile();
}

FPDocument::~FPDocument() {
#if !DEMO_ONLY
	delete sunvoxExporter;
	delete movieExporter;
	delete midiExporter;
#endif
}


/*!
 * Init
 */
void FPDocument::Init(FPDocWindow *wind) {
	SetCreatorAndType(CREATOR_CODE, BANK_FILETYPE);
	KeepOpen();
	
	SetWindow(wind);
	
	scaleMode		= scalePalette->CurrentMode();
	enharmonic		= scalePalette->Enharmonic();
	topLine			= 0;
	cursor			= 0;
	selectionEnd	= -1;
	tempoX			= 1;
	partNum			= 0;
	soloQT			= false;
	soloMIDI		= false;
	soloTransform	= false;
	
	SetTempo(480);					// also initializes "interim"
	
	for (PartIndex p=DOC_PARTS; p--;) {
		part[p].instrument		= player->GetInstrumentNumber(p);
		part[p].velocity		= player->GetVelocity(p);
		part[p].sustain			= player->GetSustain(p);
#if QUICKTIME_SUPPORT
		part[p].outQT			= player->GetQuicktimeOutEnabled(p);
#else
		part[p].outSynth		= player->GetAUSynthOutEnabled(p);
#endif
		part[p].outMIDI			= player->GetMidiOutEnabled(p);
		part[p].outChannel		= player->GetMidiChannel(p);
		part[p].transformFlag	= player->GetTransformFlag(p);
		part[p].unused			= false;
		UpdateInstrumentName(p);
	}
	
	tuning = guitarPalette->CurrentTuning();

#if !DEMO_ONLY
	sunvoxExporter	= new FPSunvoxFile(this);
	movieExporter	= new FPMovieFile(this);
	midiExporter	= new FPMidiFile(this);
#endif

}

/*!
 * SetWindow
 */
void FPDocument::SetWindow(FPDocWindow *wind) {
	window = wind;
	SetOwningWindow(wind->Window());
}

/*!
 * Window
 */
WindowRef FPDocument::Window() {
	return window->Window();
}

/*!
 * TotalBeats
 */
UInt32 FPDocument::TotalBeats() const {
	UInt32 beatCount = 0;
	
	for (ChordIndex item=Size(); item--;) {
		const FPChordGroup &group = ChordGroup(item);
		beatCount += group.PatternSize() * group.Repeat();
	}
	
	return beatCount;
}

#pragma mark - File Load - XML Format
/*!
 * InitFromFile
 */
OSStatus FPDocument::InitFromFile(const FSRef &fsref) {
	Specify(fsref);
	return InitFromFile();
}

/*!
 * InitFromFile
 */
OSStatus FPDocument::InitFromFile() {
	OSStatus err = noErr;
	UInt16 headSize, partCount;

	OpenRead();

	// Look for old-style file IDs
	UInt32 fileType;
	err = Read(&fileType, sizeof(UInt32));

	if (!err) {
		fileType = EndianU32_BtoN(fileType);
		
		switch (fileType) {
			case FILE_FORMAT_B3:
			case FILE_FORMAT_14:
			case FILE_FORMAT_21:
			case FILE_FORMAT_X1:
			case FILE_FORMAT_QQ:
				err = InitFromClassicFile();
				break;
			default:
				err = InitFromXMLFile();
				break;
		}
	}

	if (err)
		AlertError(err, CFSTR("This is not a file that FretPet recognizes."));

	return err;
}

OSStatus FPDocument::InitFromClassicFile() {
	FileHead		*head;
	FileHeadCommon	hc;
	FileHeadX1		hd;
	
	char			*buffer = new char[400];		// about 4 times larger
	UInt16			calcLen;
	OldChordInfo	info;
	FPChordGroup	group;


	UInt32 headSize = 0;
	PartIndex partCount = OLD_NUM_PARTS;
	
	OSStatus err = SetOffset(0);
	nrequire(err, Bailout);

	// Read the file ID to start
	UInt32 fileType;
	err = Read(&fileType, sizeof(UInt32));
	nrequire(err, Bailout);
	fileType = EndianU32_BtoN(fileType);

	switch (fileType) {
		case FILE_FORMAT_B3:
			headSize = sizeof(FileHeadB3);
			partCount = 1;
			break;
		case FILE_FORMAT_14:
			headSize = sizeof(FileHead14);
			break;
		case FILE_FORMAT_21:
			headSize = sizeof(FileHead21);
			break;
		case FILE_FORMAT_X1:
			headSize = sizeof(FileHeadX1);
			break;
		case FILE_FORMAT_QQ:
			headSize = sizeof(FileHeadQQ);
			break;
		default:
			err = kFPErrorBadFormat;
			break;
	}
	nrequire(err, Bailout);

	//
	// Read the entire header and swap it if necessary
	//
	err = Read(buffer + sizeof(UInt32), headSize + sizeof(FileHeadCommon) - sizeof(UInt32));
	nrequire(err, Bailout);
	
#if QUICKTIME_SUPPORT
	hd.soloQT			= false;
#else
	hd.soloSynth		= false;
#endif
	
	hd.soloMIDI			= false;
	hd.soloTransform	= false;
	
	head = (FileHead*)buffer;
	
	hc.fileType		= fileType;
	hc.lastPoint.h	= EndianS16_BtoN(head->common.lastPoint.h);
	hc.lastPoint.v	= EndianS16_BtoN(head->common.lastPoint.v);
	hc.tempo		= EndianU16_BtoN(head->common.tempo);
	hc.scaleMode	= EndianU16_BtoN(head->common.scaleMode);
	hc.enharmonic	= EndianU16_BtoN(head->common.enharmonic);
	hc.tuning		= EndianU16_BtoN(head->common.tuning);
	hc.length		= EndianU16_BtoN(head->common.length);
	hc.topLine		= EndianU16_BtoN(head->common.topLine);
	hc.cursor		= EndianS16_BtoN(head->common.cursor);
	
	//
	// Test the size of the file for discrepancies
	//
	calcLen = headSize + sizeof(FileHeadCommon) + partCount * hc.length * sizeof(OldChordInfo);
	
	//	fprintf(stderr, "Length Should be %d  -  actual length is %lld\n", calcLen, Length());
	
	if (calcLen != Length()) {
		err = kFPErrorBadFormat;
		goto Bailout;
	}
	
	
	switch (fileType) {
		case FILE_FORMAT_B3: {
			FileHeadB3 *data = &head->u.dataB3;
			
			hd.partNum	= 0;
			hd.tempoX	= 1;
			hd.tuningName[0] = 0;
			
			for (PartIndex p=DOC_PARTS; p--;) {
				hd.part[p].instrument		= p ? player->GetInstrumentNumber(p) : FPMidiHelper::FauxGMToTrueGM(EndianS16_BtoN(data->fauxGMNumber));
				hd.part[p].velocity			= BASE_VELOCITY;
				hd.part[p].sustain			= BASE_SUSTAIN;
#if QUICKTIME_SUPPORT
				hd.part[p].outQT			= true;
#else
				hd.part[p].outSynth			= true;
#endif
				hd.part[p].outMIDI			= true;
				hd.part[p].outChannel		= p;
				hd.part[p].transformFlag	= true;
			}
			
			for (int s=NUM_STRINGS; s--;)
				hd.lowNote[s] = builtinTuning[hc.tuning].tone[s];
			break;
		}
			
		case FILE_FORMAT_14: {
			FileHead14 *data = &head->u.data14;
			
			hd.partNum	= EndianU16_BtoN(data->partNum);
			hd.tempoX	= 1;
			hd.tuningName[0] = 0;
			
			for (PartIndex p=OLD_NUM_PARTS; p--;) {
				hd.part[p].instrument	= FPMidiHelper::FauxGMToTrueGM(EndianS16_BtoN(data->fauxGMNumbers[p]));
				hd.part[p].velocity		= EndianU16_BtoN(data->velocity[p]);
				hd.part[p].sustain		= EndianU16_BtoN(data->sustain[p]);
#if QUICKTIME_SUPPORT
				hd.part[p].outQT		= true;
#else
				hd.part[p].outSynth		= true;
#endif
				hd.part[p].outMIDI		= true;
				hd.part[p].outChannel	= p;
				hd.part[p].transformFlag = true;
			}
			
			for (int s=NUM_STRINGS; s--;)
				hd.lowNote[s] = builtinTuning[hc.tuning].tone[s];
			
			break;
		}
			
		case FILE_FORMAT_21: {
			FileHead21 *data = &head->u.data21;
			
			hd.partNum	= EndianU16_BtoN(data->partNum);
			hd.tempoX	= EndianU16_BtoN(data->tempoX);
			pstrcpy(hd.tuningName, data->tuningName);
			
			for (PartIndex p=OLD_NUM_PARTS; p--;) {
				hd.part[p].instrument		= FPMidiHelper::FauxGMToTrueGM(EndianS16_BtoN(data->fauxGMNumbers[p]));
				hd.part[p].velocity			= EndianU16_BtoN(data->velocity[p]);
				hd.part[p].sustain			= EndianU16_BtoN(data->sustain[p]);
#if QUICKTIME_SUPPORT
				hd.part[p].outQT			= true;
#else
				hd.part[p].outSynth			= true;
#endif
				hd.part[p].outMIDI			= true;
				hd.part[p].outChannel		= p;
				hd.part[p].transformFlag	= true;
			}
			for (int s=NUM_STRINGS; s--;)
				hd.lowNote[s] = EndianS16_BtoN(data->lowNote[s]);
			
			break;
		}
			
		case FILE_FORMAT_QQ: {
			FileHeadQQ  *data = &head->u.dataQQ;
			
			hd.partNum	= EndianU16_BtoN(data->partNum);
			hd.tempoX	= EndianU16_BtoN(data->tempoX);
			pstrcpy(hd.tuningName, data->tuningName);
			
			for (PartIndex p=OLD_NUM_PARTS; p--;) {
				hd.part[p].instrument		= FPMidiHelper::FauxGMToTrueGM(EndianS16_BtoN(data->part[p].fauxGMNumber));
				hd.part[p].velocity			= EndianU16_BtoN(data->part[p].velocity);
				hd.part[p].sustain			= EndianU16_BtoN(data->part[p].sustain);
#if QUICKTIME_SUPPORT
				hd.part[p].outQT			= data->part[p].outQT;
#else
				hd.part[p].outSynth			= data->part[p].outQT;
#endif
				hd.part[p].outMIDI			= data->part[p].outMIDI;
				hd.part[p].outChannel		= EndianU16_BtoN(data->part[p].outChannel);
				hd.part[p].transformFlag	= true;
			}
			
			for (int s=NUM_STRINGS; s--;)
				hd.lowNote[s] = EndianS16_BtoN(data->lowNote[s]);
			break;
		}
			
		case FILE_FORMAT_X1: {
			FileHeadX1  *data = &head->u.dataX1;
			
			hd.tempoX	= EndianU16_BtoN(data->tempoX);
			hd.partNum	= EndianS16_BtoN(data->partNum);
			
			for (PartIndex p=DOC_PARTS; p--;) {
				hd.part[p].instrument		= FPMidiHelper::FauxGMToTrueGM(EndianS16_BtoN(data->part[p].instrument));
				hd.part[p].velocity			= EndianU16_BtoN(data->part[p].velocity);
				hd.part[p].sustain			= EndianU16_BtoN(data->part[p].sustain);
#if QUICKTIME_SUPPORT
				hd.part[p].outQT			= data->part[p].outQT;
#else
				hd.part[p].outSynth			= data->part[p].outSynth;
#endif
				hd.part[p].outMIDI			= data->part[p].outMIDI;
				hd.part[p].outChannel		= EndianU16_BtoN(data->part[p].outChannel);
				hd.part[p].transformFlag	= data->part[p].transformFlag;
			}
			
			pstrcpy(hd.tuningName, data->tuningName);
			
			for (int s=NUM_STRINGS; s--;)
				hd.lowNote[s] = EndianS16_BtoN(data->lowNote[s]);
			
			break;
		}
			
	}
	
	//
	// Sanity check the tempo multiplier
	//
	if (hd.tempoX < 1 || hd.tempoX > 2)
		hd.tempoX = 1;
	
	//
	// Copy old-style file header data to the document object
	//
	SetTempo(hc.tempo);
	cursor		= hc.cursor;
	scaleMode	= hc.scaleMode;
	enharmonic	= hc.enharmonic;
	topLine		= hc.topLine;
	for (PartIndex p=DOC_PARTS; p--;)
		part[p] = hd.part[p];
	
	//
	// Translate old tuning info into the new format
	//
	tuning.name = CFStringCreateWithPascalString(kCFAllocatorDefault, hd.tuningName, kCFStringEncodingMacRoman);
	
	//
	// Get the tuning's tones
	//
	for (int s=NUM_STRINGS; s--;)
		tuning.tone[s] = hd.lowNote[s];
	
	//
	// Read the chords and insert them
	//
	for (ChordIndex i=0; i<=hc.length - 1; i++) {
		for (PartIndex p=0; p<DOC_PARTS; p++) {
			if (p < partCount) {
				err = Read(&info, sizeof(OldChordInfo));
				group[p].Set(info);
			}
			else {
				group[p] = group[partCount-1];
				group[p].ClearPattern();
			}
			
			if (p > 0) {
				group[p].SetRepeat(group.Repeat());
				group[p].SetPatternSize(group.PatternSize());
			}
		}
		
		chordGroupArray.append_copy(group);
	}

Bailout:
	delete [] buffer;
	
	return err;
}


/*!
 * InitFromXMLFile
 *
 * Read the bank as an XML file
 */
OSErr FPDocument::InitFromXMLFile() {
	OSErr err = SetOffset(0);
	if (err != noErr) return err;

	TDictionary *fileDict = ReadXMLFile();
	
	if ( !fileDict->IsValid() || !fileDict->IsValidPropertyList(kCFPropertyListXMLFormat_v1_0) ) {
		err = kFPErrorBadFormat;
	}
	else {
		TDictionary infoDict( fileDict->GetDictionary(kStoredInfo) );
		
		CFStringRef	versString = infoDict.GetString(kStoredVersion);
		CFRETAIN(versString);
		// Do nothing for now. Eventually might need to use this info
		CFRELEASE(versString);
		
		ChordIndex len = infoDict.GetInteger(kStoredLength);
		
		tempo		= infoDict.GetInteger(kStoredTempo);
		tempoX		= infoDict.GetInteger(kStoredTempoX);
		scaleMode	= infoDict.GetInteger(kStoredScaleMode);
		enharmonic	= infoDict.GetInteger(kStoredEnharmonic);
		topLine		= infoDict.GetInteger(kStoredTopLine);
		partNum		= infoDict.GetInteger(kStoredCurrentPart);
		soloQT		= infoDict.GetBool(kStoredSoloQT);
		soloMIDI	= infoDict.GetBool(kStoredSoloMIDI);
		soloTransform = infoDict.GetBool(kStoredSoloTrans);
		
		// Set the tempo interim!
		UpdateInterim();
		
		// Sanity-check the cursor position, which was written wrong once
		ChordIndex	curs = infoDict.GetInteger(kStoredCursor);
		CONSTRAIN(curs, 0, len);
		SetCursorLine(curs);
		
		// Sanity-check the selection for the heck of it
		ChordIndex	sel = infoDict.GetInteger(kStoredSelection, -1);
		CONSTRAIN(sel, -1, len - 1);
		SetSelectionRaw(sel);
		
		tuning.name = infoDict.GetString(kStoredSoloTuneName);
		CFRETAIN(tuning.name);
		
		(void)infoDict.GetIntArray(kStoredTuningTones, tuning.tone, NUM_STRINGS);
		
		CFArrayRef partsArray = infoDict.GetArray(kStoredPartsArray);
		CFRETAIN(partsArray);
		
		for (PartIndex p=0; p<DOC_PARTS; p++) {
			partInfo *pinfo = &part[p];
			
			TDictionary partDict( (CFMutableDictionaryRef)CFArrayGetValueAtIndex(partsArray, p) );

			UInt16 gmNum = partDict.GetInteger(kStored_Instrument);

			pinfo->instrument	= gmNum ? gmNum : FPMidiHelper::FauxGMToTrueGM(partDict.GetInteger(kStored_GMNumber));
			pinfo->velocity		= partDict.GetInteger(kStored_Velocity);
			pinfo->sustain		= partDict.GetInteger(kStored_Sustain);
			pinfo->outChannel	= partDict.GetInteger(kStored_OutChannel);
#if QUICKTIME_SUPPORT
			pinfo->outQT		= partDict.GetBool(kStored_OutQT);
#else
			pinfo->outSynth		= partDict.GetBool(kStored_OutSynth);
#endif
			pinfo->outMIDI		= partDict.GetBool(kStored_OutMidi);
			pinfo->transformFlag = partDict.GetBool(kStored_TransformFlag);
		}
		
		CFRELEASE(partsArray);
		
		//
		// Get the chord groups as an ordered array
		//
		//	Each chord group is a dictionary, and one
		//	member is an array of dictionaries
		//
		CFArrayRef docArray = fileDict->GetArray(kStoredChordGroups);
		if ( docArray ) {
			if (CFArrayGetCount(docArray) != len)
				err = kFPErrorBadFormat;
			else {
				for (ChordIndex c=0; c<len; c++) {
					CFDictionaryRef	dictRef = (CFDictionaryRef)CFArrayGetValueAtIndex(docArray, c);
					TDictionary		groupDict( dictRef );
					chordGroupArray.push_back( new FPChordGroup(groupDict) );
				}
			}
			
			CFRELEASE(docArray);
		}
		
	}
	
	delete fileDict;
	
	return err;
}

/*!
 *	HandleNavError
 */
void FPDocument::HandleNavError(OSStatus err) {
	TFile::HandleNavError(err);
	window->HandleNavError(err);
}

/*!
 *	TerminateNavDialogs
 */
void FPDocument::TerminateNavDialogs() {
	TerminateNavDialog();
#if !DEMO_ONLY
	sunvoxExporter->TerminateNavDialog();
	movieExporter->TerminateNavDialog();
	midiExporter->TerminateNavDialog();
#endif
}

#if !DEMO_ONLY

/*!
 * WriteXMLFormat
 *
 * Save as an XML file (about bloody time too)
 */
OSErr FPDocument::WriteXMLFormat() {
	SInt32		err = noErr;
	ChordIndex	len = Size();
	
	TDictionary fileDict;	// Main dictionary
	TDictionary infoDict;	// Info (header)
	
	// Version
	infoDict.SetString(kStoredVersion, FILE_FORMAT_XML_LOOSE);
	
	// And more
	infoDict.SetInteger(kStoredTempo, Tempo());							// Tempo
	infoDict.SetInteger(kStoredTempoX, TempoMultiplier());				// Tempo X
	infoDict.SetInteger(kStoredScaleMode, ScaleMode());					// Scale
	infoDict.SetInteger(kStoredEnharmonic, Enharmonic());				// Enharmonic
	infoDict.SetInteger(kStoredLength, len);							// Doc Length
	infoDict.SetInteger(kStoredTopLine, TopLine());						// Top Line
	infoDict.SetInteger(kStoredCursor, GetCursor());					// Cursor
	infoDict.SetInteger(kStoredSelection, SelectionEnd());				// The selection end (add 1 for legacy reasons)
	infoDict.SetInteger(kStoredCurrentPart, CurrentPart());				// Part Number
	infoDict.SetBool(kStoredSoloQT, soloQT);							// Solo QT			(future)
	infoDict.SetBool(kStoredSoloMIDI, soloMIDI);						// Solo MIDI		(future)
	infoDict.SetBool(kStoredSoloTrans, soloTransform);					// Solo Transform	(future)
	infoDict.SetString(kStoredSoloTuneName, tuning.name);				// Tuning Name
	infoDict.SetIntArray(kStoredTuningTones, tuning.tone, NUM_STRINGS);	// Tuning Tones
	
	// The partInfo array
	TDictionary				partDictList[DOC_PARTS];
	CFMutableDictionaryRef	partDictRefs[DOC_PARTS];
	
	for (PartIndex p=0; p<DOC_PARTS; p++) {
		partInfo	&pinfo = part[p];
		TDictionary	&partDict = partDictList[p];
		partDictRefs[p] = partDict.GetDictionaryRef();
		
		partDict.SetInteger(kStored_Instrument, pinfo.instrument);
		partDict.SetInteger(kStored_Velocity, pinfo.velocity);
		partDict.SetInteger(kStored_Sustain, pinfo.sustain);
		partDict.SetInteger(kStored_OutChannel, pinfo.outChannel);
#if QUICKTIME_SUPPORT
		partDict.SetBool(kStored_OutQT, pinfo.outQT);
#else
		partDict.SetBool(kStored_OutSynth, pinfo.outSynth);
#endif
		partDict.SetBool(kStored_OutMidi, pinfo.outMIDI);
		partDict.SetBool(kStored_TransformFlag, pinfo.transformFlag);
	}
	
	CFArrayRef partsArray = CFArrayCreate(
										  kCFAllocatorDefault,
										  (const void **)partDictRefs,
										  DOC_PARTS,
										  &kCFTypeArrayCallBacks );
	
	infoDict.SetArray(kStoredPartsArray, partsArray);
	
	CFRELEASE(partsArray);
	
	
	// Add the parts info array to the file
	fileDict.SetDictionary(kStoredInfo, infoDict);
	
	
	//
	// Build the saveable chord array as an array of dictionaries.
	//	(Most of the work is done by ChordGroup)
	//
	TDictionary		*groupDictList[len];
	CFDictionaryRef	groupRefList[len];
	for (ChordIndex c=0; c<len; c++) {
		TDictionary *dict = chordGroupArray[c].GetDictionary();
		groupDictList[c] = dict;
		groupRefList[c] = dict->GetDictionaryRef();
	}
	
	CFArrayRef chordsArray = CFArrayCreate(
										   kCFAllocatorDefault,
										   (const void **)groupRefList,
										   len,
										   &kCFTypeArrayCallBacks );
	
	fileDict.SetArray(kStoredChordGroups, chordsArray);
	
	for (ChordIndex c=len; c--;)
		delete groupDictList[c];
	
	CFRELEASE(chordsArray);
	
	// Write the dictionary out as XML data
	err = WriteXMLFile(fileDict.GetDictionaryRef());
	
	return err;
}


/*!
 * WriteFormat214
 *
 * Save the bank as a FretPet 2.14 format file.
 */
OSErr FPDocument::WriteFormat214() {
	OSErr		err = noErr;
	Str31		tuningName;
	
	CFIndex len = CFStringGetBytes(
								   tuning.name,
								   CFRangeMake(0, CFStringGetLength(tuning.name)),
								   kCFStringEncodingMacRoman, '?', false,
								   &tuningName[1], 31, NULL);
	
	tuningName[0] = (UInt8)len;
	
	int tuningIndex = 100;
	for (int i=kBaseTuningCount; i--;)
		if (builtinTuning[i] == guitarPalette->CurrentTuning()) {
			tuningIndex = i;
			break;
		}
	
	FileHeadCommon comm = {
		EndianU32_NtoB(FILE_FORMAT_21),
		{0,0},	// lastPoint is ignored by fretpet classic
		EndianU16_NtoB(tempo),
		EndianU16_NtoB(scaleMode),
		EndianU16_NtoB(enharmonic),
		EndianU16_NtoB(tuningIndex),
		EndianU16_NtoB(Size()),
		EndianU16_NtoB(topLine),
		EndianS16_NtoB(cursor)
	};
	
	// Make a header with everything in Big Endian format
	FileHead21 head = {
		partNum,
		
		{	EndianS16_NtoB(FPMidiHelper::TrueGMToFauxGM(part[0].instrument)),
			EndianS16_NtoB(FPMidiHelper::TrueGMToFauxGM(part[1].instrument)),
			EndianS16_NtoB(FPMidiHelper::TrueGMToFauxGM(part[2].instrument)),
			EndianS16_NtoB(FPMidiHelper::TrueGMToFauxGM(part[3].instrument)) },
		
		{	EndianS16_NtoB(part[0].velocity),
			EndianS16_NtoB(part[1].velocity),
			EndianS16_NtoB(part[2].velocity),
			EndianS16_NtoB(part[3].velocity) },
		
		{	EndianS16_NtoB(part[0].sustain),
			EndianS16_NtoB(part[1].sustain),
			EndianS16_NtoB(part[2].sustain),
			EndianS16_NtoB(part[3].sustain) },
		
		EndianS16_NtoB(tempoX),
		0, 0, 0,
		
		{	tuningName[0], tuningName[1], tuningName[2], tuningName[3],
			tuningName[4], tuningName[5], tuningName[6], tuningName[7],
			tuningName[8], tuningName[9], tuningName[10], tuningName[11],
			tuningName[12], tuningName[13], tuningName[14], tuningName[15],
			tuningName[16], tuningName[17], tuningName[18], tuningName[19],
			tuningName[20], tuningName[21], tuningName[22], tuningName[23],
			tuningName[24], tuningName[25], tuningName[26], tuningName[27],
			tuningName[28], tuningName[29], tuningName[30], tuningName[31] },
		
		{	EndianS16_NtoB(tuning.tone[0]),
			EndianS16_NtoB(tuning.tone[1]),
			EndianS16_NtoB(tuning.tone[2]),
			EndianS16_NtoB(tuning.tone[3]),
			EndianS16_NtoB(tuning.tone[4]),
			EndianS16_NtoB(tuning.tone[5]) }
	};
	
	err = Write(&comm, sizeof(comm));
	nrequire(err, BailSave);
	
	err = Write(&head, sizeof(head));
	nrequire(err, BailSave);
	
	err = WriteFormat214Chords();
	
BailSave:
	return err;
}


/*!
 * WriteFormat214Chords
 *	Write out all the chords as OldChordInfo structures.
 */
OSErr FPDocument::WriteFormat214Chords() {
	OSErr		err = noErr;
	
	if (Size()) {
		for (ChordIndex i=0; i<Size(); i++) {
			FPChordGroup &group = ChordGroup(i);
			for (PartIndex p=0; p<DOC_PARTS; p++) {
				err = group[p].WriteOldStyle(this);
				nrequire(err, BailSave);
			}
		}
	}
	
BailSave:
	return err;
}

#pragma mark -

/*!
 *	OpenAndSave
 */
OSErr FPDocument::OpenAndSave() {
#if KAGI_SUPPORT
	if (!fretpet->AuthorizerIsAuthorized())
		return noErr;
#endif
	
	for (FPDocumentIterator itr = FPDocWindow::docWindowList.begin(); itr != FPDocWindow::docWindowList.end(); itr++) {
		FPDocWindow	*wind = *itr;
		CFStringRef	sref = BaseName();
		
		FPDocument *doc = wind->document;
		if ( doc != this ) {
			sref = doc->BaseName();
			
			if ( ((TFile&)*this) == ((TFile&)*doc) ) {
				CFStringRef string1 = CFCopyLocalizedString( CFSTR("The document could not be saved because the file \"%@\" is currently open in FretPet. Choose another name or close the document you are trying to replace."), "Attempted to replace an open file" );
				CFStringRef muxString = CFStringCreateWithFormat( kCFAllocatorDefault, NULL, string1, doc->DisplayName() );
				AlertError(0, muxString);
				CFRELEASE(string1);
				return kErrorHandled;
			}
		}
	}
	
	return TFile::OpenAndSave();
}

/*!
 *	WriteData
 */
OSErr FPDocument::WriteData() {
	return preferences.GetBoolean(kPrefFormat214)
	? WriteFormat214()
	: WriteXMLFormat();
}

#pragma mark - File Navigator
/*!
 * NavCustomSize
 */
#define kRegularCustomWidth		190
#define kRegularCustomHeight	25
void FPDocument::NavCustomSize(Rect &customRect) {
	if (customRect.bottom - customRect.top < kRegularCustomHeight)
		customRect.bottom = customRect.top + kRegularCustomHeight;
	
	if (customRect.right - customRect.left < kRegularCustomWidth)
		customRect.right = customRect.left + kRegularCustomWidth;
}

/*!
 * NavCustomControls
 */
enum {
	kCustom214Format = 1
};
void FPDocument::NavCustomControls(NavDialogRef navDialog, DialogRef dialog) {
	Handle	ditlHandle = GetResource('DITL', 202);
	OSErr	theErr = NavCustomControl(navDialog, kNavCtlAddControlList, ditlHandle);
	
	UInt16 firstControlID;
	NavCustomControl(navDialog, kNavCtlGetFirstControlID, &firstControlID);
	
	// Always reset this preference
	preferences.SetBoolean(kPrefFormat214, false);
	ControlRef control;
	theErr = GetDialogItemAsControl(dialog, firstControlID + kCustom214Format, &control);
	SetControlValue(control, kControlCheckBoxUncheckedValue);
	
	if (Size() > 0x10000)
		DisableControl(control);
}

/*!
 * HandleCustomControl
 */
void FPDocument::HandleCustomControl(ControlRef inControl, UInt16 controlIndex) {
	SInt16 checked = !::GetControlValue(inControl);
	SetControlValue(inControl, checked);
	
	switch (controlIndex) {
		case kCustom214Format:
			preferences.SetBoolean(kPrefFormat214, checked);
			break;
	}
}

/*!
 *	NavCancel
 */
void FPDocument::NavCancel() {
	window->NavCancel();
}

/*!
 *	NavDontSave
 */
void FPDocument::NavDontSave() {
	window->NavDontSave();
}

/*!
 *	NavDiscardChanges
 */
void FPDocument::NavDiscardChanges() {
	window->NavDiscardChanges();
}

/*!
 *	NavDidSave
 */
void FPDocument::NavDidSave() {
	window->NavDidSave();
}

#pragma mark - MIDI Export
/*!
 *	MIDI FILE HELPER MACROS
 *
 *	(c)hannel	(n)ote		(v)alue		(l)ength
 *
 */

#define midi_Using					int sentOn = 99

#define midi_NoteOn(z,c,n,v,ver)	if (ver || sentOn != (c)) { buff_Byte(z, 0x90|(c)); sentOn = c; } \
									buff_Byte(z, n); buff_Byte(z, v)

#define midi_NoteOff(z,c,n,ver)		if (ver) buff_Byte(z, 0x80|(c)); \
									else if (sentOn != (c)) { buff_Byte(z, 0x90|(c)); sentOn = c; } \
									buff_Byte(z, n); buff_Byte(z, ver ? 0x40 : 0x00)

#define midi_Aftertouch(z,c,n,v)	buff_Byte(z, 0xA0|(c)); buff_Byte(z, n); buff_Byte(z, v)
#define midi_ControlChange(z,c,k,v)	buff_Byte(z, 0xB0|(c)); buff_Byte(z, k); buff_Byte(z, v)
#define midi_Program(z,c,p)			buff_Byte(z, 0xC0|(c)); buff_Byte(z, p)

#define midi_AllNotesOff(z,c)		midi_ControlChange(z, c, 123, 0)

#define midi_BankSelect(z,c,n);		midi_ControlChange(z, c, 0x00, (n) >> 8); \
									midi_NullDelay(z); \
									midi_ControlChange(z, c, 0x20, (n) & 0xFF)

#define midi_MetaEvent(z,e,l)		buff_Byte(z, 0xFF); buff_Byte(z, e); buff_Byte(z, l)

#define midi_EndMarker(z)			midi_MetaEvent(z, 0x2F, 0); sentOn = 99

#define midi_Tempo(z,t)				midi_MetaEvent(z, 0x51, 3); buff_TrioBE(w, t)

#define midi_SMPTE60(z)				midi_MetaEvent(z, 0x54, 5); \
									buff_Byte(z, 0x60); \
									buff_LongBE(z, 0x00000000)

#define midi_TimeSig(z,a,b,t,q)		midi_MetaEvent(z, 0x58, 4); \
									buff_Byte(z, a); \
									buff_Byte(z, b); \
									buff_Byte(z, t); \
									buff_Byte(z, q)

#define midi_NullDelay(z)			midi_Delay(z, 0)

#define midi_HeadTag(z)				buff_LongBE(z, 'MThd')

#define midi_TrackTag(z)			buff_LongBE(z, 'MTrk')

#define midi_Header(z,f,t,q)		midi_HeadTag(z); \
									buff_LongBE(z, 6); \
									buff_WordBE(z, f); \
									buff_WordBE(z, t); \
									buff_WordBE(z, q)


/*!
 *	midi_Delay
 */
void midi_Delay(char*& ptw, long delay);
void midi_Delay(char*& ptw, long delay) {
	char	b[4];
	int i;
	for (i=4; i--;) {
		b[i] = (delay & 0x7F) | ((i < 3) ? 0x80 : 0x00);
		if (!(delay >>= 7)) break;
	}
	while (i < 4) buff_Byte(ptw, b[i++]);
}


/*!
 *	midi_Text
 */
enum {
	kMidiTextGeneric = 1,
	kMidiTextCopyright,
	kMidiTextTrack,
	kMidiTextInstrument,
	kMidiTextLyric,
	kMidiTextMarker,
	kMidiTextCue
};
void midi_Text(char*& ptw, char event, const char *someText);
void midi_Text(char*& ptw, char event, const char *someText) {
	size_t len = strlen(someText);

	buff_Byte(ptw, 0xFF);
	buff_Byte(ptw, event);
	buff_Byte(ptw, len);

	while (len--) buff_Byte(ptw, *someText++);
}


OSStatus FPDocument::ExportMIDI() {
	return midiExporter->SaveAs();
}

/*!
 * GetFormat0
 *
 *	Build a Standard MIDI datastream with all
 *	channels combined into a single track.
 */
Handle FPDocument::GetFormat0() {
	PartIndex p;
	UInt32 msPer16th = Interim(), msPer4th = Interim() * 4;
	UInt16 stopNote[DOC_PARTS][100];
	UInt16 channel[DOC_PARTS], velocity, item, beat, str;
	bool verbose = preferences.GetBoolean(kPrefVerboseMidi, TRUE);

	midi_Using;
	//
	// PlayTempo() is BPM * 4
	// Interim() is 60 / PlayTempo() * MICROSECOND - the amount of time between individual notes
	//
	//    Quarternotes per second:	Tempo / 240 * 4
	//    Sixteenths per second:	Tempo / 240
	// Take the reciprocal to get the fractional amount of seconds:
	//    Seconds per Quarternote:	1 / QPs
	//    Seconds per Sixteenth:	1 / SPs
	// Multiply by 1,000,000 to get microseconds:
	//    MS per QuarterNote:		sPQ * MICROSECOND
	//    MS per Sixteenth:			sPS * MICROSECOND
	// Add it all together and you get...
	//    MS per QuarterNote:		(1.0 / (BPM / 60)) * MICROSECOND
	//    MS per Sixteenth:			(1.0 / (BPM / (60 * 4))) * MICROSECOND
	//
	// Old code was...
	//    msPer4th = (UInt32)(240.0 * MICROSECOND / (float)PlayTempo());
	// New code would be...
	//    msPer4th = (UInt32)(240.0 * MICROSECOND / PlayTempo());


	//
	// Allocate a buffer of a reasonable starting size
	//
	Handle midiHandle = NewHandle(BUFFER_CHUNK_SIZE);
	HLock(midiHandle);
	char *buffer = *midiHandle, *w = buffer;
	
	// Insert the standard MIDI Header
	midi_Header(w, 0, 1, TICKS_PER_4TH);	// Format 0, 1 Track...		(TODO: Try E728 for millisecond timing)
	
	// Begin the single Track
	midi_TrackTag(w);
	UInt32 sizeIndex = w - buffer;
	buff_LongBE(w, 0);
	UInt32 trackStartIndex = w - buffer;
	midi_NullDelay(w);
	
	// Give credit to FretPet
	midi_Text(w, kMidiTextCopyright, "Created with FretPet X " FRETPET_VERSION);
	midi_NullDelay(w);
	
	// SMPTE time marker
	//	midi_SMPTE60(w);
	//	midi_NullDelay(w);

	//
	// Set the tempo and Time Signature
	//
	midi_Tempo(w, msPer4th);		// Microsecond duration of each Quarter Note
	midi_NullDelay(w);
	midi_TimeSig(w, 4, 2, 24, 8);	// 4/2, 24 ticks in metronome, 8 32nd notes per 1/4
	
	
	// Format 0 contains all program changes up front
	for (p=0; p<DOC_PARTS; p++) {
		UInt16 trueGM = player->GetInstrumentNumber(p);
		channel[p] = (trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit) ? 9 : p;
		midi_NullDelay(w);
		
		//		if (select0)
		//		{
		UInt16 bank = 0;
		if (trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit)
			bank = 1;
		else if (trueGM >= kFirstGSInstrument-1 && trueGM <= kLastGSInstrument-1)
			bank = (trueGM >> 7) << 8;
		
		midi_BankSelect(w, channel[p], bank);
		midi_NullDelay(w);
		//		}
		
		midi_Program(w, channel[p], (trueGM - 1) & 0x7F);
	}
	
	
	// Prepare the note-stops array
	UInt32 stopTime[DOC_PARTS][NUM_OCTAVES * OCTAVE];
	bzero(stopTime, sizeof(stopTime));
	
	
	// Loop through the sequence writing out events
	UInt32 tickCount = 0, lastEvent = 0;
	for (item=0; item<Size(); item++) {
		for (int rept=ChordGroup(item).Repeat(); rept--;) {
			for (beat=0; beat<ChordGroup(item).PatternSize(); beat++) {
				UInt32 nextTick = tickCount + TICKS_PER_16TH;
				
				for (p=0; p<DOC_PARTS; p++) {
					FPChord &theItem = Chord(item, p);
					
					//
					// Converting jiffies (60ths/sec) into ticks (240ths/beat)
					//
					// Quarternotes per second = 1,000,000 / msPer4th (reciprocal of s/q = q/s)
					// Ticks per second = TICKS_PER_4TH * Quarternotes per second (t/q * q/s = tq/qs = t/s)
					// Ticks in Jiffies = TPS * Jiffies / 60 (t/s * s = t)
					//
					// Sustain in MIDI is expressed in ticks, 240 per second
					// Sustain in FretPet is expressed as 60ths of a second, or Jiffies
					//

					UInt32 sustainTicks = (MICROSECOND * Sustain(p) / 60.0) * TICKS_PER_4TH / msPer4th;		// Convert to ticks
					static int didDebug = 0;
					velocity = Velocity(p);
					
					if (theItem.IsBracketEnabled()) {
						for (str=0; str<NUM_STRINGS; str++) {
							SInt16 note = theItem.FretHeld(str);
							if ((note != -1) && theItem.GetPatternDot(str, beat)) {
								note += Tuning().tone[str] + LOWEST_C;
								
								UInt32 restTime = tickCount - lastEvent;			// number of ticks
								lastEvent = tickCount;
								midi_Delay(w, restTime);
								
								if (stopTime[p][note]) {							// a note is playing already?
									midi_NoteOff(w, channel[p], note, verbose);		// then shut it off first
									midi_NullDelay(w);								// insert a null delay for the next
								}
								
								midi_NoteOn(w, channel[p], note, velocity, verbose);	// Send that note event
								stopTime[p][note] = tickCount + sustainTicks;			// Set the stop time for the note
							}
						}
					}
					
					//
					// Pre-generate a list of the notes in the current
					//	part that will end during this beat 
					//
					UInt16 stopCount[DOC_PARTS];
					bzero(stopCount, sizeof(stopCount));
					for (int note=NUM_OCTAVES * OCTAVE; note--;)
						if (stopTime[p][note] && (stopTime[p][note] < nextTick))
							stopNote[p][stopCount[p]++] = note;
					
					//
					// Loop through the time during the beat and
					//	stop notes that are due to end
					//
					UInt16 count = stopCount[p];
					for (int i=TICKS_PER_16TH; i-- && count;) {
						for (int j=stopCount[p]; j--;) {
							UInt16 note = stopNote[p][j];
							if (note && (stopTime[p][note] <= tickCount)) {
								UInt32 restTime = tickCount - lastEvent;		// number of ticks
								lastEvent = tickCount;
								midi_Delay(w, restTime);						// Insert the delay, whatever it is
								midi_NoteOff(w, channel[p], note, verbose);	// Turn off that note
								stopTime[p][note] = 0;						// Flag this note as stopped
								stopNote[p][j] = 0;							// Ignore this note from now on
								count--;
							}
						}
						
						tickCount++;
						
					} //ticks within the beat
					
				} //part
				
				// Check for possible buffer overflow and handle it
				UInt32 progress = w - buffer;
				if (ExpandHandleForSize(midiHandle, progress))
					w = (buffer = *midiHandle) + progress;
				
				tickCount = nextTick;
				
			} //beat
			
		} //repeat
		
	} //item
	
	{
		// Get a list of all remaining notes
		UInt16 stopCount[DOC_PARTS];
		for (p=DOC_PARTS; p--;) {
			stopCount[p] = 0;
			for (int note=NUM_OCTAVES * OCTAVE; note--;)
				if (stopTime[p][note])
					stopNote[p][stopCount[p]++] = note;
		}
		
		// Finish all remaining notes
		UInt32 finalTick = tickCount + TICKS_PER_16TH - 1 + 50;
		UInt16 count = 0;
		
		for (p=DOC_PARTS; p--;)
			count += stopCount[p];
		
		while (count > 0) {
			for (p=DOC_PARTS; p--;) {
				for (int j=stopCount[p]; j--;) {
					UInt16 note = stopNote[p][j];
					if ( note && (tickCount >= stopTime[p][note] || tickCount >= finalTick) ) {
						UInt32 restTime = tickCount - lastEvent;
						lastEvent = tickCount;
						midi_Delay(w, restTime);
						midi_NoteOff(w, channel[p], note, verbose);
						stopNote[p][j] = 0;
						count--;
					}
				}
			}
			tickCount++;
		}
		
		// Mark the end of the track
		midi_Delay(w, finalTick > tickCount ? finalTick - tickCount : 0);
		midi_EndMarker(w);
		
		// Set the track size in its header
		char *size_long = &buffer[sizeIndex];
		buff_LongBE(size_long, w - &buffer[trackStartIndex]);
		
		UInt32 finalSize = w - buffer;
		SetHandleSize(midiHandle, finalSize);
		
#if DEBUG_MIDI
		fprintf(stderr, "Format 0 ... Total Beats: %d    Final Buffer Size: %d\n", TotalBeats(), finalSize);
#endif
		
	}
	
	return midiHandle;
}

/*!
 * GetFormat1
 *
 *	Build a Standard MIDI datastream with a
 *	separate track for each channel.
 */
Handle FPDocument::GetFormat1() {
	PartIndex p;
	UInt32 msPer16th = Interim(), msPer4th = Interim() * 4;
	UInt32 lastEvent, tickCount;
	UInt32 stopTime[NUM_OCTAVES * OCTAVE];
	SInt16 note, stopNote[100];
	bool verbose = preferences.GetBoolean(kPrefVerboseMidi, TRUE);

#if DEBUG_MIDI
	fprintf(stderr, "Building Format 1 ... µS/Q=%d  Beats=%d\n", msPer4th, TotalBeats());
#endif
	
	midi_Using;
	
	// Allocate a buffer
	Handle midiHandle = NewHandle(BUFFER_CHUNK_SIZE);
	HLock(midiHandle);
	
#if DEBUG_MIDI
	fprintf(stderr, "Allocated a %d byte handle to start\n", GetHandleSize(midiHandle));
#endif
	
	char	*buffer = *midiHandle, *w = buffer;
	
	// Insert the standard MIDI Header
	midi_Header(w, 1, DOC_PARTS + 1, TICKS_PER_4TH);									// Format 0, 1 Track...
	
#if DEBUG_MIDI
	fprintf(stderr, "Inserted MIDI Header (%d)\n", w - buffer);
#endif
	
	// Insert an Initializer Track
	midi_TrackTag(w);
	char *len_long = w;
	buff_LongBE(w, 0);
	char *trackStartAddr = w;
	midi_NullDelay(w);
	
#if DEBUG_MIDI
	fprintf(stderr, "Inserted Track Header (%d)\n", w - buffer);
#endif
	
	// Give FretPet credit
	midi_Text(w, kMidiTextCopyright, "Created with FretPet X " FRETPET_VERSION);
	midi_NullDelay(w);
	
#if DEBUG_MIDI
	fprintf(stderr, "Inserted Copyright Message (%d)\n", w - buffer);
#endif
	
	// Tempo Value
	midi_Tempo(w, msPer4th);
	midi_NullDelay(w);
	
#if DEBUG_MIDI
	fprintf(stderr, "Inserted Tempo (%d)\n", w - buffer);
#endif
	
	// Time Signature : 4/2, 24 ticks in metronome, 8 32nds per 1/4
	midi_TimeSig(w, 4, 2, 24, 8);
	midi_NullDelay(w);
	
#if DEBUG_MIDI
	fprintf(stderr, "Inserted Time Signature (%d)\n", w - buffer);
#endif
	
	// End Marker
	midi_EndMarker(w);
	
#if DEBUG_MIDI
	fprintf(stderr, "Inserted End Marker (%d)\n", w - buffer);
#endif
	
	// Store the track length
	buff_LongBE(len_long, w - trackStartAddr);
	
#if DEBUG_MIDI
	fprintf(stderr, "Track Length = %d bytes\n", w - trackStartAddr);
#endif
	
	// Each part gets its own track
	for (p=0; p<DOC_PARTS; p++) {
#if DEBUG_MIDI
		fprintf(stderr, "Starting Track for Part %d\n", part);
#endif
		
		// Prepare the note-stops array
		bzero(stopTime, sizeof(stopTime));
		
		// Uninitialized Track Header
		midi_TrackTag(w);
		UInt32 sizeIndex = w - buffer;
		buff_LongBE(w, 0);
		UInt32 trackStartIndex = w - buffer;
		midi_NullDelay(w);

#if DEBUG_MIDI
		fprintf(stderr, "Inserted Track Header (%d)\n", w - buffer);
#endif
		
		// Instrument Name
		TString instrumentName(player->GetInstrumentName(p));
		midi_Text(w, kMidiTextTrack, instrumentName.GetCString());
		midi_NullDelay(w);
		
#if DEBUG_MIDI
		fprintf(stderr, "Inserted Instrument Name (%d)\n", w - buffer);
#endif
		
		// Instrument Number
		UInt16	trueGM = player->GetInstrumentNumber(p);
		UInt16	channel = (trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit) ? 9 : p;
		
		//		if (select0)
		//		{
		UInt16 bank = 0;
		if (trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit)
			bank = 1;
		else if (trueGM >= kFirstGSInstrument-1 && trueGM <= kLastGSInstrument-1)
			bank = (trueGM >> 7) << 8;
		
		midi_BankSelect(w, channel, bank);
		midi_NullDelay(w);
		
#if DEBUG_MIDI
		fprintf(stderr, "Inserted Bank Select 0\n", w - buffer);
#endif
		//		}
		
		midi_Program(w, channel, (trueGM - 1) & 0x7F);
		
#if DEBUG_MIDI
		fprintf(stderr, "Inserted Program Change %d (%d)\n", (inst - 1) & 0x7F, w - buffer);
#endif
		
		// Converting jiffies (60ths/sec) into ticks (240ths/beat)
		//
		// Quarternotes per second = 1,000,000 / msPer4th (reciprocal of seconds per quarternote)
		// Ticks per second = TICKS_PER_4TH * Quarternotes per second (t/q * q/s = t/s)
		// Ticks in Jiffies = TPS * Jiffies / 60 (t/s * s = t)

		UInt16	vel = Velocity(p), sus = Sustain(p);
		UInt32 sustainTicks = (MICROSECOND * sus / 60.0) * TICKS_PER_4TH / msPer4th;
		tickCount = lastEvent = 0;
		
		for (ChordIndex item=0; item<Size(); item++) {
#if DEBUG_MIDI
			fprintf(stderr, "Processing Chordgroup %d ...\n", item);
#endif
			
			FPChord theItem = Chord(item, p);
			for (int rept=theItem.Repeat(); rept--;) {
#if DEBUG_MIDI
				fprintf(stderr, "Processing Repeat %d ...\n", rept);
#endif
				
				for (int beat=0; beat<=theItem.PatternSize() - 1; beat++) {
#if DEBUG_MIDI
					fprintf(stderr, "Processing Beat %d ...\n", beat);
#endif
					
					if (theItem.IsBracketEnabled()) {
						for (int str=0; str<NUM_STRINGS; str++) {
#if DEBUG_MIDI
							fprintf(stderr, "Processing String %d ...\n", str);
#endif
							
							SInt16 note = theItem.FretHeld(str);
							if ((note != -1) && theItem.GetPatternDot(str, beat)) {
								note += Tuning().tone[str] + LOWEST_C;
								
								UInt32 restTime = tickCount - lastEvent;		// number of ticks
								lastEvent = tickCount;
								midi_Delay(w, restTime);
								
#if DEBUG_MIDI
								fprintf(stderr, "Inserted Rest %d (%d)\n", restTime, w - buffer);
#endif
								
								if (stopTime[note]) {							// a note is playing already?
									midi_NoteOff(w, channel, note, verbose);	// then shut it off first
									midi_NullDelay(w);							// insert a null delay for the next
									
#if DEBUG_MIDI
									fprintf(stderr, "Inserted Note Off note=%d (%d)\n", note, w - buffer);
#endif
								}
								
								midi_NoteOn(w, channel, note, vel, verbose);	// Send that note event
								stopTime[note] = tickCount + sustainTicks;			// Set the stop time for the note
								
#if DEBUG_MIDI
								fprintf(stderr, "Inserted Note On note=%d  time=%d  endtime=%d (%d) ...\n", note, tickCount, tickCount + sus, w - buffer);
#endif
							}
						}
					}
					
					//
					// Pre-generate a list of the notes that should
					//	end during the current beat.
					//
					UInt16 stopCount = 0;
					UInt32 nextTick = tickCount + TICKS_PER_16TH;
					for (note=NUM_OCTAVES * OCTAVE; note--;)
						if (stopTime[note] && (stopTime[note] < nextTick))
							stopNote[stopCount++] = note;
					
					//
					// Loop through the time during the beat and
					//	stop notes that are due to end
					//
					UInt16 count = stopCount;
					for (int i=TICKS_PER_16TH; i-- && count;) {
						for (int j=stopCount; j--;) {
							note = stopNote[j];
							if (note && (tickCount >= stopTime[note])) {
								UInt32 restTime = tickCount - lastEvent;		// number of ticks
								lastEvent = tickCount;
								midi_Delay(w, restTime);						// Insert the delay, whatever it is
								
#if DEBUG_MIDI
								fprintf(stderr, "Inserted Rest %d (%d)\n", restTime, w - buffer);
#endif
								
								midi_NoteOff(w, channel, note, verbose);		// Turn off that note
								
#if DEBUG_MIDI
								fprintf(stderr, "Inserted Note Off %d (%d)\n", note, w - buffer);
#endif
								
								stopTime[note] = 0;								// Flag this note as stopped
								stopNote[j] = 0;								// Ignore this note from now on
							}
						}
						tickCount++;
					}
					
					tickCount = nextTick;
					
				} // beat
				
#if DEBUG_MIDI
				UInt32 oldSize = GetHandleSize(midiHandle);
#endif
				
				UInt32 progress = w - buffer;
				
				if (ExpandHandleForSize(midiHandle, progress))
					w = (buffer = *midiHandle) + progress;
				
#if DEBUG_MIDI
				UInt32 newSize = GetHandleSize(midiHandle);
				if (oldSize != newSize)
					fprintf(stderr, "(Expanding handle to %d bytes)\n", newSize);
#endif
				
				
			} //repeat
			
		} //item
		
		//
		// Pre-generate a list of the notes that
		//	need to be ended
		//
		UInt16 stopCount = 0;
		for (note=NUM_OCTAVES * OCTAVE; note--;)
			if (stopTime[note])
				stopNote[stopCount++] = note;
		
#if DEBUG_MIDI
		fprintf(stderr, "%d Notes still playing at track end\n", stopCount);
#endif
		
		//
		// Finish all remaining notes
		//
		UInt32 finalTick = tickCount + TICKS_PER_16TH - 1 + 50;
		UInt16 count = stopCount;
		while (count > 0) {
			for (int j=stopCount; j--;) {
				note = stopNote[j];
				if ( note && (tickCount >= stopTime[note] || tickCount >= finalTick) ) {
					UInt32 restTime = tickCount - lastEvent;	// number of ticks
					lastEvent = tickCount;
					midi_Delay(w, restTime);					// Insert the delay, whatever it is
					
#if DEBUG_MIDI
					fprintf(stderr, "Inserted Rest %d (%d)\n", restTime, w - buffer);
#endif
					
					midi_NoteOff(w, channel, note, verbose);	// Turn off that note
					
#if DEBUG_MIDI
					fprintf(stderr, "Inserted Note Off %d (%d)\n", note, w - buffer);
#endif
					
					stopNote[j] = 0;							// Ignore this note from now on
					count--;
				}
			}
			tickCount++;
		}
		
		// Mark the end of each track
		midi_Delay(w, tickCount < finalTick ? finalTick - tickCount : 0);
		midi_NoteOff(w, channel, LOWEST_C, verbose);
		midi_NullDelay(w);
		midi_EndMarker(w);
		
#if DEBUG_MIDI
		fprintf(stderr, "Inserted Track end stuff(%d)\n", w - buffer);
#endif
		
		// Set the track size in its header
		char *size_long = &buffer[sizeIndex];
		buff_LongBE(size_long, w - &buffer[trackStartIndex]);
		
#if DEBUG_MIDI
		fprintf(stderr, "Total track size = %d\n", w - &buffer[trackStartIndex]);
#endif
		
	} // part loop
	
	UInt32 finalSize = w - buffer;
	SetHandleSize(midiHandle, finalSize);
	
#if DEBUG_MIDI
	fprintf(stderr, "Format 1 ... Total Beats: %d    Final Buffer Size: %d\n", totalBeats, finalSize);
#endif
	
	return midiHandle;
}

#pragma mark - Movie Export
/*!
 * ExportMovie
 */
OSStatus FPDocument::ExportMovie() {
	return movieExporter->SaveAs();
}

/*!
 *	GetTuneHeader
 */

const UInt32 kNRHeadLen = ((sizeof(NoteRequest)/sizeof(UInt32)) + 2); 	// Note Request Size / 4
const UInt32 kMarkerEventLength = 1;									// Marker Event Size / 4
const UInt32 kTotalSize = (kNRHeadLen * DOC_PARTS + kMarkerEventLength);
const UInt32 kMoviePolyphony = 16;

UInt32* FPDocument::GetTuneHeader() {
	UInt32			*pHead;
	UInt32			*w1, *w2;		// the head and tail long words of the general event
	NoteRequest		*nr;
	NoteAllocator	na = NULL;		// for the NAStuffToneDescription call
	ComponentResult	err = noErr;
	
	pHead = NULL;
	
	//
	// Open the component to gain access to its functions
	//
	na = OpenDefaultComponent(kNoteAllocatorComponentType, 0);
	require(na != NULL, TuneHeaderFail);
	
	//
	// Allocate space for the tune header...
	// (one note request event per part plus the end marker)
	//
	pHead = (UInt32 *)NewPtrClear(kTotalSize * sizeof(UInt32));
	require(pHead != NULL, TuneHeaderFail);
	
	//
	// Build and store the NoteRequests
	//
	w1 = pHead;
	for (PartIndex p=1; p<=DOC_PARTS; p++) {
		// Calc the address of the last longword of general event
		w2 = w1 + (kNRHeadLen - 1);
		
		// Stuff a channel request
		qtma_StuffGeneralEvent(*w1, *w2, p, kGeneralEventNoteRequest, kNRHeadLen);
		
		// Customize the channel parameters
		nr = (NoteRequest *)(w1 + 1);
		nr->info.flags = 0;
		//		nr->info.reserved = 0;				deprecated?
#if defined(__BIG_ENDIAN__)
		nr->info.polyphony = kMoviePolyphony;
		nr->info.typicalPolyphony = kMoviePolyphony << 16;
#else
		nr->info.polyphony.bigEndianValue = EndianS16_NtoB(kMoviePolyphony);
		nr->info.typicalPolyphony.bigEndianValue = EndianS16_NtoB((SInt16)(kMoviePolyphony << 16));
#endif
		nr->info.midiChannelAssignment = kNoteRequestSpecifyMIDIChannel | MidiChannel(p - 1);

		// Stuff the tone description (specify GM sound and tone description)
		err = NAStuffToneDescription(na, GetInstrument(p-1), &nr->tone);
		require_noerr( err, TuneHeaderFail );

		// Next NoteRequest
		w1 += kNRHeadLen;
	}

	// put an end marker after the header
	*w1++ = kEndMarkerValue;

TuneHeaderFail:
	if (na)
		CloseComponent(na);
	
	if (err && pHead) {
		DisposePtr((Ptr)pHead);
		pHead = NULL;
	}
	
	return pHead;
}


/*!
 *	GetTuneSequence
 *
 *	This prepares a tune sequence for all parts
 */
UInt32** FPDocument::GetTuneSequence(long *duration) {
	enum {
		kEndAtBeat,
		kEndAtBeatPlus,
		kEndAtEnd,
		kEndAtEndPlus,
		kEndAfterCutoff
	};
	
	const int		endingStyle = kEndAtEndPlus;
	const bool		cutoffNotes = false;
	const UInt16	cutoffMargin = 1;

	float			beatJiffies = 36000.0f / (float)PlayTempo();			// BPM converted to 600ths per beat
	ChordIndex		item;
	PartIndex		p;
	SInt16			note;
	UInt32			**h;
	
	// fprintf(stderr, "Jiffies Per Beat At Tempo=%d: %.3f\n", PlayTempo(), beatJiffies);
	
	//
	// Total Beat Time is the cumulative duration of all beats
	//
	float totalBeatTime = TotalBeats() * beatJiffies;
	float lastBeatTime = totalBeatTime - beatJiffies;
	
	float longestSustain = 0.0;
	for (p=DOC_PARTS; p--;) {
		float sustain = Sustain(p) * 10.0;
		if (sustain > longestSustain)
			longestSustain = sustain;
	}
	
	//
	// Total Sustain Time is the cumulative duration of all beats
	//	minus the duration of the last beat
	//	plus the duration of the longest sustain
	//
	//	...or the point at which the last note will absolutely end
	//
	float totalSustainTime = lastBeatTime + longestSustain;
	
	// fprintf(stderr, "Last Beat: %.2f to %.2f   Final Sustain: %.3f\n", lastBeatTime, totalBeatTime, totalSustainTime);
	
	//
	//	Total Time depends on the ending style
	//	By default we try to make it suitable for looping
	//
	float totalTime = 0.0;
	
	// Allocate a decent starting size
	h = (UInt32**)NewHandle(BUFFER_CHUNK_SIZE * 10);
	if ( h ) {
		HLock((Handle)h);
		
		float	stopJiffy[DOC_PARTS][NUM_OCTAVES * OCTAVE];
		for (p=DOC_PARTS; p--;)
			for (note=NUM_OCTAVES * OCTAVE; note--;)
				stopJiffy[p][note] = 0.0f;
		
		UInt32	*w = *h;
		UInt16	tonesOn = 0, rept = 0, beat = 0;
		float	theTime = -1.0f, lastJiffy = 0.0f, nextJiffy = 0.0f;
		item = 0;
		
		while (item < Size()) {
			theTime += 1.0;
			
			// Has time gone past the next jiffy?
			if (theTime >= nextJiffy) {
				// fprintf(stderr, "\nItem Number : %d\n", item);
				// fprintf(stderr, "Jiffy Reached : %.3f >= %.3f     Beat: %d\n", theTime, nextJiffy, beat);
				
				UInt16 beats = ChordGroup(item).PatternSize();
				UInt16 repeat = ChordGroup(item).Repeat();
				
				for (p=0; p<DOC_PARTS; p++) {
					FPChord &chord = Chord(item, p);
					
					//					// fprintf(stderr, "Processing Part : %d\n", part);
					
					if (chord.IsBracketEnabled()) {
						long	vel = Velocity(p);
						float	sus = Sustain(p) * 10;					// Convert 60ths to 600ths
						
						//						// fprintf(stderr, "Velocity: %03d     Sustain 600ths: %.2f\n", vel, sus);
						
						for (int str=0; str<NUM_STRINGS; str++) {
							if (chord.GetPatternDot(str, beat)) {
								// fprintf(stderr, "String: %d   Beat: %d\n", str, beat);
								
								note = chord.FretHeld(str);
								if (note >= 0) {
									note += OpenTone(str) + LOWEST_C;
									
									UInt32	restTime = (UInt32)(theTime - lastJiffy);
									bool	tonePlaying = (stopJiffy[p][note] != 0.0f);
									bool	needCutoff = cutoffNotes && tonePlaying;
									if (restTime > 0) {
										UInt32 adjustedRest = restTime - (needCutoff ? cutoffMargin : 0);
										// fprintf(stderr, "Inserting Rest: %d (%.3f)\n", adjustedRest, theTime - lastJiffy);
										qtma_StuffRestEvent(*w, adjustedRest);
										w++;
									}
									
									if (tonePlaying) {
										tonesOn--;
										if (needCutoff) {
											qtma_StuffNoteEvent(*w, p + 1, note, 0, 0);
											w++;
											// fprintf(stderr, "Cutting Off Note : %d/%d (%d)\n", part, note, tonesOn);
											qtma_StuffRestEvent(*w, cutoffMargin);
											w++;
											// fprintf(stderr, "Inserting Post-Cutoff Rest\n");
										}
									}
									
									if (endingStyle == 0 && (theTime + sus) > totalBeatTime) {
										if (totalBeatTime > theTime)
											sus = totalBeatTime - theTime;
										else
											sus = 0.0;
									}
									
									qtma_StuffNoteEvent(*w, p + 1, note, vel, (long)sus);
									w++;
									tonesOn++;
									
									// fprintf(stderr, "Inserted Note Start: %d/%d %d %.2f (%d)\n", part, note, vel, sus, tonesOn);
									
									lastJiffy = theTime;
									stopJiffy[p][note] = theTime + sus;
									
									// fprintf(stderr, "This note ends at: %.3f\n", stopJiffy[part][note]);
								}
							}
						}
					}
				}
				
				// Insert stop events for any notes that are due to end
				for (p=0; p<DOC_PARTS; p++) {
					for (note=0; note<NUM_OCTAVES * OCTAVE; note++) {
						if (stopJiffy[p][note] > 0.0f && theTime >= stopJiffy[p][note]) {
							// fprintf(stderr, "Tone %d/%d Ending Normally : ( stopTime=%.3f t=%.3f )\n", part, note, stopJiffy[part][note], theTime);
							
							UInt32 restTime = (UInt32)(theTime - lastJiffy);
							if (restTime > 0) {
								// fprintf(stderr, "Inserting Rest: %d (%.3f)\n", restTime, theTime - lastJiffy);
								qtma_StuffRestEvent(*w, restTime);
								w++;
							}
							
							qtma_StuffNoteEvent(*w, p + 1, note, 0, 0);
							w++;
							tonesOn--;
							
							//							// fprintf(stderr, "Ending Note : %d/%d (%d)\n", part, note, tonesOn);
							
							lastJiffy = theTime;
							stopJiffy[p][note] = 0.0f;
						}
					}
				}
				
				UInt32 progress = (Ptr)w - (Ptr)*h;
				if (ExpandHandleForSize((Handle)h, progress))
					w = *h + progress;
				
				// prepare for the next beat
				// fprintf(stderr, "Next Beat...\n");
				if (++beat >= beats) {
					// fprintf(stderr, "Next Repeat...\n");
					beat = 0;
					if (++rept >= repeat) {
						// fprintf(stderr, "Next Item...\n");
						rept = 0;
						item++;
					}
				}
				
				nextJiffy += beatJiffies;
				// fprintf(stderr, "Next Jiffy : %.3f\n", nextJiffy);
			}
		}
		
		// fprintf(stderr, "Reached the end at t=%.3f with %d tones still playing\n", theTime, tonesOn);
		
		if (cutoffNotes) {
			// At the very end silence any continuing notes...
			while (tonesOn) {
				theTime += 1.0;
				
				for (p=0; p<DOC_PARTS; p++) {
					for (note=0; note<NUM_OCTAVES * OCTAVE; note++) {
						if (stopJiffy[p][note] > 0.0f && theTime >= stopJiffy[p][note]) {
							UInt32 restTime = (UInt32)(theTime - lastJiffy);
							if (restTime > 0) {
								// fprintf(stderr, "Inserting Rest: %d (%.3f)\n", restTime, theTime - lastJiffy);
								qtma_StuffRestEvent(*w, restTime);
								w++;
							}
							
							qtma_StuffNoteEvent(*w, p + 1, note, 0, 0);
							w++;
							tonesOn--;
							
							// fprintf(stderr, "Ending Note : %d/%d (%d)\n", part, note, tonesOn);
							
							lastJiffy = theTime;
							stopJiffy[p][note] = 0.0f;
						}
					}
				}
			}
			
			// fprintf(stderr, "Final note ending at t=%.3f\n", theTime);
		}
		
		//
		// Add time to the end depending on the style
		//
		switch(endingStyle) {
			case kEndAtBeat:
				totalTime = totalBeatTime;
				break;
				
			case kEndAtBeatPlus:
				totalTime = lastBeatTime + 300.0;
				break;
				
			case kEndAtEnd:
				totalTime = totalSustainTime;
				break;
				
			case kEndAtEndPlus:
				totalTime = totalSustainTime + 300.0;
				break;
				
			case kEndAfterCutoff:
				totalTime = theTime;
				break;
		}
		
		if (totalTime > theTime) {
			UInt32 finalRest = (UInt32)(totalTime - theTime);
			qtma_StuffRestEvent(*w, finalRest);
			w++;
			// fprintf(stderr, "Adding a Rest of %d for %.3f Total Jiffies\n", finalRest, totalTime);
		}
		
		*w = kEndMarkerValue;
		w++;
		SetHandleSize((Handle)h, (Ptr)w - (Ptr)*h);
		
		// fprintf(stderr, "Total Sequence Size: %d\n", (Ptr)w - (Ptr)*h);
	}
	
	*duration = (long)totalTime;
	return h;
}


#pragma mark - Sunvox Export

#define LOWEST_SUNVOX_C (LOWEST_C - OCTAVE)

/*!
 *  SUNVOX UTILITY MACROS
 *
 *	These macros and functions are intended to make
 *	it easier to insert Sunvox data into a file buffer.
 *
 *	(c)hannel	(n)ote		(v)alue		(l)ength
 *
 */

#define sunvox_Using					int sentOn = 99; char *chunkStart = NULL

#define sunvox_Chunk(z,name,size)		{ buff_LongBE(z,name); buff_LongLE(z,size); }

#define sunvox_ChunkStart(z,name)		{ buff_LongBE(z,name); chunkStart = z; buff_LongLE(z,0); }

#define sunvox_ChunkEnd(z)				{ UInt32 chunkSize = z-chunkStart-4; buff_LongLE(chunkStart,chunkSize); chunkStart = NULL; }

#define sunvox_TextPad(z,name,text,len)	{ char t[len]; strncpy(t,text,len); sunvox_Data(z,name,len,t); }

#define sunvox_Marker(z,name)			sunvox_Chunk(z,name,0)

#define sunvox_Value(z,name,val)		{ buff_LongBE(z,name); buff_LongLE(z,4); buff_LongLE(z,val); }

#define sunvox_Color(z,name,color)		{ buff_LongBE(z,name); buff_LongLE(z,3); buff_TrioBE(z,color); }

#define sunvox_Text(z,name,text)		sunvox_Data(z,name,strlen(text)+1,(char*)text)

#define sunvox_Data(z,name,len,src)		{ UInt32 l=len; char *s=src; sunvox_Chunk(z, name, len); while (l--) buff_Byte(z, *s++); }

// NN VV SS CCEE XXYY
#define sunvox_NoteEvent(z,nn,vv,ss,ccee,xxyy) { \
									buff_Byte(z,nn); \
									buff_Byte(z,vv); \
									buff_Byte(z,ss); \
									buff_Byte(z,0); \
									buff_WordLE(z,ccee); \
									buff_WordLE(z,xxyy); }

#define sunvox_NoteOn(z,nn,vv,ss)	sunvox_NoteEvent(z,nn+1,vv,ss,0,0)

#define sunvox_NoteOff(z)			sunvox_NoteEvent(z,0x80,0,0,0,0)

#define sunvox_NoteWait(z)			sunvox_NoteEvent(z,0,0,0,0,0)

#define sunvox_Header(z,ver,bpm,spd,vol) { \
									sunvox_Marker(z, 'SVOX'); \
									sunvox_Value(z, 'VERS', ver); \
									sunvox_Value(z, 'BPM ', bpm); \
									sunvox_Value(z, 'SPED', spd); \
									sunvox_Value(z, 'GVOL', vol); }

/*!
 * GetSunvoxFormat
 *
 *	Build a Sunvox file.
 */
Handle FPDocument::GetSunvoxFormat() {

	PartIndex p;
	UInt16 activePartMask = 0, activePartCount = 0;
	for (p=0; p<DOC_PARTS; p++) {
		UInt16 b = BIT(p);
		if (PartsHavePattern(b)) {
			activePartMask |= b;
			activePartCount++;
		}
	}

	char iconData[32];
	memset(iconData, 0xFF, sizeof(iconData));

	const UInt32 tpb = 24, tpl = 6;
	UInt32 bpm = PlayTempo();			// The "real" BPM

	// tempo-based factors - Examples for 480 BPM (i.e., 120 * 4)
	float	lpb = (float)tpb / tpl,	// 4 LPB
			bps = bpm / 60.0f,		// 8 BPS
			spb = 60.0f / bpm,		// 1/8 SPB
			lps = lpb / spb,		// 32 LPS
			tps = bpm * 24.0f / 60.0f;	// ticks per second

	UInt32 patternLine, timelineX = 0, timelineY = activePartCount * -16;
	SInt16 string, stopLine[NUM_STRINGS];
	
	sunvox_Using;
	
	// Allocate a buffer
	Handle sunvoxHandle = NewHandle(BUFFER_CHUNK_SIZE);
	HLock(sunvoxHandle);

	char *buffer = *sunvoxHandle, *w = buffer;
	
	// Insert the standard MIDI Header
	sunvox_Header(w, 0x01070000, bpm, tpl, 80); // Sunvox 1.07, bpm, 6 ticks-per-lines, ~30% global volume
	
	// Include the name in the file
	sunvox_Text(w, 'NAME', PrettyName().GetCString());
	
	//
	// Insert PDTA...PEND for all unique patterns in all 4 channels
	//	Each pattern includes its first occurrence in the timeline, and all patterns are in the timeline
	//
	// Insert PPAR...PEND for each repeated pattern in the timeline
	//	Lineup the original parts vertically - time will always align horizontally
	//
	// Insert SFFF...SEND for all four instruments, with their relative volumes
	//	It's hard to reproduce GM instruments with modules, but the main thing is
	//	to get the pattern working then sounds can easily be adjusted in SunVox
	//
	UInt32 currentPattern = 0;
	SInt32 patternToClone = -1;
	UInt32 patternColor[] = { 0xFFAAAA, 0xFFFFAA, 0xDDAAFF, 0xAAAAFF };
	UInt32 moduleColor[] = { 0xFF0000, 0xFFFF00, 0xDD00FF, 0x0000FF };
	UInt16 moduleIndex = 3;		// apparently OUT and Echo are 1 and 2 internally
	for (p=0; p<DOC_PARTS; p++) {

		if (!(activePartMask & BIT(p))) continue;

		UInt16 vel = Velocity(p);

		// sustain expressed in lines (1/4 beats)
		UInt32	sustainLines = floorf(lps * Sustain(p) / 60.0f);
		
		// pattern's position in the timeline is pattern line-based
		timelineX = 0;

		// Prepare the string-stops array
		for (int str=NUM_STRINGS; str--;) stopLine[str] = -1;

		// Instrument Number
//		UInt16	trueGM = player->GetInstrumentNumber(p);
//		UInt16	channel = (trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit) ? 9 : p;
		
		ChordIndex *chordPDTAIndex = (ChordIndex*)calloc(Size(), sizeof(ChordIndex));

		// Go through all the chords, creating patterns and clones as-needed
		for (ChordIndex item=0; item<Size(); item++) {

			FPChord theChord = Chord(item, p);

			// If the chord is exactly like a previous one in the same part
			// then set it to go ahead and clone the pattern
			patternToClone = -1;
			if (item > 0)
				for (int prev=0;prev<item;prev++) {
					FPChord prevChord = Chord(prev, p);
					if (theChord == prevChord) {
						patternToClone = chordPDTAIndex[prev];
						break;
					}
				}

			// If it is, set a var with the index of the previous chord
			// Then use that chord's previously-stored PDTA Index as the PDTA index of the clones

			UInt16	activeStringsMask = theChord.StringsUsedInPattern(), activeStringsCount = 0;
			for (int str=0; str<NUM_STRINGS; str++) if (activeStringsMask & BIT(str)) activeStringsCount++;

			// The chord will be repeated 1-16 times
			for (int rept=0; rept < theChord.Repeat(); rept++) {

				// For empty patterns just skip ahead in time
				if (activeStringsCount) {

					UInt32 patternFlags;

					// Create a Pattern Data if this is a unique chord
					if (patternToClone < 0) {

						// The next cloned pattern will be this one (if it repeats)
						patternToClone = currentPattern;

						// Remember the index of all PDTAs
						chordPDTAIndex[item] = currentPattern;

						// Clones use the last Pattern Data index.
						// Both patterns and clones count towards the index.
						patternLine = 0;
						patternFlags = 0x00;

						// Pattern Data Chunk
						sunvox_ChunkStart(w, 'PDTA');

						// Go through all the beats
						for (int beat = 0; beat < theChord.PatternSize(); beat++) {

							// Each beat uses the same number of lines, 24 / lpb
							for (int line = 0; line < lpb; line++) {

								// Each line has six strings, spread over tracks
								for (int str=0; str<NUM_STRINGS; str++) {

									// Skip unused strings in the pattern
									if (!(activeStringsMask & BIT(str))) continue;

									Boolean stringHasNote = false;

									// First 'line' has a note on event potentially
									if (line == 0 && theChord.IsBracketEnabled()) {
										SInt16 fretHeld = theChord.FretHeld(str);
										if ((fretHeld != -1) && theChord.GetPatternDot(str, beat)) {
											sunvox_NoteOn(w, LOWEST_SUNVOX_C + Tuning().tone[str] + fretHeld, vel, moduleIndex);
											stopLine[str] = patternLine + sustainLines;
											stringHasNote = true;
										}
									}

									if (!stringHasNote) {
										// Not the first line? Add a blank or stop the string note
										if (stopLine[str] == patternLine) {
											sunvox_NoteOff(w);
											stopLine[str] = -1;
										}
										else {
											sunvox_NoteWait(w);
										}
									}
									
								} //string

								patternLine++;

							} // lines per beat

						} // beat


						// In MIDI there may be extra OFF events here
						// Sunvox may allow extra lines while permitting overlap in the sequencer
						// Users will be annoyed with odd numbered patterns, but their patterns
						// will often already be screwy anyway

						// For now allow notes to keep sustaining at the end

						// end the PDTA Chunk (before expanding the handle)
						sunvox_ChunkEnd(w);

						// Expand the handle if buffer space is getting tight
						{
						UInt32 progress = w - buffer;
						if (ExpandHandleForSize(sunvoxHandle, progress))
							w = (buffer = *sunvoxHandle) + progress;
						}
						
						sunvox_Value(w, 'PCHN', activeStringsCount);
						sunvox_Value(w, 'PLIN', patternLine);
						sunvox_Value(w, 'PYSZ', 32);

						// Make a random bitmask
						for (int i=1; i<=7; i++) {
							UInt32 bits = (random() & 0xFE) | 0x01, stib = 0x00;
							for (int b=0; b<=7; b++) if (bits & BIT(b)) stib |= BIT(7-b);
							iconData[i * 2] = iconData[31 - (i * 2 + 1)] = bits;
							iconData[i * 2 + 1] = iconData[31 - i * 2] = stib;
						}
						sunvox_Data(w, 'PICO', 32, iconData);

						sunvox_Color(w,'PFGC', 0x000000);
						sunvox_Color(w,'PBGC', patternColor[p]);

					}
					else {
						// just repeat the previous pattern
						sunvox_Value(w, 'PPAR', patternToClone);
						patternFlags = 0x03;
					} //if (first repeat)

					// All patterns have these
					sunvox_Value(w, 'PFFF', patternFlags);
					sunvox_Value(w, 'PXXX', timelineX);
					sunvox_Value(w, 'PYYY', timelineY);
					sunvox_Marker(w, 'PEND');

					++currentPattern;

				} // has data?

				timelineX += theChord.PatternSize() * lpb;

			} //repeat

		} //item

		// Free this or leak
		free(chordPDTAIndex);

		// parts stack up in the timeline
		// note that these will be skipped by "continue" in the part loop
		timelineY += 32;
		moduleIndex++;
	
	} // part loop

	// Module #0 OUT
	sunvox_Value(w, 'SFFF', 0x043);
	sunvox_TextPad(w, 'SNAM', "OUT", 32);
	sunvox_Value(w, 'SFIN', 0);
	sunvox_Value(w, 'SREL', 0);
	sunvox_Value(w, 'SXXX', 896);	// others 128, 512
	sunvox_Value(w, 'SYYY', 512);
	sunvox_Color(w, 'SCOL', 0xFFFFFF);
	sunvox_Value(w, 'SMIC', 1);
	sunvox_Value(w, 'SMIB', -1);
	sunvox_Value(w, 'SMIP', -1);
	sunvox_ChunkStart(w, 'SLNK');
	buff_LongLE(w, 1);
	buff_LongLE(w, -1);
	sunvox_ChunkEnd(w);
	sunvox_Marker(w, 'SEND');

	// Module #1 Echo
	sunvox_Value(w, 'SFFF', 0x051);
	sunvox_TextPad(w, 'SNAM', "Echo", 32);
	sunvox_Text(w, 'STYP', "Echo");
	sunvox_Value(w, 'SFIN', 0);
	sunvox_Value(w, 'SREL', 0);
	sunvox_Value(w, 'SXXX', 512);
	sunvox_Value(w, 'SYYY', 512);
	sunvox_Value(w, 'SZZZ', 0);
	sunvox_Color(w, 'SCOL', 0xFFA37F);
	sunvox_Value(w, 'SMIC', 0);
	sunvox_Value(w, 'SMIB', -1);
	sunvox_Value(w, 'SMIP', -1);
	sunvox_ChunkStart(w, 'SLNK');
	moduleIndex = 2;
	for (p=0; p<activePartCount; p++) {
		buff_LongLE(w, moduleIndex);
		moduleIndex++;
	}
	sunvox_ChunkEnd(w);
	sunvox_Value(w, 'CVAL', 256);
	sunvox_Value(w, 'CVAL', 128);
	sunvox_Value(w, 'CVAL', 32);
	sunvox_Value(w, 'CVAL', 48);
	sunvox_Value(w, 'CVAL', 1);
	sunvox_Value(w, 'CVAL', 0);
	sunvox_Marker(w, 'SEND');

	// Create a set of Module entries
	UInt32 moduleY = 512 - 210 * (activePartCount - 1) / 2.0;
	UInt32 inst[] = { 5, 5, 5, 5 };
	for (p=0; p<DOC_PARTS; p++) {
		if (!(activePartMask & BIT(p))) continue;

		UInt16	gmNumber = player->GetInstrumentNumber(p);
		Boolean isDrum = (gmNumber >= kFirstDrumkit && gmNumber <= kLastDrumkit);
		TString instrumentName(player->GetInstrumentName(p));

		sunvox_Value(w, 'SFFF', isDrum ? 0x049 : 0x059);
		sunvox_TextPad(w, 'SNAM', instrumentName.GetCString(), 32);
		sunvox_Text(w, 'STYP', (char*)(isDrum ? "DrumSynth" : "Generator"));		// Drum: CcDd BASS, EFf HIHAT, GgAaB SNARE
		sunvox_Value(w, 'SFIN', 0);
		sunvox_Value(w, 'SREL', 0);
		sunvox_Value(w, 'SXXX', 128);
		sunvox_Value(w, 'SYYY', moduleY);
		sunvox_Color(w, 'SCOL', moduleColor[p]);
		sunvox_Value(w, 'SMIC', isDrum ? 10 : 1);
		sunvox_Value(w, 'SMIB', -1);
		sunvox_Value(w, 'SMIP', -1);
		sunvox_Chunk(w, 'SLNK', 0);					// No INs

		if (isDrum) {
			sunvox_Value(w, 'CVAL', 297);				// Volume
			sunvox_Value(w, 'CVAL', 128);				// Panning
			sunvox_Value(w, 'CVAL', 4);					// Polyphony
			sunvox_Value(w, 'CVAL', 200);				// Bass Volume
			sunvox_Value(w, 'CVAL', 256);				// Bass Power
			sunvox_Value(w, 'CVAL', 64);				// Bass Tone
			sunvox_Value(w, 'CVAL', 64);				// Bass Length
			sunvox_Value(w, 'CVAL', 256);				// Hihat Volume
			sunvox_Value(w, 'CVAL', 64);				// Hihat Length
			sunvox_Value(w, 'CVAL', 256);				// Snare Volume
			sunvox_Value(w, 'CVAL', 128);				// Snare Tone
			sunvox_Value(w, 'CVAL', 64);				// Snare Length
		}
		else {
			UInt16 gmGroup = (gmNumber - 1) / 8;

			// make a release based on sustain
			UInt32	releaseFactor = (UInt32)floorf((tps * Sustain(p) / 60.0f) / 24.0f * 256.0f);
			if (releaseFactor > 511) releaseFactor = 511;

			sunvox_Value(w, 'CVAL', 128);				// Volume
			sunvox_Value(w, 'CVAL', inst[p]);			// Type
			sunvox_Value(w, 'CVAL', 128);				// Panning
			sunvox_Value(w, 'CVAL', 0);					// Attack
			sunvox_Value(w, 'CVAL', releaseFactor);		// Release
			sunvox_Value(w, 'CVAL', 8);					// Polyphony
			sunvox_Value(w, 'CVAL', 0);					// Mode (HQ)
			sunvox_Value(w, 'CVAL', 0);					// Sustain
			sunvox_Value(w, 'CVAL', 0);					// Modulation
			sunvox_Value(w, 'CVAL', 511);				// Duty Cycle
		}
		sunvox_Marker(w, 'SEND');
		moduleY += 210;
	}
	
	UInt32 finalSize = w - buffer;
	SetHandleSize(sunvoxHandle, finalSize);
	
	return sunvoxHandle;
}

OSStatus FPDocument::ExportSunvox() {
	return sunvoxExporter->SaveAs();
}

#endif // !DEMO_ONLY

#pragma mark -
/*!
 *	Insert
 *
 *	Insert a copy of a chord group
 */
void FPDocument::Insert(const FPChordGroup &group) {
	Insert(&group, 1, false);
}


/*!
 *	Insert
 *
 *	Insert a single chord by making a chord group
 *	with the single chord in the selected part
 */
void FPDocument::Insert(const FPChord &chordRef) {
	Insert(new FPChordGroup(chordRef), 1, true);		// 1 chord  - do reset the sequence
}


/*!
 *	Insert(groupPtr, count)
 */
bool FPDocument::Insert(const FPChordGroup *groupPtr, ChordIndex count, bool bResetSeq) {
	//
	// Insertion after the cursor, which will work ok even
	// if the document is empty and the cursor is non-zero.
	//
	if (count)
		chordGroupArray.insert_copy( (Size() ? GetCursor() + 1 : 0), groupPtr );
	
	return true;
}


/*!
 *	Insert(array)
 */
bool FPDocument::Insert(const FPChordGroupArray &arrayRef) {
	//
	// Insertion after the cursor, which will work ok even
	// if the document is empty and the cursor is non-zero.
	//
	size_t arraySize = arrayRef.size();
	if (arraySize > 0)
		chordGroupArray.insert_copies(arrayRef, 0, (Size() ? GetCursor() + 1 : 0), arraySize);
	
	return true;
}


/*!
 *	DeleteSelection
 *
 *	Deletes the selection even if it's a single
 *	chord.
 */
void FPDocument::DeleteSelection() {
	ChordIndex	startDel, endDel;
	if (GetSelection(&startDel, &endDel))
		Delete(startDel, endDel);
}


/*!
 *	Delete(start, end)
 */
void FPDocument::Delete(ChordIndex startDel, ChordIndex endDel) {
	ChordIndex	oldLast = Size() - 1;							// the original last item
	
	chordGroupArray.erase(startDel, endDel);
	
	// Empty selection
	SetSelectionRaw(-1);
	
	// The cursor should move to the first chord after
	// the deletion (unless there aren't any after)
	ChordIndex c = startDel;
	
	if (Size() > 0 && endDel == oldLast)
		c--;
	
	//
	// Finally, decide where the cursor should be if
	// the piece is playing.
	//
	SetCursorLine(c);
	(void)player->AdjustForDeletion(c, Size(), startDel, endDel - startDel + 1);
	if (GetCursor() >= Size())
		SetCursorLine(Size() - 1);
}


/*!
 *	SetSelectionRaw
 */
void FPDocument::SetSelectionRaw(ChordIndex newSel) {
	if ( newSel == -1 || newSel < Size() )
		selectionEnd = newSel;
	else
		fprintf(stderr, "selection sanity fail!\n");
}

/*!
 *	SetSelection
 *
 *	FretPet selections mimic those of textedit, with
 *	one exception: The current chord is always selected.
 *
 *	A selection exists when doc.selectionEnd >= 0. Then
 *	doc.selectionEnd and doc.cursor together indicate the range.
 *
 *	This routine sets just the end point of a selection.
 *	This can be called repeatedly to extend selections, and it
 *	will draw only the document lines that change.
 *
 *	To remove a selection pass -1 to this routine.
 */
void FPDocument::SetSelection(ChordIndex newSel) {
	SetSelectionRaw((newSel < 0 || newSel == GetCursor()) ? -1 : newSel);
}


/*!
 *	GetSelection
 *
 *  Return the selected range.
 *	Returns false if there is no selection.
 */
ChordIndex FPDocument::GetSelection(ChordIndex *start, ChordIndex *end) const {
	if (Size()) {
		ChordIndex st, en, sel = SelectionEnd(), curs = GetCursor();
		if (sel >= 0) {
			st = MIN(curs, sel);
			en = MAX(curs, sel);
		}
		else
			st = en = curs;
		
		*start = st;
		*end = en;
		return en - st + 1;
	}
	else
		return *start = *end = 0;
}

/*!
 * PartsHavePattern
 *
 *	Returns true if the document has active tones in the specified parts
 *
 *	Empty and bracket-off chords are included unless noChordRequired=false
 */
bool FPDocument::PartsHavePattern(PartMask partMask, bool noChordRequired) const {
	for (ChordIndex num=0;num<Size();num++)
		for (PartIndex part=0; part<DOC_PARTS; part++)
			if ((partMask & BIT(part)) && Chord(num, part).HasPattern(noChordRequired))
				return true;
	
	return false;
}


/*!
 * SelectionHasPattern
 *
 *	Returns true if the selection has active tones
 *	(Used for playing in free edit mode, for example)
 */
bool FPDocument::SelectionHasPattern(PartMask partMask, bool anyChord) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && Chord(num, part).HasPattern(anyChord))
					return true;
	
	return false;
}


/*!
 * SelectionHasTones
 */
bool FPDocument::SelectionHasTones(PartMask partMask) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && Chord(num, part).HasTones())
					return true;
	
	return false;
}


/*!
 * SelectionHasLock
 */
bool FPDocument::SelectionHasLock(PartMask partMask, bool unlocked) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && (unlocked ? !Chord(num, part).IsLocked() : Chord(num, part).IsLocked()))
					return true;
	
	return false;
}


/*!
 * SelectionCanSplay
 */
bool FPDocument::SelectionCanSplay() const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex i=start;i<=end;i++)
			if (ChordGroup(i).Repeat() > 1)
				return true;
	
	return false;
}


/*!
 * SelectionCanCompact
 */
bool FPDocument::SelectionCanCompact() {
	ChordIndex topItem, end;
	if (GetSelection(&topItem, &end) > 1) {
		while (topItem < end) {
			FPChordGroup& cg = ChordGroup(topItem);
			if (cg == ChordGroup(topItem + 1) && cg.Repeat() != MAX_REPEAT)
				return true;
			
			topItem++;
		}
	}
	
	return false;
}


/*!
 * SelectionCanReverse
 */
bool FPDocument::SelectionCanReverse() const {
	ChordIndex start, end;
	if (GetSelection(&start, &end) > 1) {
		while (start < end) {
			if (ChordGroup(start) != ChordGroup(end))
				return true;
			
			start++;
			end--;
		}
	}
	
	return false;
}


bool FPDocument::SelectionCanReverseMelody() const {
	ChordIndex start, end;
	if (GetSelection(&start, &end) > 1) {
		FPChordGroup temp1, temp2;
		
		while (start <= end) {
			if (start == end) {
				for (PartIndex p=0; p<DOC_PARTS; p++)
					if (Chord(start, p).CanReversePattern())
						return true;
			}
			else {
				temp1 = ChordGroup(start);
				temp2 = ChordGroup(end);
				
				for (PartIndex p=DOC_PARTS; p--;) {
					temp1[p].ReversePattern();
					temp2[p].ReversePattern();
				}
				
				if (temp2 != ChordGroup(start) || temp1 != ChordGroup(end))
					return true;
			}
			start++;
			end--;
		}
	}
	
	return false;
}


/*!
 * SelectionCanHFlip
 */
bool FPDocument::SelectionCanHFlip(PartMask partMask) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && Chord(num, part).CanReversePattern())
					return true;
	
	return false;
}

/*!
 * SelectionCanVFlip
 */
bool FPDocument::SelectionCanVFlip(PartMask partMask) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && Chord(num, part).CanFlipPattern())
					return true;
	
	return false;
}

/*!
 * SelectionCanInvert
 */
bool FPDocument::SelectionCanInvert(PartMask partMask) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && Chord(num, part).CanInvertTones())
					return true;
	
	return false;
}

/*!
 * SelectionCanCleanup
 */
bool FPDocument::SelectionCanCleanup(PartMask partMask) const {
	ChordIndex start, end;
	if (GetSelection(&start, &end))
		for (ChordIndex num=start;num<=end;num++)
			for (PartIndex part=0; part<DOC_PARTS; part++)
				if ((partMask & BIT(part)) != 0 && Chord(num, part).CanCleanupTones())
					return true;
	
	return false;
}

/*!
 * TransformSelection
 */
void FPDocument::TransformSelection(MenuCommand cid, MenuItemIndex ind, UInt16 partMask, bool undoable) {
	ChordIndex		startSel, endSel, i;
	UInt16			p;
	if (GetSelection(&startSel, &endSel)) {
		FPHistoryEvent	*event = NULL;
		
		if (undoable) {
			UInt16			undoType;
			MenuCommand		undoCommand = 0;
			CFStringRef		undoName;
			bool			saveIndex = false;
			bool			saveSel = false;
			switch(cid) {
				case kFPCommandSelClearPatterns:
					saveSel = true;
					undoType = UN_S_CLEAR_SEQ;
					undoName = CFSTR("Pattern:Clear Filter");
					break;
					
				case kFPCommandSelHFlip:
					undoCommand = cid;
					undoType = UN_S_HFLIP;
					undoName = CFSTR("Pattern:Reverse Filter");
					break;
					
				case kFPCommandSelVFlip:
					undoCommand = cid;
					undoType = UN_S_VFLIP;
					undoName = CFSTR("Pattern:Flip Filter");
					break;
					
				case kFPCommandSelRandom1:
				case kFPCommandSelRandom2: {
					bool b = (cid == kFPCommandSelRandom1);
					saveSel = true;
					undoType = b ? UN_S_RANDOM1 : UN_S_RANDOM2;
					undoName = b ? CFSTR("Pattern:Random 1 Filter") : CFSTR("Pattern:Random 2 Filter");
					break;
				}
					
				case kFPCommandSelClearTones:
					saveSel = true;
					undoType = UN_S_CLEAR;
					undoName = CFSTR("Tones:Clear Filter");
					break;
					
				case kFPCommandSelInvertTones:
					undoCommand = cid;
					undoType = UN_S_INVERT;
					undoName = CFSTR("Tones:Invert Filter");
					break;
					
				case kFPCommandSelCleanupTones:
					saveSel = true;
					undoType = UN_S_CLEANUP;
					undoName = CFSTR("Tones:Cleanup Filter");
					break;
					
				case kFPCommandSelLockRoots:
					undoCommand = kFPCommandSelUnlockRoots;
					undoType = UN_S_LOCK;
					undoName = CFSTR("Lock Roots Filter");
					break;
					
				case kFPCommandSelUnlockRoots:
					undoCommand = kFPCommandSelLockRoots;
					undoType = UN_S_UNLOCK;
					undoName = CFSTR("Unlock Roots Filter");
					break;
					
				case kFPCommandSelHarmonizeUp:
					saveSel = true;
					undoType = UN_S_HARMUP;
					undoName = CFSTR("Harmonize Filter");
					break;
					
				case kFPCommandSelHarmonizeDown:
					saveSel = true;
					undoType = UN_S_HARMDOWN;
					undoName = CFSTR("Harmonize Filter");
					break;
					
				case kFPCommandSelHarmonizeBy:
					saveSel = true;
					saveIndex = true;
					undoType = UN_S_HARMBY;
					undoName = CFSTR("Harmonize Filter");
					break;
					
				case kFPCommandSelTransposeBy:
					saveSel = true;
					saveIndex = true;
					undoType = UN_S_TRANSBY;
					undoName = CFSTR("Transpose Filter");
					break;
					
				case kFPCommandSelTransposeTo:
					saveSel = true;
					saveIndex = true;
					undoType = UN_S_TRANSTO;
					undoName = CFSTR("Transpose Filter");
					break;
					
				case kFPCommandSelReverseMelody:
					undoCommand = cid;
					undoType = UN_S_REVMEL;
					undoName = CFSTR("Reverse Melody Filter");
					break;
					
				case kFPCommandSelReverseChords:
					undoCommand = cid;
					undoType = UN_S_REVERSE;
					undoName = CFSTR("Reverse Chords Filter");
					break;
					
				case kFPCommandSelScramble:
					saveSel = true;
					undoType = UN_S_SCRAMBLE;
					undoName = CFSTR("Scramble Filter");
					break;
					
				case kFPCommandSelCompact:
					saveSel = true;
					undoType = UN_S_CONSOLIDATE;
					undoName = CFSTR("Consolidate Filter");
					break;
					
				case kFPCommandSelSplay:
					saveSel = true;
					undoType = UN_S_SPLAY;
					undoName = CFSTR("Splay Filter");
					break;
					
				case kFPCommandSelDouble:
					saveSel = true;
					undoType = UN_S_DOUBLE;
					undoName = CFSTR("Double Filter");
					break;
					
				default:
					undoType = UN_CANT;
					undoName = CFSTR("Unknown Action");
					break;
			}
			
			event = window->history->UndoStart(undoType, undoName, partMask);
			event->SaveDataBefore(CFSTR("redoCommand"), cid);
			
			if (undoCommand != 0)
				event->SaveDataBefore(CFSTR("undoCommand"), undoCommand);
			
			if (saveSel)
				event->SaveSelectionBefore();
			
			if (saveIndex)
				event->SaveDataBefore(CFSTR("menuIndex"), ind);
		}
		
		switch(cid) {
			case kFPCommandSelClearPatterns:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).ClearPattern();
				break;
				
			case kFPCommandSelHFlip:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).ReversePattern();
				break;
				
			case kFPCommandSelVFlip:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).FlipPattern();
				break;
				
			case kFPCommandSelRandom1:
			case kFPCommandSelRandom2:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).RandomPattern(cid == kFPCommandSelRandom2);
				break;
				
			case kFPCommandSelClearTones:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++) {
							Chord(i, p).Clear();
							Chord(i, p).NewFingering();
						}
				break;
				
			case kFPCommandSelInvertTones:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++) {
							Chord(i, p).InvertTones();
							Chord(i, p).NewFingering();
						}
				break;
				
			case kFPCommandSelCleanupTones:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).CleanupTones();
				break;
				
			case kFPCommandSelLockRoots:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).Lock();
				break;
				
			case kFPCommandSelUnlockRoots:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).Unlock();
				break;
				
			case kFPCommandSelHarmonizeUp:
			case kFPCommandSelHarmonizeDown:
				ind = (cid == kFPCommandSelHarmonizeUp) ? 1 : NUM_STEPS-1;
				
			case kFPCommandSelHarmonizeBy:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++) {
							Chord(i, p).ResetStepInfo();
							Chord(i, p).HarmonizeBy(ind);
							Chord(i, p).NewFingering();
						}
				break;
				
			case kFPCommandSelTransposeBy:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++) {
							Chord(i, p).TransposeBy(ind);
							Chord(i, p).NewFingering();
						}
				break;
				
			case kFPCommandSelTransposeTo:
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++) {
							Chord(i, p).TransposeTo(ind - 1);
							Chord(i, p).NewFingering();
						}
				break;
				
			case kFPCommandSelReverseMelody:
				
				for (p=DOC_PARTS; p--;)
					if ((partMask & BIT(p)) != 0)
						for (i=startSel; i<=endSel; i++)
							Chord(i, p).ReversePattern();
				
				// fall through
				
			case kFPCommandSelReverseChords: {
				ChordIndex size = (endSel - startSel) / 2;
				for (i=startSel; i<=startSel + size; i++) {
					ChordIndex x = endSel - (i - startSel);
					if (x != i) {
						FPChordGroup temp(ChordGroup(i));
						ChordGroup(i) = ChordGroup(x);
						ChordGroup(x) = temp;
					}
				}
				break;
			}
				
			case kFPCommandSelScramble: {
				for (i=startSel; i<=endSel-1; i++) {
					FPChordGroup temp(ChordGroup(i));
					ChordIndex x = RANDINT(i + 1, endSel);
					ChordGroup(i) = ChordGroup(x);
					ChordGroup(x) = temp;
				}
				break;
			}
				
				
				//
				// Compact
				//
				//	Compact identical chords into repeating chords
				//
			case kFPCommandSelCompact: {
				ChordIndex cmpItem, topItem = startSel;
				while (topItem < endSel) {									// loop as far as the next-to-last item
					FPChordGroup&	cg = ChordGroup(topItem);				// a reference to topItem for convenience
					cmpItem = topItem + 1;									// always compare just the next item
					while (cmpItem <= endSel) {								// if the selection hasn't shrunk too far...
						UInt16			rep = cg.Repeat();					// its current repeat value
						
						if (ChordGroup(cmpItem) != cg || rep == MAX_REPEAT)	// if the item doesn't match, move on
							break;
						
						rep += ChordGroup(cmpItem).Repeat();				// combine the repeats
						
						if (rep > MAX_REPEAT) {								// IF we've gone over 16 then:
							cg.SetRepeat(rep - (rep / 2));					// 1. keep the repeat at 16
							ChordGroup(cmpItem).SetRepeat(rep / 2);			// 2. restore the overage to the test item
							break;											// 3. move on
						}
						else {												// ELSE IF at or under 16 then:
							cg.SetRepeat(rep);								// 1. keep the repeat value
							chordGroupArray.erase(cmpItem);
							endSel--;										// and the selection is shrunk
							
							if (GetCursor() > topItem)						// adjust the cursor and selection as necessary
								SetCursorLine(GetCursor() - 1);
							
							//							ChordIndex sel = SelectionEnd();
							//							if (sel >= 0 && sel > topItem)					// this looks like it does nothing
							//								SetSelectionRaw(sel);
							
						}
					}
					
					topItem = cmpItem;
				}
				break;
			}
				
				
				//
				// Expand
				//
				//	Expand repeated chords to less space
				//
			case kFPCommandSelSplay:
				// fallthrough
				
				
				//
				// Double
				//
				//	Double the amount of time occupied by the
				//	selected chords, spacing out the sequences.
				//
			case kFPCommandSelDouble: {
				bool isDouble = (cid == kFPCommandSelDouble);
				UInt16 mult = isDouble ? 2 : 1;
				
				// 1. Add up the repeats
				ChordIndex addedSize = 0;
				for (i=startSel; i<=endSel; i++)
					addedSize += ChordGroup(i).Repeat() * mult - 1;
				
				chordGroupArray.insert_new(endSel + 1, addedSize);
				
				// 4. Copy to the new space
				ChordIndex src = endSel;
				ChordIndex dst = endSel + addedSize;
				for ( i=endSel-startSel+1; i--; ) {
					// 5. Copy each as many times as it repeats
					for ( int n=ChordGroup(src).Repeat(); n--; ) {
						if (isDouble) {
							ChordGroup(dst) = ChordGroup(src);
							ChordGroup(dst).SetRepeat(1);
							ChordGroup(dst--).StretchPattern(true);
						}
						
						if (dst != src)
							ChordGroup(dst) = ChordGroup(src);
						
						ChordGroup(dst).SetRepeat(1);
						
						if (isDouble)
							ChordGroup(dst).StretchPattern();
						
						dst--;
					}
					src--;
				}
				
				FixSelectionAfterFilter(startSel, endSel, addedSize);
			}
		}
		
		if (undoable) {
			// Only the scramble filter needs to remember the new chords
			if (cid == kFPCommandSelScramble || cid == kFPCommandSelRandom1 || cid == kFPCommandSelRandom2 || cid == kFPCommandSelDouble)
				event->SaveSelectionAfter();
			
			event->Commit();
		}
		
	}
}


/*!
 * CloneSelection
 */
void FPDocument::CloneSelection(ChordIndex count, UInt16 clonePartMask, UInt16 cloneTranspose, UInt16 cloneHarmonize, bool undoable) {
	if (count < 2)
		return;
	
	ChordIndex startSel, endSel;
	if (GetSelection(&startSel, &endSel)) {
		FPHistoryEvent *event = NULL;
		if (undoable) {
			event = window->history->UndoStart(UN_S_CLONE, CFSTR("Clone Filter"), clonePartMask);
			event->SaveDataBefore(CFSTR("count"), count);
			event->SaveDataBefore(CFSTR("transpose"), cloneTranspose);
			event->SaveDataBefore(CFSTR("harmonize"), cloneHarmonize);
		}
		
		ChordIndex size = (endSel-startSel+1);
		ChordIndex addedSize = (count - 1) * size;
		
		chordGroupArray.insert_new(endSel + 1, addedSize);
		
		// Copy to the new space, modifying as set
		ChordIndex dst = startSel + size;
		for (ChordIndex d=count-1; d--; ) {
			for (ChordIndex i=size; i--; ) {
				ChordGroup(dst) = ChordGroup(dst - size);
				
				if (clonePartMask != 0) {
					if (cloneTranspose > 0) {
						for (PartIndex p=DOC_PARTS; p--;)
							if ((clonePartMask & BIT(p)) != 0) {
								Chord(dst, p).TransposeBy(cloneTranspose);
								Chord(dst, p).NewFingering();
							}
					}
					
					if (cloneHarmonize > 0) {
						for (PartIndex p=DOC_PARTS; p--;)
							if ((clonePartMask & BIT(p)) != 0) {
								Chord(dst, p).ResetStepInfo();
								Chord(dst, p).HarmonizeBy(cloneHarmonize);
								Chord(dst, p).NewFingering();
							}
					}
				}
				
				dst++;
			}
		}
		
		SetCursorLine(GetCursor() + addedSize);
		
		if (HasSelection())
			SetSelectionRaw(SelectionEnd() + addedSize);
		
		if (player->IsPlaying(this) && fretpet->IsEditModeEnabled()) {
			ChordIndex newPlayChord = player->PlayingChord() + addedSize;
			player->SetPlayRangeFromSelection();
			player->SetPlayingChord(newPlayChord);
		}
		
		if (undoable)
			event->Commit();
	}
}

/*!
 * FixSelectionAfterFilter
 */
void FPDocument::FixSelectionAfterFilter(ChordIndex startSel, ChordIndex endSel, ChordIndex addedSize) {
	if (HasSelection()) {							// Has a selection (more than one)
		if (GetCursor() == endSel)					// Cursor at the end?
			SetCursorLine(endSel + addedSize);		// Move it up
		else
			SetSelectionRaw(SelectionEnd() + addedSize);		// Move selection end up
	}
	else {
		SetSelectionRaw(startSel);					// set selection to the beginning
		SetCursorLine(endSel + addedSize);			// just move the cursor
	}
	
	if (player->IsPlaying(this) && fretpet->IsEditModeEnabled())
		player->SetPlayRangeFromSelection();
}

/*!
 * ReadyToPlay
 *
 *	Returns true if the selection has something playable
 *	or if the selection is a single chord, all
 */
bool FPDocument::ReadyToPlay() const {
	ChordIndex sel = SelectionSize();
	return (sel == 0) ? false : ( (sel == 1) ? HasPattern() : SelectionHasPattern() );
}

/*!
 * CopySelectionToClipboard
 */
void FPDocument::CopySelectionToClipboard() {
	ChordIndex start, end;
	
	if (GetSelection(&start, &end))
		fretpet->clipboard.Copy(chordGroupArray, CurrentPart(), start, end);
}

#pragma mark -
/*!
 * SetCurrentChord
 */
void FPDocument::SetCurrentChord(const FPChord &srcChord) {
	if (Size()) {
		CurrentChord() = srcChord;
		ChordGroup().SetPatternSize(srcChord.PatternSize());
		ChordGroup().SetRepeat(srcChord.Repeat());
		for (int i=DOC_PARTS;i--;) {
			if (i != CurrentPart()) {
				FPChord &chord = ChordGroup()[i];
				if (!chord.HasPattern() && !chord.rootLock) {
					chord.Set(srcChord.tones, srcChord.key, srcChord.root);
					chord.NewFingering();
					chord.SetRootModifier(srcChord.RootModifier());
					chord.SetRootScaleStep(srcChord.RootScaleStep());
				}
			}
		}
	}
}


/*!
 * HasPattern
 *
 *	Returns true if the document has active tones
 */
bool FPDocument::HasPattern() const {
	if (Size())
		for(ChordIndex num=0;num<Size();num++)
			if (ChordGroup(num).HasPattern())
				return true;
	
	return false;
}

/*!
 * HasFingering
 */
bool FPDocument::HasFingering() const {
	if (Size())
		for(ChordIndex num=0;num<Size();num++)
			if (ChordGroup(num).HasFingering())
				return true;
	
	return false;
}

/*!
 *	UpdateFingerings
 */
void FPDocument::UpdateFingerings() {
	if (Size())
		for (ChordIndex num=0;num<Size();num++)
			for (PartIndex part=DOC_PARTS; part--;)
				guitarPalette->NewFingering(Chord(num, part));
}

