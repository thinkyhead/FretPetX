/*!
	@file TString.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "TString.h"
#include "FPUtilities.h"

#include <stdarg.h>

TString::TString() {
	string = CFStringCreateMutable(kCFAllocatorDefault, 0);
}


TString::TString(const CFStringRef cfstring) {
	string = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, cfstring);
}


TString::TString(const CFStringRef inString, CFRange range) {
	const CFStringRef static_string = CFStringCreateWithSubstring(kCFAllocatorDefault, inString, range);
	string = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, static_string);
}


TString::TString(const char* cstring, bool isPascal) {
	string = CFStringCreateMutable(kCFAllocatorDefault, 0);

	if (isPascal)
		CFStringAppendPascalString(string, (StringPtr)cstring, kCFStringEncodingMacRoman);
	else
		CFStringAppendCString(string, cstring, kCFStringEncodingMacRoman);
}


TString::TString(const HFSUniStr255 &hstring) {
	string = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCharacters(string, hstring.unicode, hstring.length);
}


TString::TString(const UniChar* &ustring, UInt16 length) {
	string = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendCharacters(string, ustring, length);
}


TString::TString(const long number) {
	string = CFStringCreateMutable(kCFAllocatorDefault, 0);
	CFStringAppendFormat(string, NULL, CFSTR("%ld"), number);
}


TString::~TString() {
	Release();
}


#pragma mark -
TString& TString::operator=(const TString &tstring) {
	if (this != &tstring)
		CFStringReplaceAll(string, tstring.string);

	return *this;
}


TString& TString::operator=(const char* cstring) {
	CFStringRef cfstring = CFStringCreateWithCString(kCFAllocatorDefault, cstring, kCFStringEncodingMacRoman);
	CFStringReplaceAll(string, cfstring);
	CFRELEASE(cfstring);
	return *this;
}


TString& TString::operator=(const double number) {
	CFStringRef cfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%f"), number);
	CFStringReplaceAll(string, cfstring);
	CFRELEASE(cfstring);
	return *this;
}


TString& TString::operator=(const long number) {
	CFStringRef cfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), number);
	CFStringReplaceAll(string, cfstring);
	CFRELEASE(cfstring);
	return *this;
}


TString& TString::operator=(const CFStringRef cfstring) {
	CFStringReplaceAll(string, cfstring);
	return *this;
}


TString* TString::NewWithFormat(const CFStringRef fmtString, ...) {
	TString *tstring = new TString;

    va_list argList;
    va_start(argList, fmtString);

    CFStringRef newString = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, fmtString, argList);
	*tstring = newString;
	CFRELEASE(newString);

    va_end(argList);

	return tstring;
}


void TString::SetWithFormat(const CFStringRef fmtString, ...) {
    va_list argList;
    va_start(argList, fmtString);

    CFStringRef newString = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, fmtString, argList);
	CFStringReplaceAll(string, newString);
	CFRELEASE(newString);

    va_end(argList);
}


void TString::SetLocalized(const CFStringRef cfstring) {
	CFStringRef localString = CFCopyLocalizedString(cfstring, "Set with the localized version of a string");
	*this = localString;
	CFRELEASE(localString);
}

void TString::SetLocalized(const char* cstring) {
	*this = cstring;
	Localize();
}



void TString::SetWithFormatLocalized(const CFStringRef fmtString, ...) {
	CFStringRef localString = CFCopyLocalizedString(fmtString, "Set with the localized version of a string");

    va_list argList;
    va_start(argList, fmtString);
    CFStringRef newString = CFStringCreateWithFormatAndArguments(kCFAllocatorDefault, NULL, localString, argList);
    va_end(argList);

	CFStringReplaceAll(string, newString);
	CFRELEASE(newString);
	CFRELEASE(localString);
}


void TString::SetWithLiteral(const long literal) {
	CFStringRef litString = CFStringCreateWithFormat(NULL, NULL, CFSTR("%c%c%c%c"), FOUR_CHAR(literal));
	CFStringReplaceAll(string, litString);
	CFRELEASE(litString);
}


#pragma mark -
char TString::operator[](int index) {
	if (index < Length()) {
		char *cstring = this->GetCString();
		return cstring[index];
	}
	else
		return '\0';
}

TString& TString::operator+(const TString &tstring) {
	CFStringAppend(string, tstring.string);
	return *this;
}


TString& TString::operator+(const char* cstring) {
	CFStringAppendCString(string, cstring, kCFStringEncodingMacRoman);
	return *this;
}


TString& TString::operator+(const CFStringRef cfstring) {
	CFStringAppend(string, cfstring);
	return *this;
}


TString& TString::operator+(const long number) {
	CFStringRef cfstring = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%ld"), number);
	CFStringAppend(string, cfstring);
	CFRELEASE(cfstring);
	return *this;
}


TString& TString::operator+=(const TString &tstring) {
	return operator+(tstring);
}


TString& TString::operator+=(const char *cstring) {
	return  operator+(cstring);
}


TString& TString::operator+=(const CFStringRef cfstring) {
	return  operator+(cfstring);
}


TString& TString::operator+=(const long number) {
	return  operator+(number);
}


int TString::operator==(const TString &tstring) const {
	if (this == &tstring)
		return true;

	return *this == tstring.string;
}


int TString::operator==(const CFStringRef cfstring) const {
	if (Length() == CFStringGetLength(cfstring)) {
		CFRange range = CFStringFind(string, cfstring, 0);
		return (range.length != 0);
	}

	return false;
}

int TString::operator==(const char *cstring) const {
	int result = 0;
	const char *mystring = GetCString();
	if (mystring) {
		result = (strcmp(mystring, cstring) == 0);
		delete [] mystring;
	}
	return result;
}

int TString::operator==(const long number) const {
	return IntValue() == number;
}


int TString::operator==(const double number) const {
	return DoubleValue() == number;
}


void TString::Print(FILE *output) const {
	char *cstring = GetCString();
	if (cstring) {
		fprintf(output, "%s", cstring);
		delete [] cstring;
	}
}


void TString::Draw() const {
	StringPtr pstring = GetPascalString();
	if (pstring) {
//		DrawString(pstring);
		delete [] pstring;
	}
}


void TString::Draw(CGContextRef context) const {
	char *cstring = GetCString();
	if (cstring) {
		CGContextShowText(context, cstring, Length());
		delete [] cstring;
	}
}


void TString::Draw(CGContextRef context, float x, float y) const {
	char *cstring = GetCString();
	if (cstring) {
		CGContextShowTextAtPoint(context, x, y, cstring, Length());
		delete [] cstring;
	}
}


void TString::Draw( CGContextRef const inContext, const Rect &bounds, ThemeFontID const fontID, ThemeDrawState const drawState, SInt16 const inJust ) const {
	(void)DrawThemeTextBox( string, fontID, drawState, false, &bounds, inJust, inContext );
}


void TString::DrawWrapped( CGContextRef const inContext, const Rect &bounds, ThemeFontID const fontID, ThemeDrawState const drawState, SInt16 const inJust ) const {
	(void)DrawThemeTextBox( string, fontID, drawState, true, &bounds, inJust, inContext );
}


Point TString::GetDimensions(ThemeFontID const inFontID, ThemeDrawState const inState, const SInt16 inWrapWidth, SInt16 * const outBaseline) const {
	Point	textSize = { 0, inWrapWidth };
	SInt16	baseline;

	(void)GetThemeTextDimensions( string, inFontID, inState, (inWrapWidth != 0), &textSize, &baseline );

	if (outBaseline != NULL)
		*outBaseline = baseline;

	return textSize;
}


#pragma mark -
UInt16 TString::Length() const {
	return CFStringGetLength(string);
}


void TString::Trim() {
	CFStringTrimWhitespace(string);
}


void TString::Trim(const CFStringRef trimString) {
	CFStringTrim(string, trimString);
}


CFStringRef TString::GetCFString() const {
	return CFStringCreateCopy(kCFAllocatorDefault, string);
}


void TString::Localize() {
	CFStringRef localString = CFCopyLocalizedString(string, "Replace string wrapper contents with a localized string");
	*this = localString;
	CFRELEASE(localString);
}


CFStringRef TString::GetLocalizedCopy() const {
	return CFCopyLocalizedString(string, "Return localized version of string in wrapper");
}


char* TString::GetCString() const {
	CFIndex	size = Length();
	char	*cstring = new char[size+1];

	if (!CFStringGetCString(string, cstring, size+1, kCFStringEncodingMacRoman)) {
		delete [] cstring;
		cstring = NULL;
	}

	return cstring;
}


StringPtr TString::GetPascalString() const {
	CFIndex			size = Length();
	if (size > 255)	size = 255;
	StringPtr		pstring = new unsigned char[size+2];

	pstring[size+1] = '\0';

	if (!CFStringGetPascalString(string, pstring, size+2, kCFStringEncodingMacRoman)) {
		delete [] pstring;
		pstring = NULL;
	}

	return pstring;
}


void TString::SetFromPascalString(StringPtr p) {
	*this = CFSTR("");
	CFStringAppendPascalString(string, p, kCFStringEncodingMacRoman);
}


void TString::GetHFSUniStr255(HFSUniStr255 *hstring) const {
	hstring->length = CFStringGetBytes(string, CFRangeMake(0, Length()), kCFStringEncodingUnicode, 0, false, (UInt8*)hstring->unicode, sizeof(hstring->unicode), NULL);
}


void TString::GetUnicode(UniChar **pUnichar, UInt16 *length) const {
}


#pragma mark -
UInt16 TString::Replace(CFStringRef stringToFind, CFStringRef replacementString, CFIndex start, CFIndex inLength) {
	CFRange range = { start ? start : 1, inLength ? inLength : Length() };
	return (UInt16) CFStringFindAndReplace(string, stringToFind, replacementString, range, kCFCompareCaseInsensitive);
}


void TString::ConcatLeft(CFIndex inLength) {
	if (inLength < Length())
		CFStringDelete(string, CFRangeMake(inLength, Length() - inLength));
	else
		*this = CFSTR("");
}


void TString::ConcatRight(CFIndex inLength) {
	if (inLength < Length())
		CFStringDelete(string, CFRangeMake(0, Length() - inLength));
	else
		*this = CFSTR("");
}


void TString::MakeSubstring(CFIndex start, CFIndex inLength) {
	CFStringRef newString = CFStringCreateWithSubstring(kCFAllocatorDefault, string, CFRangeMake(start, inLength));
	*this = newString;
	CFRELEASE(newString);
}


TString TString::Left(CFIndex inLength) const {
	TString newString(string);
	newString.ConcatLeft(inLength);
	return newString;
}


TString TString::Right(CFIndex inLength) const {
	TString newString(string);
	newString.ConcatRight(inLength);
	return newString;
}


TString TString::Substring(CFIndex start, CFIndex inLength) const {
	TString newString(string);
	newString.MakeSubstring(start, inLength);
	return newString;
}


void TString::ChopLeft(CFIndex inLength) {
	CFStringDelete(string, CFRangeMake(0, inLength));
}


void TString::ChopRight(CFIndex inLength) {
	CFStringDelete(string, CFRangeMake(Length() - inLength, inLength));
}


void TString::Uppercase() {
	CFStringUppercase(string, 0);
}


void TString::LowerCase() {
	CFStringLowercase(string, 0);
}


void TString::Prepend(CFStringRef inString) {
	CFStringRef newString = CFStringCreateWithFormat(kCFAllocatorDefault, NULL, CFSTR("%@%@"), inString, string);
	*this = newString;
	CFRELEASE(newString);
}


#pragma mark Test and Compare


bool TString::EndsWith(const CFStringRef inCompare, bool caseInsensitive) {
	CFIndex	length = Length();
	CFIndex	cmplen = CFStringGetLength(inCompare);
	CFRange	range = { length - cmplen, cmplen };
	CFOptionFlags flags = kCFCompareAnchored | (caseInsensitive ? 0 : kCFCompareCaseInsensitive);

	return CFStringFindWithOptions( string, inCompare, range, flags, NULL );
}
