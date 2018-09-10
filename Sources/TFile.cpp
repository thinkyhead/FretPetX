/*
 *  TFile.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TFile.h"

#include "FPApplication.h"
#include "FPDocument.h"
#include "FPUtilities.h"

#include "TString.h"
#include "TDictionary.h"

NavEventUPP TFile::eventUPP = NULL;
NavEventUPP TFile::openDialogUPP = NULL;
NavObjectFilterUPP TFile::openFilterUPP = NULL;


TFile::TFile() {
	Init();
}


TFile::TFile(const CFStringRef cfPath) {
	Init();
	Specify(cfPath);
}


TFile::TFile(const char *cPath) {
	Init();
	Specify(cPath);
}


TFile::TFile(const FSSpec &spec) {
	Init();
	Specify(spec);
}


TFile::TFile(const FSRef &ref) {
	Init();
	Specify(ref);
}


TFile::TFile(const CFURLRef urlRef) {
	Init();
	Specify(urlRef);
}

TFile::TFile(const FSRef &parentRef, HFSUniStr255 *uniName) {
	Init();
	Specify(parentRef, uniName);
}

TFile::~TFile() {
	if (isOpen)			Close();
	if (displayName)	CFRELEASE(displayName);
	if (fileName)		CFRELEASE(fileName);
	if (fileExt)		CFRELEASE(fileExt);
	if (eventUPP)		DisposeNavEventUPP(eventUPP);
	if (openFilterUPP)	DisposeNavObjectFilterUPP(openFilterUPP);
}


void TFile::Init() {
	static int fileIndex = 0;
	uid = ++fileIndex;

	fileName	= CFCopyLocalizedString(CFSTR("untitled"), "default document name");
	fileExt		= NULL;
	displayName	= NULL;
	hasGoodRef	= false;
	hasGoodSpec	= false;
	exists		= false;
	isOpen		= false;
	write		= false;
	eof			= false;
	keepOpen	= false;
	refNumber	= 0;
	byteCount	= 0;
	creator		= 0;
	type		= 0;

	navAppModal	= false;
	navDialog	= NULL;
	hostWindow	= NULL;
}


TFile& TFile::operator=(const TFile &other) {
	if (this != &other) {
		if (fileName != NULL)
			CFRELEASE(fileName);

		fileName = other.fileName ? CFStringCreateCopy(kCFAllocatorDefault, other.fileName) : NULL;

		if (fileExt != NULL)
			CFRELEASE(fileExt);

		fileExt = other.fileExt ? CFStringCreateCopy(kCFAllocatorDefault, other.fileExt) : NULL;

		if (displayName != NULL)
			CFRELEASE(displayName);

		displayName = other.displayName ? CFStringCreateCopy(kCFAllocatorDefault, other.displayName) : NULL;

		hasGoodRef	= other.hasGoodRef;
		hasGoodSpec	= other.hasGoodSpec;
		exists		= other.exists;
		isOpen		= other.isOpen;
		write		= other.write;
		eof			= other.eof;
		refNumber	= other.refNumber;
		byteCount	= other.byteCount;
		creator		= other.creator;
		type		= other.type;
		hostWindow	= other.hostWindow;
		fileSpec	= other.fileSpec;
		fileRef		= other.fileRef;
		info		= other.info;
		uniFileName	= other.uniFileName;

		navAppModal	= false;
		navDialog	= NULL;
	}

	return *this;
}


int TFile::operator<(const TFile& other) const {
	return kCFCompareLessThan == CFStringCompare(BaseName(), other.BaseName(), kCFCompareCaseInsensitive);
}


int TFile::operator>(const TFile& other) const {
	return kCFCompareGreaterThan == CFStringCompare(BaseName(), other.BaseName(), kCFCompareCaseInsensitive);
}


int TFile::operator<=(const TFile& other) const {
	CFComparisonResult result = CFStringCompare(BaseName(), other.BaseName(), kCFCompareCaseInsensitive);
	return (result == kCFCompareLessThan || result == kCFCompareEqualTo);
}


int TFile::operator>=(const TFile& other) const {
	CFComparisonResult result = CFStringCompare(BaseName(), other.BaseName(), kCFCompareCaseInsensitive);
	return (result == kCFCompareGreaterThan || result == kCFCompareEqualTo);
}


//-----------------------------------------------
//
// SetName
//
//	Use this method to set the fileName attribute
//	of the instance. This name will be used until
//	the file has been saved under a different name.
//
void TFile::SetName(CFStringRef inName) {
	if (fileName != inName) {
		if (fileName)
			CFRELEASE(fileName);

		fileName = inName ? CFStringCreateCopy(kCFAllocatorDefault, inName) : NULL;
	}
}


void TFile::SetExtension(CFStringRef inExt) {
	if (fileExt)
		CFRELEASE(fileExt);

	fileExt = inExt ? (CFStringRef)CFRETAIN(inExt) : NULL;
}


//-----------------------------------------------
//
// SetCreatorAndType
//
// This function sets the Creator and Type codes
// for the file object. If the file exists the
// creator and type are changed on disk. If the
// file doesn't yet exist or if no file has been
// specified then it simply sets the default
// creator and type to be used in future operations.
//
OSErr TFile::SetCreatorAndType(OSType c, OSType t) {
	OSErr err = noErr;

	if (hasGoodSpec && (info.fdType != t || info.fdCreator != c)) {
		info.fdType = t;
		info.fdCreator = c;
		err = FSpSetFInfo(&fileSpec, &info);
	}

	creator = c;
	type = t;

	return err;
}


//-----------------------------------------------
//
// SetDocumentInfo
//
//	Set the attributes of a document file.
//	Call this method in the instantiation
//	a window,
//
void TFile::SetDocumentInfo(WindowRef wind, CFStringRef inExt, CFStringRef inName, OSType c, OSType t) {
	SetOwningWindow(wind);
	SetName(inName);
	SetExtension(inExt);
	SetCreatorAndType(c, t);
}


#pragma mark -
//-----------------------------------------------
//
// Specify(char*)
//
// This can only be used to specify an existing file.
//
// 1. Get a CFStringRef from the char*
// 2. Specify(CFStringRef)
// 3. Release the CFStringRef
//
void TFile::Specify(const char *ppath) {
	Boolean isdir = false;
	OSStatus err = FSPathMakeRef((UInt8*)ppath, &fileRef, &isdir);
	hasGoodRef = (err == noErr);
	hasGoodSpec = false;
	RefreshFileInfo();
}


//-----------------------------------------------
//
// Specify(CFStringRef)
//
// Specify the file with a full POSIX path.
//
// This can only be used to specify an existing
// file. TODO: Save the URL for potential use by
// the Create() method.
//
// 1. Get a CFURLRef from the CFString
// 2. Specify(CFURLRef)
// 3. Release the CFURLRef
//
void TFile::Specify(const CFStringRef cfPath) {
	CFURLRef cfURL = CFURLCreateWithFileSystemPath(kCFAllocatorDefault, cfPath, kCFURLPOSIXPathStyle, false);
	Specify(cfURL);
	CFRELEASE(cfURL);
}


//-----------------------------------------------
//
// Specify(CFURLRef)
//
// Specify the file with a full POSIX path.
//
// This can only be used to specify an existing
// file. TODO: Save the URL for potential use by
// the Create() method.
//
// 1. Get an FSRef from the CFURLRef
// 2. Refresh the file info
//
void TFile::Specify(const CFURLRef uref) {
	hasGoodRef = CFURLGetFSRef(uref, &fileRef);
	hasGoodSpec = false;
	RefreshFileInfo();
}


//-----------------------------------------------
//
// Specify(FSSpec)
//
// This can be used to specify a file that doesn't
// yet exist. You can then use the Create() method
// to cause the file to be created.
//
void TFile::Specify(const FSSpec &spec) {
	fileSpec = spec;
	hasGoodSpec = true;
	hasGoodRef = false;
	RefreshFileInfo();
}


//-----------------------------------------------
//
// Specify(FSRef)
//
// This can only be used to specify an existing file
//
void TFile::Specify(const FSRef &ref) {
	fileRef = ref;
	hasGoodRef = true;
	hasGoodSpec = false;
	RefreshFileInfo();
}


void TFile::Specify(const FSRef &parentRef, HFSUniStr255 *uniName) {
	FSRef newRef;
	(void)FSMakeFSRefUnicode(&parentRef, uniName->length, uniName->unicode, kTextEncodingUnknown, &newRef);
	Specify(newRef);
}


//-----------------------------------------------
//
// RefreshFileInfo
//
// 1. If there isn't a good FSRef try to get one
//		from the FSSpec. (There must be an FSSpec)
//
// 2. Get the full Unicode filename. Save it as an
//		HFSUniStr255 and a CFStringRef for kicks.
//
// Does nothing if the file doesn't exist. Always
//	test Exists() before trusting any other data.
//
void TFile::RefreshFileInfo() {
	OSErr err;

	exists = false;

	if (!hasGoodRef && hasGoodSpec) {
		err = FSpMakeFSRef(&fileSpec, &fileRef);
		hasGoodRef = (err == noErr);
	}

	if (hasGoodRef) {
		if (!hasGoodSpec) {
			err = FSGetCatalogInfo(&fileRef, kFSCatInfoNone, NULL, &uniFileName, &fileSpec, NULL);
			hasGoodSpec = (err == noErr);
		}
		else
			err = FSGetCatalogInfo(&fileRef, kFSCatInfoNone, NULL, &uniFileName, NULL, NULL);

		exists = (err == noErr);

		if (exists) {
			info = GetFInfo();
			CFStringRef	name = CFStringCreateWithCharacters(NULL, uniFileName.unicode, uniFileName.length);
			SetName(name);
			CFRELEASE(name);
		}
		else
			SetName(NULL);
	}
}


void TFile::UpdateProxyIcon() const {
	if (hostWindow) {
		if (hasGoodSpec)
			(void)SetWindowProxyFSSpec(hostWindow, &fileSpec);
		else
			(void)SetWindowProxyCreatorAndType(hostWindow, creator, type, kOnSystemDisk);
	}
}


#pragma mark -
//-----------------------------------------------
//
// Create
//
// Create a file using the available data. When a
// file is specified by a URL or an FSSpec it may
// not yet exist. This method will create the file
// and fill in the FSRef so you can start using it.
//
OSErr TFile::Create() {
	OSErr err = noErr;

	if (hasGoodSpec) {
		err = FSpCreate(&fileSpec, creator, type, smSystemScript);

		if (err == noErr) {
			hasGoodRef = false;
			hasGoodSpec = true;
			RefreshFileInfo();
		}
	}

	return err;
}


OSStatus TFile::Create(const FSRef *parentRef, const UniChar *name, UniCharCount namelen) {
	FSCatalogInfo info;
	BlockZero(&info, sizeof(FSCatalogInfo));

	hasGoodSpec = false;
	hasGoodRef = false;
	OSStatus err = FSCreateFileUnicode(parentRef, namelen, name, 0, NULL, &fileRef, NULL);	// kFSCatInfoFinderInfo
	if (err == noErr) {
		hasGoodRef = true;
		RefreshFileInfo();
		if (creator || type)
			SetCreatorAndType(creator, type);
	}

	return err;
}


OSErr TFile::Delete() {
	OSErr err = FSDeleteObject(&fileRef);

	if (err == noErr)
		hasGoodRef = false;

	return err;
}


OSErr TFile::OpenRead() {
	write = false;
	OSErr err = Open(fsRdPerm);
	return err;
}


OSErr TFile::OpenCurr() {
	write = true;
	OSErr err = Open(fsCurPerm);
	return err;
}


OSErr TFile::OpenWrite(bool appending) {
	OSErr err = noErr;

	if (!exists) {
		err = Create();
		require_noerr(err, OpenFail);
	}

	err = Open(fsWrPerm);
	require_noerr(err, OpenFail);

	if (appending) {
		err = SetFPos(refNumber, fsFromLEOF, 0);
		require_noerr(err, OpenFail);

		err = GetEOF(refNumber, &byteCount);
		require_noerr(err, OpenFail);
	}
	else {
		byteCount = 0;
		err = SetEOF(refNumber, 0);
		require_noerr(err, OpenFail);
	}

	write = true;

OpenFail:
	return err;
}


OSErr TFile::Open(SInt8 perm) {
	OSErr	err = noErr;
	bool	isWrite = (perm == fsWrPerm || perm == fsRdWrPerm || perm == fsCurPerm);

	if (isOpen && write != isWrite)
		Close();

	if (!isOpen)
		err = FSpOpenDF(&fileSpec, perm, &refNumber);

	if (err == noErr) {
		isOpen = true;
		write = isWrite;
		err = SetOffset(0);
	}

	return err;
}


short TFile::OpenResource() {
	resourceRef = FSpOpenResFile(&fileSpec, fsRdPerm);

	return resourceRef;
}


void TFile::Close() {
	OSErr err;

	if (isOpen) {
		if (write)
			err = SetEOF(refNumber, byteCount);

		FSClose(refNumber);
	}

	isOpen = write = eof = false;
	refNumber = 0;
	byteCount = 0;
}


#pragma mark -
OSErr TFile::Read(void *buffer, long len) {
	OSErr	err = FSRead(refNumber, &len, buffer);
	if (err == eofErr)
		eof = true;

	return err;
}


OSErr TFile::Write(const void *buffer, long len) {
	OSErr err = FSWrite(refNumber, &len, buffer);
	byteCount += len;
	return err;
}


OSErr TFile::SetOffset(long offs, short posMode) {
	if (posMode == fsFromLEOF && offs > 0)
		fprintf(stderr, "Offset should be <= 0 for fsFromLEOF!\n");

	OSErr err = SetFPos(refNumber, posMode, offs);

	if (write)
		byteCount = GetOffset();

	return err;
}


void* TFile::ReadAll() {
	long length = Length();
	char *buffer = new char[length];
	SetOffset(0);
	Read(buffer, length);
	return (void*)buffer;
}


SInt32 TFile::WriteXMLFile( const TDictionary &inDict ) {
	CFURLRef	url = URLPath();
	SInt32		errorCode = coreFoundationUnknownErr;

	if (NULL != url) {
		CFDataRef	xmlCFDataRef
			= CFPropertyListCreateXMLData( kCFAllocatorDefault, inDict.GetDictionaryRef() );

		if (NULL != xmlCFDataRef) {
			bool shouldReopen = isOpen && write;

			if (isOpen)
				Close();	// Close so as to not interfere with this atomic operation:

			// Write the XML data to the file.
			(void) CFURLWriteDataAndPropertiesToResource(
							url,						// this file's URL
							xmlCFDataRef,				// the XML data
							NULL,						// properties to write
							&errorCode					// error dest
							);

			if (shouldReopen)
				errorCode = OpenAppend();

			// Release the XML data
			CFRELEASE(xmlCFDataRef);
		}
	}

	return errorCode;
}


TDictionary* TFile::ReadXMLFile() {
	TDictionary			*outDict = NULL;
	CFDataRef			xmlCFDataRef;
	CFPropertyListRef	myCFPropertyListRef = NULL;
	Boolean				status;

	// Read the XML file.
	status = CFURLCreateDataAndPropertiesFromResource(
						kCFAllocatorDefault,
						URLPath(),					// the URL of the file
						&xmlCFDataRef,				// the CFData
						NULL,						// properties
						NULL,						// desiredProperties
						NULL						// errorCode dest
						);
	if (status) {
		// Reconstitute the dictionary using the XML data.
		myCFPropertyListRef = CFPropertyListCreateFromXMLData(
						kCFAllocatorDefault,
						xmlCFDataRef,				// the data ref
						kCFPropertyListImmutable,	// mutability option
						NULL						// error string dest
						);

		if (myCFPropertyListRef != NULL)
			outDict = new TDictionary((CFDictionaryRef)myCFPropertyListRef);

		// Release the XML data
		CFRELEASE(xmlCFDataRef);
	}

	return outDict;
}


#pragma mark -
UInt64 TFile::Length() const {
	UInt64 length = 0;

	if (exists) {
		FSCatalogInfo	catInfo;
		(void)FSGetCatalogInfo(&fileRef, kFSCatInfoDataSizes, &catInfo, NULL, NULL, NULL);
		length = catInfo.dataLogicalSize;
	}

	return length;
}


CFStringRef TFile::DisplayName() {
	OSStatus			err = fnfErr;

	if (displayName)
		CFRELEASE(displayName);

	if (hasGoodRef)
		err = LSCopyDisplayNameForRef(&fileRef, &displayName);

	if (err != noErr) {
		if (fileName)
			displayName = CFStringCreateCopy(kCFAllocatorDefault, fileName);
		else
			displayName	= CFCopyLocalizedString(CFSTR("untitled"), "default display name");
	}

	return CFStringCreateCopy(kCFAllocatorDefault, displayName);
}

TString TFile::PrettyName() {
	TString displayName = DisplayName();
	displayName.EndsWith(CFSTR(".fret"), true);
	displayName.ChopRight(5);
	return displayName;
}


char* TFile::PosixPath() const {
	char path[2048];
	char *outPath = NULL;
	OSStatus err = FSRefMakePath(&fileRef, (UInt8*)path, 2048);
	if (err == noErr) {
		outPath = new char[strlen(path)+1];
		strcpy(outPath, path);
	}

	return outPath;
}


char* TFile::MacPath() const {
	char *outPath = PosixPath();
	if (outPath != NULL) {
		TString workString = outPath;
		delete [] outPath;

		if (workString.Left() != CFSTR("/"))
			workString.Prepend(CFSTR("/"));

		if (workString.Left(8) == CFSTR("/Volumes"))
			workString.ChopLeft(8);
		else {
			Str255	volName;
			short	vRefNum;
			long	dirID;
			(void)HGetVol(volName, &vRefNum, &dirID);

			TString volPath;
			volPath.SetFromPascalString(volName);
			workString.Prepend(volPath.GetCFStringRef());
		}

		if (workString.Left() == CFSTR("/"))
			workString.ChopLeft();

		workString.Replace(CFSTR("/"), CFSTR(":"));
		outPath = workString.GetCString();
	}

	return outPath;
}


CFURLRef TFile::URLPath() const {
	static CFURLRef urlRef = NULL;

	if (urlRef) CFRELEASE(urlRef), urlRef = NULL;

	if (hasGoodRef)
		urlRef = CFURLCreateFromFSRef(kCFAllocatorDefault, &fileRef);

	return urlRef;
}


CFStringRef TFile::MakeNameWithExtension(CFStringRef inString) const {
	CFIndex		len = CFStringGetLength(inString);
	CFIndex		xlen = fileExt ? CFStringGetLength(fileExt) : 0;
	CFRange		range = CFRangeMake(0, (len+xlen>255) ? 255 - xlen : len);
	TString		nameBase(inString, range);

	TString		nameWithExt(nameBase);
	if (fileExt && !nameBase.EndsWith(fileExt))
		nameWithExt += fileExt;

	return nameWithExt.GetCFString();
}


AliasHandle TFile::GetAliasHandle() const {
	AliasHandle	alias = NULL;

	if (hasGoodRef)
		(void)FSNewAlias(NULL, &fileRef, &alias);

	return alias;
}


Boolean TFile::Resolve(unsigned long flags) {
	Boolean result = false;
	if (Exists()) {
		Boolean		wasChanged;
		FSRef		realRef;
		AliasHandle	theAlias = GetAliasHandle();
		OSStatus	err = FSResolveAliasWithMountFlags(&fileRef, theAlias, &realRef, &wasChanged, flags);

		// if not resolvable then leave alone and return false
		if (err == fnfErr) {
			result = false;
		}
		else if (err == noErr) {
			// result is true if the original and new are different files
			if (FSCompareFSRefs( &fileRef, &realRef ) != noErr) {
				result = true;
				Specify(realRef);
			}
		}
	}
	return result;
}

#pragma mark -
//-----------------------------------------------
//
// AskSaveChanges
//
// Presents a Confirm Save dialog that runs by itself.
//
// When the user clicks the Save button the SaveChanges
// method of the file object is called.
//
// An error is returned if the dialog cannot be created.
//
OSStatus TFile::AskSaveChanges(Boolean quitting) {
	OSStatus					theErr = noErr;
	NavAskSaveChangesAction		action = 0;
	NavDialogRef				dialog = NULL;
//	NavUserAction				userAction = kNavUserActionNone;
	NavDialogCreationOptions	dialogOptions;
	Boolean						disposeAfterRun;
	bool						modal;

	//
	// Even a "Confirm Save" dialog has options
	//
	NavGetDefaultDialogCreationOptions(&dialogOptions);

	//
	// Modality based on whether attached to a window or standalone
	//
	modal = (hostWindow == NULL || TWindow::GetTWindow(hostWindow)->RunDialogsModal());
	dialogOptions.modality = modal ? kWindowModalityAppModal : kWindowModalityWindowModal;

	//
	// Parent window in any case
	//
	dialogOptions.parentWindow	= modal ? NULL : hostWindow;
	dialogOptions.clientName	= gAppName;
	dialogOptions.saveFileName	= DisplayName();

	//
	// Appearance will depend on the context of the dialog
	//
	action = quitting ? kNavSaveChangesQuittingApplication : kNavSaveChangesClosingDocument;

	//
	// Dispose if running "app modal" and there's no context data
	//
	disposeAfterRun = modal;

	//
	// Create the dialog
	//
	theErr = NavCreateAskSaveChangesDialog(
				&dialogOptions,					// the dialog description
				action,							// the dialog situation
				GetNavEventUPP(),				// the event handler
				this,							// userdata for the dialog event handler
				&dialog );						// save the dialog pointer

	CFRELEASE(dialogOptions.saveFileName);

	if ( theErr == noErr ) {
		//
		// Start the dialog running
		//
		theErr = NavDialogRun( dialog );

		//
		// If the dialog won't run or it's supposed to be disposed...
		//
		if ( theErr != noErr || disposeAfterRun ) {
			//
			// Get the user action and dispose the dialog
			//
			//userAction = NavDialogGetUserAction( dialog );
			//NavDialogDispose( dialog );
			dialog = NULL;
		}
	}

	navDialog = dialog;
	return theErr;
}


//-----------------------------------------------
//
// ConfirmRevert
//
// Presents a Confirm Revert dialog that runs by itself.
//
// When the user clicks the Revert button the Revert()
// method of the file object is called.
//
// An error is returned if the dialog cannot be created.
//
OSStatus TFile::ConfirmRevert() {
	OSStatus					theErr = noErr;
	NavDialogRef				dialog = NULL;
//	NavUserAction				userAction = kNavUserActionNone;
	NavDialogCreationOptions	dialogOptions;
	Boolean						disposeAfterRun;
	bool						modal;

	//
	// Even a "Confirm Save" dialog has options
	//
	NavGetDefaultDialogCreationOptions(&dialogOptions);

	//
	// Modality based on whether attached to a window or standalone
	//
	modal = (hostWindow == NULL || TWindow::GetTWindow(hostWindow)->RunDialogsModal());
	dialogOptions.modality = modal ? kWindowModalityAppModal : kWindowModalityWindowModal;

	//
	// Parent window in any case
	//
	dialogOptions.parentWindow	= modal ? NULL : hostWindow;
	dialogOptions.clientName	= gAppName;
	dialogOptions.saveFileName	= DisplayName();

	//
	// Dispose if running "app modal" and there's no context data
	//
	disposeAfterRun = modal;

	//
	// Create the dialog
	//
	theErr = NavCreateAskDiscardChangesDialog(
				&dialogOptions,					// the dialog description
				GetNavEventUPP(),				// the event handler
				this,							// userdata for the dialog event handler
				&dialog );						// save the dialog pointer

	CFRELEASE(dialogOptions.saveFileName);

	if ( theErr == noErr ) {
		//
		// Start the dialog running
		//
		theErr = NavDialogRun( dialog );

		//
		// If the dialog won't run or it's supposed to be disposed...
		//
		if ( theErr != noErr || disposeAfterRun ) {
			//
			// Get the user action and dispose the dialog
			//
//			userAction = NavDialogGetUserAction( dialog );
//			HandleNavUserAction( userAction );
//			NavDialogDispose( dialog );
			dialog = NULL;
		}
	}

	navDialog = dialog;
	return theErr;
}


#pragma mark -
void TFile::SaveChanges(bool modal) {
	if (exists) {
		OSErr err = OpenAndSave();

		if (err == noErr)
			NavDidSave();
		else if (err != kErrorHandled) {
			TString message;
			message.SetWithFormatLocalized(CFSTR("The file \"%@\" could not be saved."), fileName);
			AlertError(err, message.GetCFStringRef());
		}
	}
	else
		SaveAs(modal);
}


OSErr TFile::OpenAndSave() {
	OSErr err = noErr;

	if (refNumber == 0 || !write)
		err = OpenWrite();
	else
		SetOffset(0);

	if (err == noErr) {
		err = WriteData();
		Close();

		if (err == noErr) {
			RefreshFileInfo();

			if (keepOpen)
				OpenAppend();		// This prevents the file being truncated if closed without writing
		}
	}
	else
		Close();

	return err;
}


//-----------------------------------------------
//
// SaveAs
//
// SaveAs presents a Save dialog that runs by itself.
//
// When the user clicks the Save button the Save()
// method of the file object is called. Override
// WriteData() to write out the file's data. Open
// and Close are called before and after.
//
// An error is returned if the dialog cannot be created.
//
OSStatus TFile::SaveAs(bool inModal) {
	NavDialogCreationOptions	dialogOptions;
	OSStatus					theErr = noErr;

	//
	// Dialog options
	//
	bool modal = inModal || hostWindow == NULL || TWindow::GetTWindow(hostWindow)->RunDialogsModal();
	NavGetDefaultDialogCreationOptions(&dialogOptions);
	dialogOptions.clientName	= gAppName;
	dialogOptions.saveFileName	= SaveFileName();
	dialogOptions.modality		= modal ? kWindowModalityAppModal : kWindowModalityWindowModal;
	dialogOptions.parentWindow	= modal ? NULL : hostWindow;
	dialogOptions.optionFlags	|= kNavPreserveSaveFileExtension;

    CFStringRef nabl = NavActionButtonLabel();
	if (nabl != NULL)
		dialogOptions.actionButtonLabel = nabl;

    CFStringRef ncbl = NavCancelButtonLabel();
	if (ncbl != NULL)
		dialogOptions.cancelButtonLabel = ncbl;

	CFStringRef msg = NavSaveMessage();
	if (msg != NULL)
		dialogOptions.message = msg;


	//
	// Make a Save dialog with whatever userData
	//
	// NOTE: If no userData is supplied then no action
	// will be taken when MyEventProc is called.
	//
	theErr = NavCreatePutFileDialog(
				&dialogOptions,
				type,
				creator,
				GetNavEventUPP(),
				this,
				&navDialog);

	if (theErr == noErr) {
		//
		// Start the dialog running
		//
		theErr = NavDialogRun(navDialog);

		//
		// Kill it on failure
		//
		if (theErr != noErr)
			NavDialogDispose(navDialog);

		//
		// If the dialog failed or it's running
		// app-modal then it's already gone
		//
		if (theErr != noErr || dialogOptions.modality == kWindowModalityAppModal)
			navDialog = NULL;
	}

	CFRELEASE(dialogOptions.saveFileName);
	if (msg) CFRELEASE(msg);

	return theErr;
}


OSStatus TFile::BeginSave() {
	OSStatus		status = paramErr;
	AEDesc			dirDesc;
	AEKeyword		keyword;
	CFIndex			len;
	HFSUniStr255	uniName;
	CFStringRef		nameWithExt;

	//
	// Get the reply from the dialog and fail on error
	//
	status = NavDialogGetReply(navDialog, &navReply);
	nrequire(status, Return);

	//
	// Get the first item from the reply in the form
	// of an AEDesc (An Apple Event data storage unit)
	//
	status = AEGetNthDesc(&navReply.selection, 1, typeWildCard, &keyword, &dirDesc);
	nrequire(status, DisposeReply);

	//
	// Get the save name as a Unicode string and
	// append the extension if it isn't there.
	//
	//	The extension should always be present if
	//	navigation dialogs aren't hacked, but a bug
	//	in Default Folder can cause an extension-less
	//	name to get through
	//
	nameWithExt = MakeNameWithExtension(navReply.saveFileName);

	// Convert to Unicode
	uniName.length = len = CFStringGetLength(nameWithExt);
	CFStringGetCharacters(nameWithExt, CFRangeMake(0, len), uniName.unicode);

	// Release and replace the existing name
	CFRELEASE(navReply.saveFileName);
	navReply.saveFileName = nameWithExt;

	//
	// The reply may contain an FSRef for the destination folder.
	// (This should always be the case for Mac OS X.)
	//
	if (dirDesc.descriptorType == typeFSRef) {
		FSRef			dirRef;						// FSCreateFileUnicode wants an FSRef for the parent folder

		//
		// Get the directory from the AEDesc
		//
		status = AEGetDescData(&dirDesc, &dirRef, sizeof(dirRef));
		nrequire(status, DisposeDesc);

		//
		// Try to get an FSRef for the file
		//
		Specify(dirRef, &uniName);

		//
		// If the file doesn't exist create it (also refreshes info)
		//
		if (!exists)
			status = Create(&dirRef, uniName.unicode, len);

		navFileRef = fileRef;


/*
		else {
			//
			// If some other error occurred bail out
			//
			nrequire(status, DisposeDesc);
		}
*/
	}
	//
	//	...or it might contain an FSSpec....
	//	(...but this should never happen on Mac OS X.)
	//
	else if (dirDesc.descriptorType == typeFSS) {
		FSSpec	theSpec;

		//
		// Get the FSSpec from the AEDesc
		//
		status = AEGetDescData( &dirDesc, &theSpec, sizeof( FSSpec ));
		nrequire( status, DisposeDesc );

		//
		// Get the save name and set it in the FSSpec
		//
		if ( CFStringGetPascalString(
				navReply.saveFileName,
				theSpec.name,
				sizeof(StrFileName),
				GetApplicationTextEncoding()))  {
			//
			// Try to get a reference for the save file
			// Bail on success
			//
			status = FSpMakeFSRef(&theSpec, &navFileRef);
			nrequire( status, DisposeDesc );

			//
			// Try to create the save file
			//
			status = FSpCreate(&theSpec, creator, type, smSystemScript);
			nrequire( status, DisposeDesc );
		}
		else {
			//
			// The name conversion failed!
			//
			status = bdNamErr;
			nrequire( status, DisposeDesc );
		}
	}

