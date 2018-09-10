/*
 *  FPSpriteLayer.h
 *  FretPetX
 *
 *  Created by Scott Lahteine on 3/18/07.
 *  Copyright © 2012 Scott Lahteine. All rights reserved.
 *
 */

#ifndef FPSPRITELAYER_H
#define FPSPRITELAYER_H

#include "TObjectDeque.h"

class FPSprite;
class FPContext;

class FPSpriteLayer : public TObjectDeque<FPSprite> {
	private:
		FPContext	*context;
		bool		visible;

	public:
//						FPSpriteLayer() {}
						FPSpriteLayer(FPContext *c);
		virtual			~FPSpriteLayer() {}

		void			Draw();
		virtual void	ProcessAndAnimate();
		inline void		AppendSprite(FPSprite *sprite) { push_back(sprite); }
		inline int		SpriteCount()	{ return size(); }

		inline void		SetVisible(bool v) { visible = v; }
		inline void		Show() { SetVisible(true); }
		inline void		Hide() { SetVisible(false); }
};

typedef FPSpriteLayer::iterator FPSpriteIterator;

#endif
