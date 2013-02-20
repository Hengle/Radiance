/*! \file D_SpriteBatch.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "D_SpriteBatch.h"

namespace world {

D_SpriteBatch::Ref D_SpriteBatch::New(const r::SpriteBatch::Ref &sprites) {
	D_SpriteBatch::Ref r(new D_SpriteBatch(sprites));
	return r;
}

D_SpriteBatch::D_SpriteBatch(const r::SpriteBatch::Ref &sprites) : m_spriteBatch(sprites) {
}

}
