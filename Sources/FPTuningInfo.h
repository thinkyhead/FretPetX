/*
 *  FPTuningInfo.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *	Represents a guitar tuning with name and tones as a
 *	class because it needs smart contruct, compare, and copy.
 *
 */

#ifndef FPTUNINGINFO_H
#define FPTUNINGINFO_H

class TDictionary;

struct TuningInit {
	CFStringRef	name;
	SInt32		tone[NUM_STRINGS];
};
typedef struct TuningInit TuningInit;

class FPTuningInfo;

class FPTuningInfo {
public:
	CFStringRef	name;
	SInt32		tone[NUM_STRINGS];

	FPTuningInfo();
	FPTuningInfo(const FPTuningInfo& src);
	FPTuningInfo(const TuningInit &inTuning);
	FPTuningInfo(CFStringRef inName, SInt32 *inTone=NULL);
	FPTuningInfo(TDictionary *inDict);
	~FPTuningInfo();

	const int			operator==(const FPTuningInfo& other) const;
	const int			operator!=(const FPTuningInfo& other) const { return !(*this == other); }
	FPTuningInfo&		operator=(const FPTuningInfo& other);
	FPTuningInfo&		operator=(const TuningInit &init);
	FPTuningInfo&		operator=(TDictionary *inDict);

	void				SetName(const CFStringRef inName);

	TDictionary*		GetDictionary() const;
};

#define kBaseTuningCount 21
extern TuningInit	baseTuningInit[kBaseTuningCount];
extern FPTuningInfo builtinTuning[kBaseTuningCount];

#endif
