/*!
	@file TDictionary.cpp

	@brief CFDictionary wrapper class

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPUtilities.h"
#include "TDictionary.h"

TDictionary::~TDictionary() {
	if (dict != NULL)
		CFRELEASE(dict);
}


void TDictionary::Init(CFIndex maxSize) {
	dict = CFDictionaryCreateMutable( kCFAllocatorDefault, maxSize, &kCFCopyStringDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
}


TDictionary& TDictionary::operator=(const TDictionary &src) {
	if (dict)
		CFRELEASE(dict);

	if (src.dict != NULL)
		dict = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, src.dict );
	else
		dict = NULL;

	return *this;
}


TDictionary& TDictionary::operator=(CFDictionaryRef &src) {
	if (dict)
		CFRELEASE(dict);

	if (src != NULL)
		dict = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, src );
	else
		dict = NULL;

	return *this;
}


TDictionary& TDictionary::operator=(CFMutableDictionaryRef &src) {
	if (dict)
		CFRELEASE(dict);

	if (src != NULL)
		dict = CFDictionaryCreateMutableCopy( kCFAllocatorDefault, 0, src );
	else
		dict = NULL;

	return *this;
}


//------------------------------------------------------------
//
//	SetInteger / GetInteger
//
void TDictionary::SetInteger(CFStringRef inName, SInt32 val) {
	CFNumberRef	numRef = CFNumberCreate(NULL, kCFNumberIntType, &val);
	CFDictionarySetValue(dict, inName, numRef);
	CFRELEASE(numRef);
}


SInt32 TDictionary::GetInteger(CFStringRef inName, SInt32 defaultVal) const {
	SInt32 value;

	CFNumberRef	numRef = (CFNumberRef)GetValue(inName);

	if ( numRef ) {
		if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
			(void)CFNumberGetValue(numRef, kCFNumberSInt32Type, &value);

		CFRELEASE(numRef);
	}
	else
		value = defaultVal;

	return value;
}


//------------------------------------------------------------
//
//	SetFloat / GetFloat
//
void TDictionary::SetFloat(CFStringRef inName, float val) {
	CFNumberRef	numRef = CFNumberCreate(NULL, kCFNumberFloatType, &val);
	CFDictionarySetValue(dict, inName, numRef);
	CFRELEASE(numRef);
}


float TDictionary::GetFloat(CFStringRef inName, float defaultVal) const {
	float value;

	CFNumberRef	numRef = (CFNumberRef)GetValue(inName);

	if ( numRef ) {
		if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
			(void)CFNumberGetValue(numRef, kCFNumberFloatType, &value);

		CFRELEASE(numRef);
	}
	else
		value = defaultVal;

	return value;
}


//------------------------------------------------------------
//
//	SetDouble / GetDouble
//
void TDictionary::SetDouble(CFStringRef inName, double val) {
	CFNumberRef	numRef = CFNumberCreate(NULL, kCFNumberDoubleType, &val);
	CFDictionarySetValue(dict, inName, numRef);
	CFRELEASE(numRef);
}


double TDictionary::GetDouble(CFStringRef inName, double defaultVal) const {
	double value;

	CFNumberRef	numRef = (CFNumberRef)GetValue(inName);

	if ( numRef ) {
		if ( CFGetTypeID(numRef) == CFNumberGetTypeID() )
			(void)CFNumberGetValue(numRef, kCFNumberDoubleType, &value);

		CFRELEASE(numRef);
	}
	else
		value = defaultVal;

	return value;
}


//------------------------------------------------------------
//
//	SetBool / GetBool
//
void TDictionary::SetBool(CFStringRef inName, bool val) {
	CFDictionarySetValue(dict, inName, val ? kCFBooleanTrue : kCFBooleanFalse);
}


bool TDictionary::GetBool(CFStringRef inName, bool defaultVal) const {
	bool value = defaultVal;
	CFTypeRef	numRef = GetValue(inName);

	if ( numRef ) {
		if ( CFGetTypeID(numRef) == CFBooleanGetTypeID() )
			value = CFBooleanGetValue((CFBooleanRef)numRef);

		CFRELEASE(numRef);
	}

	return value;
}


//------------------------------------------------------------
//
//	SetIntArray / GetIntArray
//
void TDictionary::SetIntArray(CFStringRef inName, const SInt32 inList[], UInt16 inSize) {
	CFStringRef listRef = CFStringCreateWithIntList(inList, inSize);
	CFDictionarySetValue(dict, inName, listRef);
	CFRELEASE(listRef);
}


UInt16 TDictionary::GetIntArray(CFStringRef inName, SInt32 outList[], UInt16 maxItems) const {
	CFStringRef	inList;
	UInt16 result = 0;
	if (CFDictionaryGetValueIfPresent(dict, inName, (CFTypeRef*)&inList)) {
		CFRETAIN(inList);
		result = CFStringToIntArray(inList, outList, maxItems);
		CFRELEASE(inList);
	}
	return result;
}


CFTypeRef TDictionary::GetValue(CFStringRef inName) const {
	CFTypeRef	typeRef = NULL;

	if (CFDictionaryGetValueIfPresent(dict, (void*)inName, (const void**)&typeRef))
		CFRETAIN(typeRef);

	return typeRef;
}


CFStringRef TDictionary::GetString(CFStringRef inName, CFStringRef defaultVal) const {
	CFStringRef	stringRef = (CFStringRef)GetValue(inName);

	if ( stringRef == NULL )
		stringRef = (CFStringRef)CFRETAIN(defaultVal);

	return stringRef;
}

static char const *cssColor[][2] = {
	{ "AliceBlue", "F0F8FF" },			{ "AntiqueWhite", "FAEBD7" },			{ "Aqua", "0FF" },					{ "Aquamarine", "7FFFD4" },
	{ "Azure", "F0FFFF" },				{ "Beige", "F5F5DC" },					{ "Bisque", "FFE4C4" },				{ "Black", "000" },
	{ "BlanchedAlmond", "FFEBCD" },		{ "Blue", "00F" },						{ "BlueViolet", "8A2BE2" },			{ "Brown", "A52A2A" },
	{ "BurlyWood", "DEB887" },			{ "CadetBlue", "5F9EA0" },				{ "Chartreuse", "7FFF00" },			{ "Chocolate", "D2691E" },
	{ "Coral", "FF7F50" },				{ "CornflowerBlue", "6495ED" },			{ "Cornsilk", "FFF8DC" },			{ "Crimson", "DC143C" },
	{ "Cyan", "0FF" },					{ "DarkBlue", "00008B" },				{ "DarkCyan", "008B8B" },			{ "DarkGoldenRod", "B8860B" },
	{ "DarkGray", "A9A9A9" },			{ "DarkGrey", "A9A9A9" },				{ "DarkGreen", "006400" },			{ "DarkKhaki", "BDB76B" },
	{ "DarkMagenta", "8B008B" },		{ "DarkOliveGreen", "556B2F" },			{ "DarkOrange", "FF8C00" },			{ "DarkOrchid", "9932CC" },
	{ "DarkRed", "8B0000" },			{ "DarkSalmon", "E9967A" },				{ "DarkSeaGreen", "8FBC8F" },		{ "DarkSlateBlue", "483D8B" },
	{ "DarkSlateGray", "2F4F4F" },		{ "DarkSlateGrey", "2F4F4F" },			{ "DarkTurquoise", "00CED1" },		{ "DarkViolet", "9400D3" },
	{ "DeepPink", "FF1493" },			{ "DeepSkyBlue", "00BFFF" },			{ "DimGray", "696969" },			{ "DimGrey", "696969" },
	{ "DodgerBlue", "1E90FF" },			{ "FireBrick", "B22222" },				{ "FloralWhite", "FFFAF0" },		{ "ForestGreen", "228B22" },
	{ "Fuchsia", "F0F" },				{ "Gainsboro", "DCDCDC" },				{ "GhostWhite", "F8F8FF" },			{ "Gold", "FFD700" },
	{ "GoldenRod", "DAA520" },			{ "Gray", "808080" },					{ "GreenYellow", "ADFF2F" },		{ "HoneyDew", "F0FFF0" },
	{ "HotPink", "FF69B4" },			{ "IndianRed", "CD5C5C" },				{ "Indigo", "4B0082" },				{ "Ivory", "FFFFF0" },
	{ "Khaki", "F0E68C" },				{ "Lavender", "E6E6FA" },				{ "LavenderBlush", "FFF0F5" },		{ "LawnGreen", "7CFC00" },
	{ "LemonChiffon", "FFFACD" },		{ "LightBlue", "ADD8E6" },				{ "LightCoral", "F08080" },			{ "LightCyan", "E0FFFF" },
	{ "LightGoldenRodYellow", "FAFAD2" },	{ "LightGray", "D3D3D3" },			{ "LightGrey", "D3D3D3" },			{ "LightGreen", "90EE90" },
	{ "LightPink", "FFB6C1" },			{ "LightSalmon", "FFA07A" },			{ "LightSeaGreen", "20B2AA" },		{ "LightSkyBlue", "87CEFA" },
	{ "LightSlateGray", "789" },		{ "LightSlateGrey", "789" },			{ "LightSteelBlue", "B0C4DE" },		{ "LightYellow", "FFFFE0" },
	{ "Lime", "0F0" },					{ "LimeGreen", "32CD32" },				{ "Linen", "FAF0E6" },				{ "Magenta", "F0F" },
	{ "Maroon", "800000" },				{ "MediumAquaMarine", "66CDAA" },		{ "MediumBlue", "0000CD" },			{ "MediumOrchid", "BA55D3" },
	{ "MediumPurple", "9370D8" },		{ "MediumSeaGreen", "3CB371" },			{ "MediumSlateBlue", "7B68EE" },	{ "MediumSpringGreen", "00FA9A" },
	{ "MediumTurquoise", "48D1CC" },	{ "MediumVioletRed", "C71585" },		{ "MidnightBlue", "191970" },		{ "MintCream", "F5FFFA" },
	{ "MistyRose", "FFE4E1" },			{ "Moccasin", "FFE4B5" },				{ "NavajoWhite", "FFDEAD" },		{ "Navy", "000080" },
	{ "OldLace", "FDF5E6" },			{ "Olive", "808000" },					{ "OliveDrab", "6B8E23" },			{ "Orange", "FFA500" },
	{ "OrangeRed", "FF4500" },			{ "Orchid", "DA70D6" },					{ "PaleGoldenRod", "EEE8AA" },		{ "PaleGreen", "98FB98" },
	{ "PaleTurquoise", "AFEEEE" },		{ "PaleVioletRed", "D87093" },			{ "PapayaWhip", "FFEFD5" },			{ "PeachPuff", "FFDAB9" },
	{ "Peru", "CD853F" },				{ "Pink", "FFC0CB" },					{ "Plum", "DDA0DD" },				{ "PowderBlue", "B0E0E6" },
	{ "Purple", "800080" },				{ "Red", "FF0000" },					{ "RosyBrown", "BC8F8F" },			{ "RoyalBlue", "4169E1" },
	{ "SaddleBrown", "8B4513" },		{ "Salmon", "FA8072" },					{ "SandyBrown", "F4A460" },			{ "SeaGreen", "2E8B57" },
	{ "SeaShell", "FFF5EE" },			{ "Sienna", "A0522D" },					{ "Silver", "C0C0C0" },				{ "SkyBlue", "87CEEB" },
	{ "SlateBlue", "6A5ACD" },			{ "SlateGray", "708090" },				{ "SlateGrey", "708090" },			{ "Snow", "FFFAFA" },
	{ "SpringGreen", "00FF7F" },		{ "SteelBlue", "4682B4" },				{ "Tan", "D2B48C" },				{ "Teal", "008080" },
	{ "Thistle", "D8BFD8" },			{ "Tomato", "FF6347" },					{ "Turquoise", "40E0D0" },			{ "Violet", "EE82EE" },
	{ "Wheat", "F5DEB3" },				{ "White", "FFF" },						{ "WhiteSmoke", "F5F5F5" },			{ "Yellow", "FF0" },
	{ "YellowGreen", "9ACD32" } };

RGBColor TDictionary::GetColor(CFStringRef inName, CFStringRef defaultVal) const {
	CFStringRef	stringRef = (CFStringRef)GetValue(inName);

	if ( stringRef == NULL )
		stringRef = (CFStringRef)CFRETAIN(defaultVal);


	int r = 0, g = 0, b = 0;
	char buff[30];
	if (CFStringGetCString(stringRef, buff, 10, kCFStringEncodingMacRoman)) {
		int l = strlen(buff);

		// Test for a valid hex color
		bool isHex = false;
		if (l == 3 || l == 6) {
			isHex = true;
			for (int i=l; i--;) {
				if ( !isxdigit(buff[i]) ) {
					isHex = false;
					break;
				}
			}
		}

		// Not hex color, look up the name
		if (!isHex) {
			for (UInt16 col=0; col<COUNT(cssColor); col++) {
				const char *name = cssColor[col][0];
				if (0 == strcasecmp(name, buff)) {
					strcpy(buff, cssColor[col][1]);
					isHex = true;
				}
			}
		}

		// If we finally have some hex, get the RGB
		if (isHex) {
			int c;
			sscanf(buff, "%x", &c);
			switch (strlen(buff)) {
				case 3:
					r = 17 * ((c >> 8) & 0x0F);
					g = 17 * ((c >> 4) & 0x0F);
					b = 17 * (c & 0x0F);
					break;
				case 6:
					r = (c >> 16) & 0xFF;
					g = (c >> 8) & 0xFF;
					b = c & 0xFF;
					break;
			}
		}
	}

	RGBColor color = { r * 65535 / 255, g * 65535 / 255, b * 65535 / 255 };
	return color;
}


CGPoint TDictionary::GetPoint(CFStringRef inName, CGPoint inPoint) const {
	CGPoint point = inPoint;
	CFDictionaryRef dict = GetDictionary(inName);

	if (dict) {
		TDictionary tdict(dict);
		point = CGPointMake(tdict.GetFloat(CFSTR("X")),tdict.GetFloat(CFSTR("Y")));
	}

	return point;
}

