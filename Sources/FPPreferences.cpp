/*!
	@file FPPreferences.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPApplication.h"
#include "TControls.h"
#include "FPPreferences.h"
#include "FPUtilities.h"
#include "FPWindow.h"
#include "FPDocWindow.h"

class FPPrefsWindow;
FPPrefsWindow *prefsWindow = NULL;

//
// The Preferences Window
// Subclassing FPWindow creates the hooks for 
//
class FPPrefsWindow : public FPWindow {
	private:
#if !APPSTORE_SUPPORT
		TCheckbox	*checkAutoUpdate;
#endif
		TCheckbox	*checkReversible, *checkSplash, *checkFinger, *checkModal;
		TRadioGroup	*groupStartup;

	public:
		FPPrefsWindow() : FPWindow(CFSTR("Preferences")) {
			const EventTypeSpec	prefEvents[] = { { kEventClassCommand, kEventCommandProcess } };
			RegisterForEvents( GetEventTypeCount( prefEvents ), prefEvents );

			WindowRef wind = Window();

			checkReversible = new TCheckbox(wind, kCommandPrefTab);
			checkReversible->Check(preferences.GetBoolean(kPrefTabReversible, TRUE));

			checkFinger = new TCheckbox(wind, kCommandPrefFingerWarning);
			checkFinger->Check(preferences.GetBoolean(kPrefFingerWarning, TRUE));

			checkModal = new TCheckbox(wind, kCommandPrefModalDialogs);
			checkModal->Check(preferences.GetBoolean(kPrefModalDialogs, TRUE));

			checkSplash = new TCheckbox(wind, kCommandPrefSplash);
			checkSplash->Check(preferences.GetBoolean(kPrefSplash, TRUE));

			groupStartup = new TRadioGroup(wind, kCommandPrefAction);
			groupStartup->SetValue(preferences.GetInteger(kPrefStartupAction, STARTUP_NEW));

#if APPSTORE_SUPPORT
			TControl *autoGroup = new TControl(wind, 'autu', 100);
			autoGroup->Hide();
			this->SetContentSize(this->Width(), this->Height() - autoGroup->Height());
			delete autoGroup;
#else
			checkAutoUpdate = new TCheckbox(wind, kCommandPrefAutoCheck);
			checkAutoUpdate->Check(preferences.GetBoolean(kPrefAutoUpdate, TRUE));
#endif
			Cascade(30, 50);
		}

		virtual ~FPPrefsWindow() {
			prefsWindow = NULL;
			delete checkReversible;
			delete checkFinger;
			delete checkModal;
			delete checkSplash;
#if !APPSTORE_SUPPORT
			delete checkAutoUpdate;
#endif
			delete groupStartup;
		}

		const CFStringRef PreferenceKey()	{ return CFSTR("pref"); }

		void Close() {
			SavePreferences();
			FPWindow::Close();
		}

		//
		// UpdateCommandStatus
		//
		// Turn off most commands for the Preference dialog
		//
		bool UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
			bool enable = false;
			bool handled = true;
			switch(cid) {
				case kFPCommandAbout:
					handled = false;
					break;

				case kHICommandNew:
				case kHICommandQuit:
				case kHICommandClose:
					enable = !fretpet->isQuitting;
					break;

				case kHICommandSelectWindow:
					enable = true;
					handled = false;
					break;

				default:
					enable = false;
					break;
			}

			if (menu) {
				if (enable)
					EnableMenuItem(menu, index);
				else
					DisableMenuItem(menu, index);
			}

			return handled;
		}


		//
		// HandleCommand
		// Set all preferences based on the button states (lazy)
		// or
		// Set the indicated preference to the appropriate state
		//
		// Finally, synchronize the preferences
		//
		bool HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
			switch(cid) {
				case kCommandPrefTab: {
					bool state = checkReversible->IsChecked();
					preferences.SetBoolean(kPrefTabReversible, state);
					fretpet->tabReversible = state;
					preferences.Synchronize();
					FPDocWindow::UpdateAll();
					break;
				}

				case kCommandPrefFingerWarning:
					preferences.SetBoolean(kPrefFingerWarning, checkFinger->IsChecked());
					preferences.Synchronize();
					break;

				case kCommandPrefModalDialogs:
					preferences.SetBoolean(kPrefModalDialogs, checkModal->IsChecked());
					preferences.Synchronize();
					break;

				case kCommandPrefSplash:
					preferences.SetBoolean(kPrefSplash, checkSplash->IsChecked());
					preferences.Synchronize();
					break;

				case kCommandPrefAction:
					preferences.SetInteger(kPrefStartupAction, groupStartup->Value());
					preferences.Synchronize();
					break;

#if !APPSTORE_SUPPORT
				case kCommandPrefAutoCheck: {
					Boolean b = checkAutoUpdate->IsChecked();
					preferences.SetBoolean(kPrefAutoUpdate, b);
					preferences.Synchronize();
					break;
				}
#endif

				default:
					return false;
			}

			return true;
		}
};

#pragma mark -
//-----------------------------------------------
//
// FPPreferences				* CONSTRUCTOR *
//
FPPreferences::FPPreferences() {
}

//-----------------------------------------------
//
// GetBoolean
//
bool FPPreferences::GetBoolean(const CFStringRef key, bool dval) const {
	Boolean isValid;
	bool result = CFPreferencesGetAppBooleanValue(key, kCFPreferencesCurrentApplication, &isValid);

	if (!isValid)
		result = dval;

	return result;
}

//-----------------------------------------------
//
// GetInteger
//
SInt32 FPPreferences::GetInteger(const CFStringRef key, SInt32 dval) const {
	SInt32	result = dval;
	Boolean isValid;

	CFTypeRef	someRef = GetCopy(key);

	if (someRef != NULL) {
		if(CFGetTypeID(someRef) == CFNumberGetTypeID())
			isValid = CFNumberGetValue((CFNumberRef)someRef, kCFNumberSInt32Type, &result);

		CFRELEASE(someRef);
	}

	return result;
}

//-----------------------------------------------
//
// GetPoint
//
Point FPPreferences::GetPoint(const CFStringRef key) const {
	CFMutableDictionaryRef dictRef = GetDictionary(key);

	CFNumberRef numX = (CFNumberRef)CFDictionaryGetValue(dictRef, CFSTR("X"));
	CFRETAIN(numX);

	CFNumberRef numY = (CFNumberRef)CFDictionaryGetValue(dictRef, CFSTR("Y"));
	CFRETAIN(numY);

	SInt32 x, y;
	(void)CFNumberGetValue(numX, kCFNumberSInt32Type, &x);
	(void)CFNumberGetValue(numY, kCFNumberSInt32Type, &y);
	CFRELEASE(dictRef);

	CFRELEASE(numX);
	CFRELEASE(numY);

	Point thePoint = { y, x };
	return thePoint;
}

//-----------------------------------------------
//
// GetDictionary
//
CFMutableDictionaryRef FPPreferences::GetDictionary(const CFStringRef key) const {
	return (CFMutableDictionaryRef) GetCopy(key);
}

//-----------------------------------------------
//
// GetCString
//
char* FPPreferences::GetCString(const CFStringRef key) const {
	CFStringRef	stringRef = (CFStringRef)GetCopy(key);
	char *string = NULL;
	if (stringRef) {
		string = CFStringToCString(stringRef);
		CFRELEASE(stringRef);
	} else {
		string = new char[1];
		string[0] = '\0';
	}

	return string;
}

//-----------------------------------------------
//
// GetAlias
//
AliasHandle FPPreferences::GetAlias(const CFStringRef key) const {
	CFDataRef	aliasData = (CFDataRef)GetCopy(key);
    CFIndex		dataSize = CFDataGetLength(aliasData);
    AliasHandle	aliasHdl = (AliasHandle) NewHandle(dataSize);
    require(NULL != aliasHdl, CantAllocateAlias);

	CFDataGetBytes(aliasData, CFRangeMake(0, dataSize), (UInt8*)*aliasHdl);

CantAllocateAlias:
	return aliasHdl;
}

//-----------------------------------------------
//
// GetFSRef
//
FSRef* FPPreferences::GetFSRef(const CFStringRef key) const {
	FSRef ref, *refPtr = NULL;

	AliasHandle	aliasHdl = GetAlias(key);

	if (aliasHdl != NULL) {
		Boolean	wasChanged;
		if (noErr == FSResolveAliasWithMountFlags(NULL, aliasHdl, &ref, &wasChanged, kResolveAliasFileNoUI)) {
			refPtr = new FSRef;
			*refPtr = ref;
		}
	}

	return refPtr;
}

//-----------------------------------------------
//
// GetCopy
//
CFTypeRef FPPreferences::GetCopy(const CFStringRef key) const {
	return CFPreferencesCopyAppValue(key, kCFPreferencesCurrentApplication);
}

#pragma mark -
//-----------------------------------------------
//
// SetValue
//
void FPPreferences::SetValue(const CFStringRef key, const CFTypeRef value) {
	CFPreferencesSetAppValue(key, value, kCFPreferencesCurrentApplication);
}

//-----------------------------------------------
//
// SetCString
//
void FPPreferences::SetCString(const CFStringRef key, char *string) {
	CFStringRef stringRef = CFStringCreateWithCString(kCFAllocatorDefault, string, kCFStringEncodingMacRoman);
	SetValue(key, stringRef);
	CFRELEASE(stringRef);
}

//-----------------------------------------------
//
// SetBoolean
//
void FPPreferences::SetBoolean(const CFStringRef key, bool state) {
	SetValue(key, state ? kCFBooleanTrue : kCFBooleanFalse);
}

//-----------------------------------------------
//
// SetInteger
//
void FPPreferences::SetInteger(const CFStringRef key, SInt32 number) {
	CFNumberRef numRef = CFNumberCreate(NULL, kCFNumberSInt32Type, &number);
	SetValue(key, numRef);
	CFRELEASE(numRef);
}

//-----------------------------------------------
//
// SetFloat
//
void FPPreferences::SetFloat(const CFStringRef key, float number) {
	CFNumberRef numRef = CFNumberCreate(NULL, kCFNumberFloatType, &number);
	SetValue(key, numRef);
	CFRELEASE(numRef);
}

//-----------------------------------------------
//
// SetPoint
//
//	This creates a dictionary having X and Y keys
//	set to two SInt32 numeric values. This demonstrates
//	how to create a dictionary from existing key-value
//	pairs stored in arrays. But what if you want to
//	
//
void FPPreferences::SetPoint(const CFStringRef key, SInt32 x, SInt32 y) {
	CFNumberRef	numX = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &x);
	CFNumberRef	numY = CFNumberCreate(kCFAllocatorDefault, kCFNumberSInt32Type, &y);
	CFStringRef keyList[] = { CFSTR("X"), CFSTR("Y") };
	CFTypeRef	valList[] = { numX, numY };

	SetDictionary(key, keyList, valList, 2);

	CFRELEASE(numX);
	CFRELEASE(numY);
}

//-----------------------------------------------
//
// SetDictionary(key, keyList, valList, numValues)
//
void FPPreferences::SetDictionary(const CFStringRef key, CFStringRef keyList[], CFTypeRef valList[], CFIndex numValues) {
	CFDictionaryRef dictRef = CFDictionaryCreate(
								kCFAllocatorDefault,
								(const void**)keyList,
								(const void**)valList,
								numValues,
								&kCFTypeDictionaryKeyCallBacks,
								&kCFTypeDictionaryValueCallBacks);
	SetValue(key, dictRef);
	CFRELEASE(dictRef);
}

//-----------------------------------------------
//
// SetFSRef(key, FSRef*)
//
void FPPreferences::SetFSRef(const CFStringRef key, FSRef *fileRef) {
	AliasHandle	alias;
	OSErr err = FSNewAlias(NULL, fileRef, &alias);
    require_noerr(err, CantMakeAlias);

	SetAlias(key, alias);
	DisposeHandle((Handle)alias);

CantMakeAlias:
	return;
}

//-----------------------------------------------
//
// SetAlias(key, alias)
//
void FPPreferences::SetAlias(const CFStringRef key, AliasHandle alias) {
	CFDataRef aliasData = CFDataCreate(kCFAllocatorDefault, (UInt8*)*alias, GetHandleSize((Handle)alias));
    require(NULL != aliasData, CantMakeCFData);

	SetValue(key, aliasData);
	CFRELEASE(aliasData);

CantMakeCFData:
	return;
}

//-----------------------------------------------
//
// Synchronize
//
bool FPPreferences::Synchronize() {
	return CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
}

//-----------------------------------------------
//
// IsSet
//
bool FPPreferences::IsSet(const CFStringRef key) {
	bool		result = false;

	CFTypeRef theKey = GetCopy(key);
	if (theKey) {
		result = true;
		CFRELEASE(theKey);
	}

	return result;
}

void FPPreferences::ShowPreferencesDialog() {
	if (prefsWindow)
		prefsWindow->Select();
	else {
		prefsWindow = new FPPrefsWindow();
		prefsWindow->LoadPreferences();
	}
}

void FPPreferences::ClosePreferencesDialog() {
	if (prefsWindow)
		prefsWindow->Close();
}

/*

CFStringRef				s;
CFNumberRef				n;
CFMutableDictionaryRef	dict, dict2;
SInt32					test = 1234;

n = CFNumberCreate(NULL, kCFNumberSInt32Type, &test);					// The original bit
s = CFStringCreateWithFormat(NULL, NULL, CFSTR("%d"), test);			// A suggested alternative

dict1 = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
dict2 = CFDictionaryCreateMutable(NULL, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
CFDictionarySetValue(dict2, n, dict1);

CFPreferencesSetAppValue(CFSTR("mypref"), dict2, kCFPreferencesCurrentApplication);
CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);


void SetLastSavedFile(FSRef * ref) {
	AliasHandle	alias;
	FSNewAlias(NULL, ref, &alias);
	CFDataRef	aliasData = CFDataCreate(NULL, (UInt8*)*alias, GetHandleSize(alias));
	DisposeHandle((Handle)alias);
	CFPreferencesSetAppValue(pathKey, aliasData, kCFPreferencesCurrentApplication);
	CFPreferencesAppSynchronize(kCFPreferencesCurrentApplication);
	CFRELEASE(aliasData);
}


*/
