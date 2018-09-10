/*!
	@file FPTuningInfo.cpp

	@section COPYRIGHT
	FretPet X
	Copyright © 2012 Scott Lahteine. All rights reserved.
*/

#include "FPTuningInfo.h"
#include "FPUtilities.h"
#include "TDictionary.h"

FPTuningInfo::FPTuningInfo() {
	name = NULL;
	*this = builtinTuning[0];
}

FPTuningInfo::FPTuningInfo(const TuningInit &inTuning) {
	name = NULL;
	*this = inTuning;
}

FPTuningInfo::FPTuningInfo(const FPTuningInfo& src) {
	name = NULL;
	*this = src;
}

FPTuningInfo::FPTuningInfo(CFStringRef inName, SInt32 *inTone) {
	name = NULL;
	SetName(inName);

	if (inTone)
		memcpy(&tone, inTone, sizeof(tone));
	else
		memcpy(&tone, builtinTuning[0].tone, sizeof(tone));
}

FPTuningInfo::FPTuningInfo(TDictionary *inDict) {
	name = NULL;

	CFStringRef	inString = (CFStringRef)inDict->GetValue(CFSTR("name"));
	SetName(inString);
	CFRELEASE(inString);

	inDict->GetIntArray(CFSTR("tones"), tone, NUM_STRINGS);
}

FPTuningInfo::~FPTuningInfo() { CFRELEASE(name); }

const int FPTuningInfo::operator==(const FPTuningInfo& other) const {
	return memcmp(tone, other.tone, sizeof(tone)) == 0;
}


FPTuningInfo& FPTuningInfo::operator=(const FPTuningInfo& other) {
	SetName(other.name);

	for (UInt16 s=NUM_STRINGS; s--;)
		tone[s] = other.tone[s];

	return *this;
}

FPTuningInfo& FPTuningInfo::operator=(const TuningInit &init) {
	SetName(init.name);

	for (UInt16 s=NUM_STRINGS; s--;)
		tone[s] = init.tone[s];

	return *this;
}

FPTuningInfo& FPTuningInfo::operator=(TDictionary *inDict) {
	CFStringRef	inString = (CFStringRef)inDict->GetValue(CFSTR("name"));
	SetName(inString);
	CFRELEASE(inString);

	inDict->GetIntArray(CFSTR("tones"), tone, NUM_STRINGS);

	return *this;
}

void FPTuningInfo::SetName(const CFStringRef inName) {
	CFStringRef oldName = name;
	name = CFStringCreateCopy(kCFAllocatorDefault, inName);

	if (oldName)
		CFRELEASE(oldName);
}

TDictionary* FPTuningInfo::GetDictionary() const {
	TDictionary	*outDict = new TDictionary();

	outDict->SetValue(CFSTR("name"), name);
	outDict->SetIntArray(CFSTR("tones"), tone, NUM_STRINGS);

	return outDict;
}

