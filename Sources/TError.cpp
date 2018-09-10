/*
 *  TError.cpp
 *
 */

#include "TError.h"
#include "TString.h"

TError::TError() {
	error = 0;
	message = CFSTR("An unspecified error occurred.");
}

TError::TError(OSErr err, TString &msg) {
	error = err;
	message = msg;
}

TError::TError(OSErr err, const CFStringRef msg) {
	error = err;
	message = msg;
}

TError::~TError() {}

void TError::Fatal() {
	(void)Alert(kAlertStopAlert);
	ExitToShell();
}

DialogItemIndex TError::Alert(AlertType type) {

	TString errString;
	errString.SetWithFormatLocalized(CFSTR("Error %d"), error);

	message.Localize();

	DialogRef		alert;
	DialogItemIndex	itemHit;
	CreateStandardAlert(type, errString.GetCFStringRef(), message.GetCFStringRef(), NULL, &alert);
	(void)RunStandardAlert(alert, NULL, &itemHit);
	return itemHit;
}

