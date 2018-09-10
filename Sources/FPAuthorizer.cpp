/*
 *  FPAuthorizer.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *
 *	FretPet				4599-1508-8595
 *	FretPetSingle		6733-1719-9740
 *	FretPetXUser		4542-2545-6515
 *
 *	Seed Combo :nn
 *	Seed Length: 20
 *	Minimum Email Length: 4
 *	Minimum Name Length: 5
 *	Minimum HotSync ID Length: 10
 *	Length of Constant: 2
 *	Constant value: V1
 *	Sequence Length: 1
 *	Alternate Text: Contact register@fretpet.com to obtain your registration code.
 *	Scramble Order: U0,U8,U13,U16,U18,D3,C0,D0,U12,U11,U15,U4,U1,S0,D2,U3,U17,U7,C1,U10,U5,U19,D1,U9,U14,U6,U2,
 *	ASCII Digit(For text to Number Conversion):2
 *	Math Operation(Example:36A(36->Operand,A-Addition)): 6A,R,12S,22S,8A,R,8A,8A,15S,15S,R,5S,R,R,R,1A,1A,1A,1S,1S,1S,7A,33S,6A,R,6A,R,
 *	New Base: 30
 *	Base Character Set: WHCG6LTEFYAX8RMQ34U2KDNPJ15790
 *	Registration code format(^ is place holder for check digit) : FPX##-#####-#####-#####-##^##-#####-#####-#####-#[-#]
 *
 */

#include "FPAuthorizer.h"

#if KAGI_SUPPORT
#include "ZonicKRM.h"
#include "GenericDecode.h"
#endif

#include "FPApplication.h"
#include "FPUtilities.h"

#include "TControls.h"
#include "TWindow.h"
#include "TFile.h"
#include "TString.h"
#include "TDictionary.h"

#define	FRETPET_AUTH_FILE		CFSTR(".authorization")
#define	FRETPET_AUTH_FILE_P		"\p.authorization"
#define kFPCommandDoAuthorize	'Auth'
#define kAuthNameKey			CFSTR("name")
#define kAuthCodeKey			CFSTR("code")
#define kAuthCodeBase			30
#define kAuthCodeLength			27
#define kAuthCheckDigit			26

FPAuthorizer authorizer;

#pragma mark -

#if !DEMO_ONLY

class FPAuthorizeDialog : public TWindow {
	private:
		TTextField		*nameField, *codeField;

	public:
		FPAuthorizeDialog() : TWindow(MAIN_NIB_NAME, CFSTR("Authorize")) {
			const EventTypeSpec	authEvents[] = { { kEventClassCommand, kEventCommandProcess } };
			RegisterForEvents( GetEventTypeCount( authEvents ), authEvents );

			nameField = new TTextField(Window(), 'auth', 100);
			codeField = new TTextField(Window(), 'auth', 101);

			if (authorizer.name) {
				nameField->SetText(authorizer.name);
			}
			else {
				CFStringRef name = CSCopyUserName(false);
				nameField->SetText(name);
				CFRELEASE(name);
			}

			if (authorizer.code)
				codeField->SetText(authorizer.code);

			nameField->DoFocus();
		}

		~FPAuthorizeDialog() {}

		bool HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index) {
			switch (cid) {
				case kFPCommandDoAuthorize:
					if ( authorizer.Authorize( codeField->GetText(), nameField->GetText(), true) ) {
						Hide();
						QuitModal();
						authorizer.Congratulations();
					}
					else
						SysBeep(1);

					return true;
					break;

				case kHICommandCancel:
					QuitModal();
					return true;
					break;

				case kHICommandQuit:
					QuitModal();
					break;
			}

			return false;
		}
};

#pragma mark -
#pragma mark Help Menu

//-----------------------------------------------
//
//  FPAuthorizer				* CONSTRUCTOR *
//
FPAuthorizer::FPAuthorizer() {
	authorized	= false;

#if KAGI_SUPPORT && !DEMO_ONLY
    name		= NULL;
	code		= NULL;
	GetSavedAuthorization();
#endif
}

#if KAGI_SUPPORT

