/*
 *  FPApplication.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *	The FPApplication class embodies application behaviors.
 *	It also takes on the role of a traditional Controller class.
 *
 *	Very little state data is embodied in the Application. Each
 *	palette embodies the states and behaviors that it represents.
 *	(For example, the Scale palette class maintains the current
 *	state of the scale data. The state of the "Eye" button *is* the
 *	state of the "Follow View" setting.)
 *
 *	There are some advantages to this approach. First, the view doesn't
 *	need to access some external object for its state data. The view
 *	state *is* the data state. Second, the application needs to refer
 *	to only one object to obtain states, set states, and update the view.
 *
 *	A more organized MVC approach would be to embody each entity
 *	into a Model unit separated from the View class. The view would
 *	make reference to the associated data object in order to update
 *	its visual state.
 *
 *	A disadvantage to this approach is that some palettes have to
 *	directly ask other palettes for the data they require. Semantically
 *	it expresses the access-order like so:
 *
 *			fretpet->thePalette->GetSomeData()
 *
 *	This requires the palette to know about the other palette. A better
 *	alternative is to make accessors in the application:
 *
 *			fretpet->GetSomeData()
 *
 *	Or even better:
 *
 *			globalObject->GetSomeData()
 *
 */

#include "NavServicesSupport.h"
#include "ZonicKRM.h"

#include "FPApplication.h"
#include "TRecentItems.h"

#include "FPPreferences.h"
#include "FPCustomTuning.h"
#include "TCarbonEvent.h"
#include "FPChord.h"
#include "FPDocWindow.h"
#include "FPPalette.h"

#include "FPAboutBox.h"
#include "FPChannelsPalette.h"
#include "FPChordPalette.h"
#include "FPCirclePalette.h"
#include "FPDiagramPalette.h"
#include "FPGuitarPalette.h"
#include "FPInfoPalette.h"
#include "FPMetroPalette.h"
#include "FPPianoPalette.h"
#include "FPScalePalette.h"
#include "FPToolbar.h"

#include "FPUtilities.h"	/* for screwy quicktime dialog */
#include "TMidiOutput.h"
#include "TMusicOutput.h"	/* to test that quicktime is ok */

#include "TFile.h"

#include "TString.h"
#include "FPPaletteGL.h"
#include "TWindow.h"
#include "TError.h"

#include "CGHelper.h"
#include "FPMidiHelper.h"

#if KAGI_SUPPORT || DEMO_ONLY
#include "FPAuthorizer.h"
#endif

extern long systemVersion;

//
//  Version and Expiration date
//
#define THIS_VERSION	0x0110
#define EXPIRE_YEAR		2008
#define EXPIRE_MONTH	0

bool ShouldRun() {
#if defined(EXPIRE_MONTH) && EXPIRE_MONTH > 0
	CFGregorianDate dt = CFAbsoluteTimeGetGregorianDate(CFAbsoluteTimeGetCurrent(), NULL);
	return (dt.year*12+dt.month-1) < (EXPIRE_YEAR*12+EXPIRE_MONTH-1);
#else
	return true;
#endif
}


FPApplication::FPApplication() {
	fretpet = this;

	gAppName = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleExecutable"));

	palettesHidden	= false;
	openDialog		= NULL;
	isQuitting		= false;
	discardingAll	= false;
	savedFlags		= QDSwapTextFlags(kQDUseCGTextRendering);
	keyOrderHalves	= false;
	eventHandler	= NULL;
	romanMode		= false;
	chordRootType	= false;
	tabReversible	= preferences.GetBoolean(kPrefTabReversible, TRUE);
	hasShownSplash	= true;
	hasShownNagger	= false;
	pertinentPart	= kCurrentChannel;
	history			= new FPHistory();

	//
	// Notify the system of custom controls
	// Control instances are claimed at creation time
	//
	const char *customName[] = {
		"scalebox", "bankscroll", "circle", "harmony", "more",
		"guitar", "diagram", "piano", "sequencer", "bracket",
		"tunename", "doctabs", "doctempo", "docvelocity",
		"docsustain", "custom", "dragger" };
	TSpriteControl::Register();
	for (unsigned i=0; i<COUNT(customName); i++) {
		TString key(CFSTR("com.thinkyhead.fretpet."));
		key += customName[i];
		RegisterCustomControl(key.GetCFStringRef());
	}

	// About Box is a Custom Window!
	CustomControlInfo info;
	FPAboutBox::RegisterCustomClass(&info.customDef, &info.customHandler);
	customControlArray.push_back(info);

	// Initialize all application resources
	Init();

	// Get recent items from preferences
	UpdateRecentItemsMenu();

	// Init Palettes according to the saved state
	InitPalettes();

	// Create handlers for standard Apple Events
	InstallAEHandlers();

#if SPARKLE_SUPPORT
	// Initialize Sparkle for version updates
	// Set auto update for last preference, in case set elsewhere
	SUSparkleInitializeForCarbon();
	SUSparkleCheckWithInterval(60*60*24*2);
#endif

}


FPApplication::~FPApplication() {
	(void)QDSwapTextFlags(savedFlags);

	if (eventHandler != NULL)
		DisposeEventHandlerUPP(eventHandler);

	CFRELEASE(gAppName);

	for (int i=kNumCursors; i--;)
		DisposeCCursor(cursors[i]);

	delete player;
}


#if KAGI_SUPPORT
CFStringRef FPApplication::AuthorizerName() {
	return authorizer.name;
}
#endif

#if KAGI_SUPPORT || DEMO_ONLY
bool FPApplication::AuthorizerIsAuthorized() {
	return authorizer.IsAuthorized();
}
bool FPApplication::AuthorizerRunDiscardDialog() {
	return authorizer.RunDiscardDialog();
}
#endif

#pragma mark - Initialization
void FPApplication::Init() {
    IBNibRef 		nibRef;
    OSStatus		err;

	toolPalette		= NULL;
	chordPalette	= NULL;

	// Set up the application event handler
	const EventTypeSpec appEvents[] = {
			{ kEventClassCommand, kEventCommandUpdateStatus },
			{ kEventClassCommand, kEventCommandProcess },
			{ kEventClassKeyboard, kEventRawKeyDown },
			{ kEventClassKeyboard, kEventRawKeyRepeat },
			{ kEventClassApplication, kEventAppActivated },
			{ kEventClassMenu, kEventMenuEndTracking }
		};
	eventHandler = NewEventHandlerUPP( FPApplication::EventHandler );
	InstallApplicationEventHandler( eventHandler, GetEventTypeCount( appEvents ), appEvents, this, NULL );

	// Prepare the menu bar
	err = CreateNibReference(MAIN_NIB_NAME, &nibRef);
	verify_action(err==noErr, throw TError(err, CFSTR("Can't open main interface.")) );

	err = SetMenuBarFromNib(nibRef, CFSTR("MenuBar"));
	verify_action(err==noErr, throw TError(err, CFSTR("Can't set menu bar.")) );

/*
	// Init the Window Menu
	MenuRef windowMenu, windowMenuAdds;
	err = CreateStandardWindowMenu(kWindowMenuIncludeRotate, &windowMenu);
	verify_action(err==noErr, throw TError(err, CFSTR("Can't create Window menu.")) );

	InsertMenu(windowMenu, 0);

	err = CreateMenuFromNib(nibRef, CFSTR("WindowMenu"), &windowMenuAdds);
	verify_action(err==noErr, throw TError(err, CFSTR("Can't load Window menu addons.")) );

	err = CopyMenuItems( windowMenuAdds, 1, CountMenuItems(windowMenuAdds), windowMenu, CountMenuItems(windowMenu) );
	verify_action(err==noErr, throw TError(err, CFSTR("Can't add Window menu addons.")) );
*/

	MenuRef			menu;
	MenuItemIndex	item;

	// Set up the Dock menu
	err = CreateMenuFromNib(nibRef, CFSTR("DockMenu"), &menu);
	err = SetApplicationDockTileMenu(menu);

	DisposeNibReference(nibRef);

	// Get the preferences menu item.
	// (This seems to jumpstart the item into working the first time)
	(void)GetIndMenuItemWithCommandID(NULL, kHICommandPreferences, 1, &menu, &item);

	// Put icons on Quicktime / AU and Midi menu items
	SetMenuIcon(kFPCommandMidiOut, CFSTR("misc/midi.png"));
#if QUICKTIME_SUPPORT
	SetMenuIcon(kFPCommandQuicktimeOut, CFSTR("misc/qt.png"));
#else
	SetMenuIcon(kFPCommandAUSynthOut, CFSTR("misc/au.png"));
#endif

	// Get rid of Sparkle Update if it's disabled
#if !SPARKLE_SUPPORT
	err = GetIndMenuItemWithCommandID(NULL, kFPCommandCheckForUpdate, 1, &menu, &item);
	DeleteMenuItem(menu, item);
#endif

	// TODO: Leave these - just disable if QT is uppity
#if !QUICKTIME_SUPPORT
//	err = GetIndMenuItemWithCommandID(NULL, kFPCommandPickInstrument, 1, &theMenu, &item);
//	DeleteMenuItem(theMenu, item);
#endif

	// Set refcons on all items in the filter submenus
	err = GetIndMenuItemWithCommandID(NULL, kFPCommandFilterSubmenuEnabled, 1, &menu, &item);
	err = GetMenuItemHierarchicalMenu(menu, item, &menu);
	SetMenuContentsRefcon(menu, kFilterEnabled);

	err = GetIndMenuItemWithCommandID(NULL, kFPCommandFilterSubmenuCurrent, 1, &menu, &item);
	err = GetMenuItemHierarchicalMenu(menu, item, &menu);
	SetMenuContentsRefcon(menu, kFilterCurrent);

	err = GetIndMenuItemWithCommandID(NULL, kFPCommandFilterSubmenuAll, 1, &menu, &item);
	err = GetMenuItemHierarchicalMenu(menu, item, &menu);
	SetMenuContentsRefcon(menu, kFilterAll);

	InitHelpMenu();
	SetupTransposeMenu();

	// Inform Custom Tunings about the menubar's Tuning menu
	GetIndMenuItemWithCommandID(NULL, kFPCommandGuitarTuning, 1, &menu, NULL);
	customTuningList.AttachMenu(menu);

	GetQDGlobalsArrow(&arrow);

	for (int i=kNumCursors; i--;)
		cursors[i] = GetCCursor(kFirstCursor + i);

	// Create the music player
	player = new FPMusicPlayer();

	// Delete this menu item ... for now
	if (!GetIndMenuItemWithCommandID(NULL, kFPCommandSplitMidiOut, 1, &menu, &item))
		DeleteMenuItem(menu, item);

	verify_action(err==noErr, throw TError(err, CFSTR("Unable to initialize.")) );

	(void)InitGSInstrumentsMenu();
}

void FPApplication::RecheckSwipeDirection() {
	preferences.Synchronize();
	swipeIsReversed = preferences.GetBoolean(CFSTR("com.apple.swipescrolldirection"), systemVersion >= 0x1070);
}

#pragma mark -

OSStatus FPApplication::InitGSInstrumentsMenu() {
	OSStatus		err = noErr;
	MenuRef			menu;
	MenuItemIndex	item;

	require_noerr(err = GetIndMenuItemWithCommandID(NULL, kFPCommandInstGroup, kMenuIndexGS, &menu, &item), home);
	require_noerr(err = GetMenuItemHierarchicalMenu(menu, item, &menu), home);

	// Use a temporary midi channel to acquire instrument names
	{
		TMidiOutput tempOut;
		TString text;
		for (UInt16 faux=kFirstFauxGS; faux<=kLastFauxGS; faux++) {
			UInt16 real = FPMidiHelper::FauxGMToTrueGM(faux);
			tempOut.Instrument(0, real);
			text.SetWithFormat(CFSTR("%04d "), real);
			text += tempOut.InstrumentName();
			AppendMenuItemTextWithCFString(menu, text.GetCFStringRef(), 0, kFPCommandGSInstrument, NULL);
		}
	}

home:
	return err;
}

bool FPApplication::QuickTimeIsOkay() {
	static int quickTimeStat = -1;

	if (quickTimeStat < 0) {

		NoteAllocator	na = NULL;
		NoteChannel		nc = 0;
		NoteRequest		nr;
		ComponentResult err = TMusicOutput::InitializeQuickTimeChannel(na, nr, nc);

		quickTimeStat = (err == noErr) ? 1 : 0;

/*
		// TO TEST THE TIMEOUT, ETC
		quickTimeStat = 0;
		preferences.SetInteger(kPrefNextQuickTimeTest, TickCount() + 60*60*2);
*/
		UInt32 testtick = preferences.GetInteger(kPrefNextQuickTimeTest, 0);
		if (!quickTimeStat && testtick != 1 && TickCount() >= testtick) {
			TString heading;
			heading.SetWithFormatLocalized(CFSTR("QuickTime Conflict (%d)"), err);
			DialogItemIndex which = RunPrompt(
				heading.GetCFStringRef(),
				CFSTR("QuickTime Note Player is conflicting with an unknown component in your system. Please use the Thinkyhead contact form to help us track down this issue."),
				CFSTR("Contact form"), CFSTR("Remind me later"), CFSTR("Don't show again")
			);
			switch (which) {
				case kAlertStdAlertOKButton: {
					CFURLRef websiteURL = CFURLCreateWithString(
						kCFAllocatorDefault,
						CFSTR("http://www.thinkyhead.com/support"),
						NULL);
					LSOpenCFURLRef(websiteURL, NULL);
					CFRELEASE(websiteURL);
					break;
				}
				case kAlertStdAlertCancelButton:
					// cancel - don't show again for a week
					preferences.SetInteger(kPrefNextQuickTimeTest, TickCount() + 60*60*60*24*4);
					break;
				case kAlertStdAlertOtherButton:
					// don't show again for a week
					preferences.SetInteger(kPrefNextQuickTimeTest, 1);
					break;
				default:
					break;
			}
		}

		/*
		// Exercise the Instrument Name function!
		TMidiOutput tempOut;
		TString text, div("============================================================\n");
		div.Print();
		for (UInt16 faux=kFirstFauxGM; faux<=kLastFauxGM; faux++) {
			UInt16 real = FPMidiHelper::FauxGMToTrueGM(faux);
			tempOut.Instrument(0, real);
			text.SetWithFormat(CFSTR("%03d "), real);
			text += tempOut.InstrumentName();
			text += "\n";
			if (text[0] != '(') text.Print(stderr);
		}
		div.Print();
		for (UInt16 faux=kFirstFauxDrum; faux<=kLastFauxDrum; faux++) {
			UInt16 real = FPMidiHelper::FauxGMToTrueGM(faux);
			tempOut.Instrument(0, real);
			text.SetWithFormat(CFSTR("%05d "), real);
			text += tempOut.InstrumentName();
			text += "\n";
			if (text[0] != '(') text.Print(stderr);
		}
		div.Print();
		for (UInt16 faux=kFirstFauxGS; faux<=kLastFauxGS; faux++) {
			UInt16 real = FPMidiHelper::FauxGMToTrueGM(faux);
			tempOut.Instrument(0, real);
			text.SetWithFormat(CFSTR("%04d "), real);
			text += tempOut.InstrumentName();
			text += "\n";
			if (text[0] != '(') text.Print(stderr);
		}
		div.Print();
		*/
	}

	return (quickTimeStat == 1);
}

