/*
 *  FPCommands.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#pragma mark FretPet Menu
#define kFPCommandAbout				'AbFP'

#pragma mark File Menu
#define kFPCommandCloseAll			'CLOS'
#define kFPCommandExportMovie		'Xmov'
#define kFPCommandExportMIDI		'Xmid'
#define kFPCommandExportSunvox		'Xsun'
#define kFPCommandRecentItem		'RecE'
#define kFPCommandClearRecent		'RecC'
#define kFPCommandRecentItemSubmenu	'SubR'


#pragma mark Edit Menu
#define kFPCommandPastePattern		'Pseq'
#define kFPCommandPasteTones		'Pton'
#define kFPCommandSelectNone		'Sel0'
#define kFPCommandPrefabSubmenu		'SubP'


#pragma mark Scale Menu
#define kFPCommandScaleMode			'Mode'
#define kFPCommandModePrev			'Mod+'
#define kFPCommandModeNext			'Mod-'
#define kFPCommandScaleShift		'Shft'


#pragma mark Chord Menu
#define kFPCommandStrumUp			'StrU'
#define kFPCommandStrumDown			'StrD'
#define kFPCommandHearButton		'HerB'
#define kFPCommandStrike			'Strk'
#define kFPCommandClearTones		kHICommandClear
#define kFPCommandClearTonesButton	'KlrB'
#define kFPCommandClearPattern		'ClrS'
#define kFPCommandClearBoth			'ClrB'
#define kFPCommandInvertChord		'Invt'
#define kFPCommandTransformMode		'TrnM'
#define kFPCommandTransformOnce		'TrnO'
#define kFPCommandTransposeBy		'TrnI'
#define kFPCommandTransposeKey		'TrnK'
#define kFPCommandTransposeNorth	'TrnN'
#define kFPCommandTransposeSouth	'TrnS'
#define kFPCommandHarmonizeUp		'Har+'
#define kFPCommandHarmonizeDown		'Har-'
#define kFPCommandHarmonizeBy		'HarS'
#define kFPCommandTransBySubmenu	'Subt'
#define kFPCommandTransToSubmenu	'SubT'
#define kFPCommandHarmonizeSubmenu	'SubH'
#define kFPCommandLock				'Lock'

#pragma mark Guitar Menu
#define kFPCommandGuitarImage		'Gimg'
#define kFPCommandTuningName		'Tnam'
#define kFPCommandGuitarTuning		'Tune'
#define kFPCommandCustomTuning		'Ctun'
#define kFPCommandCustomizeTunings	'Cust'
#define kFPCommandBracket			'Brak'
#define kFPCommandHorizontal		'Horz'
#define kFPCommandLeftHanded		'Left'
#define kFPCommandReverseStrung		'Revs'
#define kFPCommandUnusedTones		'Unus'
#define kFPCommandImageSubmenu		'SuGI'
#define kFPCommandTuningSubmenu		'SuGT'

#define kFPCommandDotsDots			'DotD'
#define kFPCommandDotsLetters		'LetD'
#define kFPCommandDotsNumbers		'NumD'


#pragma mark Diagram Palette
#define kFPCommandNumRoot			'NumR'
#define kFPCommandNumKey			'NumK'


#pragma mark Sequencer Menu
#define kFPCommandPlay				'Play'
#define kFPCommandStop				'Stop'
#define kFPCommandLoop				'Loop'
#define kFPCommandEye				'Eye '
#define kFPCommandFree				'Free'
#define kFPCommandSolo				'Solo'
#define kFPCommandCountIn			'CtIn'
#define kFPCommandAddChord			'Add '

#if QUICKTIME_SUPPORT
	#define kFPCommandQuicktimeOut	'outQ'
#else
	#define kFPCommandAUSynthOut		'outQ'
#endif

#define kFPCommandMidiOut			'outM'
#define kFPCommandSplitMidiOut		'outS'

#define kFPCommandSetDefaultSequence 'DefS'

#define kFPCommandPickInstrument	'Pick'
#define kFPCommandInstrumentMenu	'SuGM'
#define kFPCommandInstrument		'Inst'
#define kFPCommandGSInstrument		'InGS'
#define kFPCommandInstGroup			'iGrp'


#pragma mark Filter Menu
#define kFPCommandSelScramble		'Scrm'
#define kFPCommandSelDouble			'Sdbl'
#define kFPCommandSelCompact		'Scmp'
#define kFPCommandSelSplay			'Sexp'
#define kFPCommandSelReverseChords	'RevC'
#define kFPCommandSelReverseMelody	'RevM'
#define kFPCommandSelClearPatterns	'Scls'
#define kFPCommandSelHFlip			'Shfl'
#define kFPCommandSelVFlip			'Svfl'
#define kFPCommandSelRandom1		'Srn1'
#define kFPCommandSelRandom2		'Srn2'
#define kFPCommandSelClearTones		'Sclt'
#define kFPCommandSelInvertTones	'Sinv'
#define kFPCommandSelCleanupTones	'Scln'
#define kFPCommandSelLockRoots		'Slok'
#define kFPCommandSelUnlockRoots	'Sunl'
#define kFPCommandSelTransposeBy	'StBy'
#define kFPCommandSelTransposeTo	'StTo'
#define kFPCommandSelHarmonizeBy	'SHaS'
#define kFPCommandSelHarmonizeUp	'SHa+'
#define kFPCommandSelHarmonizeDown	'SHa-'
#define kFPCommandFilterSubmenuAll		'FilA'
#define kFPCommandFilterSubmenuCurrent	'FilC'
#define kFPCommandFilterSubmenuEnabled	'FilE'
#define kFPCommandSelHarmonizeSub	'SuF1'
#define kFPCommandSelTransposeToSub	'SuF2'
#define kFPCommandSelTransposeBySub	'SuF3'


#pragma mark (Clone)
#define kFPCommandSelClone			'Clon'
#define kFPCommandSelCloneYes		'CloY'
#define kFPCommandCloneTransform	'CTra'
#define kFPCommandCloneTranspose	'CTrn'
#define kFPCommandCloneHarmonize	'CHar'


#pragma mark Window Menu
#define kFPCommandToolbar			'Tool'
#define kFPCommandAnchorToolbar		'Anch'
#define kFPCommandStackDocuments	'Stak'
#define kFPCommandResetPalettes		'Rset'

#define kFPCommandWindInfo			'Info'
#define kFPCommandWindChannels		'Chan'
#define kFPCommandWindScale			'Scal'
#define kFPCommandWindGuitar		'Guit'
#define kFPCommandWindChord			'Chrd'
#define kFPCommandWindCircle		'Circ'
#define kFPCommandWindPiano			'Pian'
#define kFPCommandWindDiagram		'Diag'
#define kFPCommandWindMetronome		'Metr'
#define kFPCommandWindAnalysis		'Anal'
#define kFPCommandWindAll			'AllP'

#define kFPCommandReverseStrings	'RevS'

#pragma mark Help Menu
#define kFPCommandDocumentation		'Docs'
#define kFPCommandReleaseNotes		'Note'
#define kFPCommandWebsite			'Site'

#define kFPCommandCheckForUpdate	'sCUP'

#if KAGI_SUPPORT
#define kFPCommandEnterSerial		'Seri'
#define kFPCommandDeauthorize		'Daut'
#endif

#if KAGI_SUPPORT || DEMO_ONLY
#define kFPCommandRegister			'Regi'
#define kFPCommandShowNagger		'Nagg'
#endif

#pragma mark Toolbar buttons
#define kFPCommandAddTriad			'Tria'
#define kFPCommandSubtractTriad		'Tri-'
#define kFPCommandAddScale			'Ascl'
#define	kFPCommandRoman				'Rome'
#define kFPCommandFifths			'Fift'
#define kFPCommandHalves			'Half'


#pragma mark Scale Buttons
#define kFPCommandScaleBack			'Sbak'
#define kFPCommandScaleFwd			'Sfwd'
#define kFPCommandFlat				'Flat'
#define kFPCommandNatural			'Natu'
#define kFPCommandSharp				'Shrp'
#define kFPCommandBulb				'Bulb'


#pragma mark Chord Buttons
#define kFPCommandMoreChords		'Cmor'


#pragma mark Circle Buttons
#define kFPCommandCircleEye			'Ceye'
#define kFPCommandHold				'Hold'
#define kFPCommandRelativeMinor		'relM'


#pragma mark Info Controls
#define kFPInfoTextField			'Itxt'

#define kFPInfoCheckTransform		'pTrn'
#define kFPInfoCheckCM				'ChCM'
#define kFPInfoMidiChannel			'MidO'

#define kFPCommandTransform1		'TRP1'
#define kFPCommandTransform2		'TRP2'
#define kFPCommandTransform3		'TRP3'
#define kFPCommandTransform4		'TRP4'
#define kFPCommandTransformSolo		'TRSO'

#if QUICKTIME_SUPPORT

	#define kFPInfoCheckQT				'ChQT'
	#define kFPCommandQuicktime1		'QTP1'
	#define kFPCommandQuicktime2		'QTP2'
	#define kFPCommandQuicktime3		'QTP3'
	#define kFPCommandQuicktime4		'QTP4'
	#define kFPCommandQuicktimeSolo		'QTSO'
	#define kFPCommandQuicktimeMaster	kFPCommandQuicktimeOut

#else

	#define kFPInfoCheckSynth			'ChQT'
	#define kFPCommandSynth1			'QTP1'
	#define kFPCommandSynth2			'QTP2'
	#define kFPCommandSynth3			'QTP3'
	#define kFPCommandSynth4			'QTP4'
	#define kFPCommandSynthSolo			'QTSO'
	#define kFPCommandSynthMaster		kFPCommandAUSynthOut

#endif

#define kFPCommandCoreMidi1			'CMP1'
#define kFPCommandCoreMidi2			'CMP2'
#define kFPCommandCoreMidi3			'CMP3'
#define kFPCommandCoreMidi4			'CMP4'
#define kFPCommandCoreMidiSolo		'CMSO'
#define kFPCommandCoreMidiMaster	kFPCommandMidiOut

#define kFPCommandInstPop1			'INP1'
#define kFPCommandInstPop2			'INP2'
#define kFPCommandInstPop3			'INP3'
#define kFPCommandInstPop4			'INP4'

#define kFPCommandChannelPop1		'CHP1'
#define kFPCommandChannelPop2		'CHP2'
#define	kFPCommandChannelPop3		'CHP3'
#define kFPCommandChannelPop4		'CHP4'

#define kFPCommandVelocityDrag1		'VEP1'
#define kFPCommandSustainDrag1		'SUP1'

#pragma mark Document Commands
#define kFPDoubleTempo				'Mult'


