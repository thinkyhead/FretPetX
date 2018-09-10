/*!
 *  @file FPChord.h
 *
 *	@brief Interface for the FPChord class
 *
 *	FPChords contain all the information necessary to define a chord unit,
 *	including tones, fingering, picking pattern, and bracket position.
 *
 *	FPChords are grouped together into units of four parts, called
 *	Chord Groups. For use in documents, Chord Groups are stored in arrays.
 *
 *	Chords know how to transform themselves based on a number of parameters.
 *
 *	@section COPYRIGHT
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 * */

#ifndef FPCHORD_H
#define FPCHORD_H

#define kUndefinedRootValue -99

#include "TObjectDeque.h"

class TFile;
class TDictionary;
class FPChord;
class TString;
class FPChordGroup;


#pragma pack(2)

/*!	The struct that represents chords in FretPet Classic.
	These are still needed to read and write FretPet Classic
	document files.
*/
typedef struct {
	UInt16		tones;				//!< The chord's tones as a bitmask
	UInt16		root;				//!< The root tone of the chord
	UInt16		key;				//!< The key of the chord
	UInt16		rootType;			//!< Chord root type (ignored)
	Boolean		rootLock;			//!< Whether to lock the root
	Boolean		bracketFlag;		//!< Whether the chord bracket is on
	UInt16		brakLow, brakHi;	//!< The bracket position
	SInt16		fretHeld[6];		//!< The fretboard fingering
	UInt16		beats;				//!< The length of the sequence
	UInt16		repeat;				//!< The number of times to play
	PatternMask	pick[6];			//!< The picking pattern
} OldChordInfo;

//!	@brief A group of old chords
typedef	OldChordInfo	OldChordGroup[][OLD_NUM_PARTS];

#pragma pack()

typedef struct FatChord {
	UInt32	mask;
	Boolean	flat6, flatFlat7, flat11, flat13;
} FatChord;

/*! FPChord embodies all information necessary to define a chord,
	including tones, fingering, picking pattern, and bracket
	position. Includes several methods to transform the chord
	and to get different representations.
*/
class FPChord {
	public:
		// Chord info
		UInt16			tones;				// The tones of the chord as a bit mask
		UInt16			root;				// Root note determines naming
		UInt16			key;				// Root key determines flat/sharp
		bool			rootLock;			// the root stays put

		SInt16			rootModifier;		// The scale palette offset
		SInt16			rootScaleStep;		// The scale step offset

		// Guitar info
		bool			bracketFlag;		// chord bracket on?
		UInt16			brakLow, brakHi;	// Bracket position
		SInt16			fretHeld[NUM_STRINGS];	// Fret positions

		// Pattern info
		PatternMask		pick[NUM_STRINGS];	// a picking pattern

		UInt16			beats;				// length of sequence
		UInt16			repeat;				// number of times to play

	public:
						FPChord();
						FPChord(const FPChord &other);
						FPChord(const OldChordInfo &info);
						FPChord(UInt16 t, UInt16 k, UInt16 r);
						FPChord(const TDictionary &inDict, UInt16 b, UInt16 r);		// Constructor for stored chords
						~FPChord() {}

//		FPChord&		operator=(const FPChord &src);
		int				operator==(const FPChord &inChord) const;
		int				operator!=(const FPChord &inChord) const	{ return !(*this == inChord); }

		void			Init();
		inline FPChord* Clone() const						{ return new FPChord(*this); }

		// Accessors
		inline bool		HasTones() const					{ return (tones != 0); }
		inline UInt16	HasTones(UInt16 arg) const			{ return (tones & arg) == arg; }
		inline bool		HasTone(UInt16 t) const				{ return tones & BIT(NOTEMOD(t)); }
		UInt16			StringsUsedInPattern() const;
		bool			HasPattern(bool noChordRequired=false) const;
		bool			HasFingering() const;
		inline UInt16	PatternSize() const					{ return beats; }
		inline UInt16	Repeat() const						{ return repeat; }
		inline bool		IsBracketEnabled() const			{ return bracketFlag; }
		inline SInt16	FretHeld(UInt16 string) const		{ return fretHeld[string]; }
		inline char*	ChordName() const					{ return ChordName(root); }
		inline UInt16	Key() const							{ return key; }
		inline UInt16	Root() const						{ return root; }
		inline bool		RootNeedsScaleInfo()				{ return (rootModifier == kUndefinedRootValue); }
		inline SInt16	RootModifier() const				{ return rootModifier; }
		inline UInt16	RootScaleStep() const				{ return rootScaleStep; }
		void			UpdateStepInfo();

		// Harmonize and Transpose
		void			HarmonizeBy(const SInt16 steps);
		inline void		HarmonizeUp()						{ HarmonizeBy(1); }
		inline void		HarmonizeDown()						{ HarmonizeBy(-1); }

		bool			TransposeTo(const UInt16 newKey);
		inline void		TransposeBy(SInt16 interval)		{ TransposeTo(key+interval); }

		// Setters
		void			Set(const OldChordInfo &info);
		inline void		Set(UInt16 t, UInt16 k, UInt16 r)	{ tones = t; key = k; root = r; }

		inline void		SetTones(UInt16 t)					{ tones = t; }
		inline void		AddTones(UInt16 t)					{ tones |= t; }
		inline void		SubtractTones(UInt16 t)				{ tones &= ~t; }
		inline void		ToggleTones(UInt16 t)				{ tones ^= t; }

		inline void		AddTone(UInt16 n)					{ tones |= BIT(NOTEMOD(n)); }
		inline void		Clear()								{ SetTones(0); }

