/*
 *  FPInfoPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPApplication.h"
#include "FPInfoPalette.h"
#include "FPScalePalette.h"
#include "FPGuitarPalette.h"
#include "TString.h"


FPInfoPalette *infoPalette = NULL;


FPInfoPalette::FPInfoPalette() : FPPalette(CFSTR("Info")) {
	scaleTone1	= new TControl(Window(), 'info', 101);
	scaleFunc1	= new TControl(Window(), 'info', 102);
	scaleFunc2	= new TControl(Window(), 'info', 103);
	solfaName	= new TControl(Window(), 'info', 104);

	guitarTone1 = new TControl(Window(), 'info', 201);
	guitarFunc1 = new TControl(Window(), 'info', 202);
	guitarFunc2 = new TControl(Window(), 'info', 203);

	scaleTone2	= new TControl(Window(), 'info', 301);
	guitarTone2 = new TControl(Window(), 'info', 302);
	interval1	= new TControl(Window(), 'info', 303);

	guitarTone3 = new TControl(Window(), 'info', 401);
	scaleTone3	= new TControl(Window(), 'info', 402);
	interval2	= new TControl(Window(), 'info', 403);
}


FPInfoPalette::~FPInfoPalette() {
	delete scaleTone1;
	delete scaleFunc1;
	delete scaleFunc2;
	delete solfaName;

	delete guitarTone1;
	delete guitarFunc1;
	delete guitarFunc2;

	delete scaleTone2;
	delete guitarTone2;
	delete interval1;

	delete guitarTone3;
	delete scaleTone3;
	delete interval2;
}


void FPInfoPalette::UpdateFields(bool doScale, bool doGuitar) {
	UInt16	scaleTone = scalePalette->CurrentTone();
	UInt16	guitarTone = NOTEMOD(guitarPalette->CurrentTone());
	UInt16	root = globalChord.Root();
	UInt16	key = globalChord.Key();
	TString text;

	if (doScale) {
		text = scalePalette->NameOfCurrentNote();
		scaleTone1->SetStaticText(text.GetCFStringRef());
		scaleTone2->SetStaticText(text.GetCFStringRef());
		scaleTone3->SetStaticText(text.GetCFStringRef());

		text = NFPrimary[NOTEMOD(scaleTone - root)];
		scaleFunc1->SetStaticText(text.GetCFStringRef());

		text = NFSecondary[NOTEMOD(scaleTone - root)];
		scaleFunc2->SetStaticText(text.GetCFStringRef());

		text = NFSolFa[NOTEMOD(scaleTone - key)];
		solfaName->SetStaticText(text.GetCFStringRef());
	}

	if (doGuitar) {
		text = scalePalette->NameOfNote(key, guitarTone);
		guitarTone1->SetStaticText(text.GetCFStringRef());
		guitarTone2->SetStaticText(text.GetCFStringRef());
		guitarTone3->SetStaticText(text.GetCFStringRef());

		text = NFPrimary[NOTEMOD(guitarTone - root)];
		guitarFunc1->SetStaticText(text.GetCFStringRef());

		text = NFSecondary[NOTEMOD(guitarTone - root)];
		guitarFunc2->SetStaticText(text.GetCFStringRef());
	}

	text = NFInterval[NOTEMOD(guitarTone - scaleTone)];
	interval1->SetStaticText(text.GetCFStringRef());

	text = NFInterval[NOTEMOD(scaleTone - guitarTone)];
	interval2->SetStaticText(text.GetCFStringRef());
}