DisposeDesc:
	AEDisposeDesc( &dirDesc );

DisposeReply:
	if (status != noErr)					// Not sure whether this line is needed
		NavDisposeReply(&navReply);

Return:
	return status;
}


//
// CompleteSave
//
// Call after saving a document, passing back the reply returned by
// BeginSave. This call performs any file translation needed and disposes the reply.
// If the save didn't succeed, pass false for inDidWriteFile to avoid
// translation but still dispose the reply.
//
OSStatus TFile::CompleteSave(Boolean inDidWriteFile) {
	OSStatus theErr = noErr;

	if ( navReply.validRecord ) {
		if ( inDidWriteFile ) {
			theErr = NavCompleteSave(&navReply, kNavTranslateInPlace);
		}
		else if (!navReply.replacing) {
			// Write failed but not replacing, so delete the file
			// that was created in BeginSave.
			FSDeleteObject(&navFileRef);
		}

		(void)NavDisposeReply(&navReply);
	}

	return theErr;
}


void TFile::HandleNavError(OSStatus err) {
	NavCancel();
/*
	// ConductErrorDialog( status, cSave, cancel );
*/
}


void TFile::HandleNavUserAction(NavUserAction inUserAction) {
	OSStatus	status = noErr;

	switch (inUserAction) {
		case kNavUserActionCancel: {
			NavCancel();
		}
		break;

		//
		// kNavUserActionSaveChanges
		// Affirmative reply received in response to a "Save Changes" dialog
		//
		case kNavUserActionSaveChanges: {
			SaveChanges();
		}
		break;

		//
		// kNavUserActionDontSaveChanges
		// Negative reply received in response to a "Save Changes" dialog
		//
		case kNavUserActionDontSaveChanges: {
			NavDontSave();
		}
		break;

		//
		// kNavUserActionDontSaveChanges
		// Negative reply received in response to a "Save Changes" dialog
		//
		case kNavUserActionDiscardChanges: {
			NavDiscardChanges();
		}
		break;

		//
		// kNavUserActionSaveAs
		// Save button clicked in a Save dialog
		//
		case kNavUserActionSaveAs: {
			OSStatus			completeStatus;

			TFile tempFile(*this);

			status = BeginSave();

			if (status == noErr) {
				status = OpenAndSave();
				completeStatus = CompleteSave(status == noErr);

				if (status == noErr)
					status = completeStatus;
			}

			if (status == noErr)
				NavDidSave();
			else
				*this = tempFile;
		}
		break;
	}

	if (status != noErr)
		HandleNavError(status);
}


