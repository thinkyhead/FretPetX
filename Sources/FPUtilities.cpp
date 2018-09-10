/*!
	@file FPUtilities.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPUtilities.h"
#include "FPMacros.h"
#include "TString.h"

extern long systemVersion;

#define CG_RGBCOLOR(x)		(float)(x)->red / 65535.0, (float)(x)->green / 65535.0, (float)(x)->blue / 65535.0

static const OSType kFakeCreator = CREATOR_CODE, kFakeType = '    ';


void AlertError(OSErr err, CFStringRef inMessage) {
	TString			errString, msgString;

	if (err != noErr)
		errString.SetWithFormatLocalized(CFSTR("Error %d"), err);
	else
		errString.SetLocalized(CFSTR("An unspecified error occurred."));

	msgString.SetLocalized(inMessage);

	DialogRef		alert;
	DialogItemIndex	itemHit;
	CreateStandardAlert(kAlertStopAlert, errString.GetCFStringRef(), msgString.GetCFStringRef(), NULL, &alert);
	(void)RunStandardAlert(alert, NULL, &itemHit);
}



DialogItemIndex RunAlert(OSErr err, const char *inMessage) {
	TString			errString, msgString;

	if (err != noErr)
		errString.SetWithFormatLocalized(CFSTR("Error %d"), err);
	else
		errString.SetLocalized(CFSTR("An unspecified error occurred."));

	msgString.SetLocalized(inMessage);

	DialogRef		alert;
	DialogItemIndex	itemHit = -1;
	CreateStandardAlert(kAlertStopAlert, errString.GetCFStringRef(), msgString.GetCFStringRef(), NULL, &alert);
	(void)RunStandardAlert(alert, NULL, &itemHit);
	return itemHit;
}



DialogItemIndex RunPrompt(CFStringRef inHead, CFStringRef inMessage, CFStringRef inDefault, CFStringRef inCancel, CFStringRef inOther) {
	TString headString, msgString, defaultString, cancelString, otherString;
	headString.SetLocalized(inHead);
	msgString.SetLocalized(inMessage);
	defaultString.SetLocalized(inDefault);
	cancelString.SetLocalized(inCancel);
	otherString.SetLocalized(inOther);
	
	AlertStdCFStringAlertParamRec	param;
	(void)GetStandardAlertDefaultParams(&param, kStdCFStringAlertVersionOne);
	param.defaultText	= defaultString.Length() ? defaultString.GetCFStringRef() : NULL;
	param.cancelText	= cancelString.Length() ? cancelString.GetCFStringRef() : NULL;
	param.otherText		= otherString.Length() ? otherString.GetCFStringRef() : NULL;
	param.cancelButton	= 2;

	DialogRef		alert;
	DialogItemIndex	itemHit;
	CreateStandardAlert(kAlertNoteAlert, headString.GetCFStringRef(), msgString.Length() ? msgString.GetCFStringRef() : NULL, &param, &alert);
	(void)RunStandardAlert(alert, NULL, &itemHit);

	return itemHit;
}


#pragma mark -

void pstrcat(StringPtr destStr, const StringPtr sourceStr) {
	short	len = sourceStr[0];
	short	curLen = destStr[0];

	BlockMoveData(&sourceStr[1], &destStr[curLen + 1], len);
	destStr[0] = curLen + len;
}



void pstrcpy(StringPtr destStr, const StringPtr sourceStr) {
	BlockMoveData(&sourceStr[0], &destStr[0], sourceStr[0] + 1);
}



void c2pstrncpy(StringPtr destStr, const char* sourceStr, SInt16 len) {
	BlockMoveData(&sourceStr[0], &destStr[1], len);
	destStr[0] = len;
}



void c2pstrncat(StringPtr destStr, const char* sourceStr, SInt16 len) {
	SInt16	curLen = destStr[0];

	BlockMoveData(&sourceStr[0], &destStr[curLen + 1], len);
	destStr[0] = curLen + len;
}



void c2pstrcat(StringPtr destStr, const char* sourceStr) {
	SInt16	len = strlen(sourceStr);
	SInt16	curLen = destStr[0];

	BlockMoveData(&sourceStr[0], &destStr[curLen + 1], len);
	destStr[0] = curLen + len;
}



bool pstrcmp(const StringPtr destStr, const StringPtr sourceStr) {
	size_t	size = sourceStr[0];

	return memcmp(&sourceStr[0], &destStr[0], size + 1);
}



void prtrim(StringPtr string) {
	while(string[0] && string[string[0]] == ' ') string[0]--;
}


#pragma mark -

FSSpec trueFileSpec, tempFileSpec;
SInt16 trueFileRef, tempFileRef;
SInt32 bytesWritten;


#pragma mark -
//-----------------------------------------------
//
//	MakeIconWithPicture
//
//	Currently only makes a 32x32 icons.
//	TODO: Make minimal icon needed for pic size.
//
IconRef MakeIconWithPicture(PicHandle hPict) {
	static int icon_serial = 0;
    OSStatus			err;
    IconFamilyHandle	iconFamily;
    IconRef				iconRef;
    Handle				h1, h2;

    iconFamily = (IconFamilyHandle)NewHandle(0);

    h1 = GetIconDataHandle(hPict);
    h2 = GetIconMaskHandle(hPict);
    err = SetIconFamilyData(iconFamily, HUGE_ICONS ? (OSType)kThumbnail32BitData : (OSType)kLarge32BitData, h1);
    err = SetIconFamilyData(iconFamily, HUGE_ICONS ? (OSType)kThumbnail8BitMask : (OSType)kLarge8BitMask, h2);
    DisposeHandle(h1);
    DisposeHandle(h2);

    err = RegisterIconRefFromIconFamily(kFakeCreator, kFakeType + icon_serial, iconFamily, &iconRef);
    DisposeHandle((Handle)iconFamily);
    err = AcquireIconRef(iconRef);
    err = UnregisterIconRef(kFakeCreator, kFakeType + icon_serial++);

    return iconRef;
}

//-----------------------------------------------
//
//	GetIconDataHandle
//
//	TODO: Make minimal icon needed for pic size.
//
#define k24RGBBytesPerPixel 4
Handle GetIconDataHandle(PicHandle pict) {
    GWorldPtr offscreen, offscreen2;
    CGrafPtr savedPort;
    GDHandle savedDevice;
    Handle h;
    OSErr err;
    Rect r, pictRect;

    SetRect(&r, 0, 0, ICON_SIZE, ICON_SIZE);

    GetGWorld(&savedPort, &savedDevice);

    err = NewGWorld(&offscreen, 32, &r, NULL, NULL, 0);
    HLock((Handle)GetGWorldPixMap(offscreen));
    SetGWorld(offscreen, NULL);
    EraseRect(&r);
	QDGetPictureBounds(pict, &pictRect);
    DrawPicture(pict, &pictRect);

    h = NewHandle(ICON_SIZE * ICON_SIZE * k24RGBBytesPerPixel);
    HLock(h);
    err = NewGWorldFromPtr( &offscreen2, k24RGBPixelFormat, &r, NULL, NULL, 0, *h, ICON_SIZE * k24RGBBytesPerPixel);

    HLock((Handle)GetGWorldPixMap(offscreen2));
    SetGWorld(offscreen2, NULL);
    CopyBits(GetPortBitMapForCopyBits(offscreen),
	GetPortBitMapForCopyBits(offscreen2), &r, &r, srcCopy, NULL);

    SetGWorld(savedPort, savedDevice);

    DisposeGWorld(offscreen);

    return h;
}


//-----------------------------------------------
//
//	GetIconMaskHandle
//
//	TODO: Make minimal icon needed for pic size.
//
Handle GetIconMaskHandle(PicHandle pict) {
    GWorldPtr offscreen, offscreen2, offscreen3;
    CGrafPtr savedPort;
    GDHandle savedDevice;
    Handle h;
    CTabHandle cTab;
    OSErr err;
    Rect r, pictRect;

    SetRect(&r, 0, 0, ICON_SIZE, ICON_SIZE);

    GetGWorld(&savedPort, &savedDevice);

    err = NewGWorld(&offscreen, 32, &r, NULL, NULL, 0);
    HLock((Handle)GetGWorldPixMap(offscreen));
    SetGWorld(offscreen, NULL);
    PaintRect(&r);
	QDGetPictureBounds(pict, &pictRect);
    DrawPicture(pict, &pictRect);

    err = NewGWorld(&offscreen2, 32, &r, NULL, NULL, 0);
    HLock((Handle)GetGWorldPixMap(offscreen2));
    SetGWorld(offscreen2, NULL);
    EraseRect(&r);
    DrawPicture(pict, &pictRect);

    CopyBits(
		GetPortBitMapForCopyBits(offscreen),
        GetPortBitMapForCopyBits(offscreen2),
		&r, &r, subOver, NULL);

    h = NewHandle(ICON_SIZE*ICON_SIZE);
    HLock(h);
    cTab = GetCTable(8 + 32);	// black, 254 shades of gray, white
    err = NewGWorldFromPtr( &offscreen3, k8IndexedPixelFormat, &r, cTab, NULL, 0, *h, ICON_SIZE );

    DisposeCTable(cTab);
    HLock((Handle)GetGWorldPixMap(offscreen3));
    SetGWorld(offscreen3, NULL);
    CopyBits(GetPortBitMapForCopyBits(offscreen2),
        GetPortBitMapForCopyBits(offscreen3), &r, &r, srcCopy, NULL);

    SetGWorld(savedPort, savedDevice);

    DisposeGWorld(offscreen);
    DisposeGWorld(offscreen2);

    return h;
}



short CurrentDepth() {
	GWorldPtr	gworld;
	GDHandle	gdevice;
	GetGWorld( &gworld, &gdevice );
	PixMapHandle thePixMap = (**gdevice).gdPMap;
	return GetPixDepth(thePixMap);
}



char* ImagePath(const char *filename) {
	static int	pathindex = 0;
	static char	fullpath[10][256];
	int			index = pathindex;

	pathindex = (pathindex + 1) % 10;
	strcpy(fullpath[index], "::Resources:images:");
	strcat(fullpath[index], filename);
	return fullpath[index];
}


/**
 *  SetFont is a bit of a hack...
 */