//-----------------------------------------------
//
//	RunKRMDialog
//
void FPAuthorizer::RunKRMDialog() {
	ZKRMParameters			theParameters;
	ZKRMResult				*theResult;
	ZKRMModuleStatus		theStatus;

	// Initialise our state
	InitKRMParameters(&theParameters);
	theParameters.moduleLanguage	= kZKRMLanguageSystem;
	theParameters.moduleOptions		= (ZKRMOptions)(kZKRMOptionOfferWebOrder | kZKRMOptionEncryptedProductXML);
	theParameters.moduleUserName	= NULL;
	theParameters.moduleUserEmail	= NULL;

#if defined(CONFIG_Debug)
	CFMutableStringRef	cryptString = CFStringCreateMutableCopy(
			kCFAllocatorDefault,
			0,
			CFSTR(
			"<krmData vendorID=\"3V4\">"
			"<transactionFailureURL>http://www.thinkyhead.com/fretpet/krm/failure.html</transactionFailureURL>"
			"<connectionFailureURL>http://www.thinkyhead.com/fretpet/krm/failure.html</connectionFailureURL>"
			"<products>"
				"<product partNo=\"\" dbName=\"FretPetXUser\" supplierSKU=\"n/a\">"
					"<displayName lang=\"en\">FretPet X</displayName>"
					"<price currency=\"AUD\" basePrice=\"9.99\" baseCurrency=\"USD\"/>"
					"<price currency=\"CAD\" basePrice=\"9.99\" baseCurrency=\"USD\"/>"
					"<price currency=\"EUR\" basePrice=\"9.99\" baseCurrency=\"USD\"/>"
					"<price currency=\"GBP\" basePrice=\"9.99\" baseCurrency=\"USD\"/>"
					"<price currency=\"JPY\" basePrice=\"9.99\" baseCurrency=\"USD\"/>"
					"<price currency=\"USD\">9.99</price>"
				"</product>"
			"</products>"
			"</krmData>")
		);
	EncryptKRMString(cryptString);
	CFStringPrint(cryptString);
	CFRELEASE(cryptString);
#endif

	theParameters.productInitXML = CFSTR(
		"BFSH7169DCAE4CD46ABB5A30830DC66248FC8A119A196A698CBCF5E42C836605"
		"768F58225F69E92EF73C5CEBF20B3E94F59BA32E606C74C3CFA500F0243B3A6F"
		"8279AB960CCE9297A9A55B6745EEBC0447FB079440DFAE39805A830E728CC293"
		"61FD2E6ADAB9865A6512F33F6AF0C670FD5FDFA9A9BCACF609EFB21919062B02"
		"425558225F69E92EF73C5CEBF20B3E94F59BA32E606C74C3CFA500F0243B3A6F"
		"8279AB960CCE9297A9A55B6745EEBC0447FB079440DFAE39805A830E728CC293"
		"61FDAA38434F68142CC3B3FF0897854FE8A8BD9B0F94E05B9E389B1800D09BCE"
		"A61B511AE02B0A7A35397B2E549158F561A65CB8BFEFCF2E1D54378FAA801F04"
		"4F4135AE209639A7015EC12D997ED5D76F721119D58FBCF40071A1789E2BC0EE"
		"C06FF0787229DFAC61CA7FBFF128EC4D45116145C8ACB45225B4255E349719C6"
		"3DE15130E1EDB58944D86139621592B989A50FEFE385D2F5B319C459EF795854"
		"896971D70DCE4B930E278A25BD4C04578FF0C6DE829BB5EB2BC9950146A34B3E"
		"5ED0CA4332D34413748A19E8B99A9FCD6C69F62AE823B6BDCC4EA2EE929BE090"
		"38A727E574058F86DB8FCD2DF3F17FEAB7DA8CD26E1D1C0FAC170035AF2BA81A"
		"1B3F1BE018D4F1D60A5412A4BEE01417CE53A231083BA7EA807C3F73908F853A"
		"F219CBC54945A40891BE7582AA6BEAF7F754780889C6A91D593399AAECCC4104"
		"F7E96F7989B63017C155D036D61CDB567E38825C899220291084303BD1BD763C"
		"D9FDE93C196E63684AC285E267772D8A0B986FDFEA96B12B4C45AFF9B2B2E0B8"
		"454C950146A34B3E5ED02A531A9E596DB48C40CD8068A10EDA9817DB10D3D0FC"
		"6FA460C93FDCB64E1F229A6D1FA78DD6936CDB7F66FEBA1DB6A6A45679965330"
		"F8197DF02DE7513B913E00F4D93B76A089EAAC97F923D8425B62955B94D04FF7"
		"9768C270A0AD52AF3488F54DE72F7A92FEB8BB7818A273C6C52B"
		);

	theParameters.productStoreURL	= CFSTR("http://store.kagi.com/?3V4_LIVE&lang=en");

	CFStringRef tfypString = CFCopyLocalizedString( CFSTR("I hope you've enjoyed Kagi's new registration process."), "" );
	theParameters.productTFYP		= tfypString;

//	theParameters.productPO			= CFSTR("");
//	theParameters.productAffiliate	= CFSTR("slur");
//	theParameters.acgKeyValueCount	= 0;
//	theParameters.acgKeyValueData	= NULL;

    // Invoke the KRM
	theStatus = BeginModalKRM(&theParameters, &theResult);
    if (theStatus == kZKRMModuleNoErr) {
		if (theResult->orderStatus == kZKRMOrderSuccess) {
//			CFStringPrint(theResult->acgUserName);
//			CFStringPrint(theResult->acgRegCode);
//			CFStringPrint(theResult->kagiReplyXML);

			// Test the authorization data
			if ( Authorize(theResult->acgRegCode, theResult->acgUserName, true) ) {
				Congratulations();
			}
			else {
				CFStringRef krmError = CFCopyLocalizedString( CFSTR("KRM Reply Error"), "" );
				AlertError(kFPErrorKRM, krmError);
				CFRELEASE(krmError);
			}
		}

		DisposeKRMResult(&theResult);
	}

	CFRELEASE(tfypString);
}


