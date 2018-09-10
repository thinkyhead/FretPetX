/*
 *  main.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include <sys/time.h>

#include "TWindow.h"
#include "FPApplication.h"
#include "FPCustomTuning.h"
#include "FPMacros.h"
#include "FPMusicPlayer.h"
#include "FPPreferences.h"
#include "TString.h"
#include "FPTuningInfo.h"
#include "TError.h"

#if APPSTORE_SUPPORT
#include "AppStoreValidation.h"
#endif

FPApplication   *fretpet = NULL;
FPMusicPlayer	*player = NULL;
FPPreferences	preferences;
CFStringRef		gAppName;
long			systemVersion;

//
// Built in Guitar Tuning names and tones
//
TuningInit baseTuningInit[] = {
	{ CFSTR("Standard"),		{  4,  9, 14, 19, 23, 28 } },
	{ CFSTR("Open C"),			{  0,  7, 12, 19, 24, 28 } },
	{ CFSTR("Open Cm"),			{  3,  7, 12, 19, 24, 27 } },
	{ CFSTR("Open D"),			{  2,  9, 14, 18, 21, 26 } },
	{ CFSTR("Open D sus"),		{  2,  9, 14, 19, 21, 26 } },
	{ CFSTR("Open E"),			{  4, 11, 16, 20, 23, 28 } },
	{ CFSTR("Open Em"),			{  4, 11, 16, 19, 23, 28 } },
	{ CFSTR("Open F"),			{  5,  9, 17, 21, 24, 29 } },
	{ CFSTR("Open Fm"),			{  5,  8, 17, 20, 24, 29 } },
	{ CFSTR("Open G"),			{  2,  7, 14, 19, 23, 26 } },
	{ CFSTR("Open G sus"),		{  2,  7, 14, 19, 24, 26 } },
	{ CFSTR("G over G"),		{  7, 11, 14, 19, 23, 26 } },
	{ CFSTR("G over D"),		{  2,  9, 14, 19, 23, 26 } },
	{ CFSTR("G Top"),			{  4,  9, 14, 19, 23, 26 } },
	{ CFSTR("Open A"),			{  4,  9, 13, 21, 25, 28 } },
	{ CFSTR("Open Am"),			{  4,  9, 12, 21, 24, 28 } },
	{ CFSTR("Perfect 5ths"),	{  0,  7, 14, 21, 28, 35 } },
	{ CFSTR("Perfect 4ths"),	{  4,  9, 14, 19, 24, 29 } },
	{ CFSTR("Major 3rds"),		{  5,  9, 13, 17, 21, 25 } },
	{ CFSTR("David's Tuning"),	{  4,  9, 14, 18, 23, 28 } },
	{ CFSTR("GuitarCraft"),		{  0,  7, 14, 21, 28, 31 } }
};

FPTuningInfo builtinTuning[] = {
	FPTuningInfo(baseTuningInit[0]),
	FPTuningInfo(baseTuningInit[1]),
	FPTuningInfo(baseTuningInit[2]),
	FPTuningInfo(baseTuningInit[3]),
	FPTuningInfo(baseTuningInit[4]),
	FPTuningInfo(baseTuningInit[5]),
	FPTuningInfo(baseTuningInit[6]),
	FPTuningInfo(baseTuningInit[7]),
	FPTuningInfo(baseTuningInit[8]),
	FPTuningInfo(baseTuningInit[9]),
	FPTuningInfo(baseTuningInit[10]),
	FPTuningInfo(baseTuningInit[11]),
	FPTuningInfo(baseTuningInit[12]),
	FPTuningInfo(baseTuningInit[13]),
	FPTuningInfo(baseTuningInit[14]),
	FPTuningInfo(baseTuningInit[15]),
	FPTuningInfo(baseTuningInit[16]),
	FPTuningInfo(baseTuningInit[17]),
	FPTuningInfo(baseTuningInit[18]),
	FPTuningInfo(baseTuningInit[19]),
	FPTuningInfo(baseTuningInit[20])
};

FPCustomTuningList	customTuningList;

void initRandom() {
	struct timeval t;
	gettimeofday(&t, nil);
	unsigned int i;
	i = t.tv_sec;
	i += t.tv_usec;
	srandom(i);
}

int fretpet_main(int argc) {
	initRandom();
	(void)Gestalt(gestaltSystemVersion, &systemVersion);
	try {
		fretpet = new FPApplication();
		fretpet->Run();
	}
	catch (TError &err) {
		fretpet->hasShownSplash = true;
		fretpet->hasShownNagger = true;
		err.Fatal();
	}
	delete fretpet;
	return noErr;
}

int main(int argc, char* argv[]) {

#if APPSTORE_SUPPORT && !defined(CONFIG_Debug)

	// If the app identifier in the receipt does not match the hard-coded app id then fail
	CFStringRef bundleID = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleIdentifier"));
	if (kCFCompareEqualTo != CFStringCompare(bundleID, CFSTR(hardcoded_bidStr), (CFOptionFlags)0))
		exit(kExitAuthFail);

	// If the version identifier in the receipt does not match the hard-coded version then fail
	CFStringRef bundleVersionID = (CFStringRef)CFBundleGetValueForInfoDictionaryKey(CFBundleGetMainBundle(), CFSTR("CFBundleShortVersionString"));
	if (kCFCompareEqualTo != CFStringCompare(bundleVersionID, CFSTR(hardcoded_dvStr), (CFOptionFlags)0))
		exit(kExitAuthFail);

	// Run the more fun validation tests
    startup_call_t theCall = &fretpet_main;
    CFTypeRef obj_arg = NULL;
	validate_call_t tests[] = { firstCheck, secondCheck, thirdCheck, fourthCheck, NULL };
	validate_call_t test;
	int i = 0;
	while (argc < 63 && (test = tests[i++])) {
		argc = test(argc, &theCall, &obj_arg);
	}

    // start or exit
    return theCall(argc);

#else

	return fretpet_main(argc);

#endif
}