void SetFont(FontInfo *fontInfo, ConstStr255Param fontName, short size, short style, CGContextRef gc) {
    // Get the family for the given font name
	FMFontFamily  font = applFont;
	if (fontName[0] > 0)
		font = FMGetFontFamilyFromName(fontName);

    // This is the hackish part.
	CGrafPtr port; GetPort(&port);

    // Set the font on the current GrafPort
	SetPortTextFont(port, font);
	SetPortTextSize(port, size);
	SetPortTextFace(port, style);

    // Use the port to get the FontInfo
	if (fontInfo)
		GetFontInfo(fontInfo);

    // Set the font on the CG context, if given
	if (gc)
		CGContextSelectFont(gc, (char*)fontName+1, size, kCGEncodingMacRoman);
}



void MarkMenuItem(MenuRef menu, MenuItemIndex index, bool mark) {
	if (mark)
		SetItemMark(menu, index, diamondMark);
	else
		CheckMenuItem(menu, index, false);
}



MenuRef GetMiniMenu(CFStringRef name) {
	MenuRef		menu;
	OSStatus	err;
    IBNibRef 	nibRef;
	err = CreateNibReference(MAIN_NIB_NAME, &nibRef);
	err = CreateMenuFromNib(nibRef, name, &menu);
	DisposeNibReference(nibRef);

	err = SetMenuFont(menu, 0, 10);
	MenuRef subMenu;
	for (int i=1; i<=CountMenuItems(menu); i++) {
		err = GetMenuItemHierarchicalMenu(menu, i, &subMenu);
		if (subMenu)
			err = SetMenuFont(subMenu, 0, 10);
	}

	return menu;
}



