/*!
 *  @file FPCustomTuning.h
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#ifndef FPCUSTOMTUNINGS_H
#define FPCUSTOMTUNINGS_H

#include "TControls.h"
#include "FPTuningInfo.h"

class FPWindow;


#pragma mark -
//-----------------------------------------------
//
//	FPCustomTuningControl
//
class FPCustomTuningControl : public TCustomControl {
	private:
		UInt16	origString;

	public:
		FPCustomTuningControl(WindowRef wind);
		~FPCustomTuningControl();

		bool			Draw(const Rect &bounds);
		ControlPartCode	HitTest( Point where )		{ return kControlIndicatorPart; }
		bool			Track(MouseTrackingResult eventType, Point where);
		void			GetStringToneFromPoint(Point where, UInt16 *string, SInt16 *tone, bool useOrig);
};


#pragma mark -
//-----------------------------------------------
//
//	FPCustomTuningList					CLASS
//
//	This class encapsulates custom tunings as an
//	array of FPTuningInfo structures.
//
//	At instantiation it loads custom tunings
//	into an array of FPTuningInfo structs
//	from a <dict> in the the preference file.
//
//	Its synchronize method writes the entire set
//	of custom tunings out to a file.
//

#include <vector>

struct FPMenuInfo {
	MenuRef	menu;
	UInt16	initSize;
};
typedef struct FPMenuInfo FPMenuInfo;

class FPCustomTuningList : public std::vector<FPTuningInfo*> {
	private:
		UInt16					editTuning;
		std::vector<FPMenuInfo>	menuList;

	public:
						FPCustomTuningList();
						~FPCustomTuningList();

		UInt16			CurrentTuningIndex()			{ return editTuning; }
		FPTuningInfo*	CurrentTuning();
		FPTuningInfo*	SelectCustomTuning(int index);

		FPTuningInfo*	NewCustomTuning(CFStringRef newName);
		void			RenameCurrentTuning(CFStringRef newName);
		void			DeleteCurrentTuning();
		void			SetTuningTone(UInt16 string, SInt16 tone);
		inline SInt16	GetTuningTone(UInt16 string)	{ return !empty() ? CurrentTuning()->tone[string] : builtinTuning[0].tone[string]; }
		void			PlayStrum();

		void			ShowCustomTuningDialog();
		void			Synchronize();
		void			AttachMenu(MenuRef menu);
		void			UpdateMenus(MenuRef menu=NULL);
		MenuItemIndex	FindMatchingMenuItem(MenuRef inMenu, const FPTuningInfo &t);
};

#pragma mark -

extern FPCustomTuningList customTuningList;

#define kPrefCustomTunings		CFSTR("customTunings")
#define kCommandCustomNew		'CusN'
#define kCommandCustomDelete	'CusD'
#define kCommandCustomRename	'CusR'
#define kCommandCustomStrum		'CusS'
#define kCommandCustomPopup		'CusP'

#define kCommandCustomNewYes	'CuNY'
#define kCommandCustomDeleteYes	'CuDY'
#define kCommandCustomRenameYes	'CuRY'

#endif
