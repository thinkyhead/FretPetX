/*
 *  FPChannelsPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPChannelsPalette.h"
#include "FPGuitarPalette.h"
#include "FPApplication.h"
#include "FPMusicPlayer.h"
#include "FPHistory.h"
#include "FPDocWindow.h"
#include "FPCustomTuning.h"
#include "TCarbonEvent.h"
#include "TControls.h"
#include "FPUtilities.h"
#include "FPMidiHelper.h"

FPChannelsPalette *channelsPalette = NULL;


FPPartPopup::FPPartPopup(WindowRef wind, PartIndex p, UInt32 baseCommand, CFStringRef menuName)
	: TPicturePopup(wind, baseCommand + p, menuName, kPictNumberPopOff) {
	const EventTypeSpec	wheelEvents[] = {
			{ kEventClassMouse, kEventMouseWheelMoved }
		};
	RegisterForEvents(GetEventTypeCount(wheelEvents), wheelEvents);

	partNumber = p;
}


bool FPPartPopup::ControlHit(Point where) {
	fretpet->SetPertinentPart(partNumber);
	bool res = TPicturePopup::ControlHit(where);
	fretpet->SetPertinentPart(kCurrentChannel);
	return res;
}


#pragma mark -

Duration FPInstrumentPop::delay = 0;
EventLoopTimerUPP FPInstrumentPop::tagTimerUPP = NULL;	// UPP for an Event Loop Timer
EventLoopTimerRef FPInstrumentPop::tagTimerLoop = NULL;	// Event Loop timer object

FPInstrumentPop::FPInstrumentPop(WindowRef wind, PartIndex p) : FPPartPopup(wind, p, kFPCommandInstPop1, CFSTR("InstrumentPop")) {
	MenuRef			menu, subMenu, seqMenu, instSub;
	MenuItemIndex	instItem, subItem, pickItem, newPickItem;
	if ( ! GetIndMenuItemWithCommandID(NULL, kFPCommandInstrumentMenu, 1, &seqMenu, &instItem) ) {
		if ( ! GetMenuItemHierarchicalMenu(seqMenu, instItem, &instSub) ) {
			OSStatus err = DuplicateMenu(instSub, &popupMenu);
			if (!err) {
				if ( ! GetIndMenuItemWithCommandID(NULL, kFPCommandPickInstrument, 1, &seqMenu, &pickItem) ) {
					AppendMenuItemTextWithCFString(popupMenu, CFSTR("sep"), kMenuItemAttrSeparator, 0, NULL);
					AppendMenuItemTextWithCFString(popupMenu, CFSTR("hey"), kMenuItemAttrUpdateSingleItem, 0, &newPickItem);
					MenuItemDataRec ioData;
					ioData.whichData =
					kMenuItemDataCmdVirtualKey |
					kMenuItemDataCFString |
					kMenuItemDataAttributes |
					kMenuItemDataCommandID |
					kMenuItemDataEnabled |
					kMenuItemDataCmdKey |
					kMenuItemDataCmdKeyGlyph |
					kMenuItemDataCmdKeyModifiers;
					err = CopyMenuItemData(seqMenu, pickItem, false, &ioData);
					err = SetMenuItemData(popupMenu, newPickItem, false, &ioData);
				}
				(void)SetMenuFont(popupMenu, 0, 10);
				UInt16 numItems = CountMenuItems(popupMenu);
				for (int i=1; i<=numItems; i++) {
					(void)GetMenuItemHierarchicalMenu(popupMenu, i, &subMenu);
					if (subMenu) (void)SetMenuFont(subMenu, 0, 10);
				}
				
			}
		}
	}
	(void)HMGetTagDelay(&delay);

	// Create a timer that we can fire later
	if (tagTimerLoop == NULL) {
		tagTimerUPP = NewEventLoopTimerUPP(FPInstrumentPop::ResetTagTimerProc);
		
		(void)InstallEventLoopTimer(
									GetMainEventLoop(),
									kEventDurationForever,
									kEventDurationForever,
									tagTimerUPP,
									NULL,
									&tagTimerLoop
									);
	}
}

bool FPInstrumentPop::ControlHit(Point where) {
	SetActiveItem(player->GetInstrumentGroup(partNumber));
	return FPPartPopup::ControlHit(where);
}


bool FPInstrumentPop::Draw(const Rect &bounds) {
	(void)FPPartPopup::Draw(bounds);

	long trueGM = player->GetInstrumentNumber(partNumber);

	TString aString;
	if ( trueGM >= kFirstGMInstrument && trueGM <= kLastGMInstrument )
		aString.SetWithFormat(CFSTR("%d"), trueGM);
	else if ( trueGM >= kFirstDrumkit && trueGM <= kLastDrumkit )
		aString.SetWithFormat(CFSTR("Kit%d"), FPMidiHelper::TrueGMToFauxGM(trueGM) - kLastGMInstrument);
	else if ( trueGM >= kFirstGSInstrument && trueGM <= kLastGSInstrument)
		aString = CFSTR("GS");

	CGContextRef gc = FocusContext();

	Rect inner = bounds;
	inner.top += 3;
	inner.right -= 8;
	aString.Draw(gc, inner, kThemeMiniSystemFont, GetDrawState(), teFlushRight);

	EndContext();

	return true;
}


bool FPInstrumentPop::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	if (fretpet->swipeIsReversed) delta *= -1, deltaX *= -1;
	fretpet->AddToInstrument(partNumber, delta);
	ResetTagDelaySoon();
	return true;
}

void FPInstrumentPop::ResetTagDelaySoon() {

	(void)HMSetTagDelay(0);
	SetEventLoopTimerNextFireTime(tagTimerLoop, 1.0);	// wait a second?

//	if (tagTimerLoop) {
//		(void)RemoveEventLoopTimer(tagTimerLoop);
//		DisposeEventLoopTimerUPP(tagTimerUPP);
//		tagTimerLoop = NULL;
//		tagTimerUPP = NULL;
//	}
}

void FPInstrumentPop::ResetTagTimerProc(EventLoopTimerRef timer, void *nada) {
	(void)HMSetTagDelay(delay);
}

#pragma mark -
bool FPMidiChannelPop::ControlHit(Point where) {
	SetActiveItem(player->GetMidiChannel(partNumber) + 1);
	return FPPartPopup::ControlHit(where);
}


bool FPMidiChannelPop::Draw(const Rect &bounds) {
	(void)FPPartPopup::Draw(bounds);

	CGContextRef	gc = FocusContext();

	Rect inner = bounds;
	inner.top +=3;
	inner.right -= 8;
	(void)DrawThemeTextBox(
	   TString(player->GetMidiChannel(partNumber) + 1).GetCFStringRef(),
	   kThemeMiniSystemFont,
	   GetDrawState(),
	   false,
	   &inner,
	   teFlushRight,
	   gc
	);

	EndContext();

	return true;
}


bool FPMidiChannelPop::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	SInt32 val = player->GetMidiChannel(partNumber), oldVal = val;
	if (fretpet->swipeIsReversed) delta *= -1, deltaX *= -1;
	val += delta;
	CONSTRAIN(val, 0, 15);
	if (val != oldVal)
		fretpet->DoSetMidiChannel(partNumber, val);

	return true;
}


#pragma mark -
FPVelocityDragger::FPVelocityDragger(WindowRef wind, PartIndex index)
	: TPictureDragger(wind, kFPCommandVelocityDrag1 + index, kPictNumberDragOff) {
	partIndex = index;
	SetRange(0, 127);
}


bool FPVelocityDragger::Draw(const Rect &bounds) {
	TPictureDragger::Draw(bounds);

	Rect inner = bounds;
	inner.top += 3;
	inner.right -= 8;
	DrawText(inner, TString(Value()).GetCFStringRef());
	return true;
}


void FPVelocityDragger::ValueChanged() {
	fretpet->DoSetVelocity(Value(), partIndex, true, false);
}


bool FPVelocityDragger::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	SInt32 oldVal = Value();

	if (fretpet->swipeIsReversed) delta *= -1, deltaX *= -1;
	(void)TPictureDragger::MouseWheelMoved(delta);

	if (oldVal != Value()) {
		event = fretpet->StartUndo(UN_VELOCITY, CFSTR("Velocity Change"));
		event->SetPartAgnostic();
		event->SaveDataBefore(CFSTR("part"), partIndex);
		event->SaveDataBefore(CFSTR("velocity"), oldVal);
		event->SetMergeUnderInterim(30);
		event->SaveDataAfter(CFSTR("velocity"), Value());
		event->Commit();
	}

	return true;
}


void FPVelocityDragger::DoMouseDown() {
	event = fretpet->StartUndo(UN_VELOCITY, CFSTR("Velocity Change"));
	event->SetPartAgnostic();
	event->SaveDataBefore(CFSTR("part"), partIndex);
	event->SaveDataBefore(CFSTR("velocity"), Value());
}


void FPVelocityDragger::DoMouseUp() {
	if (event->GetDataBefore(CFSTR("velocity")) != Value()) {
		event->SaveDataAfter(CFSTR("velocity"), Value());
		event->Commit();
	}
}


#pragma mark -
FPSustainDragger::FPSustainDragger(WindowRef wind, PartIndex index)
	: TPictureDragger(wind, kFPCommandSustainDrag1 + index, kPictNumberDragOff) {
	partIndex = index;
	SetRange(1, 50);
}


bool FPSustainDragger::Draw(const Rect &bounds) {
	TPictureDragger::Draw(bounds);

	Rect inner = bounds;
	inner.top += 3;
	inner.right -= 8;
	DrawText(inner, TString(Value()).GetCFStringRef());
	return true;
}


void FPSustainDragger::ValueChanged() {
	fretpet->DoSetSustain(Value(), partIndex, true, false);
}


bool FPSustainDragger::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	SInt32 oldVal = Value();

	if (fretpet->swipeIsReversed) delta *= -1, deltaX *= -1;
	(void)TPictureDragger::MouseWheelMoved(delta);

	if (oldVal != Value()) {
		event = fretpet->StartUndo(UN_SUSTAIN, CFSTR("Sustain Change"));
		event->SetPartAgnostic();
		event->SaveDataBefore(CFSTR("part"), partIndex);
		event->SaveDataBefore(CFSTR("sustain"), oldVal);
		event->SetMergeUnderInterim(30);
		event->SaveDataAfter(CFSTR("sustain"), Value());
		event->Commit();
	}

	return true;
}


void FPSustainDragger::DoMouseDown() {
	event = fretpet->StartUndo(UN_SUSTAIN, CFSTR("Sustain Change"));
	event->SetPartAgnostic();
	event->SaveDataBefore(CFSTR("part"), partIndex);
	event->SaveDataBefore(CFSTR("sustain"), Value());
}


void FPSustainDragger::DoMouseUp() {
	if (event->GetDataBefore(CFSTR("sustain")) != Value()) {
		event->SaveDataAfter(CFSTR("sustain"), Value());
		event->Commit();
	}
}


#pragma mark -
void FPInfoEditField::ValueChanged() {
//	fprintf(stderr, "FPInfoEditField changed\n");
}


void FPInfoEditField::DidDeactivate() {
//	fprintf(stderr, "FPInfoEditField deactivated\n");
}


void FPInfoEditField::HiliteChanged() {
//	fprintf(stderr, "FPInfoEditField HiliteChanged\n");
}


bool FPInfoEditField::IsGoodKey(char keypress, UInt32 modifiers) {
	TPictureDragger *owner = (TPictureDragger*)GetOwner();

	switch (keypress) {
		case kReturnCharCode:
		case kEnterCharCode:
			owner->SetValue(CFStringGetIntValue(GetText()));
			owner->ValueChanged();
		case kEscapeCharCode:
			owner->ActivateField(false);
			return false;
			break;
	}

	return TNumericTextField::IsGoodKey(keypress, modifiers);
}


#pragma mark -
FPChannelsPalette::FPChannelsPalette() : FPPalette(CFSTR("Channels")) {
	Init();
}


void FPChannelsPalette::Init() {
	WindowRef   wind = Window();

	const EventTypeSpec	windEvents[] = {
		{ kEventClassMouse, kEventMouseDown },
		{ kEventClassMouse, kEventMouseUp }
	};

	RegisterForEvents( GetEventTypeCount( windEvents ), windEvents );

	//
	// Create the tuning popup and give it a menu with an ID
	//
//	tuningPopup			= new TPopupMenu(wind, 'info', 100);
//	customTuningList.AttachMenu(tuningPopup->GetMenu());

#if QUICKTIME_SUPPORT
	quicktimeMaster		= new TPictureControl(wind, kFPCommandQuicktimeMaster, kPictMasterOff);
	quicktimeSolo		= new TPictureControl(wind, kFPCommandQuicktimeSolo, kPictSoloOff);
#else
	synthMaster			= new TPictureControl(wind, kFPCommandSynthMaster, kPictMasterOff);
	synthSolo			= new TPictureControl(wind, kFPCommandSynthSolo, kPictSoloOff);
#endif

	midiMaster			= new TPictureControl(wind, kFPCommandCoreMidiMaster, kPictMasterOff);
	midiSolo			= new TPictureControl(wind, kFPCommandCoreMidiSolo, kPictSoloOff);

//	transformSolo		= new TPictureControl(wind, kFPCommandTransformSolo, kPictSoloOff);

	textField			= new FPInfoEditField(wind, kFPInfoTextField);

	for (PartIndex p=DOC_PARTS; p--;) {
		transLight[p]		= new TPictureControl(wind, kFPCommandTransform1 + p, kPictChannelOff);
#if QUICKTIME_SUPPORT
		quicktimeLight[p]	= new TPictureControl(wind, kFPCommandQuicktime1 + p, kPictChannelOff);
#else
		synthLight[p]		= new TPictureControl(wind, kFPCommandSynth1 + p, kPictChannelOff);
#endif
		midiLight[p]		= new TPictureControl(wind, kFPCommandCoreMidi1 + p, kPictChannelOff);

		// Numeric Popups
		instrumentNumPop[p]	= new FPInstrumentPop(wind, p);
		channelNumPop[p]	= new FPMidiChannelPop(wind, p);

		velocityDragger[p]	= new FPVelocityDragger(wind, p);
		sustainDragger[p]	= new FPSustainDragger(wind, p);

		velocityDragger[p]->AssociateEditField(textField);
		sustainDragger[p]->AssociateEditField(textField);
	}
}


void FPChannelsPalette::RefreshControls() {
	UpdateTransformCheckbox();
//	UpdateTuningPopup();
	UpdateInstrumentPopup();
	UpdateVelocitySlider();
	UpdateSustainSlider();

#if QUICKTIME_SUPPORT
	UpdateQTCheckbox();
#else
	UpdateAUSynthCheckbox();
#endif

	UpdateMidiCheckbox();
	UpdateChannelPopup();
}


OSStatus FPChannelsPalette::HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event ) {
//	fprintf( stderr, "[%08X] Channels Palette Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus err = eventNotHandledErr;

	if ( event.GetClass() == kEventClassMouse && event.GetKind() == kEventMouseDown) {
		// Let the system activate the window
		err = CallNextEventHandler(inRef, event.GetEventRef());

		if ( err != noErr ) {
			if (currentFocus) {
				void *owner = currentFocus->GetOwner();
				if (owner != NULL) {
					((TPictureDragger*)owner)->ActivateField(false);
					currentFocus = NULL;
					err = noErr;
				}
			}
		}
	}

	if (err == eventNotHandledErr)
		err = FPPalette::HandleEvent(inRef, event);

	return err;
}


void FPChannelsPalette::UpdateInstrumentPopup(PartIndex part) {
	UInt16	plo, phi;
	if (part < 0) {
		plo = 0;
		phi = DOC_PARTS - 1;
	}
	else
		plo = phi = part;

	for (int i=plo; i<=phi; i++) {
		instrumentNumPop[i]->DrawNow();
		CFStringRef iname = player->GetInstrumentName(i);
		instrumentNumPop[i]->SetHelpTag(iname);
	}
}


void FPChannelsPalette::UpdateTuningPopup() {
	tuningPopup->SetMaximum(tuningPopup->CountItems());

	MenuItemIndex	firstIndex = customTuningList.FindMatchingMenuItem(tuningPopup->GetMenu(), guitarPalette->CurrentTuning());

	if (firstIndex == kInvalidItemIndex)
		(void)GetIndMenuItemWithCommandID(tuningPopup->GetMenu(), kFPCommandGuitarTuning, 1, NULL, &firstIndex);

	tuningPopup->SetValue(firstIndex);
}


void FPChannelsPalette::UpdateTransformCheckbox(PartIndex part) {
	UInt16	plo, phi;
	PartRange(part, plo, phi);

	for (int i=plo; i<=phi; i++)
		transLight[i]->SetState(player->GetTransformFlag(i));
}


void FPChannelsPalette::UpdateVelocitySlider(PartIndex part) {
	UInt16	plo, phi;
	PartRange(part, plo, phi);

	for (int i=plo; i<=phi; i++)
		velocityDragger[i]->SetValue(player->GetVelocity(i));
}


void FPChannelsPalette::UpdateSustainSlider(PartIndex part) {
	UInt16	plo, phi;
	PartRange(part, plo, phi);

	for (int i=plo; i<=phi; i++)
		sustainDragger[i]->SetValue(player->GetSustain(i));
}

#if QUICKTIME_SUPPORT

void FPChannelsPalette::UpdateQTCheckbox(PartIndex part) {
	quicktimeMaster->SetState(player->IsQuicktimeEnabled());

	UInt16	plo, phi;
	PartRange(part, plo, phi);

	for (int i=plo; i<=phi; i++) {
		quicktimeLight[i]->SetState(player->GetQuicktimeOutEnabled(i));
	}
}

#else

void FPChannelsPalette::UpdateAUSynthCheckbox(PartIndex part) {
	synthMaster->SetState(player->IsAUSynthEnabled());
	
	UInt16	plo, phi;
	PartRange(part, plo, phi);
	
	for (int i=plo; i<=phi; i++) {
		synthLight[i]->SetState(player->GetAUSynthOutEnabled(i));
	}
}

#endif


void FPChannelsPalette::UpdateMidiCheckbox(PartIndex part) {
	bool isMidi = player->IsMidiEnabled();
	midiMaster->SetState(isMidi);

	UInt16	plo, phi;
	PartRange(part, plo, phi);

	for (int i=plo; i<=phi; i++) {
		midiLight[i]->SetState(player->GetMidiOutEnabled(i));
	}
}


void FPChannelsPalette::UpdateChannelPopup(PartIndex part) {
	UInt16	plo, phi;
	PartRange(part, plo, phi);

	for (int i=plo; i<=phi; i++)
		channelNumPop[i]->DrawNow();
}


