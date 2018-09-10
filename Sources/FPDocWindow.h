/*
 *  FPDocWindow.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 *  The FPDocWindow encompasses the document view.
 *
 */

#ifndef FPDOCWINDOW_H
#define FPDOCWINDOW_H

#define kTabHeight		54
#define kLineHeight		60
#define kDocWidth		362
#define kScrollbarSize	15

#define kDocCascadeX	10
#define kDocCascadeY	20

class TString;
class FPDocWindow;
class FPCloneSheet;
class FPHistory;
class FPBankControl;
class FPTabsControl;
class FPTempoSlider;
class FPVelocitySlider;
class FPSustainSlider;
class FPTempoDoubler;
class FPDocPopup;

#include "FPWindow.h"
#include "FPApplication.h"
#include "FPDocument.h"

/*! A structure to track scrolling */
typedef struct {
	FPDocWindow	*target;
	int			distance;
	int			counter;
	bool		keepSelection;
} ScrollInfo;

#pragma mark -
//-----------------------------------------------
//
//	FPDocWindow
//

#include <list>
typedef	std::list<FPDocWindow*>	FPDocWindowList;
typedef	FPDocWindowList::iterator	FPDocumentIterator;

/*!
 *	The document window is also a sequencer
 */
class FPDocWindow : public FPWindow {
private:
	static bool closingAllDocuments;
	static EventLoopTimerUPP scrollUPP;		// For smart drag-scrolling
	static EventLoopTimerRef scrollLoop;

	static SInt32		untitledCount;
	static GWorldPtr	chordCursGW, chordSelGW, chordGrayGW;
	static GWorldPtr	overlayCursGW, overlaySelGW, overlayGrayGW;
	static GWorldPtr	itemDrawGW, headingGW, headDrawGW;
	static GWorldPtr	repStretchGW1, repTopGW1, repStretchGW2, repTopGW2;

	static CGContextRef	itemBackCurrent, itemBackSelect, itemBackGray;
	static CGContextRef	itemOverCurrent, itemOverSelect, itemOverGray;
//		static CGContextRef	cursOverCurrent, cursOverSelect, cursOverGray;

	ControlActionUPP	liveScrollUPP;
	ControlRef			scrollbar;
	TControl			*scrollControl;

	bool				didFirstResize;
	
	FPBankControl		*sequencerControl;
	FPTabsControl		*tabsControl;
	FPDocPopup			*docPopControl;
	FPTempoSlider		*tempoControl;
	FPVelocitySlider	*velocityControl;
	FPSustainSlider		*sustainControl;
	FPTempoDoubler		*doubleTempoButton;

	TTrackingRegion		*tempoRollover;
	TTrackingRegion		*velocityRollover;
	TTrackingRegion		*sustainRollover;
	TTrackingRegion		*sequencerRollover, *repeatRollover, *lengthRollover, *blockedRollover;
	SInt32				sequencerRolloverID, repeatRolloverID, lengthRolloverID;

	Rect				currentSize;

	FPCloneSheet		*cloneSheet;

public:
	SInt16				lastBeatDrawn, lastChordDrawn;

	static FPDocWindowList	docWindowList;
	static FPDocWindow	*activeWindow, *frontWindow;
	FPDocument			*document;
	bool				isClosing;

	FPHistory			*history;
	static Rect			itemRect, repeatRect, lockRect, headRect, numRect, patternRect, lengthRect, tabOnRect, overRect1, overRect2;
	static CGRect		circRect;

						FPDocWindow(CFStringRef windowTitle);
						FPDocWindow(const FSSpec &fileSpec);
						FPDocWindow(const FSRef &fileRef);
						~FPDocWindow();