#pragma mark -
OSStatus FPApplication::InitHelpMenu() {
	OSStatus err;
	err = HMGetHelpMenu(&helpMenu, &firstHelpItem);
	require_noerr( err, FindHelpError );

	//
	// Add items here
	//
	{
		int item = firstHelpItem;

#if SPARKLE_SUPPORT
		TString checkStr;
		checkStr.SetLocalized(CFSTR("Check for Update..."));
		err = InsertMenuItemTextWithCFString(helpMenu, checkStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandCheckForUpdate);
		require_noerr( err, InsertItemError );
		err = InsertMenuItemTextWithCFString(helpMenu, CFSTR("sep"), item++, kMenuItemAttrSeparator, 0);
		require_noerr( err, InsertItemError );
#endif

		TString manualStr;
		manualStr.SetLocalized(CFSTR("FretPet Manual"));

		err = InsertMenuItemTextWithCFString(helpMenu, manualStr.GetCFStringRef(), item, kMenuItemAttrUpdateSingleItem, kFPCommandDocumentation);
		err = SetMenuItemCommandKey(helpMenu, item, false, '?');
		err = SetMenuItemModifiers(helpMenu, item, kMenuNoModifiers);
		require_noerr( err, InsertItemError );
		item++;

		TString releaseStr;
		releaseStr.SetLocalized(CFSTR("Release Notes"));
		err = InsertMenuItemTextWithCFString(helpMenu, releaseStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandReleaseNotes);
		require_noerr( err, InsertItemError );

		err = InsertMenuItemTextWithCFString(helpMenu, CFSTR("sep"), item++, kMenuItemAttrSeparator, 0);
		require_noerr( err, InsertItemError );

		TString websiteStr;
		websiteStr.SetLocalized(CFSTR("FretPet Web Site"));
		err = InsertMenuItemTextWithCFString(helpMenu, websiteStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandWebsite);
		require_noerr( err, InsertItemError );

#if KAGI_SUPPORT
		if (authorizer.IsAuthorized()) {
			TString deauthStr;
			deauthStr.SetLocalized(CFSTR("Deauthorize"));
			err = InsertMenuItemTextWithCFString(helpMenu, deauthStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandDeauthorize);
			require_noerr( err, InsertItemError );
		}
		else {
			err = InsertMenuItemTextWithCFString(helpMenu, CFSTR("sep"), item++, kMenuItemAttrSeparator, 0);
			require_noerr( err, InsertItemError );

			TString registerStr;
			registerStr.SetLocalized(CFSTR("Register FretPet..."));
			err = InsertMenuItemTextWithCFString(helpMenu, registerStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandRegister);
			require_noerr( err, InsertItemError );

			TString serialStr;
			serialStr.SetLocalized(CFSTR("Enter Serial Number..."));
			err = InsertMenuItemTextWithCFString(helpMenu, serialStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandEnterSerial);
			require_noerr( err, InsertItemError );
		}
#elif DEMO_ONLY
		err = InsertMenuItemTextWithCFString(helpMenu, CFSTR("sep"), item++, kMenuItemAttrSeparator, 0);
		require_noerr( err, InsertItemError );
		
		TString registerStr;
		registerStr.SetLocalized(CFSTR("FretPet on the App Store"));
		err = InsertMenuItemTextWithCFString(helpMenu, registerStr.GetCFStringRef(), item++, kMenuItemAttrUpdateSingleItem, kFPCommandRegister);
		require_noerr( err, InsertItemError );
#endif

	}

InsertItemError:
FindHelpError:
	return err;
}

CFURLRef FPApplication::SupportFolderURL(CFStringRef child) {
	NSFileManager* sharedFM = [NSFileManager defaultManager];
	NSArray* possibleURLs = [sharedFM URLsForDirectory:NSApplicationSupportDirectory
											 inDomains:NSUserDomainMask];

	NSURL *appSupportDir = nil, *appDirectory = nil;
	
	if ([possibleURLs count] >= 1) {
		// Use the first directory (if multiple are returned)
		appSupportDir = [possibleURLs objectAtIndex:0];
	}
	
	// If a valid app support directory exists, add the
	// app's bundle ID to it to specify the final directory.
	if (appSupportDir) {
		appDirectory = [appSupportDir URLByAppendingPathComponent:@"FretPet"];
		if (child) appDirectory = [appDirectory URLByAppendingPathComponent:(NSString*)child];
	}

	return (CFURLRef)appDirectory;
}

// TODO: Move to a menu class or Utilities - very generic
void FPApplication::SetMenuIcon(MenuCommand inCommand, CFStringRef path) {
	MenuRef			theMenu;
	MenuItemIndex	theIndex;
	CFURLRef		url = GetImageURL(path);
	CGImageRef		icon = CGLoadImage(url);
	OSStatus err = GetIndMenuItemWithCommandID(NULL, inCommand, 1, &theMenu, &theIndex);
	err = SetMenuItemIconHandle(theMenu, theIndex, kMenuCGImageRefType, (Handle)icon);
	CGImageRelease(icon);
	CFRELEASE(url);
}


CFURLRef FPApplication::GetImageURL(CFStringRef child) {
	TString fullPath("images/");
	fullPath += child;

	CFURLRef resourcesURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());
	verify_action(resourcesURL != NULL, throw TError(1001, CFSTR("CFBundleCopyResourcesDirectoryURL")) );
	CFURLRef imageURL = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, resourcesURL, fullPath.GetCFStringRef(), true);
	CFRELEASE(resourcesURL);
	verify_action(imageURL != NULL, throw TError(1002, CFSTR("CFURLCreateCopyAppendingPathComponent")) );

	return imageURL;
}

#ifndef kVK_DownArrow
#define kVK_DownArrow	0x7D
#define kVK_UpArrow		0x7E
#endif

void FPApplication::SetupTransposeMenu() {
	OSStatus		err;
	MenuRef			menu;
	MenuItemIndex	item;

	err = GetIndMenuItemWithCommandID(NULL, kFPCommandTransposeBy, 1, &menu, &item);

	typedef struct {
		MenuItemIndex	item;
		UInt16			key[2];
		SInt16			glyph[2];
	} TransposeInfo;

	static TransposeInfo infos[] = {
		{ item + 6, { kNullCharCode, kVK_DownArrow }, { kMenuNullGlyph, kMenuDownArrowGlyph } },
		{ item + 4, { kNullCharCode, kVK_UpArrow }, { kMenuNullGlyph, kMenuUpArrowGlyph } },
		{ item, { kVK_DownArrow, kNullCharCode }, { kMenuDownArrowGlyph, kMenuNullGlyph } },
		{ item + 10, { kVK_UpArrow, kNullCharCode }, { kMenuUpArrowGlyph, kMenuNullGlyph } }
	};

	for (unsigned i=0; i < COUNT(infos); i++) {
		TransposeInfo info = infos[i];
		int g = keyOrderHalves ? 0 : 1;
		err = ChangeMenuItemAttributes(menu, info.item, kMenuItemAttrAutoRepeat, kMenuItemAttrUseVirtualKey);
		err = SetMenuItemCommandKey(menu, info.item, info.key[g] != kNullCharCode, info.key[g]);
		err = SetMenuItemKeyGlyph(menu, info.item, info.glyph[g]);
		err = SetMenuItemModifiers(menu, info.item, kMenuNoModifiers);
	}

}


void FPApplication::InstallGuitarBundle(TFile &inFile) {
	AlertError(kFPErrorBadFormat, CFSTR("To install a guitar bundle copy it to the 'Application Support/FretPet' folder."));
}

void FPApplication::UpdateRecentItemsMenu() {
	OSStatus		err;
	MenuRef			menu;
	MenuItemIndex	item;

	err = GetIndMenuItemWithCommandID(NULL, kFPCommandRecentItemSubmenu, 1, &menu, &item);
	err = GetMenuItemHierarchicalMenu(menu, item, &menu);
	recentItems.PopulateMenu(menu);
}


void FPApplication::SavePreferences() {
	preferences.Synchronize();
}


void FPApplication::InitPalettes() {
	toolPalette			= new FPToolbar();
	chordPalette		= new FPChordPalette();
	scalePalette		= new FPScalePalette();
	pianoPalette		= new FPPianoPalette();
	guitarPalette		= new FPGuitarPalette();
	diagramPalette		= new FPDiagramPalette();
	circlePalette		= new FPCirclePalette();
	infoPalette			= new FPInfoPalette();
	channelsPalette		= new FPChannelsPalette();

	//
	// Prepare and display
	//
	toolPalette->LoadPreferences();
	chordPalette->LoadPreferences();
	scalePalette->LoadPreferences();
	pianoPalette->LoadPreferences();
	guitarPalette->LoadPreferences();
	diagramPalette->LoadPreferences();
	infoPalette->LoadPreferences();
	channelsPalette->LoadPreferences();
	circlePalette->LoadPreferences();

	//
	// Dependent initialization
	//
	channelsPalette->RefreshControls();
	infoPalette->UpdateFields();
	guitarPalette->ArrangeDots();
}


void FPApplication::RegisterCustomControl(CFStringRef classID) {
	CustomControlInfo info;
	TCustomControl::RegisterCustomControl(classID, &info.customDef, &info.customHandler);
	customControlArray.push_back(info);
}


void FPApplication::DisposeCustomControls() {
	for (CustomInfoIterator itr = customControlArray.begin();
			itr != customControlArray.end();
			itr++) {
		DisposeEventHandlerUPP((*itr).customHandler);
		UnregisterToolboxObjectClass((*itr).customDef);
	}

	customControlArray.clear();
}


#pragma mark -
#pragma mark Apple Events

#define INSTALL(event, handler) \
		AEInstallEventHandler(kCoreEventClass, event, handler, event, false)

void FPApplication::InstallAEHandlers() {
	INSTALL (kAEOpenDocuments, NewAEEventHandlerUPP(AE_Handler));
	INSTALL (kAEOpenApplication, NewAEEventHandlerUPP(AE_Handler));
	INSTALL (kAEReopenApplication, NewAEEventHandlerUPP(AE_Handler));
	INSTALL (kAEQuitApplication, NewAEEventHandlerUPP(AE_Handler));
//	INSTALL (kAEPrintDocuments, NewAEEventHandlerProc(PrintDocsEvent));
}

pascal OSErr FPApplication::AE_Handler(const AppleEvent *appEvent, AppleEvent *reply, SInt32 refCon) {
	OSErr err = errAEEventNotHandled;
	switch (refCon) {
		case kAEOpenDocuments:
			err = fretpet->AE_OpenDocuments(appEvent, reply);
			break;
			
		case kAEOpenApplication:
			err = fretpet->AE_OpenApplication(appEvent, reply);
			break;
			
		case kAEReopenApplication:
			err = fretpet->AE_ReopenApplication(appEvent, reply);
			break;
			
		case kAEQuitApplication:
			err = fretpet->AE_QuitApplication(appEvent, reply);
			break;

//		case kAEPrintDocuments:
//			err = fretpet->AE_PrintDocuments(appEvent, reply);
//			break;

		default:
			break;
	}
	(void)QuickTimeIsOkay();
	return err;
}


OSErr FPApplication::AE_OpenApplication(const AppleEvent *appEvent, AppleEvent *reply) {
	if (!ShouldRun()) {
		DoTogglePalettes();

		(void)RunPrompt(
			CFSTR("Beta Version Expired"),
			CFSTR("This beta version of FretPet X has expired. Visit www.thinkyhead.com to get the latest version."),
			CFSTR("OK")
			);

		DoQuitNow();
	}

	OSErr	err = AE_HasRequiredParams(appEvent);

	if (!err) {
		switch (preferences.GetInteger(kPrefStartupAction, STARTUP_NEW)) {
			case STARTUP_OPEN:
				GenerateCommandEvent(kHICommandOpen);
				break;

			case STARTUP_NEW:
				(void)NewDocWindow();

			default:
				hasShownSplash = !preferences.GetBoolean(kPrefSplash, TRUE);
		}
	}

	if (preferences.GetInteger(kPrefLastRunVersion, 0) != THIS_VERSION) {
		preferences.SetInteger(kPrefLastRunVersion, THIS_VERSION);
		DoResetPalettes();
	}

	return err;
}


OSErr FPApplication::AE_ReopenApplication(const AppleEvent *appEvent, AppleEvent *reply) {
	OSErr	err;

	err = AE_HasRequiredParams(appEvent);

	if (!err) {
		if (FPDocWindow::docWindowList.size() == 0)
			(void)NewDocWindow();
	}

	return err;
}


OSErr FPApplication::AE_OpenDocuments(const AppleEvent *appEvent, AppleEvent *reply) {
	OSErr		err;
	FSSpec		fileSpec;
	FSRef		fileRef;
	AEDescList	docList;
	SInt32		index, numberOfItems;
	Size		actualSize;
	AEKeyword	keyWord;
	DescType	returnedType;

	err = AEGetParamDesc(appEvent, keyDirectObject, typeAEList, &docList);

	if (err == noErr) {
		do {
			if ((err = AE_HasRequiredParams(appEvent)))
				break;

			if ((err = AECountItems(&docList, &numberOfItems)))
				break;

			for (index=1; index<=numberOfItems; index++) {
				if ((err = AEGetNthPtr(&docList, index, typeFSRef, &keyWord, &returnedType, (Ptr)&fileRef, sizeof(FSRef), &actualSize)))
					break;

				if ((err = FSGetCatalogInfo(&fileRef, kFSCatInfoNone, NULL, NULL, &fileSpec, NULL)))
					break;

				// Is this a guitar bundle? Does it need to be installed?
				TFile thisFile(fileSpec);
				FInfo itsInfo = thisFile.GetFInfo();

				if (0 != (0x200 & itsInfo.fdFlags) && CFStringHasSuffix(thisFile.BaseName(), CFSTR(".guitar"))) {
					(void)InstallGuitarBundle(thisFile);
				}
				else {
					(void)OpenDocument(fileRef);
				}
			}
			if (err) break;

		} while (false);

		AEDisposeDesc(&docList);
	}

	if (err)
		AlertError(err, CFSTR("Could not open the document."));

	return noErr;
}


OSErr FPApplication::AE_QuitApplication(const AppleEvent *appEvent, AppleEvent *reply) {
	OSErr	err = AE_HasRequiredParams(appEvent);

	if (!err)
		GenerateCommandEvent(kHICommandQuit);

	return err;
}


OSErr FPApplication::AE_HasRequiredParams(const AppleEvent *appEvent) {
	DescType	returnedType;
	Size		actualSize;

	OSErr err = AEGetAttributePtr(appEvent, keyMissedKeywordAttr, typeWildCard, &returnedType, NULL, 0, &actualSize);

	if (err == errAEDescNotFound)
		return noErr;
	else if (err == noErr)
		return errAEParamMissed;

	return err;
}


#pragma mark -

void FPApplication::Run() {
	player->StartPlaybackTimer();
	player->StartAnimationTimer();
    RunApplicationEventLoop();
}

#pragma mark -
#pragma mark Event Handling

pascal OSStatus FPApplication::EventHandler( EventHandlerCallRef inCallRef, EventRef inEvent, void* userData ) {
	TCarbonEvent event(inEvent);
	return ((FPApplication*)userData)->HandleEvent(inCallRef, event);
}