OSErr SetMenuContentsRefcon(MenuRef inMenu, UInt32 inRefCon) {
	OSErr err = noErr;
	MenuRef subMenu;
	UInt16 itemCount = CountMenuItems(inMenu);

	if (itemCount > 0) {
		for (int i=1; i<=itemCount; i++) {
			err = SetMenuItemRefCon(inMenu, i, inRefCon);
			if (err) break;

			err = GetMenuItemHierarchicalMenu(inMenu, i, &subMenu);
			if (err) break;

			if (subMenu) {
				err = SetMenuContentsRefcon(subMenu, inRefCon);
				if (err) break;
			}
		}
	}

	return err;
}



int CircleSegment(Point where, float circleR, float minR, float maxR, UInt16 segCount) {
	SInt16	segment = -1;
	float	h, v, radius, angle;
	
	//
	// Calculate the distance of the point
	//	from the center of the circle
	//
	h = (where.h - circleR);
	v = (where.v - circleR);
	radius = sqrt((h*h) + (v*v));

	//
	// If the point lies outside then ignore it
	//
	if ((minR == 0.0 || radius > minR) && (maxR == 0.0 || radius < maxR)) {
		//
		// Calculate the angle of the point from the center
		//	where 0¼ = 12-o-clock
		//
		if (h < 0)	angle = (180.0 + (acos(v / radius) * 180.0 / M_PI));
		else		angle = (180.0 - (acos(v / radius) * 180.0 / M_PI));

		//
		// Convert the angle into a circle tone
		//
		segment = (int)((angle + (180.0/segCount)) / (360.0/segCount)) % segCount;		
	}

	return segment;
}


