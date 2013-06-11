// D_Typeface.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "D_Typeface.h"
#include <Runtime/Font/Font.h>

namespace world {

D_Typeface::Ref D_Typeface::New(const pkg::AssetRef &asset) {
	return Ref(new (ZWorld) D_Typeface(asset));
}

D_Typeface::D_Typeface(const pkg::AssetRef &asset) : D_Asset(asset) {
	m_typeface = asset::TypefaceParser::Cast(asset);
}

void D_Typeface::PushElements(lua_State *L) {
	D_Asset::PushElements(L);
	lua_pushcfunction(L, lua_Size);
	lua_setfield(L, -2, "Size");
	lua_pushcfunction(L, lua_StringDimensions);
	lua_setfield(L, -2, "StringDimensions");
	lua_pushcfunction(L, lua_WordWrap);
	lua_setfield(L, -2, "WordWrap");
	lua_pushcfunction(L, lua_AscenderDescender);
	lua_setfield(L, -2, "AscenderDescender");
	lua_pushcfunction(L, lua_SplitStringAtSize);
	lua_setfield(L, -2, "SplitStringAtSize");
}

int D_Typeface::lua_Size(lua_State *L) {
	D_Typeface::Ref self = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 1, true);
	
	lua_pushinteger(L, self->typeface->width);
	lua_pushinteger(L, self->typeface->height);
	return 2;
}

int D_Typeface::lua_StringDimensions(lua_State *L) {
	D_Typeface::Ref self = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 1, true);

	float w, h;

	bool kern = (lua_gettop(L) < 3) || (lua_toboolean(L, 3) ? true : false);
	float kernScale = (lua_gettop(L) < 4) || (float)luaL_checknumber(L, 4);

	self->typeface->font->SetPixelSize(
		self->typeface->width,
		self->typeface->height
	);

	self->typeface->font->StringDimensions(
		luaL_checkstring(L, 2),
		w,
		h,
		kern,
		kernScale
	);

	lua_pushnumber(L, w);
	lua_pushnumber(L, h);

	return 2;
}

int D_Typeface::lua_WordWrap(lua_State *L) {
	D_Typeface::Ref self = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 1, true);

	bool kern = (lua_gettop(L) < 4) || (lua_toboolean(L, 4) ? true : false);
	float kernScale = (lua_gettop(L) < 5) ? 1.f : (float)luaL_checknumber(L, 5);

	self->typeface->font->SetPixelSize(
		self->typeface->width,
		self->typeface->height
	);

	StringVec strings = self->typeface->font->WordWrapString(
		luaL_checkstring(L, 2),
		(float)luaL_checknumber(L, 3),
		kern,
		kernScale
	);

	if (strings.empty())
		return 0;

	int ofs = 1;
	lua_createtable(L, strings.size(), 0);
	for (StringVec::const_iterator it = strings.begin(); it != strings.end(); ++it) {
		lua_pushinteger(L, ofs++);
		lua_pushstring(L, (*it).c_str);
		lua_settable(L, -3);
	}

	return 1;
}

int D_Typeface::lua_SplitStringAtSize(lua_State *L) {
	D_Typeface::Ref self = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 1, true);

	bool kern = (lua_gettop(L) < 4) || (lua_toboolean(L, 4) ? true : false);
	float kernScale = (lua_gettop(L) < 5) ? 1.f : (float)luaL_checknumber(L, 5);

	self->typeface->font->SetPixelSize(
		self->typeface->width,
		self->typeface->height
	);

	String first, second;

	self->typeface->font->SplitStringAtSize(
		luaL_checkstring(L, 2),
		first,
		second,
		(float)luaL_checknumber(L, 3),
		kern,
		kernScale
	);

	if (first.empty) {
		lua_pushnil(L);
	} else {
		lua_pushstring(L, first.c_str);
	}

	if (second.empty) {
		lua_pushnil(L);
	} else {
		lua_pushstring(L, second.c_str);
	}

	return 2;
}

int D_Typeface::lua_AscenderDescender(lua_State *L) {
	D_Typeface::Ref self = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 1, true);

	self->typeface->font->SetPixelSize(
		self->typeface->width,
		self->typeface->height
	);

	lua_pushnumber(L, self->typeface->font->ascenderPixels);
	lua_pushnumber(L, self->typeface->font->descenderPixels);
	return 2;
}

} // world