		// Setters
		inline void		SetRoot(UInt16 r)					{ root = NOTEMOD(r); }
		inline void		SetKey(UInt16 k)					{ key = k; }
		inline void		SetBracket(UInt16 lo, UInt16 hi)	{ brakLow = lo; brakHi = hi; }
		inline void		SetRepeat(UInt16 r)					{ repeat = r; }
		inline void		SetRootModifier(SInt16 rm)			{ rootModifier = rm; }
		inline void		SetRootScaleStep(SInt16 ss)			{ rootScaleStep = ss; }
		inline void		ResetStepInfo()						{ rootModifier = rootScaleStep = kUndefinedRootValue; }

		// Chord Lock
		inline bool		IsLocked() const					{ return rootLock; }
		inline void		SetLocked(bool lock)				{ rootLock = lock; }
		inline void		Lock()								{ SetLocked(true); }
		inline void		Unlock()							{ SetLocked(false); }
		inline void		ToggleLock()						{ rootLock ^= true; }

		// The pattern
		inline void		SetPattern(FPChord &src)			{ for(int s=NUM_STRINGS;s--;) pick[s] = src.pick[s]; }
		inline void		ClearPattern()						{ bzero(pick, sizeof(pick)); }
		void			RandomPattern(bool very=false);
		void			ResetPattern();
		void			SetDefaultPattern();
		void			SavePatternAsDefault();

		inline PatternMask&	GetPatternMask(UInt16 string)				{ return pick[string]; }
		inline bool		GetPatternDot(UInt16 string, UInt16 step) const	{ return (pick[string] & BIT(step)) != 0; }
		inline void		SetPatternDot(UInt16 string, UInt16 step)		{ pick[string] |= BIT(step); }
		inline void		ClearPatternDot(UInt16 string, UInt16 step)		{ pick[string] &= ~BIT(step); }
		inline void		SetPatternStep(UInt16 step)						{ UInt16 b = BIT(step); for (int s=NUM_STRINGS; s--;) pick[s] |= b; }
		inline void		ClearPatternStep(UInt16 step)					{ UInt16 b = ~BIT(step); for (int s=NUM_STRINGS; s--;) pick[s] &= b; }
		void			StretchPattern(bool latter=false);
		inline void		SetPatternSize(UInt16 b)						{ beats = b; }

		// Chord name and function strings
		char*			ChordNameOld(UInt16 r, bool bRoman=false) const;
		char*			ChordName(UInt16 r, bool bRoman=false) const;

		inline void		ChordName(StringPtr n, StringPtr e, StringPtr m, bool bRoman=false) const
						{ ChordName(root, n, e, m, bRoman); }

		void			ChordName(TString &n, TString &e, TString &m, bool bRoman=false) const;

		void			ChordName(UInt16 r, StringPtr n, StringPtr e, StringPtr m, bool bRoman=false) const;
		char*			ChordToneFunctions() const;
		FatChord		TwoOctaveChord() const;

		OSErr			Write(TFile* const file) const;
		OSErr			WriteOldStyle(TFile* const file) const;
		TDictionary*	GetDictionary() const;

		// Selection Operations
		void			FlipPattern();
		void			ReversePattern();
		void			InvertTones();
		void			CleanupTones();
		void			NewFingering();

		bool			CanFlipPattern() const;
		bool			CanReversePattern() const;
		bool			CanInvertTones() const;
		bool			CanCleanupTones() const;

};


extern FPChord globalChord;


#pragma mark -
//-----------------------------------------------
//
// FPChordGroup
//
class FPChordGroup {
	private:
		FPChord chordList[DOC_PARTS];

	public:
		FPChordGroup() {}
		FPChordGroup(const FPChord &chord);
		FPChordGroup(const FPChordGroup &group);
		FPChordGroup(const TDictionary &inDict);
		FPChordGroup(const CFArrayRef groupArray, UInt16 b, UInt16 r);

		const FPChord&	operator[](unsigned i) const		{ return chordList[i]; }
		FPChord&		operator[](unsigned i)				{ return chordList[i]; }
		int				operator==(const FPChordGroup &inGroup) const;
		int				operator!=(const FPChordGroup &inGroup) const	{ return !(*this == inGroup); }

		bool			HasPattern(bool anyChord=false) const;
		bool			HasFingering() const;
		inline UInt16	Repeat() const						{ return chordList[0].Repeat(); }
		inline UInt16	PatternSize() const					{ return chordList[0].PatternSize(); }

		void			ResetPattern();
		void			StretchPattern(bool latter=false);
		inline void		SetRepeat(UInt16 r)					{ for (PartIndex p=DOC_PARTS;p--;chordList[p].SetRepeat(r)); }
		inline void		SetPatternSize(UInt16 b)			{ for (PartIndex p=DOC_PARTS;p--;chordList[p].SetPatternSize(b)); }

		OSErr			Write(UInt16 format);
		TDictionary*	GetDictionary() const;
};


#pragma mark -
//-----------------------------------------------
//
// FPChordGroupArray
//

class FPChordGroupArray : public TObjectDeque<FPChordGroup> {
	public:
		void		InsertCopyBefore(ChordIndex index, const FPChord &chord);
		OSErr		Write(UInt16 format);

		const FPChordGroup&	operator[](unsigned i) const		{ return *TObjectDeque<FPChordGroup>::operator[](i); }
		FPChordGroup&		operator[](unsigned i)				{ return *TObjectDeque<FPChordGroup>::operator[](i); }
};

//typedef FPChordGroupArray::iterator FPChordGroupIterator;

#endif
