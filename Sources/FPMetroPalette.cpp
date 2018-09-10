/*
 *  FPMetroPalette.cpp
 *
 *	FretPet X
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "TCarbonEvent.h"
#include "FPApplication.h"
#include "FPMetroPalette.h"
#include "FPUtilities.h"

pascal void MetroSpriteMoveProc(SpritePtr pSprite);


FPMetroPalette::FPMetroPalette() : FPPaletteSW(CFSTR("Metronome"), 1)) {
	InitSprites();

    Show();
}


void FPMetroPalette::InitSprites() {
	SpritePtr pSprite = CreateSpriteFromCICN(300, 1);
	verify_action( pSprite!=NULL, throw TError(1000, CFSTR("Create Sprite Problem?")) );

	SWSetSpriteMoveProc(pSprite, MetroSpriteMoveProc);
	SWSetSpriteMoveTime(pSprite, 1000 / 30);
	SWAddSprite(layer[0], pSprite);

	LockSprites();
}


OSStatus FPMetroPalette::HandleEvent(EventHandlerCallRef inRef, TCarbonEvent &event) {
//	fprintf( stderr, "[%08X] Metro Palette Got Event %c%c%c%c : %d\n", this, FOUR_CHAR(event.GetClass()), event.GetKind() );

	OSStatus    result = eventNotHandledErr;

	switch ( event.GetClass() ) {
		case kEventClassWindow:
			switch ( event.GetKind() ) {
				case kEventWindowDrawContent:
//					SWProcessSpriteWorld(world);
//					SWAnimateSpriteWorld(world);
					//	fprintf(stderr, "kEventWindowDrawContent\n");
					//	result = noErr;
					break;

				case kEventWindowUpdate:
					//	fprintf(stderr, "kEventWindowUpdate\n");
					//	result = noErr;
					break;
			}
			break;
	}

	if (result == eventNotHandledErr)
		result = FPPaletteSW::HandleEvent(inRef, event);

	return result;
}


pascal void MetroSpriteMoveProc(SpritePtr pSprite) {
	static double count = 0;
	SWMoveSprite(pSprite, 50 * cos(count) + 50, 50 * sin(count) + 50);
	count = count + 0.05;
	if (count >= 360.0) count = 0.0;
}


