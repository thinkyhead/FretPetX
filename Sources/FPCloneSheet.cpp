/*
 *  FPCloneSheet.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPCloneSheet.h"
#include "FPDocWindow.h"
#include "FPWindow.h"
#include "FPPreferences.h"
#include "TTextField.h"
#include "TControls.h"


#define kPrefCloneCount			CFSTR("cloneCount")
#define kPrefCloneTransform		CFSTR("cloneTransform")
#define kPrefCloneTranspose		CFSTR("cloneTranspose")
#define kPrefCloneHarmonize		CFSTR("cloneHarmonize")
#define kPrefCloneWhichParts	CFSTR("cloneWhichParts")

FPCloneSheet::FPCloneSheet(FPDocWindow *host) : FPWindow(CFSTR("CloneSheet")), hostWindow(host) {
	const EventTypeSpec	cloneEvents[] = { { kEventClassCommand, kEventCommandProcess } };
	RegisterForEvents( GetEventTypeCount( cloneEvents ), cloneEvents );
	
	cloneText		= new TNumericTextField(Window(), 'clon', 100);
	cloneArrows		= new TLittleArrows(Window(), 'clon', 101);
	transCheck		= new TCheckbox(Window(), kFPCommandCloneTransform);
	transposePop	= new TPopupMenu(Window(), 'clon', 201);
	harmonizePop	= new TPopupMenu(Window(), 'clon', 202);
	partsPop		= new TPopupMenu(Window(), 'clon', 203);
	
	transposeLabel	= new TControl(Window(), 'clon', 301);
	harmonizeLabel	= new TControl(Window(), 'clon', 302);
}


FPCloneSheet::~FPCloneSheet() {
	preferences.SetInteger(kPrefCloneCount, GetCloneCount());
	preferences.SetBoolean(kPrefCloneTransform, GetTransformState());
	preferences.SetInteger(kPrefCloneTranspose, GetTransposeIndex());
	preferences.SetInteger(kPrefCloneHarmonize, GetHarmonizeIndex());
	preferences.SetInteger(kPrefCloneWhichParts, GetWhichPartsIndex());
	preferences.Synchronize();
	
	delete cloneText;
	delete transCheck;
	delete transposePop;
	delete harmonizePop;
	delete partsPop;
	delete transposeLabel;
	delete harmonizeLabel;
}


void FPCloneSheet::Run() {
	InitControls();
	cloneText->DoFocus();
	ShowSheet(hostWindow->Window());
}


ChordIndex FPCloneSheet::GetCloneCount() {
	return cloneText->GetIntegerFromText();
}


bool FPCloneSheet::GetTransformState() {
	return transCheck->IsChecked();
}


UInt16 FPCloneSheet::GetTransposeIndex() {
	SInt16 val = transposePop->Value() - 2;
	if (val < 0) val = 0;
	return val;
}


UInt16 FPCloneSheet::GetHarmonizeIndex() {
	SInt16 val = harmonizePop->Value() - 2;
	if (val < 0) val = 0;
	return val;
}


UInt16 FPCloneSheet::GetWhichPartsIndex() {
	return partsPop->Value();
}


UInt16 FPCloneSheet::GetTransformPartMask() {
	UInt16 result = 0x0000;
	
	if (transCheck->IsChecked()) {
		switch (GetWhichPartsIndex()) {
			case kFilterEnabled:
				result = hostWindow->GetTransformMask();
				break;
				
			case kFilterCurrent:
				result = BIT(hostWindow->CurrentPart());
				break;
				
			case kFilterAll:
				result = kAllChannelsMask;
				break;
		}
	}
	
	return result;
}


void FPCloneSheet::InitControls() {
	cloneArrows->SetValue(preferences.GetInteger(kPrefCloneCount, 2));
	cloneArrows->AttachTextField(cloneText);
	
	transCheck->Check(preferences.GetBoolean(kPrefCloneTransform, false));
	
	SInt32	val = preferences.GetInteger(kPrefCloneTranspose, 0);
	val = val ? val + 2 : 1;
	transposePop->SetValue(val);
	
	val = preferences.GetInteger(kPrefCloneHarmonize, 0);
	val = val ? val + 2 : 1;
	harmonizePop->SetValue(val);
	
	val = preferences.GetInteger(kPrefCloneWhichParts, 0);
	val = val ? val + 2 : 1;
	partsPop->SetValue(val);
	
	UpdateControls();
}


void FPCloneSheet::UpdateControls() {
	bool enabled = GetTransformState();
	transposePop->SetEnabled(enabled);
	harmonizePop->SetEnabled(enabled);
	transposeLabel->SetEnabled(enabled);
	harmonizeLabel->SetEnabled(enabled);
}


bool FPCloneSheet::HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
	switch (cid) {
		case kFPCommandCloneTransform:
			UpdateControls();
			break;
			
		case kHICommandCancel:
			hostWindow->KillCloneDialog();
			break;
			
		case kFPCommandSelCloneYes: {
			if (GetCloneCount() > 1) {
				hostWindow->CloneSelection(GetCloneCount(), GetTransformPartMask(), GetTransposeIndex(), GetHarmonizeIndex());
				hostWindow->KillCloneDialog();
			}
			else
				SysBeep(1);
			
			break;
		}
			
		default:
			return false;
	}
	
	return true;
}

