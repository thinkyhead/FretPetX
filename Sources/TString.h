/*!
	@file TString.h

	String wrapper class to make the code prettier.

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef TSTRING_H
#define TSTRING_H

#include "FPUtilities.h"
#include "FPMacros.h"

class TString {
	public:
		CFMutableStringRef	string;			// The string

							TString();
							TString(const TString &tstring)		{ string = CFStringCreateMutable(kCFAllocatorDefault, 0); *this = tstring; }
							TString(const CFStringRef cfstring);
							TString(const CFStringRef cfstring, CFRange range);
							TString(const char* cstring, bool isPascal=false);
							TString(const HFSUniStr255 &hstring);
							TString(const UniChar* &ustring, UInt16 length);
							TString(const long number);
		virtual				~TString();

		TString&			operator=(const TString &tstring);
		TString&			operator=(const char* cstring);
		TString&			operator=(const CFStringRef cfstring);
		TString&			operator=(const HFSUniStr255 &hstring);
		TString&			operator=(const double number);
		TString&			operator=(const float number)			{ return operator=((double)number); }
		TString&			operator=(const long number);
		TString&			operator=(const short number)			{ return operator=((long)number); }
		TString&			operator=(const unsigned short number)	{ return operator=((long)number); }
		TString&			operator=(const int number)				{ return operator=((long)number); }
		TString&			operator=(const unsigned int number)	{ return operator=((long)number); }

		void				SetFromPascalString(StringPtr p);
		void				SetWithFormat(const CFStringRef cfstring, ...);
		void				SetLocalized(const CFStringRef cfstring);
		void				SetLocalized(const char* cstring);
		void				SetWithFormatLocalized(const CFStringRef fmtString, ...);
		void				SetWithLiteral(const long literal);

		static TString*		NewWithFormat(const CFStringRef cfstring, ...);

		char				operator[](int index);

		TString&			operator+=(const TString &tstring);
		TString&			operator+=(const char* cstring);
		TString&			operator+=(const CFStringRef cfstring);
		TString&			operator+=(const HFSUniStr255 &hstring);
		TString&			operator+=(const double number);
		TString&			operator+=(const float number)			{ return operator+=((double)number); }
		TString&			operator+=(const long number);
		TString&			operator+=(const short number)			{ return operator+=((long)number); }
		TString&			operator+=(const unsigned short number)	{ return operator+=((long)number); }
		TString&			operator+=(const int number)			{ return operator+=((long)number); }
		TString&			operator+=(const unsigned int number)	{ return operator+=((long)number); }

		TString&			operator+(const TString &right);
		TString&			operator+(const char* cstring);
		TString&			operator+(const CFStringRef cfstring);
		TString&			operator+(const HFSUniStr255 &hstring);
		TString&			operator+(const double number);
		TString&			operator+(const float number)			{ return operator+((double)number); }
		TString&			operator+(const long number);
		TString&			operator+(const short number)			{ return operator+((long)number); }
		TString&			operator+(const unsigned short number)	{ return operator+((long)number); }
		TString&			operator+(const int number)				{ return operator+((long)number); }
		TString&			operator+(const unsigned int number)	{ return operator+((long)number); }

		int					operator==(const TString &tstring) const;
		int					operator==(const char* cstring) const;
		int					operator==(const CFStringRef cfstring) const;
		int					operator==(const HFSUniStr255 &hstring) const;
		int					operator==(const double number) const;
		int					operator==(const float number) const			{ return (*this == (double)number); }
		int					operator==(const long number) const;
		int					operator==(const short number) const			{ return (*this == (long)number); }
		int					operator==(const unsigned short number) const	{ return (*this == (long)number); }
		int					operator==(const int number) const				{ return (*this == (long)number); }
		int					operator==(const unsigned int number) const		{ return (*this == (long)number); }

		int					operator!=(const TString &tstring) const		{ return !(*this == tstring); }
		int					operator!=(const char* cstring) const			{ return !(*this == cstring); }
		int					operator!=(const CFStringRef cfstring) const	{ return !(*this == cfstring); }
		int					operator!=(const HFSUniStr255 &hstring) const	{ return !(*this == hstring); }
		int					operator!=(const double number) const			{ return !(*this == number); }
		int					operator!=(const float number) const			{ return !(*this == number); }
		int					operator!=(const long number) const				{ return !(*this == number); }
		int					operator!=(const short number) const			{ return !(*this == number); }
		int					operator!=(const unsigned short number) const	{ return !(*this == number); }
		int					operator!=(const int number) const				{ return !(*this == number); }
		int					operator!=(const unsigned int number) const		{ return !(*this == number); }

		UInt16				Length() const;
		inline bool			IsEmpty() const									{ return Length() == 0; }

		void				Print(FILE *output=stdout) const;
		void				Draw() const;
		void				Draw(CGContextRef context) const;
		void				Draw(CGContextRef context, float x, float y) const;

		void				Draw(	CGContextRef const inContext,
									const Rect &bounds,
									ThemeFontID const fontID=kThemeCurrentPortFont,
									ThemeDrawState const drawState=kThemeStateActive,
									SInt16 const inJust = teFlushLeft) const;

		void				DrawWrapped(
									CGContextRef const inContext,
									const Rect &bounds,
									ThemeFontID const fontID=kThemeCurrentPortFont,
									ThemeDrawState const drawState=kThemeStateActive,
									SInt16 const inJust = teFlushLeft) const;

		Point				GetDimensions(	ThemeFontID const inFontID=kThemeCurrentPortFont,
											ThemeDrawState const inState=kThemeStateActive,
											const SInt16 inWrapWidth=0,
											SInt16 * const outBaseline=NULL) const;

		inline CFStringRef	GetCFStringRef() const					{ return string; }
		CFStringRef			GetCFString() const;
		CFStringRef			GetLocalizedCopy() const;
		char*				GetCString() const;
		Boolean				GetCString(char *cstring, CFIndex size) const
			{ return CFStringGetCString(string, cstring, size, kCFStringEncodingMacRoman); }
		StringPtr			GetPascalString() const;
		Boolean				GetPascalString(StringPtr pstring, CFIndex size) const
			{ return CFStringGetPascalString(string, pstring, size, kCFStringEncodingMacRoman); }

		inline double		DoubleValue() const						{ return CFStringGetDoubleValue(string); }
		inline SInt32		IntValue() const						{ return CFStringGetIntValue(string); }

		void				GetHFSUniStr255(HFSUniStr255 *hstring) const;
		void				GetUnicode(UniChar **pUnichar, UInt16 *length) const;

		void				Release()								{ if (string) { CFRelease(string); string = NULL; } }

		// Convert and modify
		void				Localize();
		void				Trim();
		void				Trim(const CFStringRef trimString);
		UInt16				Replace(CFStringRef stringToFind, CFStringRef replacementString, CFIndex start=0, CFIndex length=0);
		void				ConcatLeft(CFIndex length);
		void				ConcatRight(CFIndex length);
		void				MakeSubstring(CFIndex start, CFIndex length);
		TString				Left(CFIndex length=1) const;
		TString				Right(CFIndex length=1) const;
		TString				Substring(CFIndex start, CFIndex length) const;
		void				ChopLeft(CFIndex length=1);
		void				ChopRight(CFIndex length=1);
		void				Uppercase();
		void				LowerCase();
		void				Prepend(CFStringRef inString);

		// Matching and testing
		bool				EndsWith(const CFStringRef inCompare, bool caseInsensitive=false);
};

#endif
