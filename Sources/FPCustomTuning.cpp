/*!
	@file FPCustomTuning.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPPreferences.h"
#include "FPCustomTuning.h"
#include "FPDocWindow.h"
#include "FPWindow.h"
#include "CGHelper.h"
#include "TControls.h"
#include "TDictionary.h"

#define TUNER_EXTRA_LINES	6
#define TUNER_LINES			(TUNER_EXTRA_LINES * 2 + 1)

//-----------------------------------------------
//
//	FPCustomTuningControl
//
FPCustomTuningControl::FPCustomTuningControl(WindowRef wind) : TCustomControl(wind, 'cust', 200) {
}

FPCustomTuningControl::~FPCustomTuningControl() {
}

#define kCornerSize 10
bool FPCustomTuningControl::Draw(const Rect &bounds) {
	CGContextRef	gc = FocusContext();
	CGRect			cgBounds = CGBounds();	// (CGPoint)origin, (CGSize)size

	float	width = cgBounds.size.width;
	float	height = cgBounds.size.height;
	float	left = cgBounds.origin.x;
	float	bottom = cgBounds.origin.y;
	float	top = bottom + height;

	CGContextSetLineWidth(gc, 2);
	CGContextSetShouldAntialias(gc, true);

	CGContextSetAlpha(gc, IsEnabled() ? 1.0 : 0.25);

	CGMakeRoundedRectPath(gc, cgBounds, kCornerSize);
	CGContextSetRGBFillColor(gc, 1.0, 1.0, 1.0, 1.0);
	CGContextFillPath(gc);

	//
	// Draw Tone Names
	//
	CGContextSelectFont(gc, "Lucida Grande", 10.0, kCGEncodingMacRoman);

	float x = left;
	CGRect lineRect = cgBounds;
	lineRect.size.height = height / 13.0;
	for (int string=0; string<NUM_STRINGS; string++) {
		float y = top, frameY = 0;
		for (int line=0; line<=12; line++) {
			y -= height / 13.0;
			if ((line % 2) && string == 0) {
				lineRect.origin.y = y;
				CGContextSetRGBFillColor(gc, 0.8, 0.8, 1.0, 1.0);
				CGContextFillRect(gc, lineRect);
			}

			SInt16 tone = builtinTuning[0].tone[string] + line - TUNER_EXTRA_LINES;
			const char *noteName = fretpet->NameOfKey(NOTEMOD(tone));

			if (line == TUNER_EXTRA_LINES)
				CGContextSetRGBFillColor(gc, 1.0, 0.0, 0.0, 1.0);
			else
				CGContextSetRGBFillColor(gc, 0.0, 0.0, 0.0, 1.0);

			CGContextShowTextAtPoint(gc, x + (width / NUM_STRINGS / 4), y + 2, noteName, strlen(noteName));

			if (tone == customTuningList.GetTuningTone(string))
				frameY = y;
		}

		CGRect frameRect = { { x + 2.0, frameY }, { width / NUM_STRINGS - 4.0, height / TUNER_LINES } };

		if (IsPressed() && origString == string)
			CGContextSetRGBStrokeColor(gc, 0.5, 0.2, 1.0, 1.0);
		else
			CGContextSetRGBStrokeColor(gc, 0.0, 0.0, 1.0, 1.0);

		CGContextStrokeRectWithWidth(gc, frameRect, 1.0);

		x += width / NUM_STRINGS;
	}

	//
	// Outline the whole thing
	//
	CGMakeRoundedRectPath(gc, cgBounds, kCornerSize);
	CGContextSetRGBStrokeColor(gc, 0.0, 0.0, 0.0, 1.0);
	CGContextStrokePath(gc);

	EndContext();

	return true;
}

bool FPCustomTuningControl::Track(MouseTrackingResult eventType, Point where) {
	UInt16			string;
	static SInt16	lastTone;
	SInt16			tone = lastTone;

	switch (eventType) {
		case kMouseTrackingMouseDown:
			GetStringToneFromPoint(where, &origString, &tone, false);
			lastTone = customTuningList.GetTuningTone(origString);
			Hilite(kControlButtonPart);
			break;

		case kMouseTrackingMouseDragged:
			GetStringToneFromPoint(where, &string, &tone, true);
			break;

		case kMouseTrackingMouseUp:
			customTuningList.Synchronize();
			Hilite(kControlNoPart);
			DrawNow();
			break;			
	}

	if (tone != lastTone) {
		lastTone = tone;
		customTuningList.SetTuningTone(origString, tone);
		player->PlayNote(kCurrentChannel, tone + OCTAVE);
		DrawNow();
	}

	return true;
}

void FPCustomTuningControl::GetStringToneFromPoint(Point where, UInt16 *string, SInt16 *tone, bool useOrig) {
	CGRect	cgBounds = CGBounds();	// (CGPoint)origin, (CGSize)size
	SInt16	line = (UInt16)(where.v / ( cgBounds.size.height / TUNER_LINES ));
	CONSTRAIN(line, 0, TUNER_LINES - 1);
	*string = (UInt16)(useOrig ? origString : where.h / ( cgBounds.size.width / NUM_STRINGS ));
	*tone = builtinTuning[0].tone[*string] + line - TUNER_EXTRA_LINES;
}


#pragma mark -

class FPCustomTuningWindow;
FPCustomTuningWindow *customWindow = NULL;

//-----------------------------------------------
//
// FPCustomTuningWindow
//
//	The Custom Tunings Window provides a simple
//	interface for adding custom tunings to FretPet.
//
//	The tuning control 
//
class FPCustomTuningWindow : public FPWindow {
	private:
		CFStringRef				rawDeleteText;
		FPCustomTuningControl	*tuningControl;
		TPopupMenu				*popupCustom;
		TControl				*buttonNew, *buttonRename, *buttonDelete, *buttonStrum;
		TTextField				*editField;
		FPWindow				*currentSheet;
//		int						stringTone[6], offset[6];

	public:
		FPCustomTuningWindow() : FPWindow(CFSTR("Customize")) {
			const EventTypeSpec	custEvents[] = { { kEventClassCommand, kEventCommandProcess } };
			RegisterForEvents( GetEventTypeCount( custEvents ), custEvents );

			rawDeleteText	= NULL;
			WindowRef wind = Window();
			currentSheet	= NULL;
			editField		= NULL;
			tuningControl	= new FPCustomTuningControl(wind);
			popupCustom		= new TPopupMenu(wind, kCommandCustomPopup);
			buttonNew		= new TControl(wind, kCommandCustomNew);
			buttonRename	= new TControl(wind, kCommandCustomRename);
			buttonDelete	= new TControl(wind, kCommandCustomDelete);
			buttonStrum		= new TControl(wind, kCommandCustomStrum);

			RefreshControls();

			Cascade(30, 50);
		}

		~FPCustomTuningWindow() {
			customWindow = NULL;

			delete tuningControl;
			delete buttonNew;
			delete buttonRename;
			delete buttonDelete;

			if (rawDeleteText)
				CFRELEASE(rawDeleteText);
		}

		const CFStringRef PreferenceKey()	{ return CFSTR("customize"); }

		void Close() {
			SavePreferences();
			FPWindow::Close();
		}

		//-----------------------------------------------
		//
		// RefreshControls
		//
		void RefreshControls() {
			UInt16 size = customTuningList.size();
			popupCustom->ClearAllItems();

			if (size > 0) {
				for (int i=0; i<size; i++)
					popupCustom->AddItem(customTuningList[i]->name, kCommandCustomPopup);

				popupCustom->SetRange(1, size);
				popupCustom->SetValue(customTuningList.CurrentTuningIndex() + 1);

				popupCustom->Enable();
				buttonRename->Enable();
				buttonDelete->Enable();
				buttonStrum->Enable();
				tuningControl->Enable();
			}
			else {
				popupCustom->Disable();
				buttonRename->Disable();
				buttonDelete->Disable();
				buttonStrum->Disable();
				tuningControl->Disable();
			}

//			popupCustom->DrawNow();
		}

		//-----------------------------------------------
		//
		// UpdateCommandStatus
		//
		// Let most commands through to the application
		//
		bool UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
			bool result = true;
			bool enable = true;
			switch(cid) {
				case kCommandCustomPopup:
					CheckMenuItem(menu, index, index == customTuningList.CurrentTuningIndex() + 1);
					break;

				default:
					result = false;
			}

			if (result && menu) {
				if (enable)
					EnableMenuItem(menu, index);
				else
					DisableMenuItem(menu, index);
			}

			return result;
		}


		//
		// HandleCommand
		//
		// New, Delete, and Rename all bring up sheet dialogs.
		//
		// As each command is completed the custom tunings
		//	are updated in the main preferences file.
		//
		// The Custom Tunings menu is also updated.
		//
		bool HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
			bool result = true;

			switch(cid) {
				case kCommandCustomPopup: {
					customTuningList.SelectCustomTuning(index-1);
					popupCustom->SetValue(index);
					tuningControl->DrawNow();
					break;
				}

				case kCommandCustomNew: {
					currentSheet	= new FPWindow(CFSTR("NewTuningSheet"));
					editField		= new TTextField(currentSheet->Window(), 'newt', 100);
					TString editStr(CFSTR("untitled tuning"));
					editStr.Localize();
					editField->SetText(editStr.GetCFStringRef());
					editField->DoFocus();
					currentSheet->ShowSheet(Window());
					break;
				}

				case kCommandCustomRename: {
					currentSheet	= new FPWindow(CFSTR("RenameTuningSheet"));
					editField		= new TTextField(currentSheet->Window(), 'rent', 100);
					FPTuningInfo *tuning = customTuningList.CurrentTuning();
					editField->SetText(tuning->name);
					editField->DoFocus();
					currentSheet->ShowSheet(Window());
					break;
				}

				case kCommandCustomDelete: {
					// Replace <tuning> with the name in the string
					currentSheet	= new FPWindow(CFSTR("DeleteTuningSheet"));
					TControl *staticText = new TControl(currentSheet->Window(), 'delt', 100);
					if (rawDeleteText == NULL)
						rawDeleteText = staticText->GetStaticText();

					CFStringRef askString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, rawDeleteText, customTuningList.CurrentTuning()->name);
					staticText->SetStaticText(askString);
					CFRELEASE(askString);
					currentSheet->ShowSheet(Window());
					break;
				}

				case kCommandCustomStrum:
					customTuningList.PlayStrum();
					break;

				case kCommandCustomNewYes:
					(void)customTuningList.NewCustomTuning(editField->GetText());
					CloseSheet();
					RefreshControls();
					break;

				case kCommandCustomRenameYes:
					(void)customTuningList.RenameCurrentTuning(editField->GetText());
					CloseSheet();
					RefreshControls();
					break;

				case kCommandCustomDeleteYes:
					customTuningList.DeleteCurrentTuning();
					CloseSheet();
					RefreshControls();
					break;

				case kHICommandCancel:
					CloseSheet();
					break;

				default:
					result = false;
			}

			return result;
		}

		void CloseSheet() {
			currentSheet->Close();

			if (editField != NULL)
				delete editField;

			editField = NULL;
			currentSheet = NULL;
		}

		bool MoveStringCursor(int string, int offset) {
			return true;
		}

		void Cascade(SInt16 h, SInt16 v) {
			FPDocWindow	*front = FPDocWindow::frontWindow;
			if (front) {
				Rect bounds = front->Bounds();
				Move( bounds.left + h, bounds.top + v );
			}
		}

};


#pragma mark -

//-----------------------------------------------
//
// FPCustomTuningList				* CONSTRUCTOR *
//
//	This is an array of structs, and each struct
//	has these values:
//
//		CFStringRef	name
//		SInt32		tone[NUM_STRINGS]
//
//	Storage looks like this:
//
//	tuningDictionary
//		tuningCount, membersDictionary
//
//	membersDictionary
//		itemNum, itemDictionary
//		itemNum, itemDictionary
//
//	itemDictionary
//		name, tones (as a string)
//

FPCustomTuningList::FPCustomTuningList() {
	editTuning = 0;

	if (preferences.IsSet(kPrefCustomTunings)) {
		SInt16 numTunings = 0;
		TDictionary tuningDict( preferences.GetDictionary(kPrefCustomTunings) );

		numTunings = tuningDict.GetInteger(CFSTR("tuningCount"), 0);
		editTuning = tuningDict.GetInteger(CFSTR("editTuning"), 0);

		if (numTunings > 0) {
			TDictionary membersDict( tuningDict.GetDictionary(CFSTR("tunings")) );

			for (SInt16 i=1; i<=numTunings; i++) {
				CFStringRef itemKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("%d"), i);
				TDictionary itemDict( membersDict.GetDictionary(itemKey) );
				CFRELEASE(itemKey);

				CFStringRef	newName = itemDict.GetString(CFSTR("name"));

				SInt32 newTones[NUM_STRINGS];
				itemDict.GetIntArray(CFSTR("tones"), (SInt32*)&newTones, (UInt16)NUM_STRINGS);

				customTuningList.push_back( new FPTuningInfo(newName, newTones) );

				CFRELEASE(newName);
			}
		}
	}
}

FPCustomTuningList::~FPCustomTuningList() {
	Synchronize();	// This will have been done already, but just to be safe
}

//-----------------------------------------------
//
// ShowCustomTuningDialog
//
void FPCustomTuningList::ShowCustomTuningDialog() {
	if (customWindow)
		customWindow->Select();
	else {
		customWindow = new FPCustomTuningWindow();
		customWindow->LoadPreferences();
	}
}

//-----------------------------------------------
//
// CurrentTuning
//
FPTuningInfo*	FPCustomTuningList::CurrentTuning() {
	return (editTuning >= size()) ? NULL : (*this)[editTuning];
}

//-----------------------------------------------
//
// SelectCustomTuning
//
FPTuningInfo*	FPCustomTuningList::SelectCustomTuning(int index) {
	editTuning = index;

	Synchronize();

	return CurrentTuning();
}

//-----------------------------------------------
//
// NewCustomTuning
//
FPTuningInfo*	FPCustomTuningList::NewCustomTuning(CFStringRef newName) {
	FPTuningInfo *newTuning = new FPTuningInfo(newName, NULL);
	push_back( newTuning );
	editTuning = size() - 1;

	Synchronize();
	UpdateMenus();

	return (*this)[editTuning];
}

//-----------------------------------------------
//
// RenameCurrentTuning
//
void FPCustomTuningList::RenameCurrentTuning(CFStringRef newName) {
	CFRELEASE((*this)[editTuning]->name);
	(*this)[editTuning]->name = newName;
	Synchronize();
	UpdateMenus();
}

//-----------------------------------------------
//
// DeleteCurrentTuning
//
void FPCustomTuningList::DeleteCurrentTuning() {
	if (editTuning < size()) {
		erase(this->begin() + editTuning);

		if (editTuning >= size())
			editTuning = size() - 1;

		Synchronize();
		UpdateMenus();
	}
}

//-----------------------------------------------
//
// SetTuningTone
//
void FPCustomTuningList::SetTuningTone(UInt16 string, SInt16 tone) {
	CurrentTuning()->tone[string] = tone;
}

//-----------------------------------------------
//
//	PlayStrum
//
void FPCustomTuningList::PlayStrum() {
	UInt64 playTime = Micro64();
	UInt64 period = U64SetU(1000000 / 120);
	for (int i=0; i<NUM_STRINGS; i++) {
		UInt16	tone = GetTuningTone(i) + OCTAVE;
		player->AddNoteToQueue(tone, playTime);
		playTime = U64Add(playTime, period);
	}
}

//-----------------------------------------------
//
// Synchronize
//
void FPCustomTuningList::Synchronize() {
	TDictionary tuningDict(2);

	SInt16 numTunings = size();

	tuningDict.SetInteger(CFSTR("tuningCount"), numTunings);
	tuningDict.SetInteger(CFSTR("editTuning"), editTuning);

	TDictionary membersDict(numTunings);

	if (numTunings) {
		for (int i=1; i<=numTunings; i++) {
			TDictionary itemDict(2);
			FPTuningInfo *tune = (*this)[i-1];
			itemDict.SetString(CFSTR("name"), tune->name);
			itemDict.SetIntArray(CFSTR("tones"), tune->tone, NUM_STRINGS);

			CFStringRef itemKey = CFStringCreateWithFormat(NULL, NULL, CFSTR("%d"), i);
			membersDict.SetDictionary(itemKey, itemDict);
		}
	}

	tuningDict.SetDictionary(CFSTR("tunings"), membersDict);

	preferences.SetValue(kPrefCustomTunings, tuningDict.GetDictionaryRef());
//	preferences.Synchronize();
}

//-----------------------------------------------
//
// AttachMenu
//
void FPCustomTuningList::AttachMenu(MenuRef menu) {
	FPMenuInfo info = { menu, CountMenuItems(menu) };
	menuList.push_back(info);
	UpdateMenus(menu);
}

//-----------------------------------------------
//
// UpdateMenus
//
void FPCustomTuningList::UpdateMenus(MenuRef inMenu) {
	for (int i=menuList.size(); i--;) {
		MenuRef		menu = menuList[i].menu;

		if (inMenu == NULL || inMenu == menu) {
			UInt16		init = menuList[i].initSize;
			SInt16		diff = CountMenuItems(menu) - init;
			if (diff > 0)
				DeleteMenuItems(menu, init + 1, diff);

			if (!empty()) {
				AppendMenuItemTextWithCFString(menu, CFSTR("-"), 0, 0, NULL);

				for (FPCustomTuningList::iterator itr = this->begin(); itr != this->end(); itr++)
					(void)AppendMenuItemTextWithCFString(menu, (*itr)->name, 0, kFPCommandCustomTuning, NULL);
			}
		}
	}
}

//-----------------------------------------------
//
// FindMatchingMenuItem
//
MenuItemIndex FPCustomTuningList::FindMatchingMenuItem(MenuRef inMenu, const FPTuningInfo &t) {
	MenuItemIndex	firstIndex;

	for (int i=0; i<kBaseTuningCount; i++) {
		if (builtinTuning[i] == t) {
			(void)GetIndMenuItemWithCommandID(inMenu, kFPCommandGuitarTuning, 1, NULL, &firstIndex);
			return firstIndex + i;
		}
	}

	for (UInt16 i=0; i<size(); i++) {
		if (*(*this)[i] == t) {
			(void)GetIndMenuItemWithCommandID(inMenu, kFPCommandCustomTuning, 1, NULL, &firstIndex);
			return firstIndex + i;
		}
	}

	return kInvalidItemIndex;
}
