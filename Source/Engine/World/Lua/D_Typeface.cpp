// D_Typeface.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "D_Typeface.h"
#include <Runtime/Font/Font.h>

namespace world {

D_Typeface::Ref D_Typeface::New(const pkg::AssetRef &asset)
{
	return Ref(new (ZWorld) D_Typeface(asset));
}

D_Typeface::D_Typeface(const pkg::AssetRef &asset) : D_Asset(asset),
m_asset(asset)
{
	m_typeface = asset::TypefaceParser::Cast(asset);
}

void D_Typeface::PushElements(lua_State *L)
{
	D_Asset::PushElements(L);
	lua_pushcfunction(L, lua_Size);
	lua_setfield(L, -2, "Size");
	lua_pushcfunction(L, lua_StringDimensions);
	lua_setfield(L, -2, "StringDimensions");
	lua_pushcfunction(L, lua_WordWrap);
	lua_setfield(L, -2, "WordWrap");
}

int D_Typeface::lua_Size(lua_State *L)
{
	D_Typeface::Ref self = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 1, true);
	
	lua_pushinteger(L, self->typeface->width);
	lua_pushinteger(L, self->typeface->height);
	return 2;
}

int D_Typeface::lua_StringDimensions(lua_State *L)
{
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
	float kernScale = (lua_gettop(L) < 5) || (float)luaL_checknumber(L, 5);

	self->typeface->font->SetPixelSize(
		self->typeface->width,
		self->typeface->height
	);

	String text(CStr(luaL_checkstring(L, 2)));
	const float kMaxWidth = luaL_checknumber(L, 3);

	string::UTF32Buf utf = text.ToUTF32();

	lua_createtable(L, 2, 0);
	int numStrings = 0;

	const U32 *start = utf.c_str;
	const U32 *end = start;

	String cur;
	const String kSpace(CStr(" "));

	for (; *end; ++end) {
		if (*end == 32) { // space (word break)
			if (start == end) {
				continue;
			}

			String x(start, end-start);
			String test;

			if (!cur.empty) {
				test = cur + kSpace + x;
			} else {
				test = x;
			}

			float w, h;

			self->typeface->font->StringDimensions(
				test.c_str,
				w,
				h,
				kern,
				kernScale
			);

			if (w > kMaxWidth) {
				lua_pushinteger(L, numStrings+1);
				lua_pushstring(L, cur.c_str);
				lua_settable(L, -3);
				cur.Clear();
				start = end+1;
				++numStrings;
			}
		}
	}

	if (start != end) {
		String x(start, end-start);
		if (cur.empty) {
			cur = x;
		} else {
			cur = cur + kSpace + x;
		}

		lua_pushinteger(L, numStrings+1);
		lua_pushstring(L, cur.c_str);
		lua_settable(L, -3);
		++numStrings;
	}

	return (numStrings > 0) ? 1 : 0;
}

} // world
