/*
 *  TFile.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *  The TFile class represents a file that may or may not
 *	exist. It extends the file paradigm to provide basic
 *	support for documents - files associated with an
 *	application that have their own window while open.
 *
 *	If the file doesn't exist you can treat this as the
 *	representation of a file type. Then when you want to
 *	get such a file via NavGetFileDialog use the 
 *
 *	NavServicesSupport.cpp contains some of the file-related
 *	methods that don't necessarily belong here.
 *
 *
 */

#ifndef TFILE_H
#define TFILE_H

class TDictionary;
class TString;

//! Wrapper for a file with some document handling abilities
class TFile {
private:
	static NavEventUPP	eventUPP;				//!< UPP for Navigation dialog events
	static NavEventUPP	openDialogUPP;			//!< UPP for Open dialog events
	static NavObjectFilterUPP openFilterUPP;	//!< UPP for the Open filter function

	bool			navAppModal;	//!< Whether the dialog is app modal
	WindowRef		hostWindow;		//!< The parent window of the sheet
	NavDialogRef	navDialog;		//!< An associated nav dialog
	NavReplyRecord	navReply;		//!< The reply from the dialog
	FSRef			navFileRef;		//!< An FSRef from a save dialog
	CFStringRef		fileExt;		//!< A string with the file extension
	CFStringRef		displayName;	//!< Name for display is based on user prefs

protected:
									// Update these with RefreshFileInfo()
	int				uid;
	CFStringRef		fileName;		//!< The name as a CFStringRef
	HFSUniStr255	uniFileName;	//!< The name as a Unicode string
	OSType			creator;		//!< Creator as a 4 character code
	OSType			type;			//!< Type as a 4 character code

	FSSpec			fileSpec;		//!< The filespec used to identify a file
	FSRef			fileRef;		//!< The fileref for the file (if existing)
	short			resourceRef;	//!< Resource file reference
	FInfo			info;			//!< Finder Info for the file
	bool			hasGoodRef;		//!< Flag if an FSRef has been created
	bool			hasGoodSpec;	//!< Flag if an FSSpec has been created
	bool			exists;			//!< Flag if the file exists

	short			refNumber;		//!< Reference number of an opened file
	bool			isOpen;			//!< If the file is open
	bool			eof;			//!< Reading reached the end of the file
	bool			write;			//!< Open in a writeable mode
	bool			keepOpen;		//!< If TRUE leave open after a Save
	long			byteCount;		//!< Number of bytes written

public:
	//! Default constructor creates an unassociated file object
	TFile();


	/*! Clone another file object.
		@param other another file object
	*/
	TFile(const TFile &other)		{ Init(); *this = other; }


	/*! Construct with a url.
		@param urlRef the url of a file
	*/
	TFile(const CFURLRef urlRef);


	/*! Construct with a stringref POSIX path.
		@param ppath the path to the file
	*/
	TFile(const CFStringRef ppath);


	/*! Construct with a c string POSIX path.
		@param ppath the path to the file
	*/
	TFile(const char *ppath);


	/*! Construct with an FSSpec.
		@param fsSpec an old-style file spec
	*/
	TFile(const FSSpec &fsSpec);


	/*! Construct with an FSRef
		The file must already exist!
		@param fsRef an opaque file ref
	*/
	TFile(const FSRef &fsRef);

	/*! Construct with a parent folder and unicode string
		The file must already exist!
		@param parentRef an opaque file ref for the containing folder
		@param uniName the unicode name of the file
	*/
	TFile(const FSRef &parentRef, HFSUniStr255 *uniName);

	//! Destructor
	virtual ~TFile();

	//! Shared Init code for all constructors
	void Init();


	/*! Set the name of the file.
		Used internally to set the file's name info
		@param inName the new name for the file
	*/
	void SetName(CFStringRef inName);


	/*! Set the extension of the file.
		Used internally to set the file's extension info
		@param inExt the new extension for the file
	*/
	void SetExtension(CFStringRef inExt);


	/*! Set the creator and type info of the file.
		Used internally to set the file's creator/type info
		@param c the new creator
		@param t the new type
	*/
	OSErr SetCreatorAndType(OSType c, OSType t);


	/*! Accessor for the keepOpen flag.
		@result Whether to leave the file open after OpenAndSave
	*/
	inline void KeepOpen() { keepOpen = true; }


	/*! Specify the window that hosts the document file.
	
		@param window the window to associate
	*/
	void SetOwningWindow(WindowRef window) { hostWindow = window; }


