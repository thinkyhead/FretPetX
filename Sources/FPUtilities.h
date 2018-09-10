/*
 *  FPUtilities.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPUTILITIES_H
#define FPUTILITIES_H

#define HUGE_ICONS 0
#define ICON_SIZE (HUGE_ICONS ? 128 : 32)
#define BUFFER_CHUNK_SIZE 4096

void				AlertError(OSErr err, CFStringRef msg);
DialogItemIndex		RunAlert(OSErr err, const char *msg);
DialogItemIndex		RunPrompt(CFStringRef inHead, CFStringRef inMessage=NULL, CFStringRef inDefault=NULL, CFStringRef inCancel=NULL, CFStringRef inOther=NULL);

void				pstrcat(StringPtr destStr, const StringPtr sourceStr);
void				pstrcpy(StringPtr destStr, const StringPtr sourceStr);
void				c2pstrncpy(StringPtr destStr, const char* sourceStr, SInt16 len);
void				c2pstrncat(StringPtr destStr, const char* sourceStr, SInt16 len);
void				c2pstrcat(StringPtr destStr, const char* sourceStr);
bool				pstrcmp(const StringPtr destStr, const StringPtr sourceStr);
void				prtrim(StringPtr string);

char*				ImagePath(const char *filename);

void				SetFont(FontInfo *fontInfo, const ConstStr255Param fontName, short size, short style, CGContextRef gc=NULL);

void				MarkMenuItem(MenuRef menu, MenuItemIndex index, bool mark);

MenuRef				GetMiniMenu(CFStringRef name);
OSErr				SetMenuContentsRefcon(MenuRef inMenu, UInt32 inRefCon);

int					CircleSegment(Point where, float circleR, float minR, float maxR, UInt16 segCount);

					// QuickDraw - deprecated!
GWorldPtr			MakeGWorldWithPicture(PicHandle hPict, UInt16 depth=16);
GWorldPtr			MakeGWorldWithPictResource(short pictID, short depth=16);
IconRef				MakeIconWithPicture(PicHandle hPict);
Handle				GetIconDataHandle(PicHandle pict);
Handle				GetIconMaskHandle(PicHandle pict);

short				CurrentDepth();

					//! Draw a string centered in a box, breaking it up into multiple lines if necessary.
void				DrawStringInBox(const StringPtr string, const Rect *pRect);

UInt64				Micro64();
Rect				MoveRect(Rect &rect, short x, short y);
bool				RectsOverlap(Rect &rect1, Rect &rect2);

CFMutableStringRef	CFStringCombine(CFStringRef srcRef, CFStringRef appRef);
char*				CFStringToCString(const CFStringRef sref);
bool				CFStringEndsWith(const CFStringRef inString, const CFStringRef inExt);
CFStringRef			CFStringTrimExtension(const CFStringRef inString);
CFStringRef			CFStringCreateWithIntList(const SInt32 inList[], UInt16 inSize);
UInt16				CFStringToIntArray(CFStringRef inString, SInt32 outList[], UInt16 maxItems);
void				CFStringPrint(const CFStringRef sref);

CFDataRef			AliasToCFData(AliasHandle alias);
AliasHandle			CFDataToAlias(CFDataRef aliasData);

void				PrintPath(CFURLRef uref);

#if DEBUG_REFCOUNTS
	CFTypeRef		MyCFRetain(CFTypeRef cf);
	void			MyCFRelease(CFTypeRef cf);
#endif

bool				ExpandHandleForSize(Handle h, UInt32 progress);

CFPropertyListRef	GetPlistFromFile( CFURLRef fileURL );
CFStringRef			CreateCFStringByExpandingTildePath(CFStringRef path);

#if !defined(PtInRect)
Boolean				PtInRect(const Point inPoint, const Rect *inRect);
#endif

#if !defined(GetPort)
void				GetPort(GrafPtr *oldPort);
#endif

#endif
