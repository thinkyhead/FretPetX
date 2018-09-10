/*!
 *  @file FPExportFile.cpp
 *
 *	@brief Files with alternative save methods for exporting
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#include "FPExportFile.h"
#include "FPPreferences.h"
#include "FPDocument.h"
#include "FPDocWindow.h"


#pragma mark -
/*!
 * FPExportFile					* CONSTRUCTOR *
 */
FPExportFile::FPExportFile(FPDocument *doc, CFStringRef extension, OSType inType, OSType inCreator) {
	document = doc;
	
	SetDocumentInfo(document->Window(), extension, CFSTR("export"), inCreator, inType);
}

/*!
 * SaveAs
 */
OSStatus FPExportFile::SaveAs() {
	CFStringRef	baseName = (CFStringRef)CFRETAIN(document->BaseName());
	
	if (CFStringEndsWith(baseName, CFSTR(".fret")))
		baseName = CFStringTrimExtension(baseName);
	
	SetName(baseName);
	CFRELEASE(baseName);
	
	return TFile::SaveAs();
}

/*!
 * HandleNavError
 */
void FPExportFile::HandleNavError(OSStatus err) {
	if (err != kErrorHandled) {
		if (err == fBsyErr)
			AlertError(err, CFSTR("The file could not be overwritten because it is in use."));
		else
			AlertError(err, CFSTR("The file could not be saved due to an error."));
	}
}

/*!
 * NavCancel
 */
void FPExportFile::NavCancel() {
	document->DocWindow()->NavCancel();
}

#pragma mark -

#if !DEMO_ONLY

//
// FPMidiFile
//

/*!
 * WriteData
 */
#define kMidiCustomWidth	240
#define kMidiCustomHeight	44

OSErr FPMidiFile::WriteData() {
	OSErr	err = noErr;
	bool	format1 = preferences.GetBoolean(kPrefFormat1, FALSE);
	char	**midiHandle = format1 ? document->GetFormat1() : document->GetFormat0();
	
	if (midiHandle != NULL) {
		char	*buffer = *midiHandle;
		err = Write(buffer, GetHandleSize(midiHandle));
		DisposeHandle(midiHandle);
	}
	else
		err = memFullErr;
	
	return err;
}

/*!
 * NavCustomSize
 */
void FPMidiFile::NavCustomSize(Rect &customRect) {
	if (customRect.bottom - customRect.top < kMidiCustomHeight)
		customRect.bottom = customRect.top + kMidiCustomHeight;
	
	if (customRect.right - customRect.left < kMidiCustomWidth)
		customRect.right = customRect.left + kMidiCustomWidth;
}

/*!
 * NavCustomControls
 */
enum {
	kCustomRunningMode = 1,
	kCustomFormat1
	//	kCustomBankSelect0
};
void FPMidiFile::NavCustomControls(NavDialogRef navDialog, DialogRef dialog) {
	// Add controls from the DITL resource
	Handle ditlHandle = GetResource('DITL', 200);
	OSErr theErr = NavCustomControl(navDialog, kNavCtlAddControlList, ditlHandle);
	
	UInt16 firstControlID;
	NavCustomControl(navDialog, kNavCtlGetFirstControlID, &firstControlID);
	
	ControlRef control;
	
	// Set checked
	bool verbose = preferences.GetBoolean(kPrefVerboseMidi, TRUE);
	theErr = GetDialogItemAsControl(dialog, firstControlID + kCustomRunningMode, &control);
	SetControlValue(control, verbose ? kControlCheckBoxUncheckedValue : kControlCheckBoxCheckedValue);
	
	bool format1 = preferences.GetBoolean(kPrefFormat1, FALSE);
	theErr = GetDialogItemAsControl(dialog, firstControlID + kCustomFormat1, &control);
	SetControlValue(control, format1 ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue);
}

/*!
 * HandleCustomControl
 */
void FPMidiFile::HandleCustomControl(ControlRef inControl, UInt16 controlIndex) {
	SInt16 checked = !::GetControlValue(inControl);
	SetControlValue(inControl, checked);
	
	switch (controlIndex) {
		case kCustomRunningMode:
			preferences.SetBoolean(kPrefVerboseMidi, !checked);
			break;
			
		case kCustomFormat1:
			preferences.SetBoolean(kPrefFormat1, checked);
			break;
			
			//		case kCustomBankSelect0:
			//			preferences.SetBoolean(kPrefBankSelect0, checked);
			//			break;
	}
}

#pragma mark -
//
// FPMovieFile
//

/*!
 * WriteData
 */