	void				Init();
	void				InitWindowAssets();
	OSStatus			InitFromFile(const FSRef &fileRef);
	static FPDocWindow*	NewUntitled();
	void				Cascade(SInt16 h, SInt16 v);
	void				SetModified(bool modified, bool always=false)		{ if (!modified || always || DocumentSize() > 0) TWindow::SetModified(modified); }
	inline void			SetCursorAndSelection(ChordIndex cursor, ChordIndex selEnd)
																			{ MoveCursor(cursor); SetSelection(selEnd, false); }

	// Convenience accessors
	inline ChordIndex	DocumentSize()										{ return document->Size(); }
	inline bool			IsDirty()											{ return IsWindowModified(Window()); }
	inline bool			IsBlocked()											{ return (activeWindow != this || document->HasDialog()); }
	inline ChordIndex	GetCursor()											{ return document->GetCursor(); }
	inline ChordIndex	GetSelection(ChordIndex *start, ChordIndex *end)	{ return document->GetSelection(start, end); }
	inline PartIndex	CurrentPart()										{ return document->CurrentPart(); }
	inline PartMask		CurrentPartMask()									{ return BIT(CurrentPart()); }
	inline ChordIndex	TopLine()											{ return document->TopLine(); }
	inline ChordIndex	VisibleLines()										{ return (Height() - kTabHeight) / kLineHeight; }
	inline ChordIndex	BottomLine()										{ return TopLine() + VisibleLines() - 1; }
	inline bool			HasSelection()										{ return document->HasSelection(); }
	inline ChordIndex	SelectionEnd()										{ return document->SelectionEnd(); }
	inline bool			HasPattern()										{ return document->HasPattern(); }
	inline bool			ReadyToPlay()										{ return document->ReadyToPlay(); }
	inline bool			GetChordPatternDot(UInt16 string, UInt16 step)		{ return document->GetChordPatternDot(string, step); }
	void				PlayStarted();
	void				PlayStopped();
	bool				IsInteractive();
	bool				IsRangePlaying();

	// Adding ChordGroups and Chords
	bool				Insert(FPChordGroup *srcGroup, ChordIndex count, bool bResetSeq);
	bool				Insert(FPChordGroupArray &srcArray);
	bool				AddChord(bool bResetSeq=false);

	void				SetScale(UInt16 newMode);
	void				SetCurrentPart(PartIndex p);
	PartMask			GetTransformMask();
	PartMask			GetTransformMaskForMenuItem(MenuRef inMenu, SInt16 inItem);

	void				PageUp();
	void				PageDown();
	void				Home();
	void				End();
	void				MoveCursor(ChordIndex line, bool keepSel=false, bool justOld=false);
	inline void			SetCurrentChord(const FPChord &srcChord)	{ document->SetCurrentChord(srcChord); UpdateLine(); }
	void				UpdateTrackingRegions(bool noExit=false);
	inline void			UpdateGlobalChord()							{ if (DocumentSize()) fretpet->SetGlobalChord(document->CurrentChord(), true); }
	inline void			UpdateDocumentFingerings()					{ document->UpdateFingerings(); InvalidateWindow(); }
	void				SetSelection(ChordIndex newSel, bool bScroll);
	void				SelectAll();
	void				SelectNone();
	void				DeleteSelection(bool undoable=true, bool inserting=false);
	void				TransformSelection(MenuCommand cid, MenuItemIndex index, PartMask partMask, bool undoable=true);
	void				CloneSelection(ChordIndex count, PartMask clonePartMask, UInt16 cloneTranspose, UInt16 cloneHarmonize, bool undoable=true);
	void				DrawAfterTransform();

	void				RunCloneDialog();
	void				KillCloneDialog();

	// Drawing of lines
	void				Draw();
	void				DrawHeading();
	void				UpdateLine(ChordIndex line=-1);
	void				UpdateSelection();
	void				DrawVisibleLines();
	void				DrawLines(ChordIndex min, ChordIndex max);
	bool				DrawLine(ChordIndex line);
	bool				DrawLineNumber(ChordIndex line);
	bool				EraseLine(ChordIndex line);
	void				DrawCircle(CGContextRef gc, UInt16 tones, UInt16 root, UInt16 key);

