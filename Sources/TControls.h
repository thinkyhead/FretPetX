/*
 *  TControls.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TCONTROLS_H
#define TCONTROLS_H

#include "TControl.h"
#include "TCustomControl.h"
#include "TTextField.h"

#pragma mark -
//-----------------------------------------------
//
//  TCheckbox
//
class TCheckbox : public TControl {
	public:
						TCheckbox() : TControl() {}
						TCheckbox(WindowRef wind, OSType sig, UInt32 cid) : TControl(wind, sig, cid) {}
						TCheckbox(WindowRef wind, UInt32 cmd) : TControl(wind, cmd) {}
						TCheckbox(DialogRef dialog, SInt16 item) : TControl(dialog, item) {}
						TCheckbox(ControlRef controlRef) : TControl(controlRef) {}
		virtual 		~TCheckbox() {}

		inline void		Check()								{ Check(true); }
		void			Check(bool check)					{ SetValue(check ? kControlCheckBoxCheckedValue : kControlCheckBoxUncheckedValue); }
		void			Check(bool isMixed, bool isChecked)	{ SetValue(isChecked ? (isMixed ? kControlCheckBoxMixedValue : kControlCheckBoxCheckedValue) : kControlCheckBoxUncheckedValue); }
		bool			IsChecked()							{ return Value(); }
};



#pragma mark -
//-----------------------------------------------
//
//  TPopupMenu
//
class TPopupMenu : public TControl {
	public:
						TPopupMenu() : TControl() {}
						TPopupMenu(WindowRef wind, OSType sig, UInt32 cid) : TControl(wind, sig, cid) {}
						TPopupMenu(WindowRef wind, UInt32 cmd) : TControl(wind, cmd) {}
						TPopupMenu(DialogRef dialog, SInt16 item) : TControl(dialog, item) {}
						TPopupMenu(ControlRef controlRef) : TControl(controlRef) {}

		virtual 		~TPopupMenu() {}

		inline MenuRef	GetMenu()						{ return GetControlPopupMenuHandle(fControl); }
		inline void		SetMenu(MenuRef menuRef)		{ SetControlPopupMenuHandle(fControl, menuRef); }
		OSStatus		ClearAllItems();
		OSStatus		ClearItems(MenuItemIndex first, ItemCount count);
		UInt16			CountItems();
		OSStatus		AddSeparator()					{ return AddItem(CFSTR("-"), 0); }
		OSStatus		AddItem(CFStringRef itemText, MenuCommand cid);
		OSStatus		InsertItem(CFStringRef itemText, MenuItemIndex index, MenuCommand cid);

		bool			ControlHit(Point where);
};



#pragma mark -
//-----------------------------------------------
//
//  TRadioGroup
//
//	(TControl has everything we need!)
//
typedef class TControl TRadioGroup;



#pragma mark -
//-----------------------------------------------
//
// TSlider
//
class TSlider : public TControl {
	private:
		ControlActionUPP	sliderAction;

	public:
						TSlider(WindowRef wind, UInt32 cmd);
	virtual				~TSlider();
	static pascal void	LiveSliderProc(ControlRef control, ControlPartCode partCode);
};


#pragma mark -
//-----------------------------------------------
//
//  TTriangle
//
class TTriangle : public TControl {
	public:
				TTriangle() : TControl() {}
				TTriangle(WindowRef wind, OSType sig, UInt32 cid) : TControl(wind, sig, cid) {}
				TTriangle(WindowRef wind, UInt32 cmd) : TControl(wind, cmd) {}
				TTriangle(DialogRef dialog, SInt16 item) : TControl(dialog, item) {}
				TTriangle(ControlRef controlRef) : TControl(controlRef) {}
				~TTriangle() {}

		bool	Draw(const Rect &bounds)	{ return false; }
		bool	GetOpenState()				{ return Value(); }
		void	SetOpenState(bool inState)	{ SetValue(inState ? 1 : 0); }
};


#pragma mark -
//-----------------------------------------------
//
//  TLittleArrows
//
class TLittleArrows : public TControl {
	private:
		TNumericTextField	*textField;
		void				Init() {
								textField = NULL;
								EnableAction();

								const EventTypeSpec	moreEvents[] = {
									{ kEventClassControl, kEventControlValueFieldChanged }
									};

								RegisterForEvents(GetEventTypeCount(moreEvents), moreEvents);
							}

	public:
							TLittleArrows(WindowRef wind, OSType sig, UInt32 cid)
								: TControl(wind, sig, cid) { Init(); }

							TLittleArrows(WindowRef wind, UInt32 cmd)
								: TControl(wind, cmd) { Init(); }

							TLittleArrows(ControlRef controlRef)
								: TControl(controlRef) { Init(); }

		virtual				~TLittleArrows() {}

		virtual void		ValueChanged()					{ if (textField) UpdateTextField(); }
		void				AttachTextField(TNumericTextField *tf)	{ textField = tf; UpdateTextField(); }
		void				UpdateTextField()				{ if (textField) { textField->SetText((SInt16)Value()); textField->SelectAll(); } }
		void				InitFromTextField()				{ if (textField) SetValue(textField->GetIntegerFromText()); }

		virtual void		Action(ControlPartCode inPart) {
								switch(inPart) {
									case kControlUpButtonPart:
										SetValue(Value() + 1);
										ValueChanged();
										break;
									
									case kControlDownButtonPart:
										SetValue(Value() - 1);
										ValueChanged();
										break;
								}
							}

};

#endif
