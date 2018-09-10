/*
 *  FPSpriteLayer.cpp
 *
 *  FretPetX
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#include "FPSpriteLayer.h"
#include "FPSprite.h"


FPSpriteLayer::FPSpriteLayer(FPContext *c) {
	context = c;
	visible = true;
}



void FPSpriteLayer::Draw() {
	if (visible) {
		for(FPSpriteIterator itr = begin(); itr != end(); itr++)
			(*itr)->Draw();
	}
}



void FPSpriteLayer::ProcessAndAnimate() {
	if (visible) {
		FPSpriteIterator itr = begin();
		while(itr != end()) {
			FPSpritePtr sprite = *itr;
			if (sprite->IsDisposed()) {
				printf("Somehow it's still in the array!");
				itr = erase(itr);
			} else {
				if (sprite->MarkedForDisposal()) {
					sprite->MarkDisposed();
					itr = erase(itr);
				} else {
					itr++;
					sprite->ProcessAndAnimate();
				}
			}
		}

	}
}
