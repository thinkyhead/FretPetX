/*
 *  FPPreferences.h
 *  FretPetX
 *
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPPREFERENCES_H
#define FPPREFERENCES_H

class FPWindow;

class FPPreferences {
	public:
								FPPreferences();

		CFTypeRef				GetCopy(const CFStringRef key) const;
		inline CFArrayRef		GetArray(const CFStringRef key) const				{ return (CFArrayRef)GetCopy(key); }
		bool					GetBoolean(const CFStringRef key, bool dval=FALSE) const;
		SInt32					GetInteger(const CFStringRef key, SInt32 dval=0) const;
		Point					GetPoint(const CFStringRef key) const;
		char*					GetCString(const CFStringRef key) const;
		FSRef*					GetFSRef(const CFStringRef key) const;
		AliasHandle				GetAlias(const CFStringRef key) const;
		CFMutableDictionaryRef	GetDictionary(const CFStringRef key) const;

		void					SetValue(const CFStringRef key, const CFTypeRef value);
		inline void				SetArray(const CFStringRef key, const CFArrayRef array)	{ SetValue(key, array); }
		inline void				SetString(const CFStringRef key, CFStringRef string)	{ SetValue(key, string); }
		void					SetCString(const CFStringRef key, char *string);
		void					SetBoolean(const CFStringRef key, bool state);
		void					SetInteger(const CFStringRef key, SInt32 number);
		void					SetFloat(const CFStringRef key, float number);
		void					SetPoint(const CFStringRef key, SInt32 x, SInt32 y);
		void					SetDictionary(const CFStringRef key, CFStringRef keyList[], CFTypeRef valList[], CFIndex numValues);
		void					SetFSRef(const CFStringRef key, FSRef *fileRef);
		void					SetAlias(const CFStringRef key, AliasHandle alias);

		void					Unset(const CFStringRef key)						{ SetValue(key, NULL); }
		bool					IsSet(const CFStringRef key);
		bool					Synchronize();

		void					ShowPreferencesDialog();
		void					ClosePreferencesDialog();
};

extern FPPreferences preferences;

#define kPrefLastRunVersion	CFSTR("lastRunVersion")

#define kPrefTabReversible	CFSTR("tabReversible")
#define kPrefFingerWarning	CFSTR("fingerWarning")
#define kPrefModalDialogs	CFSTR("modalDialogs")
#define kPrefSplash			CFSTR("splash")
#define kPrefStartupAction	CFSTR("startupAction")
#define kPrefNextQuickTimeTest CFSTR("nextQuickTimeTest")
#define kPrefAutoUpdate		CFSTR("SUEnableAutomaticChecks")

#define kPrefAnalysisMode	CFSTR("analysisMode")
#define kPrefMoreNames		CFSTR("moreNames")

#define kPrefLoop			CFSTR("loop")
#define kPrefAnchor			CFSTR("anchor")
#define kPrefFollow			CFSTR("follow")
#define kPrefFreeEdit		CFSTR("freeEdit")
#define kPrefHorizontal		CFSTR("horizontalGuitar")
#define kPrefReverseStrung	CFSTR("reverseStrung")
#define kPrefUnusedTones	CFSTR("unusedTones")
#define kPrefDotStyle		CFSTR("dotStyle")
#define kPrefLeftHanded		CFSTR("leftHanded")
#define kPrefGuitarSkin		CFSTR("guitarSkin")
#define kPrefInstrument		CFSTR("instrument")
#define kPrefRecentItems	CFSTR("recentItems")

#define kPrefKeyboardSize	CFSTR("keyboardSize")

#define kCommandPrefTab				'Ptab'
#define kCommandPrefFingerWarning	'Pfin'
#define kCommandPrefModalDialogs	'Pmod'
#define kCommandPrefSplash			'Pspl'
#define kCommandPrefAction			'Pact'
#define kCommandPrefAutoCheck		'AuCh'

#endif