//-----------------------------------------------
//
//	Congratulations
//
void FPAuthorizer::Congratulations() {
	(void)RunPrompt( CFSTR("Authorization Successful!"), CFSTR("Thank you for registering FretPet X. All restrictions have been removed from the program."), CFSTR("OK"), NULL, NULL);
}


//-----------------------------------------------
//
//	RunSerialNumberDialog
//
void FPAuthorizer::RunSerialNumberDialog() {
	FPAuthorizeDialog dialog;
	dialog.Show();
	dialog.RunModal();
}


//-----------------------------------------------
//
//	Authorize
//
//	TODO: get short username, encrypt it, and store it
//			maybe include the file's inode as well
//
bool FPAuthorizer::Authorize(CFStringRef inAuthCode, CFStringRef inTestName, bool write) {
	if (inAuthCode == NULL || inTestName == NULL)
		return false;
	
	const char *configString =	"SUPPLIERID:slur%E:4%N:5%H:10%COMBO:nn%SDLGTH:20%CONSTLGTH:2%CONSTVAL:V1%"
	"SEQL:1%ALTTEXT:Contact register@thinkyhead.com to obtain your registration code.%"
	"SCRMBL:U0,,U8,,U13,,U16,,U18,,D3,,C0,,D0,,U12,,U11,,U15,,U4,,U1,,S0,,D2,,U3,,"
	"U17,,U7,,C1,,U10,,U5,,U19,,D1,,U9,,U14,,U6,,U2,,%ASCDIG:2%MATH:6A,,R,,12S,,22S,,"
	"8A,,R,,8A,,8A,,15S,,15S,,R,,5S,,R,,R,,R,,1A,,1A,,1A,,1S,,1S,,1S,,7A,,33S,,6A,,R,,"
	"6A,,R,,%BASE:30%BASEMAP:WHCG6LTEFYAX8RMQ34U2KDNPJ15790%REGFRMT:FPX##-#####-#####-"
	"#####-##^##-#####-#####-#####-#[-#]";
	
	if (name) CFRELEASE(name);
	if (code) CFRELEASE(code);
	
	name = CFStringCreateCopy(kCFAllocatorDefault, inTestName);
	code = CFStringCreateCopy(kCFAllocatorDefault, inAuthCode);
	
	char *authString = CFStringToCString(inAuthCode);
	char *nameString = CFStringToCString(inTestName);
	
	bool is_good = false;
	
	do {
		if (0 != decodeGenericRegCode(configString, authString))
			break;
		
		setUserName(nameString);
		createUserSeed();
		
		if (strcmp(getUserSeed(), getCreatedUserSeed()))
			break;
		
		if (write)
			SaveAuthorization(inAuthCode, inTestName);
		
		is_good = true;
		
	} while (false);
	
	delete [] nameString;
	delete [] authString;
	
	return is_good;
}

//-----------------------------------------------
//
//	Deauthorize
//
void FPAuthorizer::Deauthorize() {
	if (authorized) {
		TFile *authFile = AuthorizationFile();
		if (authFile != NULL) {
			authFile->Delete();
			authorized = false;
		}
		
		CFRELEASE(name);
		CFRELEASE(code);
		name = code = NULL;
	}
}

//-----------------------------------------------
//
//	AuthorizationFile
//
TFile* FPAuthorizer::AuthorizationFile() {
	/*
	 OSErr			err = noErr;
	 FSSpec			authSpec, supportSpec = fretpet->SupportFolder();
	 TFile			suppFolder(supportSpec), *authFile = NULL;
	 
	 FSCatalogInfo	catInfo;
	 err = FSGetCatalogInfo(&suppFolder.FileRef(), kFSCatInfoVolume|kFSCatInfoNodeID, &catInfo, NULL, NULL, NULL);
	 
	 if (err == noErr) {
	 err = FSMakeFSSpec(catInfo.volume, catInfo.nodeID, FRETPET_AUTH_FILE_P, &authSpec);
	 if (err == noErr || err == fnfErr) {
	 Boolean	targetIsFolder, wasAliased;
	 err = ResolveAliasFile( &authSpec, true, &targetIsFolder, &wasAliased);
	 
	 if (err == noErr || err == fnfErr)
	 authFile = new TFile(authSpec);
	 }
	 }
	 */
	
	/*
	 TString			authName(FRETPET_AUTH_FILE);
	 HFSUniStr255	uniName;
	 authName.GetHFSUniStr255(&uniName);
	 
	 // FAILS! Because the file must exist with this constructor
	 TFile *authFile = new TFile(suppFolder.FileRef(), &uniName);
	 */
	
	// and what about this one?
	return new TFile(fretpet->SupportFolderURL(FRETPET_AUTH_FILE));
}