OSErr FPMovieFile::WriteData() {
	OSErr					err = noErr;
	Movie					movie = NULL;
	Track					track;
	Media					media;
	MusicDescriptionPtr		music = NULL;
	MusicDescriptionHandle	hMusic = NULL;
	UInt32					*header = NULL, **hTune = NULL;
	long					duration;
	const char				*userDataText = "Created with FretPet X";
	SInt16					resID;
	Handle					hTxt = NULL;
	
	// The file has been created and opened at this point
	//	but we're going to use the Movie Toolbox to write the file.
	Close();
	
	EnterMovies();
	
	// Create a file to hold the movie
	SInt16 resRefNum;
	err = CreateMovieFile(&fileSpec, 'TVOD', smCurrentScript, createMovieFileDeleteCurFile, &resRefNum, &movie);
	require_noerr(err, SaveFail);
	
	// Create a track
	track = NewMovieTrack(movie, 0, 0, kFullVolume);
	
	//
	// Create a media for the track
	//
	media = NewTrackMedia(track, MusicMediaType, 600, NULL, 0);
	
	//
	// Create a music sample description
	//
	header = document->GetTuneHeader();
	require(header!=NULL, SaveFail);
	
	//
	// Allocate space for the MusicDescription
	//
	hMusic = (MusicDescription**)
	NewHandleClear(sizeof(MusicDescription) + GetPtrSize((Ptr)header));
	
	require(hMusic!=NULL, SaveFail);
	
	HLock((Handle)hMusic);
	music = *hMusic;
	
	//
	// Stuff the MusicDescription fields & append the header
	//
	music->descSize = GetHandleSize((Handle)hMusic);
	music->dataFormat = kMusicComponentType;
	BlockMoveData(header, music->headerData, GetPtrSize((Ptr)header));
	
	// Build the tune sequence
	hTune = document->GetTuneSequence(&duration);
	require(hTune!=NULL, SaveFail);
	
	// Add the sample to the media, which writes it to disk
	err = BeginMediaEdits(media);
	require_noerr(err, SaveFail);
	
	err = AddMediaSample(media, (Handle)hTune, 0, GetHandleSize((Handle)hTune),
						 duration, (SampleDescriptionHandle)hMusic, 1, 0, NULL);
	require_noerr(err, SaveFail);
	
	err = EndMediaEdits(media);
	require_noerr(err, SaveFail);
	
	// Setup the track to refer to the whole media
	err = InsertMediaIntoTrack(track, 0, 0, duration, 1L << 16);
	require_noerr(err, SaveFail);
	
	// Create a movie resource to hold some metadata
	err = AddMovieResource(movie, resRefNum, &resID, 0);
	require_noerr(err, SaveFail);
	
	hTxt = (Handle)NewHandle(strlen(userDataText));
	strcpy(*hTxt, userDataText);
	
	err = AddUserDataText(GetMovieUserData(movie), hTxt, kUserDataTextInformation, 1, langEnglish);
	require_noerr(err, SaveFail);
	
	err = UpdateMovieResource(movie, resRefNum, resID, 0);
	require_noerr(err, SaveFail);
	
	err = CloseMovieFile(resRefNum);
	require_noerr(err, SaveFail);
	
SaveFail:
	
	if (err == noErr)
		err = MemError();
	
	ExitMovies();
	
	if (hTxt)	DisposeHandle(hTxt);
	if (header)	DisposePtr((Ptr)header);
	if (hMusic)	DisposeHandle((Handle)hMusic);
	if (hTune)	DisposeHandle((Handle)hTune);
	if (movie)	DisposeMovie(movie);
	
	return err;
}

/*!
 * NavCustomSize
 */
#define kMovieCustomWidth	150
#define kMovieCustomHeight	25
void FPMovieFile::NavCustomSize(Rect &customRect) {
	return;
	
	if (customRect.bottom - customRect.top < kMovieCustomHeight)
		customRect.bottom = customRect.top + kMovieCustomHeight;
	
	if (customRect.right - customRect.left < kMovieCustomWidth)
		customRect.right = customRect.left + kMovieCustomWidth;
}

/*!
 * NavCustomControls
 */
enum {
	kCustomMovieSplit = 1
};
void FPMovieFile::NavCustomControls(NavDialogRef navDialog, DialogRef dialog) {
	return;
	
	// Add a control from a DITL
	Handle ditlHandle = GetResource('DITL', 201);
	OSErr theErr = NavCustomControl(navDialog, kNavCtlAddControlList, ditlHandle);
	
	UInt16 firstControlID;
	NavCustomControl(navDialog, kNavCtlGetFirstControlID, &firstControlID);
	
	bool verbose = preferences.GetBoolean(kPrefMovieSplit);
	
	// Set checked
	ControlRef control;
	theErr = GetDialogItemAsControl(dialog, firstControlID + kCustomMovieSplit, &control);
	SetControlValue(control, verbose ? kControlCheckBoxUncheckedValue : kControlCheckBoxCheckedValue);
}

/*!
 * HandleCustomControl
 */
void FPMovieFile::HandleCustomControl(ControlRef inControl, UInt16 controlIndex) {
	SInt16 checked = !::GetControlValue(inControl);
	SetControlValue(inControl, checked);
	
	switch (controlIndex) {
		case kCustomMovieSplit:
			preferences.SetBoolean(kPrefMovieSplit, !checked);
			break;
	}
}


#pragma mark - FPSunvoxFile

/*!
 * WriteData
 */
OSErr FPSunvoxFile::WriteData() {
	OSErr	err = noErr;
	char	**sunvoxHandle = document->GetSunvoxFormat();
	
	if (sunvoxHandle != NULL) {
		char	*buffer = *sunvoxHandle;
		err = Write(buffer, GetHandleSize(sunvoxHandle));
		DisposeHandle(sunvoxHandle);
	}
	else
		err = memFullErr;
	
	return err;
}

#endif // !DEMO_ONLY
