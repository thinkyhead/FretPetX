/*
 *  TError.h
 *
 */

#ifndef TERROR_H
#define TERROR_H

#include "TString.h"

/*!
 * An error which can be thrown
 */
class TError {
	private:
		OSErr			error;
		TString			message;

	public:
		TError();
		TError(OSErr err, const CFStringRef msgRef);
		TError(OSErr err, TString &msgString);

		~TError();

		void			Fatal();
		DialogItemIndex StopAlert()		{ return Alert(kAlertStopAlert); }
		DialogItemIndex NoteAlert()		{ return Alert(kAlertNoteAlert); }
		DialogItemIndex CautionAlert()	{ return Alert(kAlertCautionAlert); }
		DialogItemIndex PlainAlert()	{ return Alert(kAlertPlainAlert); }
		DialogItemIndex	Alert(AlertType type);
};

#endif
