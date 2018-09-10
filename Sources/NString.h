//
//  String.h
//
//  This file declares a friendly C++ wrapper for NSString/CFString.
//  It must (of course) be used only from .mm files.
//
//  Created by Joe Strout on 5/4/10.
//  Copyright 2010 InspiringApps. All rights reserved.
//

#import <Foundation/Foundation.h>

class String {
protected:
    NSString *obj;
public:
    // constructors (including conversion to String) & destructor
    String(void) { obj = nil; }
    String(const String& s) { obj = nil; *this = s; }
    String(NSString *o) { obj = nil; *this = o; }
    String(CFStringRef o) { obj = nil; *this = (NSString *)o; }
    String(const char *c) { obj = nil; *this = c; }
    ~String(void) { [obj release]; }
	
	// inspectors
	size_t length() const { return obj.length; }
    bool empty() const { return !obj or !obj.length; }
    inline long find(const String &s, long pos=0, NSStringCompareOptions opts = NSLiteralSearch) const;
    inline long rfind(const String &s, long pos=-1, NSStringCompareOptions opts = NSLiteralSearch) const;
	inline String substr(long pos=0, long length=-1) const;
	inline String getExtension(const char delim='.') const;
    inline String replaceAll(const String &find, const String &replaceWith, NSStringCompareOptions opts = NSLiteralSearch) const;
    
    // assignment operators
    inline String& operator= (const String& s);
    inline String& operator= (NSString *o);
    inline String& operator= (CFStringRef o);
    inline String& operator= (const char* c);
    
    // conversion from String to other formats
    operator NSString *(void) const {
        return [[obj retain] autorelease];
    }
    
    operator CFStringRef (void) const {
        return (CFStringRef)[[obj retain] autorelease];
    }

	operator const char *(void) const {
        return [obj cStringUsingEncoding:NSASCIIStringEncoding];
    }

    // comparison functions and operators
    int strcomp(const String& s) const {
        if (obj) {
            if (s.obj) return [obj localizedCompare:s.obj];
            return 1;
        } else {
            if (s.obj) return -1;
            return 0;
        }
    }
    
    BOOL operator== (const String& s) const { return strcomp(s) == 0; }    
    BOOL operator!= (const String& s) const { return strcomp(s) != 0; }
    BOOL operator> (const String& s) const { return strcomp(s) > 0; }
    BOOL operator< (const String& s) const { return strcomp(s) < 0; }
    BOOL operator>= (const String& s) const { return strcomp(s) >= 0; }
    BOOL operator<= (const String& s) const { return strcomp(s) <= 0; }
    
    BOOL operator== (const char *c) const { return *this == String(c); }
    BOOL operator!= (const char *c) const { return *this != String(c); }
    BOOL operator> (const char *c) const { return *this > String(c); }
    BOOL operator< (const char *c) const { return *this < String(c); }
    BOOL operator>= (const char *c) const { return *this >= String(c); }
    BOOL operator<= (const char *c) const { return *this <= String(c); }
    
    BOOL operator== (NSString *c) const { return *this == String(c); }
    BOOL operator!= (NSString *c) const { return *this != String(c); }
    BOOL operator> (NSString *c) const { return *this > String(c); }
    BOOL operator< (NSString *c) const { return *this < String(c); }
    BOOL operator>= (NSString *c) const { return *this >= String(c); }
    BOOL operator<= (NSString *c) const { return *this <= String(c); }
    
    // Concatenation
    String& operator+= (const String& s) {
        if (!s.obj) return *this;
        if (!obj) *this = s;
        else *this = [obj stringByAppendingString:s.obj];
        return *this;
    }
    
    String operator+ (const String& s) const {
        if (!s.obj) return *this;
        if (!obj) return s;
        return String([obj stringByAppendingString:s.obj]);
    }
    
    // Handy mutators
    inline String stripExtension(const char delim='.');
    
    // String versions of common global functions
    //    friend void NSLog(String& s, ...) { va_list args; NSLogv(s.obj, args); }
    
    // Unit tests.
    static void runUnitTests();
};

String& String::operator= (const String& s) {
    [s.obj retain];
    [obj release];
    obj = s.obj;
    return *this;
}

String& String::operator= (NSString *o) {
    [o retain];
    [obj release];
    obj = o;
    return *this;
}

String& String::operator= (CFStringRef o) {
    NSString *nso = (NSString *)o;
    [nso retain];
    [obj release];
    obj = nso;
    return *this;
}

String& String::operator= (const char *c) {
    [obj release];
    obj = [[NSString alloc] initWithUTF8String:c];
    return *this;
}

String String::substr(long pos, long length) const {
	
    if (pos < 0) pos += obj.length;
	if (length == -1) length = obj.length - pos;
	
	if (pos < 0 || length < 1) return String();
    
	NSRange range = {pos, length};
	return [obj substringWithRange:range];
}

long String::find(const String &s, long pos, NSStringCompareOptions opts) const {
    if (!obj) return -1;
    if (pos < 0) pos += obj.length;
    NSRange found = [obj rangeOfString:s.obj options:opts];
    if (found.location == NSNotFound) return -1;
    return found.location;
}

long String::rfind(const String &s, long pos, NSStringCompareOptions opts) const {
    if (!obj) return -1;
    if (pos < 0) pos += obj.length;
    NSRange found = [obj rangeOfString:s.obj options:opts | NSBackwardsSearch];
    if (found.location == NSNotFound) return -1;
    return found.location;
}

inline String String::replaceAll(const String &find, const String &replaceWith, NSStringCompareOptions opts) const {
    if (!obj) return *this;
    NSRange all = {0, obj.length};
    return [obj stringByReplacingOccurrencesOfString:find
                                          withString:replaceWith
                                             options:opts
                                               range:all];
}

String String::stripExtension(const char delim) {
    // Strip off the extension from the end of this string,
    // and return it.
    char dc[2] = ".";
    dc[0] = delim;
    long pos = rfind(dc);
    if (pos < 0) {
        // Didn't find the extension.
        return "";
    }
    String out = substr(pos+1);
    *this = substr(0, pos);
    return out;
}

String String::getExtension(const char delim) const {
    char dc[2] = ".";
    dc[0] = delim;
    long pos = rfind(dc);
    if (pos < 0) {
        // Didn't find the extension.
        return "";
    }
    String out = substr(pos+1);
    return out;
	
}