GWorldPtr MakeGWorldWithPictResource(short pictID, short depth) {
	OSErr		err;
	GWorldPtr 	saveGWorld, outGWorld = NULL;
	GDHandle 	saveGDevice;
	GWorldPtr 	tempGWorld;
	Rect 		pictRect, extraRect;
	PicHandle	pictH;

	pictH = GetPicture(pictID);
	GetGWorld(&saveGWorld, &saveGDevice);

	QDGetPictureBounds(pictH, &pictRect);
	OffsetRect(&pictRect, -pictRect.left, -pictRect.top);

	// Make the GWorld wider
	extraRect = pictRect;
	if (pictRect.right < 16) {
		extraRect.right = 16;
	}

	err = NewGWorld( &tempGWorld, depth, &extraRect, NULL, NULL, 0 );

	if (err == noErr) {
		outGWorld = tempGWorld;
		SetGWorld(tempGWorld, NULL);
		(void)LockPixels( GetGWorldPixMap(tempGWorld) );
		EraseRect(&extraRect);
		DrawPicture(pictH, &pictRect);
		UnlockPixels( GetGWorldPixMap(tempGWorld) );
	}

	SetGWorld(saveGWorld, saveGDevice);
	ReleaseResource((Handle)pictH);

	return outGWorld;
}


void DrawStringInBox(const StringPtr string, const Rect *pRect) {
	FontInfo	font;
	Str31		semiString[5];
	Str63		finalString;
	SInt16		left = pRect->left, width;
	SInt16		a, v, start, space, line;
	SInt16		len = string[0], wordCount = 0;
	bool		last;

	int i = 0;
	while (++i <= len) {
		if (string[i] == ' ')
			continue;

		if (i <= len) {
			start = i;

			while (string[++i] != ' ')
				if (i > len)
					break;

			c2pstrncpy(semiString[wordCount++], (char*)&string[start], i - start);
		}
	}

	if (wordCount) {
		GetFontInfo(&font);
		a = font.ascent;
		v = pRect->top;
		width = pRect->right - left;
		space = StringWidth("\p ");

		last = false;
		line = 0;
		i = 0;
		while (i < wordCount) {								// as long as there are words...
			pstrcpy(finalString, semiString[i++]);			// start with a fresh word

			if (i < wordCount) {								// see if there are more to add
				while (StringWidth(finalString) + space
						+ StringWidth(semiString[i]) < width) {	// if the next word fits
					pstrcat(finalString, (StringPtr)"\p ");
					pstrcat(finalString, semiString[i++]);	// then tack it on with a space
					if (i >= wordCount) {					// was this the last word?
						last = true;						// (for vertical align)
						break;								// skip further checks
					}
				}
			}
			else
				last = true;

			if ((last == true) && (line == 0))	v += 4;
			MoveTo(left + width / 2 - StringWidth(finalString) / 2, v += a);
			DrawString(finalString);
			line++;
		}
	}
}


#pragma mark -

UInt64 Micro64() {
	UnsignedWide time; Microseconds(&time);
	return UnsignedWideToUInt64(time);
}



Rect MoveRect(Rect &rect, short x, short y) {
	Rect newRect = { y, x, y + rect.bottom - rect.top, x + rect.right - rect.left };
	return newRect;
}


bool RectsOverlap(Rect &rect1, Rect &rect2) {
	return (rect1.top < rect2.bottom) && (rect1.bottom > rect2.top) && (rect1.left < rect2.right) && (rect1.right > rect2.left);
}


#pragma mark -

CFMutableStringRef CFStringCombine(CFStringRef srcRef, CFStringRef appRef) {
	if (srcRef == NULL) return NULL;
	CFMutableStringRef	stringRef = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, srcRef);
	CFStringAppend(stringRef, appRef);
	return stringRef;
}


