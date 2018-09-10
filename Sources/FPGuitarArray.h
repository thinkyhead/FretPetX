/*!
 *  @file FPGuitarArray.h
 *
 *	@brief A storage container for all the loaded guitar bundles.
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#ifndef FPGUITARARRAY_H
#define FPGUITARARRAY_H

class FPGuitar;
#include "TDictionary.h"

class FPGuitarArray {
private:
	UInt16				length;				//!< The number of guitars
	CFMutableArrayRef	bundleArray;		//!< An array of all the bundles
	CFMutableArrayRef	namesArray;			//!< An array of guitar names from bundle info
	CFMutableArrayRef	groupsArray;		//!< An array of menu groups from bundle info
	SInt16				guitarIndex;		//!< The index of the current guitar
	FPGuitar			*selectedGuitar;	//!< The currently selected guitar itself
	TDictionary			guitarDict;			//!< The new storage container, grouped by MenuGroup, contains bundles keyed by name
	int					menuindex[100];		//!< An array to translate indexes around
	int					realindex[100];		//!< Another array to translate indexes

public:
	FPGuitarArray();
	~FPGuitarArray();

	//! Append more guitars from any additional folder.
	void AddBundlesFromFolder(CFURLRef folder);

	//! Use the stored info to populate the given menu.
	void PopulateMenu(MenuRef menu, MenuCommand command);

	//! Synthetic index of the given guitar name.
	SInt16 IndexOfGuitar(CFStringRef imageName);

	//! Guitar group at the given synthetic index.
	CFStringRef GuitarGroupAtIndex(UInt16 item);

	//! Guitar name at the given synthetic index.
	CFStringRef GuitarNameAtIndex(UInt16 item);

	//! The default guitar is the fallback for missing graphics.
	FPGuitar* GetDefaultGuitar();

	// TODO: Let guitars inherit from others, so each guitar defines its own fallback

	//! The default guitar name is more than the first guitar shown
	inline CFStringRef DefaultGuitarName() { return CFSTR("Big Acoustic"); }

	//! The default guitar "index" is perhaps useful?
	inline SInt16 DefaultGuitarIndex() { return IndexOfGuitar(DefaultGuitarName()); }

	//! Select the active guitar by "index"
	void SetSelectedGuitar(SInt16 index);

	//! Set the active guitar to the default
	inline void SelectDefaultGuitar() { SetSelectedGuitar(DefaultGuitarIndex()); }

	//! Get the current guitar "index"
	inline SInt16 SelectedGuitarIndex() { return guitarIndex; }

	//! The selected guitar's index in the display menu
	inline SInt16 SelectedGuitarMenuIndex() { return menuindex[guitarIndex]; }

	//! Some menu item's corresponding real index
	inline SInt16 GuitarIndexForMenuIndex(MenuItemIndex i) { return realindex[i-1]; }

	//! The name of the guitar at the given index
	inline CFStringRef SelectedGuitarName() { return GuitarNameAtIndex(guitarIndex); }

	//! The selected guitar (pointer)
	inline FPGuitar* SelectedGuitar() { return selectedGuitar; }

	//! Load the old state
	void LoadPreferences();

	//! Save the current state
	void SavePreferences();

	//! The total number of items
	inline UInt16 Size() const { return length; }

	//! A comparison handler for the array sort function
	static CFComparisonResult CompareBundles(const void *val1, const void *val2, void *context);
};

#endif