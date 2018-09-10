/*!
 *  @file FPAuthorizer.h
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#ifndef FPAUTHORIZER_H
#define FPAUTHORIZER_H

class TFile;

class FPAuthorizer {
#if !DEMO_ONLY
	private:
		bool				authorized;		// Is it authorized?

	public:
		CFStringRef			name, code;		// The name and code last used.
		FPAuthorizer();
#else
	public:
		FPAuthorizer() {}

#endif

		~FPAuthorizer() {}

#if DEMO_ONLY
        bool				IsAuthorized() { return false; }
#else
		void				RunKRMDialog();
		void				RunSerialNumberDialog();
		bool				Authorize(CFStringRef authCode, CFStringRef inTestName, bool write=false);
		void				Deauthorize();
		void				Congratulations();
		TFile*				AuthorizationFile();
		void				SaveAuthorization(CFStringRef inAuthCode, CFStringRef inTestName);
		bool				GetSavedAuthorization();
		bool				IsAuthorized() { return authorized; }
#endif
		void				RunNagDialog();
		bool				RunDiscardDialog();

};


extern FPAuthorizer authorizer;

#endif
