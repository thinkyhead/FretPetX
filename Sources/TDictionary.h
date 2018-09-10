/*!
	@file TDictionary.h

	TDictionary wraps the CFMutableDictionary core foundation class

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#ifndef TDICTIONARY_H
#define TDICTIONARY_H

class TDictionary {
	private:

		CFMutableDictionaryRef	dict;

	public:
								TDictionary(CFIndex maxSize=1)								{ Init(maxSize); }
								TDictionary(const TDictionary &src)							{ dict = NULL; *this = src; }
								TDictionary(CFDictionaryRef inDict)							{ dict = NULL; *this = inDict; }
								TDictionary(CFPropertyListRef inPlist)						{ dict = NULL; *this = (CFDictionaryRef)inPlist; }
								TDictionary(CFMutableDictionaryRef inDict)					{ dict = NULL; *this = inDict; }
		virtual					~TDictionary();

		void					Init(CFIndex maxSize=1);

		TDictionary&			operator=(const TDictionary &src);
		TDictionary&			operator=(CFDictionaryRef &src);
		TDictionary&			operator=(CFMutableDictionaryRef &src);

		inline bool				IsValid() const												{ return (dict != NULL); }
		inline bool				IsValidPropertyList(CFPropertyListFormat format)			{ return IsValid() && CFPropertyListIsValid(dict, format); }

		inline CFIndex			Count()														{ return IsValid() ? CFDictionaryGetCount(dict) : 0; }

		bool					ContainsKey(CFStringRef inName) const						{ return IsValid() && CFDictionaryContainsKey(dict, inName); }

		bool					GetBool(CFStringRef inName, bool defaultVal=false) const;
		SInt32					GetInteger(CFStringRef inName, SInt32 defaultVal=0) const;
		float					GetFloat(CFStringRef inName, float defaultVal=0.0f) const;
		double					GetDouble(CFStringRef inName, double defaultVal=0.0f) const;
		CFTypeRef				GetValue(CFStringRef inName) const;
		CFStringRef				GetString(CFStringRef inName, CFStringRef defaultVal=CFSTR("")) const;
		CFArrayRef				GetArray(CFStringRef inName) const							{ return (CFArrayRef)GetValue(inName); }
		UInt16					GetIntArray(CFStringRef inName, SInt32 outList[], UInt16 maxItems) const;
		CFDictionaryRef			GetDictionary(CFStringRef inName) const						{ return (CFDictionaryRef)GetValue(inName); }
		CGPoint					GetPoint(CFStringRef inName, CGPoint inPoint=CGPointZero) const;
		RGBColor				GetColor(CFStringRef inName, CFStringRef defaultVal=CFSTR("000")) const;

		void					SetBool(CFStringRef inName, bool val);
		void					SetInteger(CFStringRef inName, SInt32 val=0);
		void					SetFloat(CFStringRef inName, float val=0.0f);
		void					SetDouble(CFStringRef inName, double val=0.0f);
		inline void				SetValue(CFStringRef inName, CFTypeRef inVal)				{ CFDictionarySetValue(dict, inName, inVal); }
		void					SetArray(CFStringRef inName, CFArrayRef inArray)			{ SetValue(inName, inArray); }
		void					SetString(CFStringRef inName, CFStringRef inVal)			{ SetValue(inName, inVal); }
		void					SetDictionary(CFStringRef inName, CFDictionaryRef inDict)	{ SetValue(inName, inDict); }
		void					SetDictionary(CFStringRef inName, TDictionary &inDict)		{ SetValue(inName, inDict.GetDictionaryRef()); }
		void					SetPoint(CFStringRef inName, CGPoint point);
		void					SetIntArray(CFStringRef inName, const SInt32 inList[], UInt16 inSize);

		CFMutableDictionaryRef	GetDictionaryRef() const	{ return dict; }
};

#endif