	/*! Set the combined information about a document file.
		@param window the window hosting the document
		@param inExt the file extension
		@param inName the file name
		@param c the file creator code
		@param t the file type
	*/
	void SetDocumentInfo(WindowRef window, CFStringRef inExt, CFStringRef inName, OSType c, OSType t);

	//! Update the owning window's proxy icon
	void UpdateProxyIcon() const;


	/*! Specify with an FSRef.
		The file must already exist!
		@param fsRef an opaque file ref
	*/
	void Specify(const FSRef &fsRef);


	/*! Specify with an FSSpec.
		@param fsSpec an old-style file spec
	*/
	void Specify(const FSSpec &fsSpec);


	/*! Specify with a url.
		@param urlRef the url of a file
	*/
	void Specify(const CFURLRef urlRef);


	/*! Specify with a stringref POSIX path.
		@param ppath the path to the file
	*/
	void Specify(const CFStringRef ppath);


	/*! Specify with a c string POSIX path.
		@param ppath the path to the file
	*/
	void Specify(const char *ppath);


	/*! Specify with a parent FSRef and Unicode name.
		@param parentRef the containing folder
		@param uniName the unicode name of the file
	*/
	void Specify(const FSRef &parentRef, HFSUniStr255 *uniName);

	//! Update the internal state of the file object
	void RefreshFileInfo();


	/*! Accessor for the exists flag
		@result TRUE if the file exists on disk
	*/
	inline bool Exists() const { return exists; }


	/*! Accessor for the fileName data
		@result the name of the file
	*/
	inline CFStringRef BaseName() const { return fileName; }


	/*! Get the Finder displayed name of the file, or "untitled"
		@result a name suitable for display
	*/
	CFStringRef DisplayName();

	/*! Get the name of the file with no extension
	 @result a name suitable for all uses
	 */
	TString PrettyName();

	/*! Get the path of the file in POSIX format
		@result the POSIX path
	*/
	char* PosixPath() const;


	/*! Get the path of the file in Mac format
		@result the Mac path
	*/
	char* MacPath() const;


	/*! Get the path of the file as a URL
		@result the URL
	*/
	CFURLRef URLPath() const;


	/*! Make a name with extension
		This is used in the Save dialog
		@result a full candidate filename with extension
	*/
	CFStringRef MakeNameWithExtension(CFStringRef inString) const;


	/*! The byte length of the file
		@result file length as a 64 bit integer
	*/
	UInt64 Length() const;


	/*! Get the Finder Info for the file
		@result the Finder Info of the file
	*/
	inline FInfo GetFInfo() const {
		FInfo info;
		FSpGetFInfo(&fileSpec, &info);
		return info;
	}


	/*! Get the FSRef of the file
		@result the FSRef of the file
	*/
	inline const FSRef&	FileRef() const { return fileRef; }


	/*! Get a pointer to the FSSpec of the file
		@result pointer to the FSSpec of the file
	*/
	inline const FSSpec* FileSpec() const { return &fileSpec; }


	/*! Get an AliasHandle for the file
		@result a new AliasHandle
	*/
	AliasHandle GetAliasHandle() const;


	/*! Resolve this alias file to the real file, if possible
		@result true if the alias was resolvable
	*/
	Boolean Resolve(unsigned long flags=kResolveAliasFileNoUI);


	/*! Open the file with its handler
		@result a result code
	*/
	OSStatus OpenInHandler() const { return LSOpenCFURLRef(URLPath(), NULL); }


	/*! Create the file on disk
		@result a result code
	*/
	OSErr Create();


	/*! Create a file with the given location and name
		@param parentRef the location of the file
		@param name the full name of the file
		@param namelen the character count of the filename
		@result a result code
	*/
	OSStatus Create(const FSRef *parentRef, const UniChar *name, UniCharCount namelen);


	/*! Delete the file from disk
		@result a result code
	*/
	OSErr Delete();


	/*! Open the file with the given permissions
		@param perm the permissions
	*/
	OSErr Open(SInt8 perm);


	/*! Open the file's resource fork with read permission
		@result the refrence number of the fork
	*/
	short OpenResource();


	/*! Open the file for read (and write if possible)
		@result a result code
	*/
	OSErr OpenCurr();


	/*! Open the file for read
		@result a result code
	*/
	OSErr OpenRead();


	/*! Open the file for write
		@result a result code
	*/
	OSErr OpenWrite(bool appending=false);


	/*! Open the file for write with the file pointer at the end
		@result a result code
	*/
	OSErr OpenAppend() { return OpenWrite(true); }


