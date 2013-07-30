/*! \file D_SpriteBatch.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include "../../Renderer/Sprites.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_SpriteBatch : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<D_SpriteBatch> Ref;

	static Ref New(const r::SpriteBatch::Ref &sprites);

	RAD_DECLARE_READONLY_PROPERTY(D_SpriteBatch, spriteBatch, const r::SpriteBatch::Ref&);

protected:

	D_SpriteBatch(const r::SpriteBatch::Ref &sprites);

private:

	RAD_DECLARE_GET(spriteBatch, const r::SpriteBatch::Ref&) {
		return m_spriteBatch;
	}

	r::SpriteBatch::Ref m_spriteBatch;
};

}

#include <Runtime/PopPack.h>