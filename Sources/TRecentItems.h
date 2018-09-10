/*
 *  TRecentItems.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TRECENTITEMS_H
#define TRECENTITEMS_H

class TFile;

class TRecentItems {
	private:
		CFMutableArrayRef		recentItemsArray;

	public:
		TRecentItems();
		~TRecentItems();

		CFIndex		GetCount();
		AliasHandle	GetAlias(CFIndex item);
		OSStatus	GetFSRef(UInt16 item, FSRef *outRef);
		bool		ItemStillExists(UInt16 item);
		void		SetPreferenceKey();
		void		Prepend(TFile &file);
		void		PopulateMenu(MenuRef menu);
		void		Clear();
};

#endif
