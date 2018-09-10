/*
 *  FPCloneSheet.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPCLONESHEET_H
#define FPCLONESHEET_H

#include "FPWindow.h"

class FPDocWindow;
class TNumericTextField;
class TLittleArrows;
class TCheckbox;
class TPopupMenu;
class TControl;

/*! Sheet alert for the Clone operation
 */
class FPCloneSheet : public FPWindow {
private:
	FPDocWindow			*hostWindow;
	TNumericTextField	*cloneText;
	TLittleArrows		*cloneArrows;
	TCheckbox			*transCheck;
	TPopupMenu			*partsPop, *transposePop, *harmonizePop;
	TControl			*transposeLabel, *harmonizeLabel;

public:
	FPCloneSheet(FPDocWindow *host);

	~FPCloneSheet();

	void			Run();
	ChordIndex		GetCloneCount();
	bool			GetTransformState();
	UInt16			GetTransposeIndex();
	UInt16			GetHarmonizeIndex();
	UInt16			GetWhichPartsIndex();
	PartMask		GetTransformPartMask();
	void			InitControls();
	void			UpdateControls();
	bool			HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index);
};


#endif