	/*! Touch the file
		@result a result code
	*/
	inline OSErr Touch() { OSErr err = OpenWrite(); Close(); return err; }


	/*! Get the current file position
		@result the position of the file pointer
	*/
	inline long GetOffset() { long offs; GetFPos(refNumber, &offs); return offs; }


	/*! Set the current file position
		@result a result code
	*/
	OSErr SetOffset(long offs, short posMode=fsFromStart);


	/*! Set the current file position
		@param offset (optional) the offset
		@result a result code
	*/
	inline OSErr SetOffsetEOF(long offset=0) { return SetOffset(offset, fsFromLEOF); }


	/*! Read bytes from the file on disk
		@param buffer pointer to a destination buffer
		@param length number of bytes to read
		@result a result code
	*/
	OSErr Read(void *buffer, long length);


	/*! Write bytes to the file on disk
		@param buffer pointer to a source buffer
		@param length number of bytes to write
		@result a result code
	*/
	OSErr Write(const void *buffer, long length);


	/*! Write an XML dictionary to disk
		@param inDict the dictionary to write
		@result an error code
	*/
	SInt32 WriteXMLFile( const TDictionary &inDict );


	/*! Read an XML dictionary from disk
		@result a dictionary pointer or NULL on failure
	*/
	TDictionary* ReadXMLFile();


	/*! Read the entire file
		@result a pointer to the file data or NULL on failure
	*/
	void* ReadAll();

	//! Close the file
	void Close();


	/*! Save the existing file or invoke the Save dialog
		@param modal use a modal dialog
	*/
	void SaveChanges(bool modal=false);


	/*! Save an existing file.
		Override this in subclasses to do things like
		check that the destination file is not already
		open in a second window.
		@result an error code
	*/
	virtual OSErr OpenAndSave();


	/*! Invoke the Save Changes dialog.
		@param quitting alter behavior for the quit cycle
		@result a result code
	*/
	OSStatus AskSaveChanges(Boolean quitting);


	/*! Invoke the Revert Changes dialog.
		@result a result code
	*/
	OSStatus ConfirmRevert();


	/*! Invoke the Save dialog and save the file.
		@result a result code
	*/
	virtual OSStatus SaveAs(bool modal=false);


	/*! Begin a new file based on the Save dialog results.
		@result a result code
	*/
	OSStatus BeginSave();


	/*! Complete the Save.
		@result a result code
	*/
	OSStatus CompleteSave(Boolean inDidWriteFile);


	/*! The event handler proc for navigation dialogs.
		@param callbackSelector the nav dialog event
		@param callbackParms event data
		@param callbackUD the file object invoking the dialog
	*/
	static pascal void NavEventProc(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms, NavCallBackUserData callbackUD);


	/*! The event handler for navigation dialogs.
		@param callbackSelector the nav dialog event
		@param callbackParms event data
	*/
	void HandleNavEvent(const NavEventCallbackMessage callbackSelector, NavCBRecPtr callbackParms);


	/*! The handler for navigation dialog user events.
		@param inUserAction the action to respond to
	*/
	void HandleNavUserAction(NavUserAction inUserAction);


	/*! The custom items size negotiation handler.
		Override this method to make space for custom items
		@param customRect the rect to modify
	*/
	virtual void NavCustomSize(Rect &customRect) {}


	/*! Install custom controls on the nav dialog.
		Override this method to add your own items
		@param navDialog a nav dialog reference
		@param dialog a standard dialog reference
	*/
	virtual void NavCustomControls(NavDialogRef navDialog, DialogRef dialog) {}


	/*! Handler for custom controls.
		Override this method to implement your own items
		@param inControl the control itself
		@param controlIndex the control index
	*/
	virtual void HandleCustomControl(ControlRef inControl, UInt16 controlIndex) {}


	/*! Provider for the navigation dialog action button label.
		Override this method to change the button title
		@result the title for the button
	*/
	virtual CFStringRef NavActionButtonLabel() { return NULL; }


	/*! Provider for the navigation dialog cancel button label.
		Override this method to change the button title
		@result the title for the button
	*/
	virtual CFStringRef NavCancelButtonLabel() { return NULL; }


	/*! Provider for the Save dialog message.
		Override this method to change the message
		@result the message
	*/
	virtual CFStringRef NavSaveMessage() { return NULL; }


	/*! Provider for the default Save name.
		Override this method to customize the default Save name
		@result the name
	*/
	virtual CFStringRef	SaveFileName() { return MakeNameWithExtension(fileName); }