OSStatus FPApplication::HandleEvent(EventHandlerCallRef inCallRef, TCarbonEvent &event) {
//	fprintf( stderr, "[%08X] App Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus		result = eventNotHandledErr;

	switch ( event.GetClass() ) {
		case kEventClassApplication: {
			switch ( event.GetKind() ) {
				case kEventAppActivated:

					RecheckSwipeDirection();
					
					if (!hasShownSplash)
						GenerateCommandEvent(kFPCommandAbout);

#if KAGI_SUPPORT || DEMO_ONLY
					if (!hasShownNagger)
						GenerateCommandEvent(kFPCommandShowNagger);
#endif
					break;

				default:
//					fprintf(stderr, "Application Event: %d\n", event.GetKind());
					break;
			}
		}
		break;

		case kEventClassCommand: {
			HICommand command;
			(void) event.GetParameter(kEventParamDirectObject, &command);
			switch ( event.GetKind() ) {
				case kEventCommandUpdateStatus:
//					fprintf(stderr, "App kEventCommandUpdateStatus '%c%c%c%c'...\n\n", FOUR_CHAR(command.commandID));
					if ( UpdateCommandStatus(command.commandID, command.menu.menuRef, command.menu.menuItemIndex) )
						result = noErr;
					break;

				case kEventCommandProcess:
//					fprintf(stderr, "\nApp kEventCommandProcess...\n\n");
					result = HandleCommand(command.commandID, command.menu.menuRef, command.menu.menuItemIndex);
					break;
			}
		}
		break;

		case kEventClassKeyboard: {
			char		key;
			(void) event.GetParameter(kEventParamKeyMacCharCodes, &key);
			UInt16		mods = GetCurrentEventKeyModifiers();
			switch ( event.GetKind() ) {
				case kEventRawKeyDown:
//					fprintf(stderr, "kEventRawKeyDown\n");
					if (!IsDialogFront() && (HAS_COMMAND(mods) || HandleKey(key, mods)))
						result = noErr;
					break;
					
				case kEventRawKeyRepeat:
//					fprintf(stderr, "kEventRawKeyRepeat\n");
					if ( HAS_COMMAND(mods) || (DoesKeyRepeat(key, mods) && HandleKey(key, mods, true)) )
						result = noErr;
					break;
			}

//			fprintf(stderr, "App kEventClassKeyboard result = %d\n", result);
		}	
		break;

		//
		// Handle Menu Closed for the sake of the mouse cursor
		//
		case kEventClassMenu: {
			switch ( event.GetKind() ) {
				case kEventMenuEndTracking:
					ResetMouseCursor();
					break;
			}
		}

	}

	return result;
}

//-----------------------------------------------
//
//	UpdateItemStatus
//
//	At the application level this routine catches
//	anything the front document doesn't handle.
//
//	All the application menus are automatically
//	disabled by all the modal dialogs, including
//	the Instrument Picker. However the Instrument
//	Picker does call through to this function.
//
bool FPApplication::UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	MenuRef amenu;
	(void) GetMenuItemHierarchicalMenu(menu, index, &amenu);
	bool	enable = true;
	bool	handled = true;

	switch ( cid ) {
		#pragma mark FretPet Menu
		//
		// Apple Menu
		//
		case kFPCommandAbout:
			enable = !(FPAboutBox::boxIsActive);
			break;

		case kHICommandPreferences: {
			enable = !isQuitting;
			break;
		}

		case kHICommandQuit:
			enable = !isQuitting;
			break;

		#pragma mark File Menu
		//
		// File Menu
		//
		case kHICommandNew:
			// Always enabled!
			break;

		case kHICommandOpen: {
			enable = (openDialog == NULL);
			break;
		}

		case kHICommandClose:
			enable = IsDialogFront();
			break;

		case kHICommandSave:
		case kHICommandSaveAs:
		case kHICommandRevert:
		case kFPCommandExportMovie:
		case kFPCommandExportMIDI:
        case kFPCommandExportSunvox:

		case kHICommandPrint:
		case kHICommandPageSetup:
			enable = false;
			break;

		case kFPCommandRecentItemSubmenu:
			enable = recentItems.GetCount() > 0;
			break;

		case kFPCommandRecentItem:
			enable = recentItems.ItemStillExists(index - 1);
			break;

		#pragma mark Edit Menu
		//
		// Edit Menu
		//
		case kHICommandUndo: {
			if (IsDialogFront())
				handled = false;
			else {
				TString	undoTitle;
				undoTitle.SetLocalized(CFSTR("Undo "));

				FPHistory	*hist = ActiveHistory();
				CFStringRef	action = hist->UndoName();
				if (action)
					undoTitle += action;

				(void)SetMenuItemTextWithCFString(menu, index, undoTitle.GetCFStringRef());
				enable = hist->CanUndo();
			}
			break;
		}

		case kHICommandRedo: {
			if (IsDialogFront())
				handled = false;
			else {
				TString	redoTitle;
				redoTitle.SetLocalized(CFSTR("Redo "));

				FPHistory	*hist = ActiveHistory();
				CFStringRef	action = hist->RedoName();
				if (action)
					redoTitle += action;

				(void)SetMenuItemTextWithCFString(menu, index, redoTitle.GetCFStringRef());
				enable = hist->CanRedo();
			}
			break;
		}

		//
		// These items can be handled by the system
		//	so just let them through here.
		//
		case kHICommandPaste:
			(void)SetMenuItemTextWithCFString(menu, index, CFSTR("Paste"));
			if (IsDialogFront())
				handled = false;
			else
				enable = false;
			break;

		case kHICommandCut:
		case kHICommandCopy:
		case kHICommandSelectAll:
			if (IsDialogFront())
				handled = false;
			else
				enable = false;
			break;

		case kFPCommandClearTones:
			enable = globalChord.HasTones();
			break;

		case kFPCommandPasteTones:
		case kFPCommandPastePattern:
			enable = false;
			break;

		case kFPCommandSelectNone: {
//			bool was = IsMenuItemEnabled(menu, index);
			enable = false;
			// fprintf(stderr, "Changing Select None Item from %s to %s\n", was ? "on" : "off", enable ? "on" : "off");
			break;
		}

		case kFPCommandPrefabSubmenu:
			break;

		#pragma mark Circle Palette
		case kFPCommandTransposeNorth:
		case kFPCommandTransposeSouth:
			// (here to prevent command-arrow shortcuts from being disabled)
			break;

		#pragma mark Chord Menu
		case kFPCommandHarmonizeUp:
		case kFPCommandHarmonizeDown:
		case kFPCommandHarmonizeBy:
		case kFPCommandTransposeBy:
		case kFPCommandTransposeKey:
		case kFPCommandTransformMode:
		case kFPCommandTransformOnce:
		case kFPCommandTransBySubmenu:
		case kFPCommandTransToSubmenu:
		case kFPCommandHarmonizeSubmenu:
			break;

		case kFPCommandLock: {
			TString lockString;
			lockString.SetLocalized(globalChord.IsLocked() ? CFSTR("Unlock Root") : CFSTR("Lock Root"));
			(void)SetMenuItemTextWithCFString(menu, index, lockString.GetCFStringRef());
			break;
		}

		case kFPCommandStrumUp:
		case kFPCommandStrumDown:
		case kFPCommandStrike:
			enable = globalChord.HasTones();
			break;

		case kFPCommandAddTriad:
		case kFPCommandSubtractTriad:
		case kFPCommandAddScale:
			break;

		case kFPCommandClearPattern:
		case kFPCommandClearBoth:
			enable = false;
			break;

		case kFPCommandInvertChord:
//			enable = !globalChord.HasTones(scalePalette->CurrentScaleMask());
			break;

		#pragma mark Guitar Menu
		//
		// Guitar Menu
		//
		case kFPCommandGuitarTuning: {
			MenuItemIndex firstIndex;
			(void)GetIndMenuItemWithCommandID(menu, kFPCommandGuitarTuning, 1, NULL, &firstIndex);
			MarkMenuItem(menu, index, builtinTuning[index-firstIndex] == guitarPalette->CurrentTuning());
			break;
		}

		case kFPCommandCustomTuning: {
			MenuItemIndex firstIndex;
			(void)GetIndMenuItemWithCommandID(menu, kFPCommandCustomTuning, 1, NULL, &firstIndex);
			MarkMenuItem(menu, index, *customTuningList[index-firstIndex] == guitarPalette->CurrentTuning());
			break;
		}

		case kFPCommandHorizontal:
			CheckMenuItem(menu, index, guitarPalette->IsHorizontal());
			break;

		case kFPCommandLeftHanded:
			CheckMenuItem(menu, index, guitarPalette->IsLeftHanded());
			enable = guitarPalette->IsHorizontal();
			break;

		case kFPCommandReverseStrung:
			CheckMenuItem(menu, index, guitarPalette->IsReverseStrung());
			break;

		case kFPCommandBracket:
			CheckMenuItem(menu, index, globalChord.IsBracketEnabled());
			break;

		case kFPCommandUnusedTones:
			CheckMenuItem(menu, index, guitarPalette->ShowingUnusedTones());
			break;

		case kFPCommandDotsDots:
			MarkMenuItem(menu, index, guitarPalette->ShowingDottedDots());
			break;

		case kFPCommandDotsLetters:
			MarkMenuItem(menu, index, guitarPalette->ShowingLetterDots());
			break;

		case kFPCommandDotsNumbers:
			MarkMenuItem(menu, index, guitarPalette->ShowingNumberedDots());
			break;

		case kFPCommandGuitarImage:
			MarkMenuItem(menu, index, index == guitarPalette->SelectedGuitarImageMenuIndex());
			break;

		case kFPCommandImageSubmenu:
		case kFPCommandTuningSubmenu:
			break;

		#pragma mark Sequencer Menu
		//
		// Filter Menu
		//
		case kFPCommandPlay:
		case kFPCommandStop: {
			FPDocWindow	*wind = FPDocWindow::frontWindow;
			bool		play = player->IsPlaying();
			enable = wind && (wind->ReadyToPlay() || play);

			TString playText;

			if (play) {
				playText.SetLocalized(CFSTR("Stop"));
			}
			else if (wind == NULL) {
				if (globalChord.HasPattern())
					playText.SetLocalized(CFSTR("Strum"));
			}
			else {
				if (wind->ReadyToPlay()) {
					ChordIndex	start, end;
					player->GetRangeToPlay(start, end);
					if (start == 0 && (end == -1 || end == wind->DocumentSize() - 1)) {
						playText.SetLocalized(CFSTR("Play All"));
					}
					else {
						if (end == start)
							playText.SetWithFormatLocalized(CFSTR("Play %d"), start + 1);
						else
							playText.SetWithFormatLocalized(CFSTR("Play %d to %d"), start + 1, end + 1);
					}
				}
			}

			if (playText.IsEmpty())
				playText.SetLocalized(CFSTR("Play"));

			(void)SetMenuItemTextWithCFString(menu, index, playText.GetCFStringRef());
			(void)SetMenuItemCommandID(menu, index, play ? kFPCommandStop : kFPCommandPlay);
			break;
		}

		case kFPCommandLoop:
			CheckMenuItem(menu, index, IsLoopingEnabled());
			enable = !IsDialogFront();
			break;

		case kFPCommandEye:
			CheckMenuItem(menu, index, IsFollowViewEnabled());
			enable = !IsDialogFront();
			break;

		case kFPCommandFree:
			CheckMenuItem(menu, index, IsEditModeEnabled());
			enable = !IsDialogFront();
			break;

		case kFPCommandSolo:
			CheckMenuItem(menu, index, IsSoloModeEnabled());
			enable = !IsDialogFront();
			break;

		case kFPCommandCountIn:
//			CheckMenuItem(menu, index, IsSoloModeEnabled());
			enable = !IsDialogFront();
			break;

		case kFPCommandAddChord: {
			FPDocWindow *wind = FPDocWindow::activeWindow;
			enable = !IsDialogFront() && (!wind || wind->IsInteractive());
			break;
		}

#if QUICKTIME_SUPPORT
		case kFPCommandQuicktimeOut:
			CheckMenuItem(menu, index, player->IsQuicktimeEnabled());
#else
		case kFPCommandAUSynthOut:
			CheckMenuItem(menu, index, player->IsAUSynthEnabled());
#endif
			enable = !IsDialogFront();
			break;

		case kFPCommandMidiOut:
			CheckMenuItem(menu, index, player->IsMidiEnabled());
			enable = !IsDialogFront();
			break;

		case kFPCommandSplitMidiOut:
			CheckMenuItem(menu, index, player->IsMidiOutputSplit());
			enable = !IsDialogFront();
			break;

		case kFPInfoMidiChannel:
			CheckMenuItem(menu, index, player->GetMidiChannel(pertinentPart) == index - 1);
			break;

		case kFPCommandPickInstrument:
			enable = !IsDialogFront() && QuickTimeIsOkay();
			break;

		#pragma mark Filter Menu
		//
		// Filter Menu
		//
		case kFPCommandFilterSubmenuAll:
		case kFPCommandFilterSubmenuEnabled:
		case kFPCommandFilterSubmenuCurrent:
			enable = false;
			break;

		case kFPCommandSelScramble:
		case kFPCommandSelDouble:
		case kFPCommandSelSplay:
		case kFPCommandSelCompact:
		case kFPCommandSelClone:
		case kFPCommandSelReverseChords:
		case kFPCommandSelReverseMelody:
		case kFPCommandSelClearPatterns:
		case kFPCommandSelHFlip:
		case kFPCommandSelVFlip:
		case kFPCommandSelRandom1:
		case kFPCommandSelRandom2:
		case kFPCommandSelClearTones:
		case kFPCommandSelInvertTones:
		case kFPCommandSelCleanupTones:
		case kFPCommandSelLockRoots:
		case kFPCommandSelUnlockRoots:
		case kFPCommandSelTransposeBy:
		case kFPCommandSelTransposeTo:
		case kFPCommandSelHarmonizeUp:
		case kFPCommandSelHarmonizeDown:
		case kFPCommandSelHarmonizeBy:
		case kFPCommandSelHarmonizeSub:
		case kFPCommandSelTransposeToSub:
		case kFPCommandSelTransposeBySub:
			enable = false;
			break;



		#pragma mark Window Menu
		//
		// Window Menu
		//
		case kHICommandMinimizeWindow:
		case kHICommandMinimizeAll:
		case kHICommandBringAllToFront:
		case kHICommandZoomWindow:
			handled = false;
			break;

//		case kFPCommandResetPalettes:
//			break;

		case kFPCommandToolbar:
			CheckMenuItem(menu, index, toolPalette->IsVisible());
			break;

		case kFPCommandAnchorToolbar: {
			TString toolString;
			toolString.SetLocalized(toolPalette->IsAnchored() ? CFSTR("Unanchor Toolbar") : CFSTR("Anchor Toolbar"));
			enable = toolPalette->IsVisible();
			(void)SetMenuItemTextWithCFString(menu, index, toolString.GetCFStringRef());
			break;
		}

		case kHICommandRotateWindowsForward:
		case kHICommandRotateWindowsBackward:
		case kFPCommandStackDocuments:
			enable = FPDocWindow::docWindowList.size() > 1 && !IsDialogFront();
			break;

		case kFPCommandWindInfo:
			CheckMenuItem(menu, index, infoPalette->IsVisible());
			break;

		case kFPCommandWindChannels:
			CheckMenuItem(menu, index, channelsPalette->IsVisible());
			break;

		case kFPCommandWindScale:
			CheckMenuItem(menu, index, scalePalette->IsVisible());
			break;

		case kFPCommandWindGuitar:
			CheckMenuItem(menu, index, guitarPalette->IsVisible());
			break;

		case kFPCommandWindChord:
			CheckMenuItem(menu, index, chordPalette->IsVisible());
			break;

		case kFPCommandWindCircle:
			CheckMenuItem(menu, index, circlePalette->IsVisible());
			break;

		case kFPCommandWindPiano:
			CheckMenuItem(menu, index, pianoPalette->IsVisible());
			break;

		case kFPCommandWindDiagram:
			CheckMenuItem(menu, index, diagramPalette->IsVisible());
			break;

		case kFPCommandWindMetronome:
//			CheckMenuItem(menu, index, metroPalette->IsVisible());
			break;

		case kFPCommandWindAnalysis:
			enable = false;
//			CheckMenuItem(menu, index, analPalette->IsVisible());
			break;

		case kFPCommandWindAll:
//			enable = !IsDocumentBlocked();			// Because Tab is needed in the Open dialog
			break;

		#pragma mark Scale Menu
		//
		// Scale Menu
		//
		case kFPCommandScaleMode:
			MarkMenuItem(menu, index,
				scalePalette->CurrentMode() == index - ((index > NUM_MODES) ? 2 : 1));

			// nobreak

		case kFPCommandModePrev:
		case kFPCommandModeNext:
		case kFPCommandScaleShift:
			break;

		#pragma mark Instrument Menu
		//
		// Instrument
		//
		case kFPCommandGSInstrument:
//			fprintf(stderr, "gs-instr %d\n", GetMenuID(menu) + index);
		case kFPCommandInstrument:
		{
			// 001-128 : no change
			// 129-137 : drums
			// 138+    : gs instr.
			//
			// thus: if 167 is the instrument the faux should be 138 (?)
//			UInt16	trueGM = player->GetInstrumentNumber(pertinentPart),
//					fauxGM = FPMidiHelper::TrueGMToFauxGM(trueGM);
//			fprintf(stderr, "setting is %d %d\n", trueGM, fauxGM);
			MarkMenuItem(menu, index, FPMidiHelper::TrueGMToFauxGM(player->GetInstrumentNumber(pertinentPart)) == GetMenuID(menu) + index);
			break;
		}

		case kFPCommandInstGroup:
			MarkMenuItem(menu, index, player->GetInstrumentGroup(pertinentPart) == index);
			break;

#if KAGI_SUPPORT
		#pragma mark Help Menu
		//
		// Help
		//
		case kFPCommandEnterSerial:
		case kFPCommandRegister:
			enable = !authorizer.IsAuthorized();
			break;

		case kFPCommandDeauthorize:
			enable = authorizer.IsAuthorized();
			break;
#elif DEMO_ONLY
		case kFPCommandRegister:
			enable = true;
			break;
#endif
	}

	if (handled) {
		if (enable)
			EnableMenuItem(menu, index);
		else
			DisableMenuItem(menu, index);
	}

	return handled;
}

