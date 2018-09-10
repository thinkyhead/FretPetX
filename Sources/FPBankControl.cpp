//
//  FPBankControl.cpp
//  FretPetX
//
//  Created by Scott Lahteine on 12/1/12.
//
//

#include "FPBankControl.h"
#include "FPHistory.h"
#include "FPDocControls.h"

#pragma mark -
//-----------------------------------------------
//
// FPBankControl
//
FPBankControl::FPBankControl(WindowRef wind) : TCustomControl(wind, 'docu', 102) {
	const EventTypeSpec	bankEvents[] = {
		/*
		 { kEventClassControl, kEventControlDragEnter },
		 { kEventClassControl, kEventControlDragLeave },
		 { kEventClassControl, kEventControlDragWithin },
		 { kEventClassControl, kEventControlDragReceive },	// */
		{ kEventClassControl, kMouseTrackingMouseEntered },
		{ kEventClassControl, kMouseTrackingMouseExited },
		{ kEventClassMouse, kEventMouseWheelMoved }
	};
	
	RegisterForEvents(GetEventTypeCount(bankEvents), bankEvents);
}


bool FPBankControl::MouseWheelMoved(SInt16 delta, SInt16 deltaX) {
	FPDocWindow *wind = (FPDocWindow*)GetTWindow();
	wind->MouseWheelMoved(delta);
	return true;
}


ControlPartCode FPBankControl::HitTest(Point where) {
	FPDocWindow *wind = (FPDocWindow*)GetTWindow();
	return wind->IsInteractive() ? kControlIndicatorPart : kControlNoPart;
}