NavEventUPP TFile::GetNavEventUPP() {
	if (eventUPP == NULL)
		eventUPP = NewNavEventUPP(TFile::NavEventProc);

	return eventUPP;
}


pascal void TFile::NavEventProc(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD) {
	TFile	*file = (TFile*)callbackUD;
	file->HandleNavEvent(callbackSelector, callbackParms);
}


void TFile::HandleNavEvent(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms) {
	switch ( callbackSelector ) {
		case kNavCBEvent: {
			//
			// Event generated by a custom control
			// also: New Folder button
			//
//			fprintf(stderr, "What Event: %d\n", callbackParms->eventData.eventDataParms.event->what);
//			fprintf(stderr, "Item Hit: %d\n", callbackParms->eventData.itemHit);

			if (callbackParms->eventData.eventDataParms.event->what == mouseDown && callbackParms->eventData.itemHit != -1) {
				// Get the first custom control id
				UInt16 firstControlID;
				NavCustomControl(callbackParms->context, kNavCtlGetFirstControlID, &firstControlID);

				// Get the control from the dialog
				ControlRef	controlHit;
				UInt16		controlIndex = callbackParms->eventData.itemHit - firstControlID;
				(void)GetDialogItemAsControl(GetDialogFromWindow(callbackParms->window), controlIndex, &controlHit);

				// Call the file class's method to handle the control
				HandleCustomControl(controlHit, controlIndex);
			}

/*
			//
			// These don't seem to apply to Mac OS X
			//
			switch (callbackParms->eventData.eventDataParms.event->what) {
				case updateEvt:
				case activateEvt:
					fretpet->HandleEvent(callbackParms->eventData.eventDataParms.event);
				break;
			}
*/
		}
		break;

		case kNavCBCustomize: {
			NavCustomSize(callbackParms->customRect);
		}
		break;

		case kNavCBStart: {
			DialogRef dialog = GetDialogFromWindow(callbackParms->window);
			if (dialog != NULL) {
				NavCustomControls(callbackParms->context, dialog);
			}
		}
		break;

		case kNavCBUserAction: {
			HandleNavUserAction(callbackParms->userAction);
		}
		break;

		case kNavCBTerminate: {
			NavDialogDispose(callbackParms->context);

			if (navDialog == callbackParms->context)
				navDialog = NULL;
		}
		break;

		default:
		break;
	}
}