#pragma mark -

OSStatus FPApplication::HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	OSStatus		err = noErr;

	switch ( cid ) {
		#pragma mark FretPet Menu
		//
		// FretPet Menu
		//
		case kFPCommandAbout:
			DoAboutFretPet();
			break;

		case kHICommandPreferences:
			preferences.ShowPreferencesDialog();
			break;

		case kHICommandQuit:
			if (!isQuitting)
				DoQuit();
			break;


		#pragma mark File Menu
		//
		// File Menu
		//
		case kHICommandNew:
			(void)NewDocWindow();
			break;

		case kHICommandOpen:
			DoOpen();
			break;

		case kFPCommandRecentItem:
			OpenRecentItem(index - 1);
			break;


		#pragma mark Edit Menu
		//
		// Edit Menu
		//
		case kHICommandUndo:
			if (IsDialogFront())
				err = eventNotHandledErr;
			else
				ActiveHistory()->DoUndo();
			break;

		case kHICommandRedo:
			if (IsDialogFront())
				err = eventNotHandledErr;
			else
				ActiveHistory()->DoRedo();
			break;

		case kHICommandCut:
		case kHICommandCopy:
		case kHICommandPaste:
		case kFPCommandSelectNone:
			//
			// TODO: Implement for the global chord when no dialog is up
			//	Otherwise let the system handle it
			//
			err = eventNotHandledErr;
			break;

		case kFPCommandClearTonesButton: {
			UInt32 mods = GetCurrentEventKeyModifiers();
			DoClear(IS_COMMAND(mods) ? 2 : IS_OPTION(mods) ? 1 : 0);
		}

		case kFPCommandClearTones:
			DoClear(0);
			break;

		case kFPCommandClearPattern:
			DoClear(1);
			break;

		case kFPCommandClearBoth:
			DoClear(2);
			break;

		case kFPCommandClearRecent:
			ClearRecentItems();
			break;

		case kFPCommandInvertChord:
			DoInvertTones();
			break;


		#pragma mark Chord Menu
		//
		// Chord Menu
		//
		case kFPCommandTransposeBy:
			DoTransposeInterval(index);
			break;

		case kFPCommandTransposeKey:
			DoTransposeKey(index-1);
			break;

		case kFPCommandHarmonizeBy:
			DoHarmonizeBy(index);
			break;

		case kFPCommandHarmonizeUp:
			DoHarmonizeBy(1);
			break;

		case kFPCommandHarmonizeDown:
			DoHarmonizeBy(-1);
			break;

		case kFPCommandLock:
			DoToggleChordLock();
			break;


		#pragma mark Guitar Menu
		//
		// Guitar Menu
		//
		case kFPCommandGuitarTuning:
		case kFPCommandCustomTuning: {
			MenuItemIndex	firstIndex;
			(void)GetIndMenuItemWithCommandID(menu, cid, 1, NULL, &firstIndex);
			DoSetGuitarTuning(cid == kFPCommandGuitarTuning ? builtinTuning[index-firstIndex] : *customTuningList[index-firstIndex]);
			break;
		}

		case kFPCommandCustomizeTunings:
			customTuningList.ShowCustomTuningDialog();
			break;

		case kFPCommandGuitarImage:
			guitarPalette->LoadGuitar(guitarPalette->GuitarIndexForMenuIndex(index));
			break;

		case kFPCommandHorizontal:
			DoToggleHorizontal();
			break;
	
		case kFPCommandBracket:
			DoToggleBracket();
			break;

		case kFPCommandLeftHanded:
			DoToggleLefty();
			break;

		case kFPCommandReverseStrung:
			DoToggleReverseStrung();
			break;

		case kFPCommandUnusedTones:
			DoToggleUnusedTones();
			break;

		case kFPCommandDotsDots:
			DoSetDotStyle(kDotStyleDots);
			break;

		case kFPCommandDotsLetters:
			DoSetDotStyle(kDotStyleLetters);
			break;

		case kFPCommandDotsNumbers:
			DoSetDotStyle(kDotStyleNumbers);
			break;



		#pragma mark Toolbar Buttons
		//
		// Toolbar buttons
		//
		case kFPCommandAddTriad: {
			if ( IS_OPTION(GetCurrentKeyModifiers()) )
				DoSubtractScaleTriad();
			else
				DoAddScaleTriad( IS_SHIFT(GetCurrentKeyModifiers()) );
			break;
		}

		case kFPCommandSubtractTriad:
			DoSubtractScaleTriad();
			break;

		case kFPCommandAddScale:
			DoAddScale( IS_SHIFT(GetCurrentKeyModifiers()) );
			break;

		case kFPCommandTransformMode:
			DoToggleTransformMode( GetCurrentKeyModifiers() );
			break;

		#pragma mark Sequencer Menu
		//
		// Sequencer menu
		//
		case kFPCommandPlay:
			DoPlay();
			break;

		case kFPCommandStop:
			DoStop();
			break;

		case kFPCommandStrumUp:
			DoHearChord( optionKey );
			break;

		case kFPCommandStrumDown:
			DoHearChord( 0 );
			break;

		case kFPCommandStrike:
			DoHearChord( shiftKey );
			break;

		case kFPCommandHearButton:
			DoHearChord( GetCurrentKeyModifiers() );
			break;

		case kFPCommandLoop:
			toolPalette->loopButton->Toggle();
			break;

		case kFPCommandEye:
			DoToggleFollowView();
			break;

		case kFPCommandFree:
			DoToggleEditMode();
			break;

		case kFPCommandSolo:
			toolPalette->soloButton->Toggle();
			break;

		case kFPCommandCountIn:
//			DoToggleCountIn();
			break;

		case kFPCommandRoman:
			DoToggleRoman();
			break;

		case kFPCommandAddChord:
			DoAddGlobalChord();
			break;

		case kFPCommandFifths:
		case kFPCommandHalves:
			DoSetKeyOrder(cid == kFPCommandHalves);
			break;

#if QUICKTIME_SUPPORT
		case kFPCommandQuicktimeOut:
			DoToggleQuicktime();
			break;

		case kFPCommandQuicktime1:
		case kFPCommandQuicktime2:
		case kFPCommandQuicktime3:
		case kFPCommandQuicktime4:
			DoToggleQuicktime(cid - kFPCommandQuicktime1);
			break;
#else
		case kFPCommandAUSynthOut:
			DoToggleAUSynth();
			break;
			
		case kFPCommandSynth1:
		case kFPCommandSynth2:
		case kFPCommandSynth3:
		case kFPCommandSynth4:
			DoToggleAUSynth(cid - kFPCommandSynth1);
			break;
#endif

		case kFPCommandCoreMidi1:
		case kFPCommandCoreMidi2:
		case kFPCommandCoreMidi3:
		case kFPCommandCoreMidi4:
			DoToggleMidi(cid - kFPCommandCoreMidi1);
			break;

		case kFPCommandMidiOut:
			DoToggleMidi();
			break;

		case kFPCommandSplitMidiOut:
			DoToggleSplitMidiOut();
			break;

		case kFPInfoMidiChannel:
			DoSetMidiChannel(pertinentPart, index - 1);
			break;

//#if QUICKTIME_SUPPORT
		case kFPCommandPickInstrument: {
			UInt16 trueGM = player->PickInstrument(kCurrentChannel);
			DoSetCurrentInstrument(trueGM);
			break;
		}
//#endif

		#pragma mark Window Menu
		//
		// Window Menu
		//
		case kFPCommandToolbar:
			toolPalette->ToggleVisiblity();
			break;

		case kFPCommandAnchorToolbar:
			toolPalette->ToggleAnchored();
			break;

//		case kFPCommandResetWindows:
//			// handled
//			err = eventNotHandledErr;
//			break;

		case kFPCommandStackDocuments:
			DoArrangeInFront();
			break;

		case kFPCommandWindInfo:
			infoPalette->ToggleVisiblity();
			break;

		case kFPCommandWindChannels:
			channelsPalette->ToggleVisiblity();
			break;

		case kFPCommandWindScale:
			scalePalette->ToggleVisiblity();
			break;

		case kFPCommandWindGuitar:
			guitarPalette->ToggleVisiblity();
			break;

		case kFPCommandWindChord:
			chordPalette->ToggleVisiblity();
			break;

		case kFPCommandWindCircle:
			circlePalette->ToggleVisiblity();
			break;

		case kFPCommandWindPiano:
			pianoPalette->ToggleVisiblity();
			break;

		case kFPCommandWindDiagram:
			diagramPalette->ToggleVisiblity();
			break;

		case kFPCommandWindMetronome:
//			metroPalette->ToggleVisiblity();
			break;

		case kFPCommandWindAll:
			DoTogglePalettes();
			break;

		case kFPCommandResetPalettes:
			DoResetPalettes();
			break;


		#pragma mark Help Menu
		//
		// Help Menu
		//
		case kFPCommandWebsite: {
			CFURLRef websiteURL = CFURLCreateWithString(kCFAllocatorDefault,
#if APPSTORE_SUPPORT
				CFSTR("http://www.thinkyhead.com/fretpet/appstore")
#elif KAGI_SUPPORT
				CFSTR("http://www.thinkyhead.com/fretpet/kagi")
#else
				CFSTR("http://www.thinkyhead.com/fretpet")
#endif
				, NULL);
			LSOpenCFURLRef(websiteURL, NULL);
			CFRELEASE(websiteURL);
			break;
		}

		case kFPCommandDocumentation: {
			CFURLRef manualURL = CFURLCreateWithString(kCFAllocatorDefault, CFSTR("http://www.thinkyhead.com/fretpet/manual"), NULL);
			LSOpenCFURLRef(manualURL, NULL);
			CFRELEASE(manualURL);
			break;
		}

		case kFPCommandReleaseNotes: {
			CFURLRef fileURL = CFBundleCopyResourceURL(CFBundleGetMainBundle(), CFSTR("Release Notes.pdf"), NULL, NULL);
			LSOpenCFURLRef(fileURL, NULL);
			CFRELEASE(fileURL);
			break;
		}

#if KAGI_SUPPORT || DEMO_ONLY
		case kFPCommandRegister: {
#if DEMO_ONLY
			CFURLRef websiteURL = CFURLCreateWithString(kCFAllocatorDefault,
														CFSTR("macappstore://itunes.apple.com/app/id424551638?mt=12"),
														NULL);
			LSOpenCFURLRef(websiteURL, NULL);
			CFRELEASE(websiteURL);
#else
			authorizer.RunKRMDialog();
#endif
			break;
		}

		case kFPCommandShowNagger:
			hasShownNagger = true;
			authorizer.RunNagDialog();
			break;
#endif

#if KAGI_SUPPORT
		case kFPCommandEnterSerial:
			authorizer.RunSerialNumberDialog();
			break;

		case kFPCommandDeauthorize:
			authorizer.Deauthorize();
			break;
#endif

		#pragma mark Scale Menu
		//
		// Mode / Scale and Instrument
		//
		case kFPCommandScaleMode:
			if (index > NUM_MODES) index--;
			DoSetScale(index-1);
			break;

		case kFPCommandModePrev:
			DoChangeEnharmonic(true);
			break;

		case kFPCommandModeNext:
			DoChangeEnharmonic(false);
			break;

		case kFPCommandScaleShift:
			DoSetNoteModifier((scalePalette->NoteModifier() + 2) % 3 - 1);
			break;



		#pragma mark Instrument Menu
		//
		// Instrument Menu
		//
		case kFPCommandInstrument:
		case kFPCommandGSInstrument:
			DoSetCurrentInstrument(FPMidiHelper::FauxGMToTrueGM(::GetMenuID(menu) + index));
			break;

		case kFPCommandInstGroup:
		{
			// get the submenu of the current menu
			MenuRef submenu = NULL;
			(void)GetMenuItemHierarchicalMenu(menu, index, &submenu);
			UInt16 faux = ::GetMenuID(submenu);
			DoSetCurrentInstrument(FPMidiHelper::FauxGMToTrueGM(faux + 1));
			break;
		}



		#pragma mark Scale Palette
		//
		// Scale Palette Commands
		//
		case kFPCommandScaleBack:
			DoScaleBack();
			break;

		case kFPCommandScaleFwd:
			DoScaleForward();
			break;

		case kFPCommandFlat:
			DoSetNoteModifier(-1);
			break;

		case kFPCommandNatural:
			DoSetNoteModifier(0);
			break;

		case kFPCommandSharp:
			DoSetNoteModifier(1);
			break;

		case kFPCommandBulb:
			scalePalette->ToggleIllumination();
			break;




		#pragma mark Chord Palette
		//
		// Chord Palette Commands
		//
		case kFPCommandMoreChords:
			chordPalette->UpdateExtendedView();
			break;


		case kFPCommandTransposeNorth:
			DoTransposeArrow(false);
			break;

		case kFPCommandTransposeSouth:
			DoTransposeArrow(true);
			break;



		#pragma mark Circle Palette
		//
		// Circle Palette Commands
		//
		case kFPCommandCircleEye:
			DoToggleCircleEye();
			break;

		case kFPCommandHold:
			DoToggleCircleHold();
			break;


		#pragma mark Guitar Palette
		//
		// Guitar Palette Commands
		//
		case kFPCommandTuningName:
			DoToggleTuningName();
			break;

		#pragma mark Diagram Palette
		//
		// Diagram Palette Commands
		//
		case kFPCommandNumRoot:
		case kFPCommandNumKey:
			DoSetRootType(cid == kFPCommandNumKey);
			break;


		#pragma mark Info Palette
		//
		// Info Palette Commands
		//
		case kFPCommandTransform1:
		case kFPCommandTransform2:
		case kFPCommandTransform3:
		case kFPCommandTransform4:
			DoToggleTransformFlag(cid - kFPCommandTransform1);
			break;

		case kFPInfoCheckTransform:
			DoToggleTransformFlag();
			break;

#if QUICKTIME_SUPPORT
		case kFPInfoCheckQT:
			DoToggleQuicktime(kCurrentChannel);
#else
		case kFPInfoCheckSynth:
			DoToggleAUSynth(kCurrentChannel);
#endif
			break;

		case kFPInfoCheckCM:
			DoToggleMidi(kCurrentChannel);
			break;

		default:
			err = eventNotHandledErr;
			break;
	}

	return err;
}


