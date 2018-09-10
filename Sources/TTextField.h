/*
 *  TTextField.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef TTEXTFIELD_H
#define TTEXTFIELD_H

#include "TControl.h"

class TTextField : public TControl {
	private:
		TControl		*parentControl;

		void			Init();

	public:
						TTextField(WindowRef wind, OSType sig, UInt32 cid)
							: TControl(wind, sig, cid) { Init(); }

						TTextField(WindowRef wind, UInt32 cmd)
							: TControl(wind, cmd) { Init(); }

						TTextField(ControlRef controlRef)
							: TControl(controlRef) { Init(); }

		virtual			~TTextField() {}

		inline void		SetOwner(TControl *owner)		{ parentControl = owner; }
		inline TControl* GetOwner()						{ return parentControl; }

		inline void		SetItVisible(bool show)			{ if (show) parentControl->Hide(); SetVisible(show); if (!show) parentControl->Show(); }
		inline void		DoFocus()						{ GetTWindow()->SetUserFocus(this); TControl::Focus(); }
		inline void		UnFocus()						{ SetUserFocusWindow(kUserFocusAuto); }

		inline void		SetText(CFStringRef str)		{ (void)SetControlData(fControl, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &str); }
		void			SetText(SInt16 num);
		CFStringRef		GetText();
		inline CFIndex	GetTextLength()					{ return CFStringGetLength(GetText()); }

		inline virtual bool IsGoodKey(char keypress, UInt32 modifiers=0) { return true; }

		inline void		SelectAll()						{ Select(0, GetTextLength()); }

		inline virtual void	Select(SInt16 inStart, SInt16 inEnd);
		//
		// May need to handle events like the following:
		//	kEventClassControl / kEventControlTitleChanged
		//	kEventClassControl / kEventControlValueFieldChanged
		//	kEventClassTextInput / kEventTextInputUnicodeForKeyEvent
		//
};

inline void TTextField::Select(SInt16 inStart, SInt16 inEnd) {
	ControlEditTextSelectionRec sel = { inStart, inEnd };
	SetControlData(fControl, kControlEntireControl, kControlEditTextSelectionTag, sizeof(ControlEditTextSelectionRec), &sel);
}


#pragma mark -

class TLittleArrows;

class TNumericTextField : public TTextField {
	friend class TLittleArrows;

	private:
		TLittleArrows	*littleArrows;
		void			Init();

	public:
						TNumericTextField(WindowRef wind, OSType sig, UInt32 cid)
							: TTextField(wind, sig, cid) { Init(); }

						TNumericTextField(WindowRef wind, UInt32 cmd)
							: TTextField(wind, cmd) { Init(); }

						TNumericTextField(ControlRef controlRef)
							: TTextField(controlRef) { Init(); }

		virtual			~TNumericTextField() {}

		virtual bool	IsGoodKey(char keypress, UInt32 modifiers=0);

		bool			HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );

		inline SInt32	GetIntegerFromText() {
			CFStringRef text = GetText();
			return CFStringGetIntValue(text);
		}
};


#endif
