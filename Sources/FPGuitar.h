/*!
 *  @file FPGuitar.h
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * *
 */

#ifndef FPGUITAR_H
#define FPGUITAR_H

#include "TString.h"
#include "TDictionary.h"

class FPGuitar {
	private:
		TDictionary *infoDict;

	protected:
		CFBundleRef	bundle;

	public:
		CFStringRef	name;

		RGBColor	titleColor;
		RGBColor	labelColor;

		TString		titleFont;
		UInt16		titleFontSize;
		UInt8		titleFontStyle;
		float		titleMargin;

		TString		labelFont;
		UInt16		labelFontSize;
		UInt8		labelFontStyle;
		CGPoint		labelPegPosition[NUM_STRINGS];
		bool		usingPegs;
	
		CFURLRef	backgroundFile;
		CFURLRef	dotsFile;

		CFURLRef	bracketFileVertical[4];
		CFURLRef	bracketFileHorizontal[4];

		SInt16		bracketTopHeight, bracketBottomHeight, bracketMiddleHeight, bracketStretchHeight, bracketWidth;

		UInt16		bracketPosX;
		UInt16		bracketGripLeft;
		UInt16		bracketGripRight;
		SInt16		bracketTopOffset;
		SInt16		bracketBottomOffset;

		CFURLRef	cursorFile;
		CGPoint		cursorHandle;
		UInt16		cursorSpeed;

		CFURLRef	effectFile;
		CGPoint		effectHandle;
		UInt16		effectFrames;
		double		effectSpeed;

		UInt16		nutSize;
		UInt16		numberOfFrets;
		UInt16		openFretY;
		UInt16		firstFretY;
		UInt16		twelfthFretY;
		UInt16		*fretPositionY;
		bool		fretsEvenlySpaced;

		UInt16		numberOfStrings;
		UInt16		firstStringX;
		UInt16		lastStringX;
		UInt16		*stringPositionX;
		UInt16		stringSpacing;
		UInt16		firstStringLowestFret;

	public:
		FPGuitar(CFBundleRef b);
		FPGuitar(FSSpec *imageFile);
		~FPGuitar();

		void	Init();
		void	InitWithBundle(CFBundleRef b);
		void	ExtractFontInfo(TString &info, TString &font, UInt16 &size, UInt8 &style);

		CFURLRef	BackgroundFile() const				{ return backgroundFile; }
		CFURLRef	DotsFile() const					{ return dotsFile; }
		CFURLRef	BracketTopFile(bool h) const		{ return h ? bracketFileHorizontal[0] : bracketFileVertical[0]; }
		CFURLRef	BracketMiddleFile(bool h) const		{ return h ? bracketFileHorizontal[1] : bracketFileVertical[1]; }
		CFURLRef	BracketStretchFile(bool h) const	{ return h ? bracketFileHorizontal[2] : bracketFileVertical[2]; }
		CFURLRef	BracketBottomFile(bool h) const		{ return h ? bracketFileHorizontal[3] : bracketFileVertical[3]; }
		CFURLRef	CursorFile() const					{ return cursorFile; }
		CFURLRef	EffectFile() const					{ return effectFile; }

		SInt16	BracketTopHeight() const				{ return bracketTopHeight; }
		SInt16	BracketBottomHeight() const				{ return bracketBottomHeight; }
		SInt16	BracketMiddleHeight() const				{ return bracketMiddleHeight; }
		SInt16	BracketStretchHeight() const			{ return bracketStretchHeight; }
		SInt16	BracketWidth() const					{ return bracketWidth; }

		UInt16	BracketPosX() const						{ return bracketPosX; }
		UInt16	BracketGripLeft() const					{ return bracketGripLeft; }
		UInt16	BracketGripRight() const				{ return bracketGripRight; }
		SInt16	BracketOffsetTop() const				{ return bracketTopOffset; }
		SInt16	BracketOffsetBottom() const				{ return bracketBottomOffset; }

		CGPoint	CursorHandle() const					{ return cursorHandle; }
		UInt16	CursorSpeed() const						{ return cursorSpeed; }

		CGPoint	EffectHandle() const					{ return effectHandle; }
		UInt16	EffectFrames() const					{ return effectFrames; }
		double	EffectSpeed() const						{ return effectSpeed; }

		UInt16	NumberOfFrets() const					{ return numberOfFrets; }
		UInt16	OpenFretY() const						{ return openFretY; }
		UInt16	FirstFretY() const						{ return firstFretY; }
		UInt16	TwelfthFretY() const					{ return twelfthFretY; }
		UInt16	GetFretTop(UInt16 fret) const			{ UInt16 n = numberOfFrets + 1; return (fret > n) ? fretPositionY[n] + fret - n : fretPositionY[fret]; }
		UInt16	GetFretBottom(UInt16 fret) const		{ UInt16 n = numberOfFrets + 1; return (fret > n-1) ? fretPositionY[n] + fret+1 - n : fretPositionY[fret+1] + (fret == 0 ? -nutSize : 0); }
		bool	FretsEvenlySpaced() const				{ return fretsEvenlySpaced; }