bool FPBankControl::Track(MouseTrackingResult eventType, Point where) {
	enum {
		kFPClickNone,
		kFPClickLock,
		kFPClickChord,
		kFPClickCircle,
		kFPClickRepeat,
		kFPClickPattern
	};
	
	static ChordIndex		oldLine, startLine;
	static UInt16			startStep, startString;
	static SInt16			oldStep, oldString;
	static UInt16			eventMode;
	static bool				circleSet, shiftMode;
	static PatternMask		olds[NUM_STRINGS];
	static FPHistoryEvent	*event;
	
	FPDocWindow				*window = (FPDocWindow*)GetTWindow();
	ChordIndex				line = oldLine,
	pline = where.v / kLineHeight - (where.v < 0 ? 1 : 0);
	bool					b;
	int						segment;
	
	switch (eventType) {
		case kMouseTrackingMouseDown: {
			Point here = { where.v - pline * kLineHeight, where.h };
			line = pline + window->TopLine();
			
			eventMode = kFPClickNone;
			if (line < window->DocumentSize()) {
				startLine = line;
				eventMode = kFPClickChord;
				if (line == window->GetCursor()) {
					CGPoint point = { here.h, here.v };
					if (CGRectContainsPoint(FPDocWindow::circRect, point) && CircleSegment(here, CIRC/2, 4.0, 0.0, OCTAVE) > 0)
						eventMode = kFPClickCircle;
					else if (PtInRect(here, &FPDocWindow::lockRect))
						eventMode = kFPClickLock;
					else if (PtInRect(here, &FPDocWindow::repeatRect))
						eventMode = kFPClickRepeat;
					else if (PtInRect(here, &FPDocWindow::patternRect)) {
						SInt16	string, step;
						GetPatternPosition(here, string, step);
						eventMode = (step > globalChord.PatternSize()) ? kFPClickChord : kFPClickPattern;
					}
				}
			}
			
			shiftMode = (eventMode == kFPClickChord && HAS_SHIFT(GetCurrentEventKeyModifiers()) && !(player->IsPlaying() && fretpet->IsFollowViewEnabled()));
			
		} // pass-thru
			
		case kMouseTrackingMouseDragged: {
			if ( eventMode == kFPClickPattern || eventMode == kFPClickRepeat || eventMode == kFPClickCircle )
				line = startLine;
			else
				line = pline + window->TopLine();
			
			break;
		}
			
		case kMouseTrackingMouseUp: {
			line = startLine;
			
			//
			// TODO: Test chord for changes and maybe save an undo item
			//
			if ( eventMode == kFPClickPattern )
				window->UpdateTrackingRegions(true);
			
			if ( window->HasSelection() && window->SelectionEnd() == window->GetCursor() )
				window->document->SetSelection(-1);
			
			break;
		}
	}
	
	int realV = where.v;
	where.v -= (startLine - window->TopLine()) * kLineHeight;
	
	switch (eventMode) {
			//
			// Chord Click: Drag to extend the selection
			//
		case kFPClickChord:
			switch (eventType) {
				case kMouseTrackingMouseDown: {
					oldLine = line;
					if (shiftMode) {
						// Move the cursor, preserving the selection
						ChordIndex selEnd = window->SelectionEnd();
						startLine = (selEnd >= 0) ? selEnd : window->GetCursor();
						window->MoveCursor(line, true);
					} else
						// Just move the cursor
						window->MoveCursor(line);
					
					break;
				}
					
				case kMouseTrackingMouseUp: {
					FPDocWindow::DisposeScrollTimer();
					break;
				}
					
				case kMouseTrackingMouseDragged: {
					if (realV < 0 || realV > (int)Height()) {
						window->StartScrollTimer(realV - (realV < 0 ? 0 : Height()));
					}
					else {
						FPDocWindow::DisposeScrollTimer();
						
						CONSTRAIN(line, MAX(0, window->TopLine()), MIN(window->DocumentSize()-1, window->BottomLine()));
						
						if (oldLine != line) {
							window->MoveCursor(line, !(fretpet->IsFollowViewEnabled() && player->IsPlaying()));
							oldLine = line;
						}
					}
					
				} break;
					
			} break;
			
			//
			// Circle Click: Enable and disable tones
			//
		case kFPClickCircle:
			where.h -= CIRCX;
			where.v -= CIRCY;
			segment = KEY_FOR_INDEX(CircleSegment(where, CIRC/2, 4.0, 0.0, OCTAVE));
			b = globalChord.HasTone(segment);
			switch (eventType) {
				case kMouseTrackingMouseDown:
					event = window->history->UndoStart(UN_EDIT, CFSTR("Edit Chord"), kAllChannelsMask);
					event->SaveCurrentBefore();
					
					circleSet = !b;
					
				case kMouseTrackingMouseDragged:
					if (circleSet) {
						if (!b) fretpet->DoAddTone(segment);
					}
					else {
						if (b) fretpet->DoSubtractTone(segment);
					}
					break;
					
				case kMouseTrackingMouseUp:
					event->SaveCurrentAfter();
					event->Commit();
					break;
			}
			break;
			
		case kFPClickRepeat: {
			switch (eventType) {
				case kMouseTrackingMouseDown:
					event = window->history->UndoStart(UN_EDIT, CFSTR("Edit Repeat"), kAllChannelsMask);
					event->SaveCurrentBefore();
					
				case kMouseTrackingMouseDragged: {
					int rep = (where.v - REPTY) / 2 - 1;
					CONSTRAIN(rep, 0, MAX_REPEAT - 1);
					fretpet->DoSetChordRepeat(MAX_REPEAT - rep);
					break;
				}
					
				case kMouseTrackingMouseUp:
					event->SaveCurrentAfter();
					event->Commit();
					break;
			}
			break;
		}
			
		case kFPClickLock: {
			switch (eventType) {
				case kMouseTrackingMouseDown:
					fretpet->DoToggleChordLock();
					break;
			}
			break;
		}
			
		case kFPClickPattern: {
			SInt16 string, ostep, step, ssize;
			GetPatternPosition(where, string, ostep);
			step = ostep;
			CONSTRAIN(step, 0, MAX_BEATS-1);
			ssize = globalChord.PatternSize();
			
			if (fretpet->IsTablatureReversed())
				string = NUM_STRINGS - 1 - string;
			
			enum {
				kPatternDragNone,
				kPatternDragLength,
				kPatternDragEdit,
				kPatternDragEditColumn,
				kPatternDragEditCLine,
				kPatternDragEditLine,
				kPatternDragMove,
				kPatternDragDupe
			};
			
			static int dragType = kPatternDragNone;
			static bool onMode;
			switch (eventType) {
				case kMouseTrackingMouseDown:
					
					if (ostep == ssize) {
						dragType = kPatternDragLength;
						oldStep = ostep;
						
						event = window->history->UndoStart(UN_EDIT, CFSTR("Edit Pattern Length"), kAllChannelsMask);
						event->SaveCurrentBefore();
					}
					else if (ostep < ssize) {
						event = window->history->UndoStart(UN_EDIT, CFSTR("Edit Pattern"), kAllChannelsMask);
						event->SaveCurrentBefore();
						
						UInt16 mods = GetCurrentEventKeyModifiers();
						
						dragType =	HAS_SHIFT(mods) ?
						HAS_OPTION(mods) ? kPatternDragDupe : kPatternDragMove :
						HAS_COMMAND(mods) ?
						/* HAS_OPTION(mods) ? kPatternDragEditCLine : */ kPatternDragEditLine :
						HAS_OPTION(mods) ? kPatternDragEditColumn : kPatternDragEdit;
						
						onMode = !globalChord.GetPatternDot(string, step);
						startString = string;
						startStep = ostep;
						oldString = oldStep = -1;
						
						for (int i=NUM_STRINGS; i--;)
							olds[i] = globalChord.GetPatternMask(i);
					}
					else {
						dragType = kPatternDragNone;
					}
					// fall through
					
				case kMouseTrackingMouseDragged: {
					if (oldString == string && oldStep == ostep)
						break;
					
					oldString = string;
					oldStep = ostep;
					
					switch(dragType) {
						case kPatternDragEdit:
							if (step < ssize && window->GetChordPatternDot(string, step) != onMode) {
								if (onMode)
									fretpet->DoSetPatternDot(string, step);
								else
									fretpet->DoClearPatternDot(string, step);
							}
							break;
							
						case kPatternDragEditColumn:
							if (step < ssize) {
								if (onMode)
									fretpet->DoSetPatternStep(step);
								else
									fretpet->DoClearPatternStep(step);
							}
							break;
							
						case kPatternDragEditCLine:
						case kPatternDragEditLine: {
							SInt16	x = startStep;
							SInt16	y = startString;
							SInt16	dh = step - x;
							SInt16	dv = string - y;
							SInt16	ah = SGN(dh); dh = ABS(dh);
							SInt16	av = SGN(dv); dv = ABS(dv);
							SInt16	dt = MAX(dh, dv);
							SInt16	ch = (dt + 1) / 2, cv = ch;
							
							PatternMask	news[NUM_STRINGS];
							bzero(news, sizeof(news));
							
							for (int i=0; i<=dt; i++) {
								news[y] |= BIT(x);
								
								if ((ch += dh) > dt) {
									ch -= dt;
									x += ah;
								}
								
								if ((cv += dv) > dt) {
									cv -= dt;
									y += av;
								}
							}
							
							for (int s=NUM_STRINGS; s--;) {
								globalChord.GetPatternMask(s) = (dragType == kPatternDragEditCLine)
								?	( news[s] ^ olds[s] )
								:	( onMode ? (news[s] | olds[s]) : (~news[s] & olds[s]) );
							}
							
							fretpet->ReflectPatternChanges();
							break;
						}
							
						case kPatternDragMove:
						case kPatternDragDupe: {
							PatternMask	news[NUM_STRINGS];
							
							SInt16	x = startStep,	y = startString;
							SInt16	dh = step - x,	dv = string - y;
							
							if (dh || dv) {
								for (int s=NUM_STRINGS; s--;) {
									int ss = LIMIT(s + dv, NUM_STRINGS);
									news[ss] = (dragType == kPatternDragDupe) ? olds[ss] : 0x0000;
									
									for (int b=ssize; b--;)
										if (olds[s] & BIT(b))
											news[ss] |= BIT(LIMIT(b + dh, ssize));
									
									globalChord.GetPatternMask(ss) = news[ss];
								}
								
							}
							else {
								for (int s=NUM_STRINGS; s--;)
									globalChord.GetPatternMask(s) = olds[s];
							}
							
							fretpet->ReflectPatternChanges();
							
							break;
						}
							
						case kPatternDragLength:
							CONSTRAIN(ostep, 1, MAX_BEATS);
							if (ostep != ssize)
								fretpet->DoSetPatternSize(ostep);
							break;
					}
					break;
				}
					
				case kMouseTrackingMouseUp: {
					switch (dragType) {
						case kPatternDragEdit:
						case kPatternDragEditColumn:
						case kPatternDragEditCLine:
						case kPatternDragEditLine:
						case kPatternDragLength:
						case kPatternDragMove:
						case kPatternDragDupe:
							event->SaveCurrentAfter();
							event->Commit();
							break;
					}
					break;
				}
			}
			break;
		}
			
	}
	
	return true;
}

void FPBankControl::GetPatternPosition(Point where, SInt16 &string, SInt16 &ostep) {
	SInt16 str = NUM_STRINGS - 1 - (where.v - SEQY - (SEQV/2)) / SEQV;
	CONSTRAIN(str, 0, NUM_STRINGS-1);
	string = str;
	
	ostep = (where.h - SEQX) / SEQH;
}

bool FPBankControl::Draw(const CGContextRef gc, const Rect &bounds) {
	// TODO: Before this can draw we need to load assets from image files
	// Either converted from RSRC or saved out of Photoshop!
	return false;
}