void FPApplication::GenerateCommandEvent(MenuCommand cid) {
	OSStatus err;
	HICommand command = { 0, cid, { NULL, 0 } };

	err = ProcessHICommand(&command);

//	EventQueueRef queue = GetCurrentEventQueue();
//	TCarbonEvent event(kEventClassCommand, kEventCommandProcess);
//	err = event.SetParameter(kEventParamDirectObject, command);
//	event.PostToQueue(queue);
}


bool FPApplication::DoesKeyRepeat(char key, UInt16 mods) {
	bool result = false;

	if (!HAS_COMMAND(mods)) {
		switch (key) {
			case '[':
			case ']':
			case kUpArrowCharCode:
			case kDownArrowCharCode:
			case kLeftArrowCharCode:
			case kRightArrowCharCode:
				result = true;
				break;
		}
	}

	return result;
}


bool FPApplication::HandleKey(unsigned char key, UInt16 mods, bool repeating) {
	if (IsDialogFront())
		return false;

	FPHistoryEvent	*event;
	bool			result = true;

	switch( key ) {
		//
		// B
		// Fret Bracket
		//
		case 'B':
		case 'b':
			DoToggleBracket();
			break;

		//
		// H
		// Horizontal View
		//
		case 'H':
		case 'h':
			DoToggleHorizontal();
			break;
	
		//
		// D
		// Toggle Fancy Dots
		//
		case 'D':
		case 'd':
			DoToggleFancyDots();
			break;

		//
		// R
		// Reverse Strung
		//
		case 'R':
		case 'r':
			DoToggleReverseStrung();
			break;

		//
		// T / Option-T
		// Transform Mode
		//
		case 'T':
		case 't':
		case 0xA0:
			DoToggleTransformMode( mods );
			break;

		//
		// U
		// Unused Tones
		//
		case 'U':
		case 'u':
			DoToggleUnusedTones();
			break;

		//
		// Option-Tab
		// Add a scale tone and move up a third
		//
		case kTabCharCode:
			if ( IS_OPTION(mods) ) {
				if ( scalePalette->IsVisible() )
					DoAddScaleTone(true);
			}
			break;

		//
		// ESC = Stop
		// Doubles for Command-.
		//
//		case kEscapeCharCode:
//			if (player->IsPlaying())
//				DoStop();
//			break;

		//
		// SPACE = Play / Stop
		// Doubles for Command-.
		//
		case kSpaceCharCode: {
			FPDocWindow *wind = FPDocWindow::frontWindow;
			if (player->IsPlaying())
				DoStop();
			else if (wind && wind->ReadyToPlay())
				DoPlay();
			else
				DoHearChord(mods);

			}
			break;

		//
		// Option-SPACE = Hear
		//
		case kNonBreakingSpaceCharCode:
			DoHearChord( optionKey );
			break;

		//
		// [ ] = Move the Fret Bracket
		//
		case '[':
		case ']': {
			SInt16 dir = guitarPalette->IsLeftHanded() && guitarPalette->IsHorizontal() ? ( (key == '[') ? 1 : -1 ) : ( (key == '[') ? -1 : 1 );
			SInt16 newLow = globalChord.brakLow + dir;
			SInt16 newHi = globalChord.brakHi + dir;
			if (newLow >= 0 && newHi <= guitarPalette->NumberOfFrets()) {
				event = fretpet->StartUndo(UN_EDIT, CFSTR("Move Fret Bracket"));
				event->SaveCurrentBefore();

				if (repeating)
					event->SetMerge();

				DoMoveFretBracket(newLow, newHi);

				event->SaveCurrentAfter();
				event->Commit();
			}
			break;
		}

		//
		// Option-Arrows = Move the Scale Cursor
		//
		case kUpArrowCharCode:
		case kDownArrowCharCode:
		case kLeftArrowCharCode:
		case kRightArrowCharCode:

			if (IS_OPTION(mods)) {
				UInt16	func = scalePalette->CurrentFunction();
				UInt16	line = INDEX_FOR_KEY(scalePalette->CurrentKey());

				switch (key) {
					case kUpArrowCharCode:
						line += OCTAVE - 1;
						break;

					case kDownArrowCharCode:
						line++;
						break;

					case kLeftArrowCharCode:
						func = (func + NUM_STEPS - 1) % NUM_STEPS;
						break;

					case kRightArrowCharCode:
						func = (func + 1) % NUM_STEPS;
						break;
				}

				FPHistoryEvent	*event = NULL;
				bool transforming = IsTransformModeEnabled();

				if (!globalChord.IsLocked() || transforming) {
					event = StartUndo(UN_EDIT, transforming ? CFSTR("Direct Transform") : CFSTR("Root Change"));
					event->SaveCurrentBefore();
					if (repeating)
						event->SetMerge();
				}

				DoMoveScaleCursor(func + 1, NOTEMOD(line));

				if (event != NULL) {
					event->SaveCurrentAfter();
					event->Commit();
					EndTransformOnce();
				}
			}
			else {
				result = false;
			}
			break;

		default:
			// fprintf(stderr, "Application Key %d\n", key);
			result = false;
			break;

	}

	return result;
}


static int stickyCursor = 0;
static int mouseCursor = 0;
void FPApplication::SetMouseCursor(int cursIndex, bool sticky) {
	if (sticky) {
		int oldSticky = stickyCursor;
		stickyCursor = cursIndex;
		if (mouseCursor == oldSticky)
			SetMouseCursor(cursIndex);
	}
	else {
		if ( cursIndex == 0 )
			ResetMouseCursor();
		else {
			mouseCursor = cursIndex;
			SetCCursor( cursors[cursIndex - kFirstCursor] );
		}
	}
}


void FPApplication::ResetMouseCursor() {
	mouseCursor = stickyCursor;

	if ( stickyCursor == 0 )
		SetThemeCursor(kThemeArrowCursor);
	else
		SetCCursor( cursors[stickyCursor - kFirstCursor] );
}

#pragma mark -
//===============================================
//
//	Application-level Commands
//	and
//	Controller Methods
//
//	With very few exceptions the state-changing
//	methods in the view classes don't update the
//	view.
//
//	In addition the view methods do not call the
//	methods of other views directly.
//
//	All actions initiated from views go through
//	these controller methods. Any update to the
//	initiating view is purely in the course of
//	updating all appropriate views for a given
//	state change.
//
//	All the views contain the data pertaining only
//	to their representation. For example, the
//	Scale Palette contains the state of the cursor
//	and extrapolates the current scale, tones,
//	and other data.
//
//	In the case of the Scale Palette the cursor,
//	notes, etc., must remain synchronized with
//	the current chord state. The cleanest way to
//	accomplish this is through a central controller.
//	
//===============================================


#pragma mark -
#pragma mark Chord Operations

void FPApplication::DoAddTones(UInt16 newTones, UInt16 startTone, bool redoFingering) {
	globalChord.AddTones(newTones);
	player->PlayArpeggio(newTones, startTone);	

	if (redoFingering)
		guitarPalette->DropFingering();

	ReflectChordChanges();
}


void FPApplication::DoSubtractTones(UInt16 tones) {
	globalChord.SubtractTones(tones);
	guitarPalette->DropFingering();
	ReflectChordChanges();
}


void FPApplication::DoToggleTone(SInt16 tone, bool redoFingering) {
	/*
		if (globalChord.tones == 0) {
			globalChord.SetRoot(scalePalette->CurrentTone());
			globalChord.SetKey(scalePalette->CurrentKey());
		}
	*/

	UInt16	newTones = BIT(NOTEMOD(tone));

	globalChord.ToggleTones(newTones);

	if (globalChord.tones & newTones)
		player->PlayArpeggio(newTones, tone);

	if (redoFingering)
		guitarPalette->DropFingering();

	ReflectChordChanges();
} 


bool FPApplication::DoAddTriadForTone(SInt16 tone, bool redoFingering) {
	SInt16	func = scalePalette->FunctionOfTone(tone);
	if (func >= 0) {
		UInt16	newTones = scalePalette->GetTriad(func);
		DoAddTones(newTones, tone, redoFingering);
	}

	return (func >= 0);
}


void FPApplication::DoInvertTones(bool undoable) {
	FPHistoryEvent *event = NULL;

	if (undoable)
		event = StartUndo(UN_INVERT, CFSTR("Invert Tones"));

	globalChord.InvertTones();
	guitarPalette->DropFingering();
	ReflectChordChanges();

	if (undoable)
		event->Commit();
}


