/*
 *  FPToolbar.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPTOOLBAR_H
#define FPTOOLBAR_H

#include "FPPalette.h"
#include "TPictureControl.h"

class FPToolbar : public FPPalette {
	public:
		TPictureControl	*triadButton, *scaleButton;
		TPictureControl	*transButton;
		TPictureControl	*clearButton, *plusButton /* , *undoButton */;
		TPictureControl	*hearButton, *stopButton, *playButton;
		TPictureControl	*loopButton, *eyeButton, *editButton;
		TPictureControl	*soloButton, *romanButton;
		TPictureControl	*fifthButton, *halfButton;
		TPictureControl	*rootButton, *keyButton;

		FPToolbar();
		~FPToolbar();

		void	Init();
		void	SavePreferences();

		void	DrawKeyOrder();
		void	DrawRootType();
		bool	IsAnchored();
		void	ToggleAnchored() { SetAnchored(!IsAnchored()); }
		void	SetAnchored(bool anchor);

		void	UpdateButtonStates();
		void	UpdatePlayTools();

		const CFStringRef	PreferenceKey()	{ return CFSTR("toolbar"); }
		bool				LoadPreferences(bool showing=true);
};

//
// Toolbar images
//
enum {
	kPictTriad1 = 1000, kPictTriad2,
	kPictScale1, kPictScale2,
	kPictTrans1, kPictTrans2, kPictTrans3, kPictTrans4, kPictTrans5,
	kPictClearNew1, kPictClearNew2,
	kPictClearOld1, kPictClearOld2,
	kPictUndo1, kPictUndo2,
	kPictPlus1, kPictPlus2,
	kPictHear1, kPictHear2,
	kPictStop1, kPictStop2,
	kPictPlay1, kPictPlay2, kPictPlay3, kPictPlay4, kPictPlay5,
	kPictLoop1, kPictLoop2, kPictLoop3, kPictLoop4,
	kPictEye1, kPictEye2, kPictEye3, kPictEye4,
	kPictEdit1, kPictEdit2, kPictEdit3, kPictEdit4,
	kPictSolo1, kPictSolo2, kPictSolo3, kPictSolo4,
	kPictRoman1, kPictRoman2, kPictRoman3, kPictRoman4,
	kPictLock1, kPictLock2, kPictLock3, kPictLock4,
	kPictFifth1, kPictFifth2, kPictFifth3,
	kPictHalf1, kPictHalf2, kPictHalf3,
	kPictNewRoman1, kPictNewRoman2, kPictNewRoman3,

	kPictRoot1 = 1300, kPictRoot2, kPictRoot3,
	kPictKey1, kPictKey2, kPictKey3
};

#endif
