// D_ScreenOverlay.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include "../ScreenOverlayDef.h"
#include "D_Asset.h"
#include <Runtime/PushPack.h>

namespace world {

class World;

class RADENG_CLASS D_ScreenOverlay : public lua::SharedPtr
{
public:
	typedef boost::shared_ptr<D_ScreenOverlay> Ref;

	static Ref New(const ScreenOverlayRef &overlay);

	void FadeIn(float time);
	void FadeOut(float time);

	RAD_DECLARE_READONLY_PROPERTY(D_ScreenOverlay, fading, bool);

private:

	static int lua_FadeIn(lua_State *L);
	static int lua_FadeOut(lua_State *L);
	static int lua_Fading(lua_State *L);

	D_ScreenOverlay(const ScreenOverlayRef &overlay);

	RAD_DECLARE_GET(fading, bool);

	virtual void PushElements(lua_State *L);

	ScreenOverlayRef m_overlay;
};

} // world

#include <Runtime/PopPack.h>