void FPApplication::DoToggleChordLock() {
	bool locked = chordPalette->lockButton->Toggle();

	FPHistoryEvent *event = StartUndo(UN_EDIT, locked ? CFSTR("Lock Root") : CFSTR("Unlock Root"));
	event->SaveCurrentBefore();

	globalChord.SetLocked(locked);
	ReflectChordChanges();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoDirectHarmonize(SInt16 tone) {
	tone = NOTEMOD(tone);
	if (scalePalette->ScaleHasTone(tone)) {
		SInt16 newFunc	= scalePalette->FunctionOfTone(tone);
		SInt16 kfunc	= scalePalette->FunctionOfTone(globalChord.Root());
		SInt16 fdiff	= newFunc - ( (kfunc >= 0) ? kfunc : globalChord.RootScaleStep() );

		if (fdiff) {
			if (!globalChord.IsLocked())
				scalePalette->MoveScaleCursor(newFunc + 1, -1);

			globalChord.ResetStepInfo();
			globalChord.HarmonizeBy(fdiff);
			ReflectNewFingering();
		}
	}
}


#pragma mark -
#pragma mark Interface Updaters

void FPApplication::ReflectPatternChanges() {
	UpdateDocumentChord();
	UpdatePlayTools();
}


void FPApplication::ReflectFingeringChanges() {
	guitarPalette->ArrangeDots();
	diagramPalette->UpdateDiagram();
	pianoPalette->PositionPianoDots();
	UpdateDocumentChord();
}


void FPApplication::ReflectChordChanges(bool scaleUpdate, bool docIsSource) {
	static FPChord	previousChord((UInt16)0,0,0);
	bool			toneChange = previousChord.tones != globalChord.tones;
	bool			rootChange = previousChord.root != globalChord.root;
	bool			keyChange = previousChord.key != globalChord.key;
	bool			lockChange = previousChord.IsLocked() != globalChord.IsLocked();
	bool			modChange = previousChord.RootModifier() != globalChord.RootModifier();
	bool			stepChange = previousChord.RootScaleStep() != globalChord.RootScaleStep();
	bool			anyChange = toneChange || rootChange || keyChange || scaleUpdate;

	if (scalePalette) {
		if ( (scaleUpdate || lockChange) && (!globalChord.IsLocked() || globalChord.RootNeedsScaleInfo()) )
			scalePalette->SetCursorWithChord(globalChord);		// Move cursor to the key/root

		if (anyChange || lockChange || modChange || stepChange)
			scalePalette->DrawChanges();	// Locked: distinct cursor/root markers   Unlocked: cursor=root
	}

	if (anyChange) {
		if (infoPalette)
			infoPalette->UpdateFields();

		if (chordPalette)
			chordPalette->UpdateNow();

		if (circlePalette) {
			circlePalette->UpdateCircle();
			circlePalette->UpdateHarmonyDots();
		}
	}

	if ( anyChange || modChange || (lockChange && !globalChord.IsLocked()) ) {
		if (guitarPalette)
			guitarPalette->ArrangeDots();
	}

	if (anyChange) {
		if (diagramPalette)
			diagramPalette->UpdateDiagram();

		if (pianoPalette)
			pianoPalette->PositionPianoDots();

//		else if (FPDocWindow::activeWindow)
//			FPDocWindow::activeWindow->UpdateLine();
	}

	if (anyChange || lockChange) {
		if (!docIsSource && FPDocWindow::activeWindow != NULL)
			UpdateDocumentChord();

		if (lockChange && chordPalette)
			chordPalette->UpdateLockButton();
	}


	if (toneChange) {
		if (toolPalette)
			toolPalette->UpdateButtonStates();
	}

	if (keyChange || scaleUpdate ) {
		if (pianoPalette)
			pianoPalette->UpdatePianoKeys();
	}

	previousChord = globalChord;
}


void FPApplication::SetGlobalChord(const FPChord &chord, bool fromDocument) {
	globalChord = chord;
	ReflectChordChanges(true, fromDocument);
	guitarPalette->UpdateBracket();
}


void FPApplication::UpdateDocumentChord(bool setDirty) {
	FPDocWindow *wind = FPDocWindow::frontWindow;
	if (wind)
		wind->SetCurrentChord(globalChord);
}


//-----------------------------------------------
//
//	ReflectNewFingering
//
//	Reflect changes of a single fingering on the
//	guitar palette.
//
void FPApplication::ReflectNewFingering() {
	guitarPalette->DropFingering();

	ReflectChordChanges(true);

	if (!player->IsPlaying())
		DoHearChord( GetCurrentKeyModifiers() );
}


#pragma mark -
#pragma mark Document Window

void FPApplication::DoPlay() {
	FPDocWindow *wind = FPDocWindow::frontWindow;

	if (wind != NULL) {
		if (player->IsPlaying()) {
			if (!IsFollowViewEnabled())
				player->SetPlayRangeFromSelection();
		}
		else {
			player->PlayDocument();
			UpdatePlayTools();
		}
	}
}


void FPApplication::DoStop() {
	if (player->IsPlaying()) {
		player->Stop();
/*
		if (IsFollowViewEnabled() && doc->Count())
			wind->UpdateLine();
*/
		UpdatePlayTools();
	}
}


void FPApplication::DoWindowActivated(FPDocWindow *wind) {
	FPDocWindow::activeWindow = wind;
	FPDocWindow::frontWindow = wind;
	FPDocument *doc = wind->document;

	//
	// Update Music Player
	//
	for (int part=DOC_PARTS; part--;) {
		// Set the Channel before setting the Instrument
		player->SetTransformFlag(part, doc->TransformFlag(part));
#if QUICKTIME_SUPPORT
		player->SetQuicktimeOutEnabled(part, doc->GetQuicktimeOutEnabled(part));
#else
		player->SetAUSynthOutEnabled(part, doc->GetSynthOutEnabled(part));
#endif
		player->SetMidiOut(part, doc->MidiOut(part));
		player->SetMidiChannel(part, doc->MidiChannel(part));
		player->SetInstrument(part, doc->GetInstrument(part));
		player->SetVelocity(part, doc->Velocity(part));
		player->SetSustain(part, doc->Sustain(part));
		doc->UpdateInstrumentName(part);
	}

	player->SetCurrentPart(doc->CurrentPart());

	//
	// Update Scale Palette
	//
	scalePalette->SetEnharmonic(doc->Enharmonic());
	scalePalette->SetScale(doc->ScaleMode());
	scalePalette->DrawChanges();
	scalePalette->nameControl->DrawNow();

	//
	// Update Guitar Palette
	//
	guitarPalette->SetTuning(doc->Tuning());

	//
	// Update Channels Palette
	//
	channelsPalette->RefreshControls();

	//
	// Update Toolbar
	//
	UpdatePlayTools();

	//
	// Print out some stats
	//
//	doc->PrintStatistics();

	//
	// Finally get the chord
	//
	wind->UpdateGlobalChord();

	//
	// This is needed because the Doc popup gets the name from
	// quicktime, so it isn't available until after the player
	// has been updated
	//
	wind->UpdateDocPopup();
}


void FPApplication::DoSelectPart(PartIndex part) {
	FPDocWindow *wind = FPDocWindow::frontWindow;

	player->SetCurrentPart(part);
	if (wind) wind->SetCurrentPart(part);

//	channelsPalette->RefreshControls();
}


//-----------------------------------------------
//
//	UpdateDocumentFingerings
//
//	Update the fingerings of all document chords
//	after changing the tuning
//
void FPApplication::UpdateDocumentFingerings() {
	FPDocWindow *wind = FPDocWindow::frontWindow;
	if (wind)
		wind->UpdateDocumentFingerings();
}


void FPApplication::DoAddGlobalChord() {
	FPDocWindow	*wind = FPDocWindow::frontWindow;

	if (wind == NULL)
		wind = NewDocWindow();

	if (wind != NULL) {
		FPHistoryEvent *event = StartUndo(UN_INSERT, CFSTR("Add Chord"));
		if ( wind->AddChord(HAS_COMMAND(GetCurrentEventKeyModifiers())) ) {
			event->SaveCurrentAfter();
			event->Commit();
		}
	}

	UpdatePlayTools();
}


#pragma mark -
#pragma mark Chord Sequencer

void FPApplication::DoSetChordRepeat(UInt16 rep) {
	globalChord.SetRepeat(rep);
	ReflectPatternChanges();
}


void FPApplication::DoSetPatternDot(UInt16 string, UInt16 step) {
	globalChord.SetPatternDot(string, step);

	if (!player->IsPlaying())
		player->PickString(string);

	ReflectPatternChanges();
}


void FPApplication::DoClearPatternDot(UInt16 string, UInt16 step) {
	globalChord.ClearPatternDot(string, step);
	ReflectPatternChanges();
}


void FPApplication::DoSetPatternStep(UInt16 step) {
	globalChord.SetPatternStep(step);

	if (!player->IsPlaying())
		player->StrikeChord(globalChord);

	ReflectPatternChanges();
}


void FPApplication::DoClearPatternStep(UInt16 step) {
	globalChord.ClearPatternStep(step);
	ReflectPatternChanges();
}


void FPApplication::DoSetPatternSize(UInt16 size) {
	globalChord.SetPatternSize(size);
	ReflectPatternChanges();
}

#pragma mark -
#pragma mark Toolbar Buttons

void FPApplication::DoToggleTransformMode(UInt32 mods) {
	if (toolPalette->transButton->Toggle()) {
		SetMouseCursor(kCursTrans, true);
		transformOnce = IS_OPTION(mods);
	}
	else
		SetMouseCursor(0, true);
}


void FPApplication::DoToggleEditMode(bool drawRegions) {
	bool edit = toolPalette->editButton->Toggle();
	bool play = player->IsPlaying();

	// When edit mode turns on the selection becomes relevant
	if (edit && play)
		player->SetPlayRangeFromSelection();

	// When playing the Blocked cursor comes and goes
	// if edit mode is enabled
	if (drawRegions && play) {
		FPDocWindow *wind = FPDocWindow::activeWindow;
		if (wind)
			wind->UpdateTrackingRegions();
	}
}


void FPApplication::DoToggleFollowView() {
	bool		folo = toolPalette->eyeButton->Toggle();
	FPDocWindow	*wind = FPDocWindow::frontWindow;

	if (player->IsPlaying()) {
		if (folo) {
			if (IsEditModeEnabled())
				DoToggleEditMode(false);

			if (wind) {
				wind->SetSelection(-1, true);

				//
				// Edit mode is always disabled above
				// but this behavior might change if
				//
				if (IsEditModeEnabled()) {
					UInt16 curs = wind->GetCursor();
					player->SetPlayRange(curs, curs);
					if (player->PlayingChord() != curs)
						player->SetPlayingChord(curs);
				}
			}
		}

		// For the sake of the Blocked cursor
		if (wind)
			wind->UpdateTrackingRegions();
	}
}


void FPApplication::DoToggleRoman() {
	romanMode = toolPalette->romanButton->Toggle();
	circlePalette->UpdateCircle();
	chordPalette->InvalidateWindow();
	FPDocWindow::UpdateAll();
}

//-----------------------------------------------
//
//  DoSetKeyOrder
//
//	Set the order in which keys and tones appear.
//	By default tones are arranged in harmonic fifths.
//	But they can also appear in half-step order.
//
void FPApplication::DoSetKeyOrder(bool isHalves) {
	keyOrderHalves = isHalves;
	scalePalette->InitBoxesForKeyOrder();
	scalePalette->DrawChanges();
	circlePalette->UpdateCircle();
	chordPalette->UpdateMoreNow();
	toolPalette->DrawKeyOrder();
	FPDocWindow::UpdateAll();

	SetupTransposeMenu();
}


void FPApplication::DoSetRootType(bool isKey) {
	chordRootType = isKey;
	toolPalette->DrawRootType();
	diagramPalette->InvalidateWindow();

	if (guitarPalette->ShowingNumberedDots() /* && globalChord.root != globalChord.key */)
		guitarPalette->ArrangeDots();
}


#pragma mark -
#pragma mark Scale Palette

void FPApplication::DoScaleForward() {
	scalePalette->SetScaleForward();
	scalePalette->nameControl->DrawNow();
//	globalChord.SetRoot(scalePalette->CurrentTone());

	FPDocWindow *wind;
	if ((wind = FPDocWindow::activeWindow))
		wind->SetScale(scalePalette->CurrentMode());

	if (pianoPalette)
		pianoPalette->UpdatePianoKeys();

	ReflectChordChanges(true);
}


void FPApplication::DoScaleBack() {
	scalePalette->SetScaleBack();
	scalePalette->nameControl->DrawNow();
//	globalChord.SetRoot(scalePalette->CurrentTone());

	FPDocWindow *wind;
	if ((wind = FPDocWindow::activeWindow))
		wind->SetScale(scalePalette->CurrentMode());

	if (pianoPalette)
		pianoPalette->UpdatePianoKeys();

	ReflectChordChanges(true);
}


void FPApplication::DoSetNoteModifier(SInt16 offset, bool undoable) {
	FPHistoryEvent *event = NULL;
	bool	locked = globalChord.IsLocked();
	SInt16	oldModifier = scalePalette->NoteModifier();
	scalePalette->SetNoteModifier(offset);

	if (!locked) {
		if (undoable) {
			event = StartUndo(UN_MODIFY, offset ? ( (offset < 0) ? CFSTR("Flatten Tone") : CFSTR("Sharpen Tone") ) : CFSTR("Natural Tone") );
			event->SaveDataBefore(CFSTR("modifier"), oldModifier);
		}

		globalChord.SetRoot(scalePalette->CurrentTone());
		globalChord.SetRootModifier(offset);
		globalChord.SetRootScaleStep(scalePalette->CurrentFunction());
		ReflectChordChanges();

		if (undoable) {
			event->SaveDataAfter(CFSTR("modifier"), offset);
			event->Commit();
		}
	}
	else {
		scalePalette->DrawChanges();
		guitarPalette->ArrangeDots();
	}

}


void FPApplication::DoMoveScaleCursor(UInt16 inFunction, UInt16 keyIndex) {
//	scalePalette->SetNoteModifier(0);

	if (IsTransformModeEnabled()) {
		UInt16	newKey = KEY_FOR_INDEX(keyIndex);

		if (NOTEMOD(newKey - globalChord.Key()))
			globalChord.TransposeTo(newKey);

		if (inFunction > 0) {
			SInt16 fdiff = inFunction - 1 - globalChord.RootScaleStep();

			if (fdiff) {
				globalChord.ResetStepInfo();
				globalChord.HarmonizeBy(fdiff);
			}
		}

		scalePalette->MoveScaleCursor(inFunction, keyIndex);
		ReflectNewFingering();
	}
	else {
		scalePalette->MoveScaleCursor(inFunction, keyIndex);

		if (globalChord.IsLocked()) {
			// NOTE: ReflectChordChanges called on toggle only
			scalePalette->DrawChanges();
			infoPalette->UpdateFields(true, false);
			guitarPalette->ArrangeDots();				// to blink the right dots
		}
		else {
			globalChord.SetKey(scalePalette->CurrentKey());
			globalChord.SetRoot(scalePalette->CurrentTone());
			globalChord.SetRootScaleStep(scalePalette->CurrentFunction());
			globalChord.SetRootModifier(scalePalette->NoteModifier());
			ReflectChordChanges();
		}

		if (!scalePalette->IsIlluminated() || globalChord.HasTone(scalePalette->CurrentTone()) || HAS_COMMAND(GetCurrentEventKeyModifiers()))
			scalePalette->PlayCurrentTone();
	}
}


#pragma mark -
#pragma mark Guitar Palette

SInt16 FPApplication::StringTone(UInt16 string) {
	int f = globalChord.fretHeld[string];
	return (f < 0) ? -1 : guitarPalette->Tone(string, f);
}


UInt16 FPApplication::DotColor(UInt16 tone) {
	return guitarPalette->DotColor(tone);
}


bool FPApplication::IsBracketEnabled() {
	return globalChord.bracketFlag;
}


void FPApplication::DoSetGuitarTuning(const FPTuningInfo &t, bool undoable) {
	FPHistoryEvent	*event = NULL;
	FPDocWindow		*wind = FPDocWindow::activeWindow;
	bool			doCommit = false;

	if (undoable) {
		event = StartUndo(UN_TUNING_CHANGE, CFSTR("Change Tuning"));
		TDictionary *dict = guitarPalette->CurrentTuning().GetDictionary();
		event->SaveDataBefore(CFSTR("tuning"), dict->GetDictionaryRef());
		delete dict;
	}

	if ( wind ) {
		FPDocument	*doc = wind->document;
		ChordIndex	size = doc->Size();

		if ( !undoable || wind->AskTuningChange() ) {
			if (undoable) {
				if (size > 0)
					event->SaveGroupsBefore(0, size - 1);
				else
					event->SaveCurrentBefore();
			}

			guitarPalette->SetTuning(t, size > 0);
			doc->SetTuning(t);

			UpdateDocumentFingerings();

			if (undoable)
				doCommit = true;

			wind->UpdateGlobalChord();
		}
	}
	else {
		if (undoable)
			event->SaveCurrentBefore();

		guitarPalette->SetTuning(t);
		guitarPalette->DropFingering();
		ReflectFingeringChanges();

		if (undoable)
			doCommit = true;
	}

	if (doCommit) {
		TDictionary *dict = t.GetDictionary();
		event->SaveDataAfter(CFSTR("tuning"), dict->GetDictionaryRef());
		delete dict;
		event->Commit();
	}
}


void FPApplication::DoMoveFretCursor(UInt16 string, UInt16 fret) {
	guitarPalette->MoveFretCursor(string, fret);

	if (IsTransformModeEnabled())
		DoDirectHarmonize(guitarPalette->CurrentTone());

	infoPalette->UpdateFields(false, true);
}


void FPApplication::DoOverrideFingering() {
	if (guitarPalette->OverrideFingering(globalChord)) {
		ReflectFingeringChanges();

		UInt16	string = guitarPalette->CurrentString();
		SInt16	fret = globalChord.fretHeld[string];
		if (fret >= 0) {
			Tone tone = guitarPalette->Tone(string, fret);
			player->PlayNote(kCurrentChannel, tone);
			guitarPalette->TwinkleTone(string, fret);
		}

		UpdateDocumentChord();
	}
}


void FPApplication::DoToggleGuitarTone(UInt16 string, UInt16 fret) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Toggle Guitar Tone"));
	event->SaveCurrentBefore();

	UInt16 tone = guitarPalette->Tone(string, fret);

	if (!globalChord.HasTone(tone)) {
		globalChord.fretHeld[string] = fret;
		guitarPalette->TwinkleTone(string, fret);
	}

	DoToggleTone(tone - OCTAVE, false);

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoToggleGuitarTone() {
	DoToggleGuitarTone(guitarPalette->CurrentString(), guitarPalette->CurrentFret());
}


void FPApplication::DoAddGuitarTriad() {
	DoAddTriadForTone(guitarPalette->CurrentTone(), false);
}


void FPApplication::DoToggleTuningName() {
	guitarPalette->ToggleTuningName();
}


void FPApplication::DoMoveFretBracket(UInt16 newLow, UInt16 newHigh) {
	globalChord.SetBracket(newLow, newHigh);
	guitarPalette->UpdateBracket();
	guitarPalette->DropFingering();
	ReflectFingeringChanges();

	UpdateDocumentChord();
}


void FPApplication::DoToggleBracket() {
	bool newBracket = !globalChord.bracketFlag;

	FPHistoryEvent *event = fretpet->StartUndo( UN_EDIT, newBracket ? CFSTR("Enable Fret Bracket") : CFSTR("Disable Fret Bracket") );
	event->SaveCurrentBefore();

	globalChord.bracketFlag = newBracket;
	guitarPalette->UpdateBracket();
	guitarPalette->DropFingering();
	ReflectFingeringChanges();

	UpdateDocumentChord();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoToggleHorizontal() {
	bool rev = IsTablatureReversed();
	guitarPalette->ToggleHorizontal();
	if (rev != IsTablatureReversed())
		FPDocWindow::UpdateAll();
}


void FPApplication::DoToggleLefty() {
	guitarPalette->ToggleLefty();
	if (guitarPalette->IsHorizontal())
		FPDocWindow::UpdateAll();
}


void FPApplication::DoToggleReverseStrung() {
	guitarPalette->ToggleReverseStrung();
	FPDocWindow::UpdateAll();
}


void FPApplication::DoToggleUnusedTones() {
	guitarPalette->ToggleUnusedTones();
}


void FPApplication::DoToggleFancyDots() {
	guitarPalette->ToggleFancyDots();
}


void FPApplication::DoSetDotStyle(UInt16 style) {
	guitarPalette->SetDotStyle(style);
}


#pragma mark -
#pragma mark Keyboard Palette

void FPApplication::SetPianoKeyPlaying(SInt16 key, bool isPlaying) {
	pianoPalette->SetKeyPlaying(key, isPlaying);
}


#pragma mark -
#pragma mark Circle Palette

void FPApplication::DoTransposeArrow(bool down) {
//	fprintf(stderr, "Adding %d\n", (down ? KEY_FOR_INDEX(1) : KEY_FOR_INDEX(OCTAVE-1)));
	DoTransposeKey(globalChord.key + (down ? KEY_FOR_INDEX(1) : KEY_FOR_INDEX(OCTAVE-1)));
}


#pragma mark -
#pragma mark Chord Palette

void FPApplication::DoChooseName(UInt16 line) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Change Chord Name"));
	event->SaveCurrentBefore();

	globalChord.SetRoot(globalChord.Root() + (KEY_FOR_INDEX(1)*(line+1)));
	globalChord.ResetStepInfo();
	ReflectChordChanges(true);

	event->SaveCurrentAfter();
	event->Commit();
}


#pragma mark -
#pragma mark FretPet Menu

void FPApplication::DoAboutFretPet() {
	hasShownSplash = true;
	FPAboutBox aboutBox;
	aboutBox.RunModal();
}


#pragma mark -
#pragma mark File Menu

FPDocWindow* FPApplication::NewDocWindow() {
	return FPDocWindow::NewUntitled();
}


FPDocWindow* FPApplication::OpenRecentItem(CFIndex item) {
	FSRef			ref;
	FPDocWindow		*doc = NULL;

	if (noErr == recentItems.GetFSRef(item, &ref))
		doc = OpenDocument(ref);

	return doc;
}


FPDocWindow* FPApplication::OpenDocument(const FSRef &fref) {
	DoStop();

	ClosePlaceholderDocument();

	FPDocWindow *wind = NULL;
	FPDocumentIterator itr = FPDocWindow::docWindowList.begin();
	while ( itr != FPDocWindow::docWindowList.end() ) {
		FPDocWindow *wind = *itr;
		if (wind->document->CompareFSRef(fref))
			break;
		itr++;
	}

	if (wind)
		wind->Select();
	else {
		wind = new FPDocWindow(CFSTR("loading..."));
		OSStatus err = wind->InitFromFile(fref);
		if (err == noErr) {
			AddToRecentItems(*wind->document);
			DoWindowActivated(wind);	// Do this before showing, because the scale must be set before drawing
			wind->Show();
		}
		else {
			delete wind;
			wind = NULL;
		}
	}

	return wind;
}


void FPApplication::ClosePlaceholderDocument() {
	static bool firstOpen = false;
	if (FPDocWindow::docWindowList.size() == 1 && !firstOpen) {
		firstOpen = true;
		FPDocWindow *wind = FPDocWindow::frontWindow;
		if (wind != NULL && (!(wind->document->HasFile() || wind->IsDirty())))
			wind->Close();
	}
}


OSErr FPApplication::DoOpen() {
	OSType typeList[] = { BANK_FILETYPE };
	OpenFileDialog(CREATOR_CODE, 1, typeList);
	return noErr;
}


//-----------------------------------------------
//
//  DoQuit
//
//	TODO: Use TextEdit as the model for quit behavior:
//
//	- Close any modal (Open or Preferences) dialogs
//	- Ask to review changes if more than one document is dirty
//	- Review all modified documents, bringing them to front as-needed
//	- If no cancel occurred, close all unmodified documents
//
void FPApplication::DoQuit() {
	TerminateOpenFileDialog();
	preferences.ClosePreferencesDialog();
	FPDocWindow::CancelSheets();

	NavUserAction action;
	AskReviewChangesDialog(&action, true);

	switch (action) {
		case kNavUserActionReviewDocuments:
			isQuitting = true;
			FPDocWindow::frontWindow->TryToClose(true);
			break;

		case kNavUserActionDiscardDocuments:
			DoQuitNow();
			break;
	}
}


void FPApplication::DoQuitNow() {
	// A doc window will remain if "discarding all"
	// So save its position
	FPDocWindow *front = FPDocWindow::frontWindow;
	if (front) front->SavePreferences();

	if (toolPalette != NULL) {
		toolPalette->SavePreferences();
		scalePalette->SavePreferences();
		chordPalette->SavePreferences();
		pianoPalette->SavePreferences();
		guitarPalette->SavePreferences();
		diagramPalette->SavePreferences();
		infoPalette->SavePreferences();
		channelsPalette->SavePreferences();
	//	metroPalette->SavePreferences();
		circlePalette->SavePreferences();
		player->SavePreferences();
	}

	SavePreferences();
	QuitApplicationEventLoop();
}


#pragma mark -
#pragma mark Circle Palette

void FPApplication::DoToggleCircleEye() {
	circlePalette->ToggleEye();
}


void FPApplication::DoToggleCircleHold() {
	circlePalette->ToggleHold();
	circlePalette->UpdateCircle();
}


#pragma mark -
#pragma mark Undo

FPHistory* FPApplication::ActiveHistory() {
	FPDocWindow *wind = FPDocWindow::activeWindow;
	return ( wind )
			? wind->history
			: history;
}


FPHistoryEvent* FPApplication::StartUndo(UInt16 act, CFStringRef opName, PartMask partMask) {
	return ActiveHistory()->UndoStart(act, opName, partMask);
}


#pragma mark -
#pragma mark Scale Menu

void FPApplication::DoSetScale(UInt16 newMode) {
	scalePalette->SetScale(newMode);
	globalChord.SetRoot(scalePalette->CurrentTone());
	scalePalette->UpdateNameControl();
	scalePalette->DrawChanges();

	FPDocWindow *wind;
	if ((wind = FPDocWindow::activeWindow))
		wind->SetScale(newMode);

	if (pianoPalette)
		pianoPalette->UpdatePianoKeys();

	ReflectChordChanges(true);
}


void FPApplication::DoChangeEnharmonic(bool bDec) {
	SInt16	temp = scalePalette->Enharmonic();

	if (bDec)
		{ if (--temp < 1) temp = 6; }
	else
		{ if (++temp > 6) temp = 1; }

	scalePalette->SetEnharmonic(temp);
	scalePalette->SetScale(scalePalette->CurrentMode());		// Triggers drawing
//	scalePalette->DrawChanges();

	FPDocument *doc = ActiveDocument();
	if (doc)
		doc->SetEnharmonic(temp);

	ReflectChordChanges(true);
}


#pragma mark -
#pragma mark Chord Menu

void FPApplication::DoClear(int type) {
/*
	WindowRef	wind = FrontNonFloatingWindow();
	TWindow		*twind;
	if (wind)	twind = TWindow::GetTWindow(wind);

	fprintf(stderr, "FrontNonFloatingWindow=[%08X] TWindow=[%08X]\n", wind, twind);
	HideWindow(wind);
*/
	CFStringRef undoName[] = { CFSTR("Clear Tones"), CFSTR("Clear Pattern"), CFSTR("Clear Tones & Pattern") };
	FPHistoryEvent	*event = StartUndo(UN_EDIT, undoName[type]);
	event->SaveCurrentBefore();

	switch(type) {
		case 0:
		globalChord.Clear();
		ReflectChordChanges();
		break;

		case 1:
		globalChord.ClearPattern();
		UpdateDocumentChord();
		break;

		case 2:
		globalChord.Clear();
		globalChord.ClearPattern();
		ReflectChordChanges();
		break;
	}

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoAddTone(UInt16 tone, SInt16 refTone) {
	if (!globalChord.HasTone(tone)) {
		globalChord.AddTone(tone);

		guitarPalette->DropFingering();

		if (!player->IsPlaying()) {
			if (tone < refTone) tone += OCTAVE;				// Never play a tone below the reference tone
			player->PlayNote(kCurrentChannel, tone);
		}

		ReflectChordChanges();
	}
}


void FPApplication::DoSubtractTone(UInt16 tone) {
	if (globalChord.HasTone(tone)) {
		globalChord.SubtractTones(BIT(tone));
		guitarPalette->DropFingering();
		ReflectChordChanges();
	}
}


void FPApplication::DoHarmonizeBy(SInt16 steps) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Harmonize Chord"));
	event->SaveCurrentBefore();

	globalChord.ResetStepInfo();
	globalChord.HarmonizeBy(steps);
	ReflectNewFingering();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoTransposeKey(UInt16 key) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Transpose Chord"));
	event->SaveCurrentBefore();

	globalChord.TransposeTo(NOTEMOD(key));
	ReflectNewFingering();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoTransposeInterval(SInt16 interval) {
	DoTransposeKey(globalChord.key + interval);
}


void FPApplication::DoToggleScaleTone() {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Toggle Scale Tone"));
	event->SaveCurrentBefore();

	DoToggleTone(scalePalette->CurrentTone());

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoHearChord(short modifiers) {
	if (globalChord.IsBracketEnabled()) {
		if (HAS_SHIFT(modifiers))
			player->StrikeChord(globalChord);
		else
			player->StrumChord(globalChord, (HAS_OPTION(modifiers)));
	}
	else {
		player->PlayArpeggio(globalChord);
	}
}


void FPApplication::DoAddScaleTriad(bool setTones) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Add Scale Triad"));
	event->SaveCurrentBefore();

	UInt16	newTones = scalePalette->CurrentTriad();

	setTones ? globalChord.SetTones(newTones) : globalChord.AddTones(newTones);

	player->PlayArpeggio(newTones, scalePalette->CurrentTone());
	
	guitarPalette->DropFingering();
	ReflectChordChanges();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoSubtractScaleTriad() {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Subtract Scale Triad"));
	event->SaveCurrentBefore();

	globalChord.SubtractTones(scalePalette->CurrentTriad());
	guitarPalette->DropFingering();
	ReflectChordChanges();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoAddScale(bool setTones) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Add Scale"));
	event->SaveCurrentBefore();

	UInt16	newTones = scalePalette->CurrentScaleMask();

	setTones ? globalChord.SetTones(newTones) : globalChord.AddTones(newTones);
	player->PlayArpeggio(newTones, scalePalette->CurrentTone(), false, 8);
	
	guitarPalette->DropFingering();
	ReflectChordChanges();

	event->SaveCurrentAfter();
	event->Commit();
}


void FPApplication::DoAddScaleTone(bool upThird) {
	FPHistoryEvent	*event = StartUndo(UN_EDIT, CFSTR("Add Scale Tone"));
	event->SaveCurrentBefore();

	UInt16	func = scalePalette->CurrentFunction();
	UInt16	tone = scalePalette->CurrentTone();

	if (upThird)
		scalePalette->MoveScaleCursor( (func + 2) % 7 + 1, -1 );

	if (!globalChord.HasTone(tone)) {
		globalChord.AddTone(tone);

		if (!player->IsPlaying()) {
			UInt16	newTones = BIT(tone);
			if (tone < globalChord.Key())
				tone += OCTAVE;
			player->PlayArpeggio(newTones, tone);
		}

		guitarPalette->DropFingering();
	}

	if (!globalChord.IsLocked()) {
		globalChord.SetRoot(scalePalette->CurrentTone());
		globalChord.SetRootScaleStep(scalePalette->CurrentFunction());
	}

	ReflectChordChanges();

	event->SaveCurrentAfter();
	event->Commit();
}


#pragma mark -
#pragma mark Window Menu

void FPApplication::DoTogglePalettes() {
	if ( !palettesHidden && (
				scalePalette->IsVisible()
			||	chordPalette->IsVisible()
			||	pianoPalette->IsVisible()
			||	guitarPalette->IsVisible()
			||	diagramPalette->IsVisible()
			||	infoPalette->IsVisible()
			||	channelsPalette->IsVisible()
//			||	metroPalette->IsVisible()
			||	circlePalette->IsVisible()
		)
	) {
		scalePalette->Hide();
		chordPalette->Hide();
		pianoPalette->Hide();
		guitarPalette->Hide();
		diagramPalette->Hide();
		infoPalette->Hide();
		channelsPalette->Hide();
//		metroPalette->Hide();
		circlePalette->Hide();
		palettesHidden = true;
	}
	else {
		scalePalette->ShowIfShown();
		chordPalette->ShowIfShown();
		pianoPalette->ShowIfShown();
		guitarPalette->ShowIfShown();
		diagramPalette->ShowIfShown();
		infoPalette->ShowIfShown();
		channelsPalette->ShowIfShown();
//		metroPalette->ShowIfShown();
		circlePalette->ShowIfShown();
		palettesHidden = false;
	}
}


void FPApplication::DoResetPalettes() {
	toolPalette->Reset();
	toolPalette->SetAnchored(TRUE);

	Rect		avail;

/*
	if (systemVersion >= 0x1050) {
		HIRect availableRect;
		HICoordinateSpace space;
		CGDirectDisplayID display;
		(void)HIWindowGetAvailablePositioningBounds(display, space, &availableRect);
	} else		// */
	{
		(void) GetAvailableWindowPositioningBounds(GetMainDevice(), &avail);
	}

	Rect	toolInner = toolPalette->Bounds();
	Rect	toolPos = toolPalette->StructureBounds();

	int	xpos = toolPos.left + 1,
		margin_t = toolInner.top - toolPos.top + 1,
		margin_b = toolPos.bottom - toolInner.bottom + 1,
//		margin_l = toolInner.bottom - toolPos.left + 1,
		margin_r = toolPos.right - toolInner.right + 1;

	chordPalette->Move(xpos, toolPos.bottom + margin_t);
	Rect	chordPos = chordPalette->StructureBounds();

	scalePalette->Move(xpos, chordPos.bottom + margin_t);
	Rect	scalePos = scalePalette->StructureBounds();

	channelsPalette->Move(xpos, scalePos.bottom + margin_t);
	Rect	channelsPos = channelsPalette->StructureBounds();

	pianoPalette->Move(xpos, channelsPos.bottom + margin_t);
	Rect	pianoPos = pianoPalette->StructureBounds();

	int documentMargin;

	// Two options for the guitar placement
	if (guitarPalette->IsHorizontal()) {
		Rect guitarSize = guitarPalette->GetContentSize();
		guitarPalette->Move(avail.right - guitarSize.right - margin_r, avail.bottom - guitarSize.bottom - margin_b);
		documentMargin = scalePos.right;
	}
	else {
		guitarPalette->Move(scalePos.right + 1, toolPos.bottom + margin_t);
		Rect guitarPos = guitarPalette->StructureBounds();
		documentMargin = guitarPos.right;
	}

	// Align these palettes to the right side
	circlePalette->Move(avail.right - circlePalette->Width(), toolInner.top + margin_t);
	Rect	circlePos = circlePalette->StructureBounds();

	infoPalette->Move(circlePos.left, circlePos.bottom + margin_t);
	Rect	infoPos = infoPalette->StructureBounds();

	diagramPalette->Move(infoPos.left, infoPos.bottom + margin_t);
	Rect	diagramPos = diagramPalette->StructureBounds();


	FPDocWindow *wind = FPDocWindow::frontWindow;

	if (wind) {
		wind->Reset();
		Rect windPos = wind->Bounds();
		wind->Move(documentMargin + 5, windPos.top);
		DoArrangeInFront(true);
	} else {
		preferences.SetPoint(CFSTR("wpos:bank"), documentMargin + 5, toolPos.bottom + margin_t + 14);
	}
}


void FPApplication::DoArrangeInFront(bool frontGuide) {
	FPDocWindow *front = FPDocWindow::frontWindow;

	if (front != NULL) {
		FPDocumentIterator	itr = FPDocWindow::docWindowList.begin(),
							itr2 = itr,
							iend = FPDocWindow::docWindowList.end();
		Rect tlmost = front->Bounds();
		FPDocWindow *wind;

		if (!frontGuide) {
			// Get the window nearest the top of the screen
			while (itr != iend) {
				wind = *(itr++);
				Rect abound = wind->Bounds();
				if (abound.top < tlmost.top)
					tlmost = abound;
			}
		}

		front->Move(tlmost.left, tlmost.top);

		while (itr2 != iend) {
			wind = *(itr2++);
			if (wind != front) {
				tlmost.left += kDocCascadeX;
				tlmost.top += kDocCascadeY;
				wind->Move(tlmost.left, tlmost.top);
				wind->SendBehind(front);
			}
		}
	}
}


#pragma mark -
#pragma mark Shared States

FPDocument* FPApplication::ActiveDocument() {
	FPDocWindow *wind = FPDocWindow::activeWindow;

	if (wind)
		return wind->document;
	else
		return NULL;
}

//-----------------------------------------------
//
//  IsDialogFront
//
//	TRUE if there is a front window
//	other than a document window.
//
bool FPApplication::IsDialogFront() {
	FPDocWindow *dwind = FPDocWindow::frontWindow;
	WindowRef	front = FrontNonFloatingWindow();
	return ( front != NULL && (dwind == NULL || front != dwind->Window()) );
}

bool FPApplication::IsFollowViewEnabled() {
	return toolPalette->eyeButton->State();
}

bool FPApplication::IsEditModeEnabled() {
	return toolPalette->editButton->State();
}

bool FPApplication::IsLoopingEnabled() {
	return toolPalette->loopButton->State();
}

bool FPApplication::IsSoloModeEnabled() {
	return toolPalette->soloButton->State();
}

bool FPApplication::IsReverseStrung() {
	return guitarPalette->IsReverseStrung();
}

bool FPApplication::IsTransformModeEnabled() {
	return toolPalette->transButton->State();
}

bool FPApplication::IsTablatureReversed() {
	if (tabReversible) {
		if (guitarPalette->IsHorizontal())
			return IsReverseStrung() ^ guitarPalette->IsLeftHanded();
		else
			return IsReverseStrung();
	}

	return false;
}

const char* FPApplication::NameOfKey(UInt16 key) {
	return scalePalette->NameOfKey(key);
}

const char* FPApplication::NameOfNote(UInt16 tone) {
	return scalePalette->NameOfNote(tone);
}


#pragma mark -
#pragma mark Music Player

void FPApplication::DoToggleTransformFlag(PartIndex part) {
	bool trans = player->ToggleTransformFlag(part);

	FPDocument *doc = ActiveDocument();
	if (doc && part != kAllChannels)
		doc->SetTransformFlag(part, trans);

	channelsPalette->UpdateTransformCheckbox(part);
}


#if QUICKTIME_SUPPORT

void FPApplication::DoToggleQuicktime(PartIndex part) {
	bool out = player->ToggleQuicktime(part);

	FPDocument *doc = ActiveDocument();
	if (doc && part != kAllChannels)
		doc->SetQuicktimeOutEnabled(part, out);

	channelsPalette->UpdateQTCheckbox(part);
}

#else

void FPApplication::DoToggleAUSynth(PartIndex part) {
	bool out = player->ToggleAUSynth(part);
	
	FPDocument *doc = ActiveDocument();
	if (doc && part != kAllChannels)
		doc->SetSynthOutEnabled(part, out);
	
	channelsPalette->UpdateAUSynthCheckbox(part);
}

#endif

void FPApplication::DoToggleMidi(PartIndex part) {
	bool cmout = player->ToggleMidi(part);

	FPDocument *doc = ActiveDocument();
	if (doc && part != kAllChannels)
		doc->SetMidiOut(part, cmout);

	channelsPalette->UpdateMidiCheckbox(part);
}


void FPApplication::DoToggleSplitMidiOut() {
	player->ToggleSplitMidiOut();
}

void FPApplication::DoSetMidiChannel(PartIndex part, UInt16 channel, bool undoable) {
	UInt16 oldChannel = player->GetMidiChannel(part);

	if (oldChannel != channel) {
		player->SetMidiChannel(part, channel);

		FPDocument *doc = ActiveDocument();
		if (doc) doc->SetMidiChannel(part, channel);

		channelsPalette->UpdateChannelPopup(part);

		if (undoable) {
			FPHistoryEvent *event = StartUndo(UN_MIDI_CHANNEL, CFSTR("Change MIDI Channel"));
			event->SetPartAgnostic();
			event->SetMergeUnderInterim(30);
			event->SaveDataBefore(CFSTR("part"), part);
			event->SaveDataBefore(CFSTR("channel"), oldChannel);
			event->SaveDataAfter(CFSTR("channel"), channel);
			event->Commit();
		}
	}

	//
	// Send Bank Select and Program message
	//
	player->SetMidiInstrument(part, player->GetInstrumentNumber(part));
}

void FPApplication::AddToInstrument(PartIndex part, SInt16 delta) {
	if (part == kCurrentChannel)
		part = player->CurrentPart();

	UInt16	span = kLastFauxGS,
			trueGM = player->GetInstrumentNumber(part),
			fauxGM = FPMidiHelper::TrueGMToFauxGM(trueGM),
			newFauxGM = ((fauxGM + span + delta - kFirstFauxGM) % span) + kFirstFauxGM,
			newTrueGM = FPMidiHelper::FauxGMToTrueGM(newFauxGM);

	DoSetInstrument(part, newTrueGM);
}

void FPApplication::DoSetInstrument(PartIndex part, UInt16 inTrueGM, bool undoable) {

	if (part == kCurrentChannel)
		part = player->CurrentPart();

	UInt16	oldTrueGM = player->GetInstrumentNumber(part);

	if (oldTrueGM != inTrueGM) {
		player->SetInstrument(part, inTrueGM);
		channelsPalette->UpdateInstrumentPopup(part);
		FPDocWindow *wind = FPDocWindow::activeWindow;
		if (wind) {
			wind->document->SetInstrument(part, inTrueGM);
			wind->document->UpdateInstrumentName(part);
			wind->UpdateDocPopup();
		}
		
		if (undoable) {
			FPHistoryEvent *event = StartUndo(UN_INSTRUMENT, CFSTR("Change Instrument"));
			event->SetPartAgnostic();
			event->SetMergeUnderInterim(30);
			event->SaveDataBefore(CFSTR("part"), part);
			event->SaveDataBefore(CFSTR("instrument"), oldTrueGM);
			event->SaveDataAfter(CFSTR("instrument"), inTrueGM);
			event->Commit();
		}
	}
}


//-----------------------------------------------
//
//	DoSetTempo, DoSetVelocity, DoSetSustain
//
//	1. Change the setting in the document
//	2. Update the setting in the player
//	3. Update the sliders in the info palette
//	4. TODO: Update the document sliders (if flagged)
//
void FPApplication::DoSetTempo(UInt16 t, bool updateDoc) {
	FPDocWindow *docWind = FPDocWindow::activeWindow;

	if (docWind) {
		docWind->document->SetTempo(t);

		if (updateDoc)
			docWind->UpdateTempoSlider();
	}
}


UInt16 FPApplication::CurrentTempo() {
	FPDocument *activeDocument = ActiveDocument();
	return activeDocument ? activeDocument->PlayTempo() : 120*4;
}


void FPApplication::DoSetVelocity(UInt16 v, SInt16 part, bool updateDoc, bool updateChan) {
	FPDocWindow *docWind = FPDocWindow::activeWindow;

	player->SetVelocity(part, v);

	if (docWind) {
		docWind->document->SetVelocity(part, v);

		if (updateDoc)
			docWind->UpdateVelocitySlider();
	}

	if (updateChan)
		channelsPalette->UpdateVelocitySlider(part);
}


void FPApplication::DoSetSustain(UInt16 s, PartIndex part, bool updateDoc, bool updateChan) {
	FPDocWindow *docWind = FPDocWindow::activeWindow;

	player->SetSustain(part, s);

	if (docWind) {
		docWind->document->SetSustain(part, s);

		if (updateDoc)
			docWind->UpdateSustainSlider();
	}

	if (updateChan)
		channelsPalette->UpdateSustainSlider(part);
}


void FPApplication::AddToRecentItems(TFile &file) {
	recentItems.Prepend(file);
	recentItems.SetPreferenceKey();
	SavePreferences();
	UpdateRecentItemsMenu();
}


void FPApplication::ClearRecentItems() {
	recentItems.Clear();
	recentItems.SetPreferenceKey();
	SavePreferences();
	UpdateRecentItemsMenu();
}


#pragma mark -
#pragma mark Navigation Services Dialogs
//
// OpenFileDialog
//
// Displays the NavGetFile modeless dialog. Safe to
// call multiple times, if the dialog is already
// displayed, it will come to the front.
//
OSStatus FPApplication::OpenFileDialog(OSType applicationSignature, SInt16 numTypes, OSType typeList[]) {
	OSStatus theErr = noErr;
	if ( openDialog == NULL ) {
		NavDialogCreationOptions	dialogOptions;
		NavTypeListHandle			openList	= NULL;

		//
		// Get the default dialog creation options
		//
		NavGetDefaultDialogCreationOptions( &dialogOptions );

		//
		// The Open dialog should ideally be non-modal
		//
		dialogOptions.modality = kWindowModalityAppModal;
//		dialogOptions.optionFlags |= kNavDontAddTranslateItems | kNavNoTypePopup;

		//
		// Standard title data for the dialog heading
		//
		dialogOptions.clientName = CFStringCreateWithPascalString( NULL, LMGetCurApName(), GetApplicationTextEncoding() );

		// borrowed from the original
//		openList = (NavTypeListHandle)GetResource('open', 128);

		//
		// The kinds of files we care about (to be replaced by a callback?)
		// Currently this only works with Creator/Type codes
		//
//		openList = (NavTypeListHandle)NewOpenHandle( applicationSignature, numTypes, typeList );

		//
		// Put up the dialog!
		//
		theErr = NavCreateGetFileDialog(
			&dialogOptions,
			NULL,
			GetOpenDialogUPP(),				// Always the non-contextual event handler
			NULL,							// preview
			GetOpenFilterUPP(),				// filter
			NULL,							// generally because the userData is NULL
			&openDialog
			);

		if ( theErr == noErr ) {
			//
			// Try to start the dialog running
			//
			theErr = NavDialogRun( openDialog );
			if ( theErr != noErr ) {
				//
				// If an error occured kill the dialog
				//
				NavDialogDispose( openDialog );
				openDialog = NULL;
			}
		}

		//
		// Dispose of the stuff we created
		//
		if (openList != NULL)
			DisposeHandle((Handle)openList);

		if ( dialogOptions.clientName != NULL )
			CFRELEASE( dialogOptions.clientName );
	}
	else {
		//
		// Bring an existing Open dialog to the front
		//
		if ( NavDialogGetWindow( openDialog ) != NULL )
			SelectWindow( NavDialogGetWindow( openDialog ));
	}

	return theErr;
}


//
// TerminateOpenFileDialog
//
// Closes the OpenFileDialog, if there is one. Call when the app is quitting
//
void FPApplication::TerminateOpenFileDialog() {
	if (openDialog)
		TerminateNavDialog(openDialog);
}


//
// DisposeOpenFileDialog
//
void FPApplication::DisposeOpenFileDialog() {
	if (openDialog) {
		NavDialogDispose(openDialog);
		openDialog = NULL;
	}
}


//
// AskReviewChangesDialog
//
// Make an application modal dialog asking to review documents.
// When used for Quit it runs NavCreateAskReviewDocumentsDialog.
// When used for Close All it runs a Standard Alert.
//
OSStatus FPApplication::AskReviewChangesDialog(NavUserAction *outUserAction, bool quitting) {
	UInt32 count = FPDocWindow::CountDirtyDocuments();

	if (count < 2) {
		*outUserAction = count ? kNavUserActionReviewDocuments : kNavUserActionDiscardDocuments;
		return noErr;
	}

	OSStatus					theErr = noErr;
	NavUserAction				userAction = kNavUserActionNone;
//	NavEventUPP					eventUPP;

	if (quitting) {
		NavDialogCreationOptions	dialogOptions;
		NavGetDefaultDialogCreationOptions( &dialogOptions );

		dialogOptions.modality = kWindowModalityAppModal;
		dialogOptions.clientName = CFStringCreateWithPascalString(NULL, LMGetCurApName(), GetApplicationTextEncoding());

//		eventUPP = GetOpenDialogUPP();

		//
		// Create the dialog
		//
		NavDialogRef dialog = NULL;
		theErr = NavCreateAskReviewDocumentsDialog(
					&dialogOptions,					// the dialog description
					count,							// number of documents
					NULL,							// no event handler for modal
					NULL,							// NULL userData
					&dialog );						// save the dialog pointer

		if ( theErr == noErr ) {
			theErr = NavDialogRun( dialog );

			if (theErr == noErr)
				userAction = NavDialogGetUserAction( dialog );

			NavDialogDispose( dialog );
		}

		if (dialogOptions.clientName != NULL)
			CFRELEASE( dialogOptions.clientName );
	}
	else {

		TString headStr;
		headStr.SetWithFormatLocalized(CFSTR("You have %d FretPet documents with unsaved changes. Do you want to review these changes before closing all?"), count);

		DialogItemIndex answer = RunPrompt(
			headStr.GetCFStringRef(),
			CFSTR("If you don't review your documents, all your changes will be lost."),
			CFSTR("Review Changes..."),
			CFSTR("Cancel"),
			CFSTR("Discard Changes")
			);

		switch(answer) {
			case kAlertStdAlertOKButton:
				userAction = kNavUserActionReviewDocuments;
				break;

			case kAlertStdAlertCancelButton:
				userAction = kNavUserActionCancel;
				break;

			case kAlertStdAlertOtherButton:
				userAction = kNavUserActionDiscardDocuments;
				break;
		}
	}

	//
	// Output the user action
	//
	if ( outUserAction != NULL )
		*outUserAction = userAction;

	return theErr;
}


