/*!
	@file FPGuitarArray.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPGuitarArray.h"
#include "FPGuitar.h"

#include "FPPreferences.h"
#include "FPMacros.h"
#include "FPUtilities.h"
#include "TString.h"

#include "TFile.h"
#include "FPApplication.h"

#include "TDictionary.h"

//-----------------------------------------------
//
//	FPGuitarArray constructor
//
//	The FPGuitarArray class represents all the guitar bundles
//	FretPet knows about when it first starts up.
//
//	One of these guitar bundles (Hollywood) is the default bundle
//	in that it contains the bracket, the dots, the cursor, etc.
//	for those bundles that don't provide their own.
//
FPGuitarArray::FPGuitarArray() {
	selectedGuitar = NULL;
	guitarIndex = -1;

	// Make a mutable array
	bundleArray = CFArrayCreateMutable(kCFAllocatorDefault, 100, &kCFTypeArrayCallBacks);

	// The "Resources" folder within the application bundle
	CFURLRef	resourcesURL = CFBundleCopyResourcesDirectoryURL(CFBundleGetMainBundle());

	// The "Guitars" folder within "Resources"
	CFURLRef	guitarsFolder = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, resourcesURL, CFSTR("Guitars/"), true);

	AddBundlesFromFolder(guitarsFolder);

	CFRELEASE(guitarsFolder);
	CFRELEASE(resourcesURL);

/*
	// This is an alternative to using a TFile object...

	FSRef folder;
	OSErr err = FSFindFolder(kUserDomain, kApplicationSupportFolderType, kCreateFolder, &folder);
	if (err == noErr) {
		// The "Application Support" folder in ~/Library
		CFURLRef appSupportFolder = CFURLCreateFromFSRef(kCFAllocatorDefault, &folder);

		// The "Guitars" folder within "FretPet"
		CFURLRef guitarsFolder2 = CFURLCreateCopyAppendingPathComponent(kCFAllocatorDefault, appSupportFolder, CFSTR("FretPet/"), true);

		AddBundlesFromFolder(guitarsFolder2);
	}
*/

	// ~/Library/Application Support/FretPet
	TFile	*appSupportObj = new TFile(fretpet->SupportFolderURL());

	if (appSupportObj) {
		if (appSupportObj->Exists())
			AddBundlesFromFolder(appSupportObj->URLPath());

		delete appSupportObj;
	}

	// Get all the names into an array
	namesArray = CFArrayCreateMutable(kCFAllocatorDefault, length, &kCFTypeArrayCallBacks);
	groupsArray = CFArrayCreateMutable(kCFAllocatorDefault, length, &kCFTypeArrayCallBacks);
	for (int i=0; i<length; i++) {
		CFBundleRef b = (CFBundleRef)CFArrayGetValueAtIndex(bundleArray, i);
		TDictionary dict1(CFBundleGetInfoDictionary(b));
		CFStringRef s = dict1.GetString(kGuitarName);
		CFArrayAppendValue(namesArray, s);
		CFStringRef g = dict1.GetString(kGuitarMenuGroup);
		CFArrayAppendValue(groupsArray, g);
	}

	// Now it's ready to handle the saved guitar name
	LoadPreferences();
}


FPGuitarArray::~FPGuitarArray() {
	CFRELEASE(bundleArray);
	CFRELEASE(namesArray);
	CFRELEASE(groupsArray);

	if (selectedGuitar != NULL)
		delete selectedGuitar;
}


void FPGuitarArray::AddBundlesFromFolder(CFURLRef folder) {
	CFArrayRef bundles = CFBundleCreateBundlesFromDirectory(kCFAllocatorDefault, folder, CFSTR("guitar"));
	CFArrayAppendArray(bundleArray, bundles, CFRangeMake(0, CFArrayGetCount(bundles)));
	length = CFArrayGetCount(bundleArray);
	CFArraySortValues(bundleArray, CFRangeMake(0, length), CompareBundles, this);
}


