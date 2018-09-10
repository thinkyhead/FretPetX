/*!
 *	@file	FPExportFile.h
 *
 *	@brief	Subclasses of TFile for export formats
 *
 *	@section COPYRIGHT
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPEXPORTFILE_H
#define FPEXPORTFILE_H

#include "TFile.h"
#include <QuickTime/QuickTime.h>

class FPDocument;

#pragma mark -
/*!
 *	A helper class to export in FretPet format.
 */
class FPExportFile : public TFile {
	protected:
		FPDocument	*document;

	public:
					FPExportFile(FPDocument *doc, CFStringRef extension, OSType t, OSType c=0);
		virtual		~FPExportFile() {}

		// TFile Overrides:
		OSStatus	SaveAs();
		void		NavCancel();
		void		HandleNavError(OSStatus err);
		CFStringRef	NavActionButtonLabel()	{ return CFSTR("Export"); }
};

#if !DEMO_ONLY

#pragma mark -
/*!
 *	A helper class to export in MIDI format.
 */
class FPMidiFile : public FPExportFile {
	public:
					FPMidiFile(FPDocument *doc) : FPExportFile(doc, CFSTR(".mid"), 'midi') {}
		virtual		~FPMidiFile() {}

		// TFile Overrides:
		OSErr		WriteData();
		void		NavCustomSize(Rect &customRect);
		void		NavCustomControls(NavDialogRef navDialog, DialogRef dialog);
		void		HandleCustomControl(ControlRef inControl, UInt16 controlIndex);
		CFStringRef	NavSaveMessage() { return CFCopyLocalizedString( CFSTR("Export as a Standard MIDI Format (SMF) file"), "Export MIDI dialog heading text" ); }
};

#pragma mark -
/*!
 *	A helper class to export in MOV format.
 */
class FPMovieFile : public FPExportFile {
	public:
					FPMovieFile(FPDocument *doc) : FPExportFile(doc, CFSTR(".mov"), kQTFileTypeMovie, 'TVOD') {}
		virtual		~FPMovieFile() {}

		// TFile Overrides:
		OSErr		WriteData();
		void		NavCustomSize(Rect &customRect);
		void		NavCustomControls(NavDialogRef navDialog, DialogRef dialog);
		void		HandleCustomControl(ControlRef inControl, UInt16 controlIndex);
		CFStringRef	NavSaveMessage() { return CFCopyLocalizedString( CFSTR("Export as a QuickTime Music Movie file"), "Export movie nav dialog heading" ); }
};

#pragma mark -
/*!
 *	A helper class to export in Sunvox format.
 */
class FPSunvoxFile : public FPExportFile {
public:
	FPSunvoxFile(FPDocument *doc) : FPExportFile(doc, CFSTR(".sunvox"), '????') {}
	virtual		~FPSunvoxFile() {}
	
	// TFile Overrides:
	OSErr		WriteData();
	CFStringRef	NavSaveMessage() { return CFCopyLocalizedString( CFSTR("Export as a Sunvox Sequencer file"), "Export Sunvox nav dialog heading" ); }
};

#endif // !DEMO_ONLY

#define kPrefVerboseMidi	CFSTR("MidiVerbose")
#define kPrefFormat1		CFSTR("MidiFormat1")
#define kPrefMovieSplit		CFSTR("MovieSplit")
#define kPrefFormat214		CFSTR("Format214")


#endif