char* CFStringToCString(const CFStringRef sref) {
	CFIndex		size = CFStringGetLength(sref);
	char		*string = new char[size+1];

	if (!CFStringGetCString(sref, string, size+1, kCFStringEncodingMacRoman)) {
		delete [] string;
		string = NULL;
	}

	return string;
}


bool CFStringEndsWith(const CFStringRef inString, const CFStringRef inExt) {
	CFIndex	length = CFStringGetLength(inString);
	CFIndex	extlen = CFStringGetLength(inExt);
	CFRange	range = { length-extlen, extlen };

	return CFStringFindWithOptions(
		inString,
		inExt,
		range,
		kCFCompareCaseInsensitive|kCFCompareAnchored,
		NULL
	);
}


CFStringRef CFStringTrimExtension(const CFStringRef inString) {
	CFIndex	length = CFStringGetLength(inString);
	CFRange	match = CFStringFind(inString, CFSTR("."), kCFCompareBackwards);

	CFMutableStringRef mutString;
	mutString = CFStringCreateMutableCopy(kCFAllocatorDefault, 0, inString);

	if (match.location != kCFNotFound) {
		CFRange trim = { match.location, length-match.location };
		CFStringDelete(mutString, trim);
	}

	return mutString;
}


CFStringRef CFStringCreateWithIntList(const SInt32 inList[], UInt16 inSize) {
	CFMutableArrayRef intArray =
		CFArrayCreateMutable(kCFAllocatorDefault, inSize, &kCFTypeArrayCallBacks);

	for (int i=0; i<inSize; i++) {
		CFStringRef intString = CFStringCreateWithFormat(NULL, NULL, CFSTR("%ld"), inList[i]);
		CFArrayAppendValue(intArray, intString);
		CFRELEASE(intString);
	}

	return CFStringCreateByCombiningStrings(kCFAllocatorDefault, intArray, CFSTR(","));
}


UInt16 CFStringToIntArray(CFStringRef inString, SInt32 outList[], UInt16 maxItems) {
	CFArrayRef newArray = CFStringCreateArrayBySeparatingStrings(kCFAllocatorDefault, inString, CFSTR(","));

	CFIndex arrayLen = CFArrayGetCount(newArray);
	if (arrayLen > maxItems)
		arrayLen = maxItems;

	for (int i=0; i<arrayLen; i++) {
		CFStringRef	itemRef = (CFStringRef)CFArrayGetValueAtIndex(newArray, i);
		CFRETAIN(itemRef);
		outList[i] = CFStringGetIntValue(itemRef);
		CFRELEASE(itemRef);
	}

	CFRELEASE(newArray);
	return arrayLen;
}

void CFStringPrint(const CFStringRef sref) {
	char *string = CFStringToCString(sref);
	if (string) {
		fprintf(stderr, "%s\n", string);
		delete [] string;
	}
}

#pragma mark -

void PrintPath(CFURLRef uref) {
	CFStringRef sref = CFURLCopyFileSystemPath(uref, kCFURLPOSIXPathStyle);
	CFStringPrint(sref);
	CFRELEASE(sref);

	sref = CFURLCopyFileSystemPath(uref, kCFURLHFSPathStyle);
	CFStringPrint(sref);
	CFRELEASE(sref);
}

#pragma mark -
CFDataRef AliasToCFData(AliasHandle alias) {
	return CFDataCreate(kCFAllocatorDefault, (UInt8*)*alias, GetHandleSize((Handle)alias));
}

AliasHandle CFDataToAlias(CFDataRef aliasData) {
    CFIndex		dataSize = CFDataGetLength(aliasData);
    AliasHandle	aliasHdl = (AliasHandle) NewHandle(dataSize);
    require(NULL != aliasHdl, CantAllocateAlias);

	CFDataGetBytes(aliasData, CFRangeMake(0, dataSize), (UInt8*)*aliasHdl);

CantAllocateAlias:
	return aliasHdl;
}


#pragma mark -
#if DEBUG_REFCOUNTS

CFTypeRef MyCFRetain(CFTypeRef cf) {
	CFIndex rc1 = CFGetRetainCount(cf);
	if (rc1 != 0x7FFFFFFF) {
		(void)CFRetain(cf);
		CFIndex rc2 = CFGetRetainCount(cf);
		fprintf(stderr, "%08X Retain %ld > %ld", cf, rc1, rc2);
		if (rc2 > 2) {
			fprintf(stderr, " (Large?)\n");
			CFShow(cf);
		}
	}
	else {
		fprintf(stderr, " (Internal Storage)");
	}
	fprintf(stderr, "\n");
	return cf;
}

