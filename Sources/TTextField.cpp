/*
 *  TTextField.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TTextField.h"
#include "TCarbonEvent.h"

void TTextField::Init() {
	parentControl = NULL;
}


void TTextField::SetText(SInt16 num) {
	CFStringRef numString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%d"), num);
	SetText(numString);
	CFRELEASE(numString);
}


CFStringRef TTextField::GetText() {
	CFStringRef str;
	(void)GetControlData(fControl, 0, kControlEditTextCFStringTag, sizeof(CFStringRef), &str, NULL);
	return str;
}


#pragma mark -
void TNumericTextField::Init() {
	const EventTypeSpec	editEvents[] = {
		{ kEventClassTextInput, kEventTextInputUnicodeForKeyEvent }
	};

	RegisterForEvents( GetEventTypeCount(editEvents), editEvents );
}


bool TNumericTextField::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
//	fprintf( stderr, "[%08X] Control Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus			result = eventNotHandledErr;

	switch ( event.GetClass() ) {
		case kEventClassTextInput:

			switch ( event.GetKind() ) {
				case kEventTextInputUnicodeForKeyEvent: {
					EventRef	keyboardEvent;
					UInt32		modifiers;
					char		keypress;

					GetEventParameter(event, kEventParamTextInputSendKeyboardEvent, typeEventRef, NULL, sizeof(keyboardEvent), NULL, &keyboardEvent);
					event.GetParameter(kEventParamTextInputSendKeyboardEvent, &keyboardEvent);

					TCarbonEvent keyEvent(keyboardEvent);

					GetEventParameter(keyboardEvent, kEventParamKeyModifiers, typeUInt32, NULL, sizeof(modifiers), NULL, &modifiers);
					keyEvent.GetParameter(kEventParamKeyModifiers, &modifiers);

					GetEventParameter(keyboardEvent, kEventParamKeyMacCharCodes, typeChar, NULL, sizeof(keypress),  NULL, &keypress);
					keyEvent.GetParameter(kEventParamKeyMacCharCodes, &keypress);

					if (IsGoodKey(keypress, modifiers)) {
						// TODO: For good keys sanity check the value
					}
					else
						result = noErr;				// suppress the event by "handling" it

					break;
				}
			}

			break;
	}

	if (result == eventNotHandledErr)
		return TTextField::HandleEvent(inRef, event);
	else
		return true;
}


bool TNumericTextField::IsGoodKey(char keypress, UInt32 modifiers) {
	modifiers &= 0xFFFF;
	bool result = false;

	// fprintf(stderr, "IsGoodKey(%02X, %04X)\n", keypress, modifiers);

	switch (keypress) {
		case kReturnCharCode:
		case kBackspaceCharCode:
		case kEscapeCharCode:
		case kUpArrowCharCode:
		case kDownArrowCharCode:
		case kLeftArrowCharCode:
		case kRightArrowCharCode:
		case kTabCharCode:
			result = true;
			break;
	}

	if (result == false && ((modifiers & ~alphaLock) == 0) && keypress >= '0' && keypress <= '9')
		result = true;

	return result;
}