//-----------------------------------------------
//
//	SaveAuthorization
//
void FPAuthorizer::SaveAuthorization(CFStringRef inAuthCode, CFStringRef inTestName) {
	authorized = true;
	
	TDictionary fileDict;
	
	fileDict.SetString(kAuthNameKey, inTestName);
	fileDict.SetString(kAuthCodeKey, inAuthCode);
	
	TFile *authFile = AuthorizationFile();
	
	if (authFile != NULL) {
		if (noErr == authFile->Create())
			(void)authFile->WriteXMLFile(fileDict);
		
		delete authFile;
	}
}

//-----------------------------------------------
//
//	GetSavedAuthorization
//
//	TODO: Test the saved username / inode
//			and fail quietly if incorrect
//
bool FPAuthorizer::GetSavedAuthorization() {
	TFile	*authFile = AuthorizationFile();
	
	if (authFile != NULL && authFile->Exists()) {
		TDictionary *fileDict = authFile->ReadXMLFile();
		
		if ( fileDict->IsValidPropertyList(kCFPropertyListXMLFormat_v1_0) ) {
			name = fileDict->GetString(kAuthNameKey);
			code = fileDict->GetString(kAuthCodeKey);
			authorized = Authorize(code, name);
			CFRELEASE(name);
			CFRELEASE(code);
		}
		
		delete fileDict;
	}
	
	delete authFile;
	return authorized;
}

#endif // DEMO_ONLY

#endif // KAGI_SUPPORT

#if KAGI_SUPPORT || DEMO_ONLY

//-----------------------------------------------
//
//	RunNagDialog
//
void FPAuthorizer::RunNagDialog() {

#if DEMO_ONLY
	DialogItemIndex item = RunPrompt(
		 CFSTR("FretPet X Demo"),
		 CFSTR("This DEMO of FretPet has most of the features of FretPet, but SAVING IS DISABLED."),
		 CFSTR("Buy FretPet Now!"),
		 CFSTR("Or Later..."),
		 NULL
		 );

	if (item == kAlertStdAlertOKButton)
		fretpet->GenerateCommandEvent(kFPCommandRegister);

#else
	if ( ! IsAuthorized() ) {
		DialogItemIndex item = RunPrompt(
			 CFSTR("Welcome To FretPet X"),
			 CFSTR("If you find this program useful, educational, or entertaining you can support Thinkyhead by purchasing a license key from Kagi's secure online store. Save will be disabled during the evaluation period."),
			 CFSTR("Register Now"),
			 CFSTR("Register Later"),
			 CFSTR("Enter Serial Number")
			 );

        if (item == kAlertStdAlertOKButton)
            RunKRMDialog();
		else if (item == kAlertStdAlertOtherButton)
            RunSerialNumberDialog();
		
	}
#endif
}

#endif

//-----------------------------------------------
//
//	RunDiscardDialog
//
//	result: TRUE if the user opted to discard
//
bool FPAuthorizer::RunDiscardDialog() {
	bool result = false;

#if DEMO_ONLY
	DialogItemIndex item = RunPrompt(
		 CFSTR("FOR DEMONSTRATION ONLY"),
		 CFSTR("This demo of FretPet is save-disabled. You can capture your composition by recording its MIDI output in an application like GarageBand. Help ensure a brighter future for FretPet. Purchase yours today!"),
		 CFSTR("Continue Editing"),
		 CFSTR("Discard"),
		 CFSTR("Purchase FretPet")
		 );
	
	if (item == kAlertStdAlertCancelButton) // Discard
		result = true;
	else if (item == kAlertStdAlertOtherButton) // Purchase
		fretpet->GenerateCommandEvent(kFPCommandRegister);

	// and/or DON'T DISCARD

#elif KAGI_SUPPORT
	if ( ! IsAuthorized() ) {
		DialogItemIndex item = RunPrompt( 
			CFSTR("Are you sure you want to discard this document?"),
			CFSTR("If you close this document all your work will be lost. Register now to remove all restrictions from FretPet X and help support quality shareware."),
			CFSTR("Register Now"),
			CFSTR("Cancel"),
			CFSTR("Discard Changes")
		);

		if (item == kAlertStdAlertOKButton)
			RunKRMDialog();
		else if (item == kAlertStdAlertOtherButton)
			result = true;
	}
#endif

	return result;
}