void MyCFRelease(CFTypeRef cf) {
	CFIndex rc1 = CFGetRetainCount(cf);
	fprintf(stderr, "%08lX Release %ld", cf, rc1);
	if (rc1 == 0x7FFFFFFF) {
		fprintf(stderr, " (Internal Storage)");
	}
	else if (rc1 <= 0) {
		fprintf(stderr, " (Zero! - break here to trap)\n");
		CFShow(cf);
	}
	else {
		CFRelease(cf);
		CFIndex rc2 = CFGetRetainCount(cf);
		fprintf(stderr, " > %ld", rc2);
		if (rc2 > 100 || rc2 < 0) {
			fprintf(stderr, " (Odd Value)\n");
			CFShow(cf);
		}
		else if (rc1 == 1) {
			fprintf(stderr, " (Disposing)");
		}
	}
	fprintf(stderr, "\n");
}

#endif

bool ExpandHandleForSize(Handle h, UInt32 progress) {
	bool didExpand = false;
	UInt32 handleSize = GetHandleSize(h);

	if (progress + BUFFER_CHUNK_SIZE > handleSize) {
		char oldState = HGetState(h);
		handleSize += BUFFER_CHUNK_SIZE;
		HUnlock(h);
		SetHandleSize(h, handleSize);
		HSetState(h, oldState);
		didExpand = true;
	}

	return didExpand;
}

// /Library/.GlobalPreferences.plist
CFPropertyListRef GetPlistFromFile( CFURLRef fileURL ) {
	CFPropertyListRef	propertyList;
	CFStringRef			errorString;
	CFDataRef			resourceData;
	Boolean				status;
	SInt32				errorCode;
	
	// Read the XML file.
	status = CFURLCreateDataAndPropertiesFromResource(
													  kCFAllocatorDefault,
													  fileURL,
													  &resourceData,            // place to put file data
													  NULL,
													  NULL,
													  &errorCode);
	
	// Reconstitute the dictionary using the XML data.
	propertyList = CFPropertyListCreateFromXMLData( kCFAllocatorDefault,
												   resourceData,
												   kCFPropertyListImmutable,
												   &errorString);
	
	CFRelease( resourceData );
	return propertyList;
}

#import <glob.h>

char* CreatePathByExpandingTildePath(char* path)
{
	glob_t globbuf;
	char **v;
	char *expandedPath = NULL, *result = NULL;
	
	assert(path != NULL);
	
	if (glob(path, GLOB_TILDE, NULL, &globbuf) == 0) //success
	{
		v = globbuf.gl_pathv; //list of matched pathnames
		expandedPath = v[0]; //number of matched pathnames, gl_pathc == 1
		
		result = (char*)calloc(1, strlen(expandedPath) + 1); //the extra char is for the null-termination
		if(result)
			strncpy(result, expandedPath, strlen(expandedPath) + 1); //copy the null-termination as well
		
		globfree(&globbuf);
	}
	
	return result;
}

CFStringRef CreateCFStringByExpandingTildePath(CFStringRef path)
{
	char pcPath[PATH_MAX];
	char *pcResult = NULL;
	CFStringRef result = NULL;
	
	if (CFStringGetFileSystemRepresentation(path, pcPath, PATH_MAX)) //CFString to CString
	{
		pcResult = CreatePathByExpandingTildePath(pcPath); //call the POSIX version
		if (pcResult)
		{
			result = CFStringCreateWithCString(NULL, pcResult, kCFStringEncodingUTF8); //CString to CFString
			free(pcResult); //free the memory allocated in CreatePathByExpandingTildePath()
		}
	}
	
	return result;
}

#if defined(LION_VERSION)

/**
 *	Replace Deprecated PtInRect
 */
Boolean PtInRect(const Point inPoint, const Rect *inRect) {
	return (inPoint.h >= inRect->left && inPoint.h <= inRect->right
			&& inPoint.v >= inRect->top && inPoint.v <= inRect->bottom);
}

/**
 *	Deprecated GetPort placeholder, for modernization
 */
void GetPort(GrafPtr *oldPort) { *oldPort = NULL; }

#endif
