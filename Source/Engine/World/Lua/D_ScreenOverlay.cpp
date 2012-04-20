// D_ScreenOverlay.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "D_ScreenOverlay.h"
#include "../ScreenOverlay.h"

namespace world {

D_ScreenOverlay::Ref D_ScreenOverlay::New(const ScreenOverlay::Ref &overlay)
{
	return Ref(new (ZWorld) D_ScreenOverlay(overlay));
}

D_ScreenOverlay::D_ScreenOverlay(const ScreenOverlay::Ref &overlay) :
m_overlay(overlay)
{
}

void D_ScreenOverlay::FadeIn(float time)
{
	m_overlay->FadeIn(time);
}

void D_ScreenOverlay::FadeOut(float time)
{
	m_overlay->FadeOut(time);
}

bool D_ScreenOverlay::RAD_IMPLEMENT_GET(fading)
{
	return m_overlay->fading;
}

void D_ScreenOverlay::PushElements(lua_State *L)
{
	lua_pushcfunction(L, lua_FadeIn);
	lua_setfield(L, -2, "FadeIn");
	lua_pushcfunction(L, lua_FadeOut);
	lua_setfield(L, -2, "FadeOut");
	lua_pushcfunction(L, lua_Fading);
	lua_setfield(L, -2, "Fading");
}

int D_ScreenOverlay::lua_FadeIn(lua_State *L)
{
	Ref r = lua::SharedPtr::Get<D_ScreenOverlay>(L, "ScreenOverlay", 1, true);
	float time = (float)luaL_checknumber(L, 2);
	r->FadeIn(time);
	return 0;
}

int D_ScreenOverlay::lua_FadeOut(lua_State *L)
{
	Ref r = lua::SharedPtr::Get<D_ScreenOverlay>(L, "ScreenOverlay", 1, true);
	float time = (float)luaL_checknumber(L, 2);
	r->FadeOut(time);
	return 0;
}

int D_ScreenOverlay::lua_Fading(lua_State *L)
{
	Ref r = lua::SharedPtr::Get<D_ScreenOverlay>(L, "ScreenOverlay", 1, true);
	lua_pushboolean(L, r->fading.get() ? 1 : 0);
	return 1;
}

} // world