	//! Terminate the nav dialog right away.
	inline void TerminateNavDialog() { if (navDialog) NavCustomControl(navDialog, kNavCtlTerminate, NULL); }

	static NavEventUPP			GetNavEventUPP();
	static NavObjectFilterUPP	GetNavFilterUPP();


	/*!
		Compare an FSRef to this file.

		@param fsRef the FSRef to compare
		@result TRUE if the FSRef refers to the same file
	*/
	bool CompareFSRef(const FSRef &fsRef) const {
		return (hasGoodRef && FSCompareFSRefs( &fsRef, &fileRef ) == noErr);
		}

	virtual TFile&	operator=(const TFile &other);
	int				operator==(const TFile &other) const	{ OSErr err = FSCompareFSRefs(&fileRef, &other.fileRef); return (err == noErr); }
	int				operator==(const FSRef &fref) const		{ return (FSCompareFSRefs( &fref, &fileRef ) == noErr); }
	int				operator!=(const TFile &other) const	{ return !(*this == other); }
	int				operator<(const TFile& other) const;
	int				operator>(const TFile& other) const;
	int				operator<=(const TFile& other) const;
	int				operator>=(const TFile& other) const;


	/*! Accessor for the navDialog state.
		@result TRUE if the FSRef refers to the same file
	*/
	inline bool HasDialog() { return navDialog != NULL; }
	virtual OSErr		WriteData()		{ return noErr; }			// Override for a document file that can write itself

	/*! The Save Changes dialog was dismissed with the "Don't Save" button.
		You should override this method to close the associated
		window if it is already closing (which it probably should be).
	*/
	virtual void NavDontSave() {}


	/*! The "Discard Changes" button was clicked.
		You should override this method to Revert your
		document to its file's current contents.
	*/
	virtual void		NavDiscardChanges()	{}


	/*! The Navigation Dialog (Save Changes or Save) was cancelled.
		Override this method to halt window closing and application quit.
	*/
	virtual void NavCancel() {}


	/*! The file was successfully saved.
		Override this method to update your document
		window's title bar and modified state.
	*/
	virtual void NavDidSave() {}


	/*! There was some kind of error.
		@param err the error (which may be kErrorHandled)
	*/
	virtual void HandleNavError(OSStatus err);


	/*! Sub-provider for nav dialog display filtering.
		Override this method to do custom filtering
		@param objectSpec the FSSpec of the file or folder
		@param fileType the OSType of the file
		@param isFolder TRUE for folders
	*/
	virtual bool ShouldDisplayObject(FSSpec* objectSpec, const OSType fileType, const Boolean isFolder) { return true; }


	/*! Sub-provider for nav dialog display filtering.
		Override this method to do custom filtering
		@param objectSpec the FSRef of the file or folder
		@param fileType the OSType of the file
		@param isFolder TRUE for folders
	*/
	virtual bool ShouldDisplayObject(FSRef* objectSpec, const OSType fileType, const Boolean isFolder) { return true; }


	/*! Provider for nav dialog display filtering.
		Override this method to do custom filtering
		@param theItem an AE descriptor for the item
		@param info a NavFileOrFolderInfo structure
		@param filterMode the dialog part (such as Browser or Favorites) being filtered
	*/
	bool NavFilter(AEDesc *theItem, void *info, NavFilterModes filterMode);


	/*! The NavObjectFilterProc that calls the class

		Override this method to do custom filtering

		@param theItem an AE descriptor for the item
		@param info a NavFileOrFolderInfo structure
		@param callBackUD the TFile object providing filters
		@param filterMode the dialog part (such as Browser or Favorites) being filtered
	*/
	static pascal Boolean NavFilterProc(AEDesc *theItem, void *info, void *callBackUD, NavFilterModes filterMode);
};

//
// File Utility Functions
//

/*! Test the validity of an FSRef.
	@param fsRef the FSRef to test
	@result TRUE if the FSRef is valid
*/
inline Boolean FSRefIsValid( const FSRef &fsRef ) {
	return FSGetCatalogInfo( &fsRef, kFSCatInfoNone, NULL, NULL, NULL, NULL ) == noErr;
}

/*! Give C++ the ability to compare FSRefs.
	Example: if (fsref1 == fsref2)
*/
inline Boolean operator== ( const FSRef &lhs, const FSRef &rhs ) {
	return FSCompareFSRefs( &lhs, &rhs ) == noErr;
}

#endif