NavObjectFilterUPP TFile::GetNavFilterUPP() {
	if (openFilterUPP == NULL)
		openFilterUPP = NewNavObjectFilterUPP(TFile::NavFilterProc);

	return openFilterUPP;
}


bool TFile::NavFilter(AEDesc *theItem, void *info, NavFilterModes filterMode) {
	NavFileOrFolderInfo	*nfi = (NavFileOrFolderInfo*) info;
	FSSpec				spec;
	FSRef				fref;
	OSType				ft;

	if ( nfi->isFolder )
		ft = nfi->fileAndFolder.folderInfo.folderType;
	else
		ft = nfi->fileAndFolder.fileInfo.finderInfo.fdType;

	switch( theItem->descriptorType ) {
		case typeFSS:
			AEGetDescData( theItem, &spec, sizeof( FSSpec ));
			return ShouldDisplayObject( &spec, ft, nfi->isFolder );
			break;

		case typeFSRef:
			AEGetDescData( theItem, &fref, sizeof( FSRef ));
			return ShouldDisplayObject( &fref, ft, nfi->isFolder );
			break;
	}

	return true;
}


pascal Boolean TFile::NavFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode) {
	Boolean show = true;

	TFile *file = (TFile*)callBackUD;

	if (file)
		show = file->NavFilter(theItem, info, filterMode);

	return show;
}