CFComparisonResult FPGuitarArray::CompareBundles(const void *val1, const void *val2, void *context) {
	TDictionary dict1(CFBundleGetInfoDictionary((CFBundleRef)val1));
	TDictionary dict2(CFBundleGetInfoDictionary((CFBundleRef)val2));

	// Items in different groups are always sorted apart
	CFStringRef group1 = dict1.GetString(kGuitarMenuGroup);
	CFStringRef group2 = dict2.GetString(kGuitarMenuGroup);

	// Only items in the same group sort by name
	CFComparisonResult compare = CFStringCompare(group1, group2, kCFCompareCaseInsensitive|kCFCompareLocalized);
	if (compare == kCFCompareEqualTo) {
		CFStringRef name1 = dict1.GetString(kGuitarName);
		CFStringRef name2 = dict2.GetString(kGuitarName);
		compare = CFStringCompare(name1, name2, kCFCompareCaseInsensitive|kCFCompareLocalized);
	}

	return compare;
}



void FPGuitarArray::LoadPreferences() {
	CFStringRef storedName;
	bool		gotImage = false;

	if (preferences.IsSet(kPrefGuitarSkin)) {
		storedName = (CFStringRef)preferences.GetCopy(kPrefGuitarSkin);
		SetSelectedGuitar(IndexOfGuitar(storedName));
		CFRELEASE(storedName);
		gotImage = (guitarIndex >= 0);
	}

	if (!gotImage) {
		SelectDefaultGuitar();
		SavePreferences();
	}
}


void FPGuitarArray::SavePreferences() {
	CFStringRef shortName;
	if (guitarIndex >= 0 && guitarIndex < Size())
		shortName = SelectedGuitarName();
	else
		shortName = CFStringCreateCopy(kCFAllocatorDefault, DefaultGuitarName());

	preferences.SetValue(kPrefGuitarSkin, shortName);
	preferences.Synchronize();
	CFRELEASE(shortName);
}


void FPGuitarArray::PopulateMenu(MenuRef menu, MenuCommand command) {
	UInt16 size = CountMenuItems(menu);
	if (size > 0)
		DeleteMenuItems(menu, 1, size);

	unsigned y = 0;
	if (Size() > 0) {
		CFStringRef prevGroup = CFSTR("");

		for (UInt16 i=0; i<Size(); i++) {
			if (CFStringCompare(prevGroup, GuitarGroupAtIndex(i), 0) != kCFCompareEqualTo) {
				if (i > 0) {
					y++;
					(void)AppendMenuItemTextWithCFString(menu, CFSTR("-"), kMenuItemAttrSeparator, 0, NULL);
				}

				prevGroup = GuitarGroupAtIndex(i);

				y++;
				(void)AppendMenuItemTextWithCFString(menu, prevGroup, kMenuItemAttrSectionHeader, 0, NULL);
			}

			menuindex[i] = i + y + 1;
			realindex[i + y] = i;

			(void)AppendMenuItemTextWithCFString(menu, GuitarNameAtIndex(i), 0, command, NULL);
		}
	}
}


SInt16 FPGuitarArray::IndexOfGuitar(CFStringRef guitarName) {
	return CFArrayGetFirstIndexOfValue(namesArray, CFRangeMake(0, length), guitarName);
}


CFStringRef FPGuitarArray::GuitarGroupAtIndex(UInt16 index) {
	return (CFStringRef)CFArrayGetValueAtIndex(groupsArray, index);
}

CFStringRef FPGuitarArray::GuitarNameAtIndex(UInt16 index) {
	return (CFStringRef)CFArrayGetValueAtIndex(namesArray, index);
}


void FPGuitarArray::SetSelectedGuitar(SInt16 index) {
	if (guitarIndex != index) {
		guitarIndex = index;

		if (selectedGuitar != NULL) {
			delete selectedGuitar;
			selectedGuitar = NULL;
		}

		if (index >= 0)
			selectedGuitar = new FPGuitar((CFBundleRef)CFArrayGetValueAtIndex(bundleArray, index));
	}
}


FPGuitar* FPGuitarArray::GetDefaultGuitar() {
	static FPGuitar *defaultGuitar = new FPGuitar((CFBundleRef)CFArrayGetValueAtIndex(bundleArray, DefaultGuitarIndex()));
	return defaultGuitar;
}
