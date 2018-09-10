/*!
 *  @file FPApplication.h
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#ifndef FPAPPLICATION_H
#define FPAPPLICATION_H

#include "FPToolbar.h"
#include "FPClipboard.h"
#include "TRecentItems.h"

class FPDocWindow;
class FPDocument;
class FPChordPalette;
class FPTuningInfo;
class FPHistoryEvent;
class FPHistory;
class FPChord;

class TFile;
class TWindow;
class TCustomControl;

typedef struct {
	ToolboxObjectClassRef   customDef;
	EventHandlerUPP			customHandler;
} CustomControlInfo;


#include <vector>
typedef std::vector<CustomControlInfo>	CustomControlArray;
typedef CustomControlArray::iterator	CustomInfoIterator;

#pragma mark -

class FPApplication {
private:
	CustomControlArray	customControlArray;
	TRecentItems		recentItems;

	EventHandlerUPP		eventHandler;
	UInt32				savedFlags;

	MenuRef				helpMenu;				// The help menu
	MenuItemIndex		firstHelpItem;			// The first item which is ours

	bool				palettesHidden;
	FPDocWindow			*frontDocWindow;
	FPToolbar			*toolPalette;
	FPChordPalette		*chordPalette;

	PartIndex			pertinentPart;

	FPHistory			*history;

public:

	bool				isQuitting;
	bool				discardingAll;
	NavDialogRef		openDialog;
	bool				hasShownSplash;
	bool				hasShownNagger;

	Cursor				arrow;					// The mouse arrow
	FPClipboard			clipboard;

	CCrsrHandle			cursors[kNumCursors];

	// Preference (global) Settings
	bool				keyOrderHalves;
	bool				romanMode;
	bool				tabReversible;
	bool				transformOnce;
	bool				chordRootType;
	bool				swipeIsReversed;

	FPApplication();
	~FPApplication();

#if KAGI_SUPPORT
	CFStringRef			AuthorizerName();
#endif

#if KAGI_SUPPORT || DEMO_ONLY
	bool				AuthorizerIsAuthorized();
	bool				AuthorizerRunDiscardDialog();
#endif

	static bool			QuickTimeIsOkay();

	FPDocument*			ActiveDocument();
	CFURLRef			SupportFolderURL(CFStringRef child=NULL);
	CFURLRef			GetImageURL(CFStringRef child);

	void				SetGlobalChord(const FPChord &chord, bool fromDocument=false);
	void				DoSetChordRepeat(UInt16 rep);

	void				RegisterCustomControl(CFStringRef classID);
	void				DisposeCustomControls();

	void				Init();
	void				RecheckSwipeDirection();
	OSStatus			InitHelpMenu();
	OSStatus			InitGSInstrumentsMenu();
	void				SavePreferences();
	void				SetupTransposeMenu();
	void				InitPalettes();
	void				Run();

	#pragma mark Chord Change Responders
	void				ReflectPatternChanges();
	void				ReflectChordChanges(bool scaleUpdate=false, bool docIsSource=false);
	void				ReflectFingeringChanges();
	void				ReflectNewFingering();
	void				UpdatePlayTools() { toolPalette->UpdatePlayTools(); }
	inline void			SetPertinentPart(PartIndex p) { pertinentPart = p; }

	#pragma mark Event Handlers
	void				GenerateCommandEvent(MenuCommand cid);
	OSStatus			HandleEvent(EventHandlerCallRef inCallRef, TCarbonEvent &event);
	bool				UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index);
	OSStatus			HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index);
	bool				DoesKeyRepeat(char key, UInt16 mods);
	bool				HandleKey(unsigned char key, UInt16 mods, bool repeating=false);

	static pascal OSStatus  EventHandler(EventHandlerCallRef inCallRef, EventRef inEvent, void* userData);

	void				InstallAEHandlers();
	static pascal OSErr	AE_Handler(const AppleEvent *appEvent, AppleEvent *reply, SInt32 refcon);
	OSErr				AE_OpenApplication(const AppleEvent *appEvent, AppleEvent *reply);
	OSErr				AE_ReopenApplication(const AppleEvent *appEvent, AppleEvent *reply);
	OSErr				AE_OpenDocuments(const AppleEvent *appEvent, AppleEvent *reply);
	OSErr				AE_QuitApplication(const AppleEvent *appEvent, AppleEvent *reply);
	static OSErr		AE_HasRequiredParams(const AppleEvent *appEvent);

	void				InstallGuitarBundle(TFile &inFile);

	#pragma mark General Utilities
	static void			SpriteTimerCallback( CFRunLoopTimerRef timer, void *palette );
	static void			SpriteLoopTimerProc( EventLoopTimerRef timer, void *palette );

	void				SetMouseCursor(int cursIndex, bool sticky=false);
	void				ResetMouseCursor();

	void				SetMenuIcon(MenuCommand inCommand, CFStringRef path);

	#pragma mark Accessors
	//
	// Accessors for things that come from the interface state
	//
	bool				IsFollowViewEnabled();		// Eye
	bool				IsEditModeEnabled();		// Pencil
	bool				IsLoopingEnabled();			// Loop
	bool				IsSoloModeEnabled();		// Solo
	bool				IsReverseStrung();			// From the guitar palette
	bool				IsTransformModeEnabled();	// Transform
	inline void			EndTransformOnce()			{ if (IsTransformModeEnabled() && transformOnce) DoToggleTransformMode(); }
	bool				IsTablatureReversed();		// Based on multiple factors
	const char*			NameOfNote(UInt16 tone);
	const char*			NameOfKey(UInt16 key);
	UInt16				CurrentTempo();				// From the document, or 120

	//
	// Commands
	//
	#pragma mark Chord Operations

	// GENERAL USE
	void				DoAddTone(UInt16 tone, SInt16 refTone=-1);
	void				DoSubtractTone(UInt16 tone);
	void				DoClear(int type=0);
	void				DoAddTones(UInt16 newTones, UInt16 startTone, bool redoFingering=true);
	void				DoSubtractTones(UInt16 newTones);
	void				DoToggleTone(SInt16 tone, bool redoFingering=true);
	bool				DoAddTriadForTone(SInt16 tone, bool redoFingering=true);
	void				DoInvertTones(bool undoable=true);
	void				DoToggleChordLock();
	void				DoDirectHarmonize(SInt16 tone);

	// APPLE MENU
	#pragma mark Apple Menu
	void				DoAboutFretPet();
	void				DoQuit();
	void				DoQuitNow();

	// FILE MENU
	#pragma mark File Menu
	void				AddToRecentItems(TFile &file);
	void				UpdateRecentItemsMenu();
	void				ClearRecentItems();
	void				ClosePlaceholderDocument();
	FPDocWindow*		NewDocWindow();
	OSErr				DoOpen();
	FPDocWindow*		OpenDocument(const FSRef &fref);
	FPDocWindow*		OpenRecentItem(CFIndex item);

	// EDIT MENU
	#pragma mark Edit Menu
	FPHistory*			ActiveHistory();
	FPHistoryEvent*		StartUndo(UInt16 act, CFStringRef opName, PartMask partMask=kCurrentChannelMask);

	// SCALE PALETTE
	#pragma mark Scale Palette
	void				DoToggleScaleTone();
	void				DoAddScaleTone(bool upThird=false);
	void				DoAddScaleTriad(bool setTones=false);
	void				DoSubtractScaleTriad();
	void				DoAddScale(bool setTones=false);
	void				DoChangeEnharmonic(bool bDec);

	// GUITAR PALETTE
	#pragma mark Guitar Palette
	SInt16				StringTone(UInt16 string);
	UInt16				DotColor(UInt16 tone);
	void				DoSetGuitarTuning(const FPTuningInfo &t, bool undoable=true);
	void				DoMoveFretCursor(UInt16 string, UInt16 fret);
	void				DoOverrideFingering();
	void				DoToggleGuitarTone(UInt16 string, UInt16 fret);
	void				DoToggleGuitarTone();
	void				DoAddGuitarTriad();
	void				DoMoveFretBracket(UInt16 newLow, UInt16 newHigh);
	bool				IsBracketEnabled();
	void				DoToggleBracket();
	void				DoToggleHorizontal();
	void				DoToggleLefty();
	void				DoToggleReverseStrung();
	void				DoToggleUnusedTones();
	void				DoToggleFancyDots();
	void				DoSetDotStyle(UInt16 style);
	void				DoToggleTuningName();

	// PIANO PALETTE
	#pragma mark Piano Palette
	void				SetPianoKeyPlaying(SInt16 key, bool isPlaying);

	// CIRCLE PALETTE
	#pragma mark Circle Palette
	void				DoTransposeArrow(bool down);
	void				DoToggleCircleEye();
	void				DoToggleCircleHold();
	void				DoTransposeKey(UInt16 root);
	void				DoTransposeInterval(SInt16 interval);
	void				DoHarmonizeBy(SInt16 steps);

	// TOOLBAR COMMANDS
	#pragma mark Toolbar Commands
	void				DoToggleTransformMode(UInt32 mods=0);
	void				DoToggleFollowView();
	void				DoToggleEditMode(bool drawRegions=true);
	void				DoToggleRoman();
	void				DoSetKeyOrder(bool isHalves);
	void				DoSetRootType(bool isKey);
	void				DoHearChord(short modifiers);
	void				DoStop();
	void				DoPlay();

	// SCALE OPS
	#pragma mark Scale Operations
	void				DoScaleForward();
	void				DoScaleBack();
	void				DoSetScale(UInt16 mode);
	void				DoMoveScaleCursor(UInt16 func, UInt16 keyIndex);
	void				DoSetNoteModifier(SInt16 offset, bool undoable=true);

	// CHORD PALETTE
	#pragma mark Chord Palette
	void				DoChooseName(UInt16 line);

	// SEQUENCER MENU
	#pragma mark Sequencer Menu
	void				DoSetInstrument(PartIndex part, UInt16 inTrueGM, bool undoable=true);
	inline void			DoSetCurrentInstrument(UInt16 inTrueGM) { DoSetInstrument(pertinentPart, inTrueGM); }
	void				AddToInstrument(PartIndex part, SInt16 delta);
	void				AddToCurrentInstrument(SInt16 delta) { AddToInstrument(pertinentPart, delta); }

	void				DoAddGlobalChord();
	void				DoToggleTransformFlag(PartIndex part=kCurrentChannel);
	void				DoToggleMidi(PartIndex part=kAllChannels);

#if QUICKTIME_SUPPORT
	void				DoToggleQuicktime(PartIndex part=kAllChannels);
#else
	void				DoToggleAUSynth(PartIndex part=kAllChannels);
#endif

	void				DoSetMidiChannel(PartIndex part, UInt16 channel, bool undoable=true);
	void				DoToggleSplitMidiOut();

	// WINDOW MENU
	#pragma mark Window Menu
	void				DoTogglePalettes();
	void				DoArrangeInFront(bool frontGuide=false);
	void				DoResetPalettes();

	// HELP MENU
	#pragma mark Help Menu
	void				DoRegistration();
	void				DoAuthorization();
	bool				Authorize(CFStringRef authCode, CFStringRef inTestName, CFStringRef inTestVersion);

	// DOCUMENT
	#pragma mark Document
	void				DoWindowActivated(FPDocWindow *wind);
	void				DoSelectPart(PartIndex part);
	void				DoSetPatternSize(UInt16 size);
	void				DoSetPatternDot(UInt16 string, UInt16 step);
	void				DoClearPatternDot(UInt16 string, UInt16 step);
	void				DoSetPatternStep(UInt16 step);
	void				DoClearPatternStep(UInt16 step);
	void				UpdateDocumentChord(bool setDirty=true);
	void				UpdateDocumentFingerings();

	void				DoSetTempo(UInt16 t, bool updateDoc);
	void				DoSetVelocity(UInt16 v, PartIndex part, bool updateDoc, bool updateChan);
	void				DoSetSustain(UInt16 s, PartIndex part, bool updateDoc, bool updateChan);

	#pragma mark File Dialogs
	OSStatus			OpenFileDialog(OSType applicationSignature, SInt16 numTypes, OSType typeList[]);
	OSStatus			AskReviewChangesDialog(NavUserAction *outUserAction, bool quitting);
	void				TerminateOpenFileDialog();
	void				DisposeOpenFileDialog();
	bool				IsDialogFront();

};


extern	FPApplication	*fretpet;
extern	CFStringRef		gAppName;

enum { STARTUP_NEW = 1, STARTUP_OPEN, STARTUP_NOTHING };


#endif