	// Scrolling for insert, delete, and cursor
	void				ScrollLines(ChordIndex top, ChordIndex distance, bool bFix);
	bool				ScrollToReveal(ChordIndex newLine);

	// Music player animation and follow
	void				FollowPlayer();
	void				DrawPlayDots(bool force=false);
	void				DrawOneBeat(ChordIndex chord, SInt16 beat, bool light);

	// Controls
	void				RefreshControls();
	void				UpdateDocPopup();
	void				UpdateTempoSlider();
	void				UpdateDoubleTempoButton();
	void				UpdateVelocitySlider();
	void				UpdateSustainSlider();

	// FPWindow overrides
	void				TryToClose(bool closeAll=false);
	void				Close();
	void				DoCloseAll();

#if !DEMO_ONLY
	OSStatus			DoSaveAs();
#endif

	// Window methods mirroring document methods
	// to do windowy things corresponding with nav events
	bool				RunDialogsModal();
	void				NavCancel();
	void				NavDiscardChanges();
	void				NavDontSave()								{ if (isClosing) Close(); }
	void				NavDidSave();
	void				HandleNavError(OSStatus err);
	void				TerminateNavDialogs();
	bool				AskTuningChange();

	// Local Clipboard methods
	void				DoCopy();
	void				DoCut();
	void				DoPaste();
	void				DoPastePattern();
	void				DoPastePattern(FPChordGroupArray &clipSource, UInt16 part);
	void				DoPasteTones();
	void				DoPasteTones(FPChordGroupArray &clipSource, UInt16 part);
	void				DoReplace(FPChordGroupArray &groupArray, bool undoable=true, bool replace=false);
	void				ReplaceSelection(FPChordGroupArray &src, UInt16 partMask=kAllChannelsMask);
	void				ReplaceRange(FPChordGroupArray &src, ChordIndex start, ChordIndex end, UInt16 partMask=kAllChannelsMask);
	void				ReplaceAll(FPChordGroupArray &src, UInt16 partMask=kAllChannelsMask);

	void				DoToggleTempoMultiplier(bool undoable=true);

	// Event handling
	Point				GetIdealSize();
	OSStatus			HandleEvent(EventHandlerCallRef inRef, TCarbonEvent &event);
	void				HandleTrackingRegion(TTrackingRegion *region, TCarbonEvent &event);
	void				ConstrainSize(TCarbonEvent &inEvent);
	void				Resized();
	void				Activated();
	void				Deactivated();
	bool				MouseWheelMoved(SInt16 delta, SInt16 deltaX=0);

	// Receiving a drag
	OSErr				DragEnter(DragRef theDrag);
	OSErr				DragLeave(DragRef theDrag);
	OSErr				DragWithin(DragRef theDrag);
	OSErr				DragReceive(DragRef theDrag);

	bool				UpdateCommandStatus(MenuCommand cid, MenuRef menu, MenuItemIndex index);
	bool				HandleCommand(MenuCommand cid, MenuRef menu, MenuItemIndex index);
	void				LiveScroll(ControlHandle control, SInt16 part);
	void				UpdateScrollState();
	void				SetScrollPosition(ChordIndex pos)			{ scrollControl->SetValue(pos); }

	static pascal void	LiveScrollProc(ControlHandle control, SInt16 part);
	static void			UpdateAll();

	void				StartScrollTimer(int distance);
	static void			DisposeScrollTimer();
	static void			ScrollTimerProc( EventLoopTimerRef timer, void *info );
	void				DoScrollTimer(ScrollInfo *info);

	static int			CountDirtyDocuments();
	static void			CancelSheets();

	const CFStringRef	PreferenceKey()	{ return CFSTR("bank"); }
};

enum {
	kPictDocHeading = 1200,
	kPictTabOn
};

#endif
