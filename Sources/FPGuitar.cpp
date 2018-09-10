/*!
	@file FPGuitar.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPGuitar.h"

#include "FPMacros.h"
#include "TString.h"
#include "CGHelper.h"
#include "TDictionary.h"

CFStringRef bracketParts[] = {
	CFSTR("top"),
	CFSTR("middle"),
	CFSTR("stretch"),
	CFSTR("bottom"),
	};

FPGuitar::FPGuitar(CFBundleRef b) {
	Init();
	InitWithBundle(b);
}

FPGuitar::FPGuitar(FSSpec *imageFile) {
}

FPGuitar::~FPGuitar() {
	delete infoDict;
	if (fretPositionY) delete [] fretPositionY;
	if (stringPositionX) delete [] stringPositionX;
}


//
// ExtractFontInfo
//
typedef struct {
	TString		name;
	UInt8		value;
} astyle;

void FPGuitar::ExtractFontInfo(TString &info, TString &font, UInt16 &size, UInt8 &style) {
	astyle styleInfo[] = {
		{ "normal",		normal		},
		{ "bold",		bold		},
		{ "italic",		italic		},
		{ "underline",	underline	},
		{ "outline",	outline		},
		{ "shadow",		shadow		},
		{ "condense",	condense	},
		{ "extend",		extend		}
	};

	font = "";

	CFArrayRef split = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, info.GetCFStringRef(), CFSTR(" "));

	if (split) {
		// Get the name and size first
		CFIndex i = 0;
		for (; i<CFArrayGetCount(split); i++) {
			TString part = (CFStringRef)CFArrayGetValueAtIndex(split, i);
			UInt16 val = part.IntValue();
			if (val == 0) {
				if (!font.IsEmpty())
					font += " ";
				font += part;
			} else {
				size = val;
				break;
			}
		}

		// Add up all the style flags
		style = normal;
		for (++i; i<CFArrayGetCount(split); i++) {
			TString part = (CFStringRef)CFArrayGetValueAtIndex(split, i);
			for (UInt16 j = 0; j < COUNT(styleInfo); j++) {
				if (part == styleInfo[j].name)
					style += styleInfo[j].value;
			}
		}

		CFRELEASE(split);
	}
}
							   

//
// Init()
//
//	Default values are based on the original guitar palette metrics
//
void FPGuitar::Init() {
	bundle = NULL;
	infoDict = NULL;
	name = NULL;

	titleFont = "Georgia";
	titleFontSize = 11;
	titleFontStyle = normal;
	titleMargin = 10;
	
	labelFont = "Geneva";
	labelFontSize = 9;
	labelFontStyle = normal;
	usingPegs = false;
	
	backgroundFile = NULL;
	dotsFile = NULL;
	cursorFile = NULL;
	effectFile = NULL;

	bzero(bracketFileVertical, sizeof(bracketFileVertical));
	bzero(bracketFileHorizontal, sizeof(bracketFileHorizontal));

	bracketPosX = 6;						// kBracketDefaultPosX
	bracketGripLeft = 15;					// kBracketDefaultGripLeft
	bracketGripRight = 0;					// kBracketDefaultGripRight
	bracketTopOffset = -2;					// kBracketDefaultTopOffset
	bracketBottomOffset = -5;				// kBracketDefaultBottomOffset

	bracketTopHeight = 10;					// kBracketDefaultTopHeight
	bracketBottomHeight = 12;				// kBracketDefaulBottomHeight
	bracketMiddleHeight = 9;				// kBracketDefaulMiddleHeight
	bracketStretchHeight = 10;				// kBracketDefaultStretchHeight
	bracketWidth = 96;						// kBracketDefaultWidth

	cursorHandle = CGPointMake(6, 6);		// kCursorDefaultHandle
	cursorSpeed = 16;						// kCursorDefaultSpeed

	effectHandle = CGPointMake(9, 9);		// kEffectDefaultHandle
	effectFrames = 6;						// kEffectDefaultFrames
	effectSpeed = 20;						// kEffectDefaultSpeed

	numberOfFrets = 18;						// kGuitarDefaultFrets
	openFretY = 24;							// kGuitarDefaultOpenFretY
	nutSize = 2;							// kGuitarDefaultNutSize
	twelfthFretY = 327;						// TODO: Get the right value here!
	fretPositionY = new UInt16[numberOfFrets];
	fretsEvenlySpaced = false;

	numberOfStrings = 6;					// kGuitarDefaultStrings
	firstStringX = 30;						// kGuitarDefaultFirstStringX
	lastStringX = 85;						// kGuitarDefaultLastStringX
	firstStringLowestFret = 0;				// kGuitarDefaultFirstStringLowestFret
}


//
// InitWithBundle(b)
//
//	Initializes the guitar from a guitar bundle. This consists of
//	locating the image files and storing references, and of
//	reading the Info.plist file and initializing our member data.
//
//	Image files aren't actually loaded here.
//
void FPGuitar::InitWithBundle(CFBundleRef b) {
	int i;

	bundle = b;
	infoDict = new TDictionary(CFBundleGetInfoDictionary(b));

	name = infoDict->GetString(kGuitarName);

	// Bracket metrics
	bracketPosX = infoDict->GetInteger(kGuitarBracketPosX, 6);
	bracketGripLeft = infoDict->GetInteger(kGuitarBracketGripLeft, 15);
	bracketGripRight = infoDict->GetInteger(kGuitarBracketGripRight, 0);
	bracketTopOffset = infoDict->GetInteger(kGuitarBracketOffsetTop, -2);
	bracketBottomOffset = infoDict->GetInteger(kGuitarBracketOffsetBottom, -5);

	// The cursor sprite
	cursorHandle = infoDict->GetPoint(kGuitarCursorHandle, CGPointMake(6, 6));
	cursorSpeed = infoDict->GetInteger(kGuitarCursorSpeed, 15);

	// The effect sprite
	effectHandle = infoDict->GetPoint(kGuitarEffectHandle, CGPointMake(9, 9));
	effectFrames = infoDict->GetInteger(kGuitarEffectFrames, 6);
	effectSpeed = infoDict->GetDouble(kGuitarEffectSpeed, 20);

	// Drop any previous fret Y pos array
	if (fretPositionY) {
		delete [] fretPositionY;
		fretPositionY = NULL;
	}

	// Are frets evenly spaced on this guitar?
	fretsEvenlySpaced = infoDict->GetBool(kGuitarLastFretY, false);

	// If the bundle has an array of fret positions, use them.
	// (An array of frets causes "NumberOfFrets" to be ignored)
	CFArrayRef fretArray = infoDict->GetArray(kGuitarFretY);
	if (fretArray) {
		numberOfFrets = CFArrayGetCount(fretArray);
		fretPositionY = new UInt16[numberOfFrets];
		for (i=0; i < numberOfFrets; i++) {
			CFNumberRef numRef = (CFNumberRef)CFArrayGetValueAtIndex(fretArray, (CFIndex)i);
			if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
				(void)CFNumberGetValue(numRef, kCFNumberSInt16Type, &fretPositionY[i]);
		}

		numberOfFrets -= 2;
		CFRELEASE(fretArray);

		openFretY = fretPositionY[0];
		firstFretY = fretPositionY[1];
		twelfthFretY = fretPositionY[13];

	} else {

		// Calculate fret positions based on a compact description
		const float factor[] = { 0.0, 0.056126, 0.109101, 0.159104, 0.206291, 0.250847,
							0.292893, 0.33258, 0.370039, 0.405396, 0.438769, 0.470268,
							0.5, 0.528063, 0.554551, 0.579552, 0.60315, 0.625423,
							0.646447, 0.66629, 0.68502, 0.702698, 0.719385, 0.735134,
							0.75, 0.778063, 0.804551, 0.829552 };

		openFretY = infoDict->GetInteger(kGuitarOpenFretY, 24);
		firstFretY = infoDict->GetInteger(kGuitarOpenFretY, 43);
		twelfthFretY = infoDict->GetInteger(kGuitarTwelfthFretY, 327);

		numberOfFrets = infoDict->GetInteger(kGuitarNumberOfFrets, 18);
		fretPositionY = new UInt16[numberOfFrets];

		fretPositionY[0] = openFretY;

		if (fretsEvenlySpaced) {
			float spacing = (twelfthFretY - firstFretY) / 12;
			for (int n=1; n<numberOfFrets + 2; n++)
				fretPositionY[n] = firstFretY + spacing * (n - 1);
		} else {
			float scaleLength = (twelfthFretY - firstFretY) * 2;
			for (int n=1; n<numberOfFrets + 2; n++)
				fretPositionY[n] = firstFretY + (scaleLength * factor[n]);
		}
	}

	// The nutsize affects centering of open dots
	nutSize = infoDict->GetInteger(kGuitarNutSize, 2);

	// If the bundle has an array of string positions, use them.
	// (An array of strings causes "NumberOfStrings" to be ignored)
	CFArrayRef stringArray = infoDict->GetArray(kGuitarStringX);
	if (stringArray) {
		numberOfStrings = CFArrayGetCount(stringArray);
		stringPositionX = new UInt16[numberOfStrings];
		for (i=numberOfStrings; i--;) {
			CFNumberRef numRef = (CFNumberRef)CFArrayGetValueAtIndex(stringArray, (CFIndex)i);
			if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
				(void)CFNumberGetValue(numRef, kCFNumberSInt16Type, &stringPositionX[i]);
		}

		CFRELEASE(stringArray);

		firstStringX = stringPositionX[0];
		lastStringX = stringPositionX[numberOfStrings-1];

	} else {

		numberOfStrings = infoDict->GetInteger(kGuitarNumberOfStrings, 6);
		stringPositionX = new UInt16[numberOfStrings];
		firstStringX = infoDict->GetInteger(kGuitarFirstStringX, 30);
		lastStringX = infoDict->GetInteger(kGuitarLastStringX, 85);

		UInt16 stringX = lastStringX;
		UInt16 diff = (lastStringX - firstStringX) / (numberOfStrings - 1);
		for (i=numberOfStrings; i--;) {
			stringPositionX[i] = stringX;
			stringX -= diff;
		}
	}

	stringSpacing = stringPositionX[1] - stringPositionX[0];

	// A banjo may not have a full-length low string
	firstStringLowestFret = infoDict->GetInteger(kGuitarFirstStringLowestFret, 0);

	// Get references for all image files
	const CFStringRef png = CFSTR("png");

	backgroundFile = GetBundleResource(CFSTR("background"), png, NULL);

	for (i=COUNT(bracketParts); i--;)
		bracketFileHorizontal[i] = GetBundleResource(bracketParts[i], png, CFSTR("bracketh"));

	for (i=COUNT(bracketParts); i--;)
		bracketFileVertical[i] = GetBundleResource(bracketParts[i], png, CFSTR("bracketv"));

	CGPoint	size = CGGetImageDimensions(bracketFileVertical[2]);
	bracketStretchHeight = size.y;
	bracketWidth = size.x;
	bracketTopHeight = CGGetImageDimensions(bracketFileVertical[0]).y;
	bracketMiddleHeight = CGGetImageDimensions(bracketFileVertical[1]).y;
	bracketBottomHeight = CGGetImageDimensions(bracketFileVertical[3]).y;

	cursorFile = GetBundleResource(CFSTR("cursor"), png, CFSTR("cursor"));
	dotsFile = GetBundleResource(CFSTR("dots"), png, CFSTR("dots"));
	effectFile = GetBundleResource(CFSTR("effect"), png, CFSTR("effect"));

	titleMargin = infoDict->GetFloat(kGuitarTitleMargin);

	TString tFont = infoDict->GetString(kGuitarTitleFont, CFSTR("Georgia 11 normal")),
			lFont = infoDict->GetString(kGuitarLabelFont, CFSTR("Geneva 9 condense"));

	ExtractFontInfo(tFont, titleFont, titleFontSize, titleFontStyle);
	ExtractFontInfo(lFont, labelFont, labelFontSize, labelFontStyle);

	titleColor = infoDict->GetColor(kGuitarTitleColor, CFSTR("000"));
	labelColor = infoDict->GetColor(kGuitarLabelColor, CFSTR("000"));

	//
	// Is there a list of pegs?
	//
	CFDictionaryRef pegDict = infoDict->GetDictionary(kGuitarLabelPegs);
	if (pegDict) {
		usingPegs = true;
		TDictionary dict(pegDict);
		for (unsigned i = 0; i < 6; i++)
			labelPegPosition[i] = dict.GetPoint(TString(i+1).GetCFStringRef());

		CFRELEASE(pegDict);
	}
}


CFURLRef FPGuitar::GetBundleResource(CFStringRef resourceName, CFStringRef resourceType, CFStringRef subDirName) {
	return CFBundleCopyResourceURL(bundle, resourceName, resourceType, subDirName);
}

