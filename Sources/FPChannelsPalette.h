/*
 *  FPChannelsPalette.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPCHANNELSPALETTE_H
#define FPCHANNELSPALETTE_H

#include "FPPalette.h"
#include "FPHistory.h"
#include "FPMusicPlayer.h"
#include "TPictureControl.h"
#include "TControls.h"

//-----------------------------------------------
//
// FPPartPopup
//
class FPPartPopup : public TPicturePopup {
	protected:
		SInt16			partNumber;

	public:
						FPPartPopup(WindowRef wind, PartIndex p, UInt32 baseCommand, CFStringRef menuName);
		virtual			~FPPartPopup() {}

		// tells the app which channel before proceeding
		virtual bool	ControlHit(Point where);
};


#pragma mark -
//-----------------------------------------------
//
// FPInstrumentPop
//
class FPInstrumentPop : public FPPartPopup {
	private:
		static Duration delay;
		static EventLoopTimerUPP tagTimerUPP;
		static EventLoopTimerRef tagTimerLoop;
	public:
					FPInstrumentPop(WindowRef wind, PartIndex p);

		bool		ControlHit(Point where);
		bool		Draw(const Rect &bounds);
		bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
		void		ResetTagDelaySoon();
		static void	ResetTagTimerProc(EventLoopTimerRef timer, void *nada);
};


#pragma mark -
//-----------------------------------------------
//
// FPMidiChannelPop
//
class FPMidiChannelPop : public FPPartPopup {
	public:
					FPMidiChannelPop(WindowRef wind, PartIndex p)
						: FPPartPopup(wind, p, kFPCommandChannelPop1, CFSTR("ChannelPop")) {}

		bool		ControlHit(Point where);
		bool		Draw(const Rect &bounds);
		bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
};


#pragma mark -
//-----------------------------------------------
//
// FPVelocityDragger
//
class FPVelocityDragger : public TPictureDragger {
	private:
		PartIndex		partIndex;
		FPHistoryEvent	*event;

	public:
					FPVelocityDragger(WindowRef wind, PartIndex index);
					~FPVelocityDragger() {}

		bool		Draw(const Rect &bounds);
		bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
		void		DoMouseDown();
		void		DoMouseUp();
		void		ValueChanged();
};


#pragma mark -
//-----------------------------------------------
//
// FPSustainDragger
//
class FPSustainDragger : public TPictureDragger {
	private:
		PartIndex		partIndex;
		FPHistoryEvent	*event;

	public:
					FPSustainDragger(WindowRef wind, PartIndex index);
					~FPSustainDragger() {}

		bool		Draw(const Rect &bounds);
		bool		MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);
		void		DoMouseDown();
		void		DoMouseUp();
		void		ValueChanged();
};


#pragma mark -
//-----------------------------------------------
//
// FPInfoEditField
//
class FPInfoEditField : public TNumericTextField {
	public:
					FPInfoEditField(WindowRef wind, UInt32 cmd)
						: TNumericTextField(wind, cmd) { }
					~FPInfoEditField() {}

//		bool		Draw(const Rect &bounds);
		void		ValueChanged();
		bool		IsGoodKey(char keypress, UInt32 modifiers=0);
		void		DidDeactivate();
		void		HiliteChanged();
};


#pragma mark -
//-----------------------------------------------
//
// FPChannelsPalette
//
class FPChannelsPalette : public FPPalette {
	private:
		TPopupMenu			*tuningPopup;

		TPictureControl		*transLight[DOC_PARTS], *midiLight[DOC_PARTS];
		FPSustainDragger	*sustainDragger[DOC_PARTS];
		FPVelocityDragger	*velocityDragger[DOC_PARTS];
		FPPartPopup			*instrumentNumPop[DOC_PARTS], *channelNumPop[DOC_PARTS];
#if QUICKTIME_SUPPORT
		TPictureControl		*quicktimeMaster, *quicktimeSolo, *quicktimeLight[DOC_PARTS];
#else
		TPictureControl		*synthMaster, *synthSolo, *synthLight[DOC_PARTS];
#endif
		TPictureControl		*midiMaster, *midiSolo, *transformSolo;
		FPInfoEditField		*textField;

	public:
					FPChannelsPalette();
					~FPChannelsPalette() {}

		inline void	PartRange(PartIndex part, UInt16 &plo, UInt16 &phi) {
			if (part == kAllChannels) {
				plo = 0;
				phi = DOC_PARTS - 1;
			}
			else if (part == kCurrentChannel)
				plo = phi = player->CurrentPart();
			else
				plo = phi = part;
		}

		void		Init();

		void		RefreshControls();
		void		UpdateTuningPopup();
		void		UpdateCustomTunings();
		void		UpdateTransformCheckbox(SInt16 part=kAllChannels);
		void		UpdateInstrumentPopup(SInt16 part=kAllChannels);
		void		UpdateVelocitySlider(SInt16 part=kAllChannels);
		void		UpdateSustainSlider(SInt16 part=kAllChannels);
#if QUICKTIME_SUPPORT
		void		UpdateQTCheckbox(SInt16 part=kAllChannels);
#else
		void		UpdateAUSynthCheckbox(SInt16 part=kAllChannels);
#endif
		void		UpdateMidiCheckbox(SInt16 part=kAllChannels);
		void		UpdateChannelPopup(SInt16 part=kAllChannels);

		const CFStringRef PreferenceKey()	{ return CFSTR("channels"); }

		OSStatus	HandleEvent( EventHandlerCallRef inRef, TCarbonEvent &event );
};



extern FPChannelsPalette *channelsPalette;



//
// Info palette control images
//
enum {
	kPictChannelOff = 1150,	kPictChannelPress,	kPictChannelOn,
	kPictMasterOff,			kPictMasterPress,	kPictMasterOn,
	kPictSoloOff,			kPictSoloPress,		kPictSoloOn,
	kPictNumberPopOff,		kPictNumberPopOn,
	kPictNumberDragOff,		kPictNumberDragOn
};

#endif


