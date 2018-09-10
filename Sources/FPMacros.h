/*
 *  FPMacros.h
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPMACROS_H
#define FPMACROS_H

#define	FRETPET_VERSION		"1.4"
#define	MAIN_NIB_NAME		CFSTR("FretPet")
#define	CREATOR_CODE		0x46728E54

#define MICROSECOND			1000000.0

#define COUNT(x)			(sizeof(x)/sizeof(x[0]))

#if !defined(ABS)
	#define	ABS(x)			( ((x) < 0) ? -(x) : (x) )
#endif
#if !defined(MIN)
	#define	MIN(x, y)		( ((x) < (y)) ? x : y )
#endif
#if !defined(MAX)
	#define	MAX(x, y)		( ((x) > (y)) ? x : y )
#endif
#define	SGN(x)				( ((x) == 0) ? 0 : ( ((x) < 0) ? -1 : 1 ))
#define	BIT(x)				( 1L << (x) )
#define	LO(x)				((x) & 0xFFFF)
#define	HI(x)				((x) >> 16)

#define	LIMIT(x,y)			( ( ( (x) + (y) * 100 ) ) % (y) )
#define	NOTEMOD(x)			LIMIT(((x)+1200),12)
#define	CONSTRAIN(x, y, z)	{ if ((x) < (y)) { x = y; } \
								if ((x) > (z)) { x = z; } }
#define	EQU_MOD(x,y,z)		x = (y) % (z)
#define	EQU_LIMIT(x,y,z)	{ if ((x = (y)) >= (z)) x -= z; }
#define	EQU_RANGE(x,y,z)	( x = ((y) + (z) * 100) % (z) )
#define	ADD_MOD(x,y,z)		( x = ( (x) + (y) + (z) * 100 ) % z )
#define	SUB_MOD(x,y,z)		( x = ( (x) - (y) + (z) * 100 ) % z )
#define	ADD_WRAP(x,y,z)		{ if ((x += (y)) >= z) (x) -= (z); }
#define	SUB_WRAP(x,y,z)		{ if ((x -= (y)) < 0) (x) += (z); }
#define	INC_WRAP(x,y)		{ if (++x >= (y)) x = 0; }
#define	DEC_WRAP(x,y)		{ if (--x < 0) x = (y) - 1; }
#define	LOOP(x,y,z)			for (x = (y) ; x <= (z) ; x++)
#define	REPEAT(x, y)		for (x = 1 ; x <= (y) ; x++)
#define KEYBIT(x)			((theKeys[(x) >> 3] >> ((x) & 7)) & 1)
#define RAD(d)				((d) * M_PI / 180.0)
#define DEG(r)				((r) * 180.0 / M_PI)
#define FOUR_CHAR(x)		(int)((x) >> 24) & 0xFF, (int)((x) >> 16) & 0xFF, (int)((x) >> 8) & 0xFF, (int)(x) & 0xFF
#define	SetHVRect(r, x, y, h, v)	SetRect((Rect*)(r), x, y, (x)+(h), (y)+(v))
#define	SetRectX(r, x)		OffsetRect((Rect*)(r), (x) - (*(r)).left, 0)
#define	SetRectY(r, y)		OffsetRect((Rect*)(r), 0, (y) - (*(r)).top)
#define	OffsetRectX(r, x)	( (r).left += (x), (r).right += (x) )
#define	OffsetRectY(r, x)	( (r).top += (x), (r).bottom += (x) )
#define TopLeft(r)			(* (Point *) &(r).top)
#define BottomRight(r)		(* (Point *) &(r).bottom)
#define	until(x)			while (!(x))
#define SWAP(x, y, z)		( z = y, y = x, x = z )
#define MIN_NOTE(x, y)		((x)<(y)?(x)+12:(x))

#define RANDINT(x, y)		(random() % ((y)-(x)+1) + (x))

#define HAS_COMMAND(x)		(((x) & cmdKey) != 0)
#define HAS_SHIFT(x)		(((x) & shiftKey) != 0)
#define HAS_OPTION(x)		(((x) & optionKey) != 0)
#define HAS_CONTROL(x)		(((x) & controlKey) != 0)
#define IS_COMMAND(x)		(((x) & ~alphaLock) == cmdKey)
#define IS_SHIFT(x)			(((x) & ~alphaLock) == shiftKey)
#define IS_OPTION(x)		(((x) & ~alphaLock) == optionKey)
#define IS_CONTROL(x)		(((x) & ~alphaLock) == controlKey)
#define IS_CONTROL_SHIFT(x)	(((x) & ~alphaLock) == (controlKey|shiftKey))
#define IS_NOMOD(x)			(((x) & ~alphaLock) == 0)

#define CG_RGBCOLOR(x)		(float)(x)->red / 65535.0, (float)(x)->green / 65535.0, (float)(x)->blue / 65535.0

#if DEBUG_REFCOUNTS
	#import "FPUtilities.h"
	#define	CFRETAIN(x)		MyCFRetain(x)
	#define	CFRELEASE(x)	MyCFRelease(x)
#else
	#define	CFRETAIN(x)		CFRetain(x)
	#define	CFRELEASE(x)	CFRelease(x)
#endif

//
//	Global tone order used to determine tone
//
#define FIFTHS_POSITION(x)	(((x)*7)%12)
#define INDEX_FOR_KEY(x)	( fretpet->keyOrderHalves ? (x) : FIFTHS_POSITION(x) )
#define KEY_FOR_INDEX(x)	( fretpet->keyOrderHalves ? (x) : FIFTHS_POSITION(x) )

#endif
