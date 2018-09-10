/*!
 *  @file TRecentItems.cpp
 *
 *	@brief A Recent Items menu.
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#include "TRecentItems.h"
#include "FPPreferences.h"
#include "FPUtilities.h"
#include "TFile.h"
#include "TString.h"

TRecentItems::TRecentItems() {
	if (preferences.IsSet(kPrefRecentItems)) {
		CFArrayRef	recArray = preferences.GetArray(kPrefRecentItems);
		recentItemsArray = CFArrayCreateMutableCopy(kCFAllocatorDefault, 0, recArray);
		CFRELEASE(recArray);
	}
	else
		recentItemsArray = CFArrayCreateMutable( kCFAllocatorDefault, 0, &kCFTypeArrayCallBacks );
}


TRecentItems::~TRecentItems() {
	CFRELEASE(recentItemsArray);
}


CFIndex TRecentItems::GetCount() {
	return CFArrayGetCount(recentItemsArray);
}


AliasHandle TRecentItems::GetAlias(CFIndex item) {
	AliasHandle talias = NULL;

	if (item < GetCount()) {
		CFDataRef	data = (CFDataRef)CFArrayGetValueAtIndex(recentItemsArray, item);
		CFRETAIN(data);
		talias = CFDataToAlias(data);
		CFRELEASE(data);
	}

	return talias;
}


OSStatus TRecentItems::GetFSRef(UInt16 item, FSRef *outRef) {
	OSStatus err = fnfErr;

	AliasHandle	alias = GetAlias(item);
	if (alias != NULL) {
		Boolean	wasChanged;
		err = FSResolveAliasWithMountFlags(NULL, alias, outRef, &wasChanged, kResolveAliasFileNoUI);
		DisposeHandle((Handle)alias);
	}

	return err;
}


bool TRecentItems::ItemStillExists(UInt16 item) {
	FSRef	ref;
	return (GetFSRef(item, &ref) == noErr);
}


void TRecentItems::SetPreferenceKey() {
	preferences.SetValue(kPrefRecentItems, recentItemsArray);
}


void TRecentItems::Prepend(TFile &file) {
	AliasHandle alias = file.GetAliasHandle();
	if (alias) {
		CFIndex rcount = GetCount();

		//
		// Delete all existing items that match this alias
		//
		if (rcount > 0) {
			for (CFIndex i=0; i<rcount; i++) {
				FSRef ref;
				if (noErr == GetFSRef(i, &ref) && file == ref) {
					CFArrayRemoveValueAtIndex(recentItemsArray, i);
					i--;
					rcount--;
				}
			}
		}

		CFDataRef aliasData = AliasToCFData(alias);
		CFArrayInsertValueAtIndex(recentItemsArray, 0, aliasData);
		CFRELEASE(aliasData);
		DisposeHandle((Handle)alias);
	}
}


void TRecentItems::PopulateMenu(MenuRef menu) {
	UInt16 size = CountMenuItems(menu);
	if (size > 0)
		DeleteMenuItems(menu, 1, size);

	CFIndex rcount = GetCount();

	if (rcount > 0) {
		int usable = 0;
		for (CFIndex i=0; i<rcount; i++) {
			AliasHandle	alias = GetAlias(i);

			if (alias != NULL) {
				OSStatus	err = noErr;
				FSRef		ref;
				Boolean	wasChanged;
				if (noErr == FSResolveAliasWithMountFlags(NULL, alias, &ref, &wasChanged, kResolveAliasFileNoUI)) {
					TFile tempFile(ref);
					err = AppendMenuItemTextWithCFString(menu, tempFile.DisplayName(), 0, kFPCommandRecentItem, NULL);
				} else {
					HFSUniStr255 targetName;
					err = FSCopyAliasInfo(alias, &targetName, NULL, NULL, NULL, NULL);
					TString itemName(targetName);
					err = AppendMenuItemTextWithCFString(menu, itemName.GetCFStringRef(), kMenuItemAttrDisabled, kFPCommandRecentItem, NULL);
				}

				DisposeHandle((Handle)alias);

				if (err == noErr)
					usable++;
			}
		}

		if (usable > 0) {
			TString clearMenu;
			clearMenu.SetLocalized(CFSTR("Clear Menu"));
			(void)AppendMenuItemTextWithCFString(menu, CFSTR("-"), 0, 0, NULL);
			(void)AppendMenuItemTextWithCFString(menu, clearMenu.GetCFStringRef(), 0, kFPCommandClearRecent, NULL);
		}
	}
}


void TRecentItems::Clear() {
	CFArrayRemoveAllValues(recentItemsArray);
}

