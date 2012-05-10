// UITextLabel.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "UITextLabel.h"
#include "../World/Lua/D_Typeface.h"
#include <Runtime/StringBase.h>

using world::D_Typeface;

namespace ui {

TextLabel::TextLabel() : m_autoSize(false), m_clip(false), m_syncClipRect(true)
{
}

TextLabel::TextLabel(const Rect &r) : Widget(r), m_autoSize(false), m_clip(false), m_syncClipRect(true)
{
}

void TextLabel::SetText(
	const r::TextModel::String *strings,
	int numStrings
)
{
	SetText(0, strings, numStrings);
}

void TextLabel::SetText(
	const char *utf8String, 
	float x,
	float y, 
	float z,
	bool kern,
	float kernScale,
	float scaleX, 
	float scaleY
)
{
	SetText(
		0,
		utf8String,
		x,
		y,
		z,
		kern,
		kernScale,
		scaleX,
		scaleY
	);
}

void TextLabel::Clear()
{
	if (m_textModel)
		m_textModel->Clear();
}

void TextLabel::SetText(
	lua_State *L,
	const r::TextModel::String *strings,
	int numStrings
)
{
	RAD_ASSERT(m_textModel);

	if (m_textModel)
	{
		m_textModel->SetText(strings, numStrings);
		RecalcSize();
	}
}

void TextLabel::SetText(
	lua_State *L,
	const char *utf8String, 
	float x,
	float y, 
	float z,
	bool kern,
	float kernScale,
	float scaleX, 
	float scaleY
)
{
	RAD_ASSERT(m_textModel);

	if (m_textModel)
	{
		m_textModel->SetText(
			utf8String,
			x,
			y,
			z,
			kern,
			kernScale,
			scaleX,
			scaleY
		);
		RecalcSize();
	}
}

bool TextLabel::BindTypeface(
	const pkg::AssetRef &typeface
)
{
	if (typeface && m_typeface && typeface.get() == m_typeface.get())
		return true;
	if (!typeface)
	{
		m_typeface.reset();
		m_textModel.reset();
		m_parser.reset();
		return true;
	}

	if (typeface->type != asset::AT_Typeface)
		return false;

	m_typeface = typeface;

	m_parser = asset::TypefaceParser::Cast(typeface);
	if (!m_parser || !m_parser->valid)
		return false;

	if (!m_textModel)
		m_textModel = r::TextModel::New();

	m_textModel->SetFont(
		m_parser->fontAsset,
		m_parser->width,
		m_parser->height
	);

	RecalcSize();
	return true;
}

void TextLabel::Dimensions(float &w, float &h)
{
	RAD_ASSERT(m_textModel);
	float x = 0.f;
	float y = 0.f;

	w = h = 0.f;
	if (m_textModel)
		m_textModel->Dimensions(x, y, w, h);

	w += x;
	h += y;
}

void TextLabel::CreateFromTable(lua_State *L)
{
	Widget::CreateFromTable(L);
	
	lua_getfield(L, -1, "autoSize");
	if (!lua_isnil(L, -1))
		m_autoSize = lua_toboolean(L, -1) ? true : false;
	lua_pop(L, 1);

	lua_getfield(L, -1, "clipRect");
	if (lua_istable(L, -1))
	{
		m_clip = true;
		m_syncClipRect = false;
		m_clipRect = lua::Marshal<Rect>::Get(L, -1, true);
	}
	else if (lua_isboolean(L, -1))
	{
		m_syncClipRect = true;
		m_clip = lua_toboolean(L, -1) ? true : false;
		if (!m_autoSize && m_clip)
			m_clipRect = *this->rect.get();
	}
	else
	{
		m_clip = false;
		m_syncClipRect = true; // don't leave this uninitialized
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "typeface");
	D_Typeface::Ref f = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", -1, false);
	if (f)
		BindTypeface(f->asset);
	lua_pop(L, 1);
}

void TextLabel::RecalcSize()
{
	if (!m_autoSize || !m_textModel)
		return;

	Dimensions(
		this->rect->w,
		this->rect->h
	);

	if (m_syncClipRect)
		m_clipRect = *this->rect.get();
}

int TextLabel::lua_SetText(lua_State *L)
{
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);