		UInt16	NumberOfStrings() const					{ return numberOfStrings; }
		UInt16	FirstStringX() const					{ return firstStringX; }
		UInt16	LastStringX() const						{ return lastStringX; }
		UInt16	GetStringX(UInt16 string) const			{ return stringPositionX[string]; }
		UInt16	GetStringSpacing() const				{ return stringSpacing; }
		UInt16	GetStringLeft(UInt16 string) const		{ return stringPositionX[string] - stringSpacing / 2; }
		UInt16	GetStringRight(UInt16 string) const		{ return stringPositionX[string] + stringSpacing / 2; }
		UInt16	FirstStringLowestFret() const			{ return firstStringLowestFret; }

		const RGBColor&	GetTitleColor() const			{ return titleColor; }
		const TString&	GetTitleFont() const			{ return titleFont; }
		UInt16	GetTitleFontSize() const				{ return titleFontSize; }
		UInt8	GetTitleFontStyle() const				{ return titleFontStyle; }
		float	GetTitleMargin() const					{ return titleMargin; }

		const RGBColor&	GetLabelColor() const			{ return labelColor; }
		const TString&	GetLabelFont() const			{ return labelFont; }
		UInt16	GetLabelFontSize() const				{ return labelFontSize; }
		UInt8	GetLabelFontStyle() const				{ return labelFontStyle; }

		bool	UsingPegs() const						{ return usingPegs; }
		CGPoint	GetLabelPegPosition(unsigned peg) const	{ return labelPegPosition[peg]; }
	
		// Use this to get the position from a pre-made array
		// based on the above values for fret/string positions
		CGPoint	GetDotPosition(UInt16 string, UInt16 fret, bool horizontal=false, bool reverse=false, bool lefty=false) const;

		void	SetFontForTitle();
		void	SetFontForTones();
		UInt16	TitleY();

		int*	NativeTuning();
		SInt16	OctaveModifier();

		CFURLRef	GetBundleResource(CFStringRef resourceName, CFStringRef resourceType, CFStringRef subDirName);
};

#define kGuitarName						CFSTR("Name")
#define kGuitarMenuGroup				CFSTR("MenuGroup")
#define kGuitarDots						CFSTR("DotsFile")

#define kGuitarBracketV					CFSTR("BracketFileVertical")	/* not in use - from files */
#define kGuitarBracketH					CFSTR("BracketFileHorizontal")	/* not in use - from files */
#define kGuitarBracketPosX				CFSTR("BracketPosX")
#define kGuitarBracketGripLeft			CFSTR("BracketGripLeft")
#define kGuitarBracketGripRight			CFSTR("BracketGripRight")
#define kGuitarBracketOffsetTop			CFSTR("BracketOffsetTop")
#define kGuitarBracketOffsetBottom		CFSTR("BracketOffsetBottom")

#define kGuitarCursorFile				CFSTR("CursorFile")		/* not in use */
#define kGuitarCursorHandle				CFSTR("CursorHandle")
#define kGuitarCursorSpeed				CFSTR("CursorSpeed")

#define kGuitarEffectFile				CFSTR("EffectFile")		/* not in use */
#define kGuitarEffectFrames				CFSTR("EffectFrames")	/* as of 3/18/7 uses image frames */
#define kGuitarEffectSpeed				CFSTR("EffectSpeed")
#define kGuitarEffectHandle				CFSTR("EffectHandle")

#define kGuitarEffectName				CFSTR("EffectName")			/* built-in effects like grow and fade */
#define kGuitarEffectParameters			CFSTR("EffectParameters")	/* parameters */

#define kGuitarNumberOfFrets			CFSTR("NumberOfFrets")
#define kGuitarOpenFretY				CFSTR("OpenFretY")
#define kGuitarTwelfthFretY				CFSTR("TwelfthFretY")
#define kGuitarLastFretY				CFSTR("LastFretY")
#define kGuitarFretsEvenlySpaced		CFSTR("FretsEvenlySpaced")
#define kGuitarFretY					CFSTR("FretY")
#define kGuitarNutSize					CFSTR("NutSize")

#define kGuitarNumberOfStrings			CFSTR("NumberOfStrings")
#define kGuitarFirstStringX				CFSTR("FirstStringX")
#define kGuitarLastStringX				CFSTR("LastStringX")
#define kGuitarFirstStringLowestFret	CFSTR("FirstStringLowestFret")
#define kGuitarStringX					CFSTR("StringX")

#define kGuitarTitleColor				CFSTR("TitleColor")
#define kGuitarLabelColor				CFSTR("LabelColor")
#define kGuitarTitleFont				CFSTR("TitleFont")
#define kGuitarLabelFont				CFSTR("LabelFont")
#define kGuitarLabelPegs				CFSTR("LabelPegs")

#define kGuitarTitleMargin				CFSTR("TitleMargin")
#define kGuitarNativeTuning				CFSTR("NativeTuning")
#define kGuitarOctaveModifier			CFSTR("OctaveModifier")

#endif