	int numArgs = lua_gettop(L) - 1; // don't count self
	const char *string = luaL_checkstring(L, 2);

	float x = 0.f;
	float y = 0.f;

	if (numArgs > 2)
		x = (float)luaL_checknumber(L, 3);
	if (numArgs > 3)
		y = (float)luaL_checknumber(L, 4);

	bool kern = true;
	if (numArgs > 4)
		kern = lua_toboolean(L, 5) ? true : false;

	float kernScale = 1.f;
	if (numArgs > 5)
		kernScale = (float)luaL_checknumber(L, 6);

	float sx = 1.f;
	float sy = 1.f;
	if (numArgs > 6)
		sx = (float)luaL_checknumber(L, 7);
	if (numArgs > 7)
		sy = (float)luaL_checknumber(L, 8);

	w->SetText(
		string,
		x,
		y,
		0.f,
		kern,
		kernScale,
		sx,
		sy
	);

	return 0;
}

int TextLabel::lua_Dimensions(lua_State *L)
{
	Ref r = GetRef<TextLabel>(L, "TextLabel", 1, true);

	float w, h;
	r->Dimensions(w, h);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}

void TextLabel::OnDraw()
{
	if (!m_textModel || !m_parser)
		return;

	Rect screen = this->screenRect;
	Rect clip;

	if (m_clip)
		clip = Map(m_clipRect);

	rbDraw->DrawTextModel(
		screen,
		m_clip ? &clip : 0,
		*m_parser->material.get(),
		*m_textModel,
		true,
		this->color
	);
}

void TextLabel::AddedToRoot()
{
	Root::Ref r = this->root;
	if (r && m_parser)
		r->AddTickMaterial(m_parser->materialAsset);
}

void TextLabel::PushCallTable(lua_State *L)
{
	Widget::PushCallTable(L);
	lua_pushcfunction(L, lua_SetText);
	lua_setfield(L, -2, "SetText");
	lua_pushcfunction(L, lua_Dimensions);
	lua_setfield(L, -2, "Dimensions");
	LUART_REGISTER_GETSET(L, AutoSize);
	LUART_REGISTER_GETSET(L, ClipRect);
	LUART_REGISTER_SET(L, Typeface);
}

UIW_GET(TextLabel, AutoSize, bool, m_autoSize);
int TextLabel::LUART_SETFN(AutoSize)(lua_State *L)
{
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	w->m_autoSize = lua_toboolean(L, 2) ? true : false;
	w->RecalcSize();
	return 0;
}

UIW_GET(TextLabel, ClipRect, Rect, m_clipRect);
int TextLabel::LUART_SETFN(ClipRect)(lua_State *L)
{
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	if (lua_istable(L, 2))
	{
		w->m_clip = true;
		w->m_syncClipRect = false;
		w->m_clipRect = lua::Marshal<Rect>::Get(L, 2, true);
	}
	else if (lua_isboolean(L, 2))
	{
		w->m_syncClipRect = true;
		w->m_clip = lua_toboolean(L, 2) ? true : false;
		if (!w->m_autoSize && w->m_clip)
			w->m_clipRect = *w->rect.get();
	}
	else
	{
		w->m_clip = false;
		w->m_syncClipRect = true; // don't leave this uninitialized
	}
	w->RecalcSize();

	return 0;
}

int TextLabel::LUART_SETFN(Typeface)(lua_State *L)
{
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	D_Typeface::Ref f = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 2, false);
	if (f)
		w->BindTypeface(f->asset);
	return 0;
}

} // ui
