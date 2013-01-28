/*! \file UITextLabel.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#include RADPCH
#include "UITextLabel.h"
#include "../World/Lua/D_Typeface.h"
#include <Runtime/StringBase.h>

using world::D_Typeface;

namespace ui {

TextLabel::TextLabel() : m_clip(false) {
}

TextLabel::TextLabel(const Rect &r) : MatWidget(r), m_clip(false) {
}

void TextLabel::SetText(
	const r::TextModel::String *strings,
	int numStrings
) {
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
) {
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

void TextLabel::Clear() {
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

	if (m_textModel) {
		m_textModel->SetText(strings, numStrings);
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
) {
	RAD_ASSERT(m_textModel);

	if (m_textModel) {
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
	}
}

bool TextLabel::BindTypeface(
	const pkg::AssetRef &typeface
) {
	if (typeface && m_typeface && typeface.get() == m_typeface.get())
		return true;
	if (!typeface) {
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

	return true;
}

void TextLabel::Dimensions(float &w, float &h) {
	RAD_ASSERT(m_textModel);
	float x = 0.f;
	float y = 0.f;

	w = h = 0.f;
	if (m_textModel)
		m_textModel->Dimensions(x, y, w, h);

	w += x;
	h += y;
}

void TextLabel::CreateFromTable(lua_State *L) {
	MatWidget::CreateFromTable(L);
	
	lua_getfield(L, -1, "clipRect");
	if (lua_istable(L, -1)) {
		m_clip = true;
		m_clipRect = lua::Marshal<Rect>::Get(L, -1, true);
	} else if (lua_isboolean(L, -1)) {
		m_clip = lua_toboolean(L, -1) ? true : false;
		if (m_clip)
			m_clipRect = *this->rect.get();
	} else {
		m_clip = false;
	}
	lua_pop(L, 1);

	lua_getfield(L, -1, "typeface");
	D_Typeface::Ref f = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", -1, false);
	if (f)
		BindTypeface(f->asset);
	lua_pop(L, 1);
}

int TextLabel::lua_SetText(lua_State *L) {
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

int TextLabel::lua_Dimensions(lua_State *L) {
	Ref r = GetRef<TextLabel>(L, "TextLabel", 1, true);

	float w, h;
	r->Dimensions(w, h);
	lua_pushnumber(L, w);
	lua_pushnumber(L, h);
	return 2;
}

void TextLabel::OnDraw(const Rect *outerClip) {
	MatWidget::OnDraw(outerClip);

	if (!m_textModel || !m_parser)
		return;

	Rect screen = this->screenRect;
	Rect clip;

	if (m_clip) {
		clip = Map(m_clipRect);
		if (outerClip)
			clip.Intersect(*outerClip);
	}

	rbDraw->DrawTextModel(
		screen,
		m_clip ? &clip : outerClip ? outerClip : 0,
		this->zRotScreen,
		*m_parser->material.get(),
		*m_textModel,
		true,
		this->color
	);
}

void TextLabel::AddedToRoot() {
	MatWidget::AddedToRoot();
	Root::Ref r = this->root;
	if (r && m_parser)
		r->AddTickMaterial(m_parser->materialAsset);
}

void TextLabel::PushCallTable(lua_State *L) {
	MatWidget::PushCallTable(L);
	lua_pushcfunction(L, lua_SetText);
	lua_setfield(L, -2, "SetText");
	lua_pushcfunction(L, lua_Dimensions);
	lua_setfield(L, -2, "Dimensions");
	LUART_REGISTER_GETSET(L, ClipRect);
	LUART_REGISTER_SET(L, Typeface);
}

UIW_GET(TextLabel, ClipRect, Rect, m_clipRect);
int TextLabel::LUART_SETFN(ClipRect)(lua_State *L) {
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	if (lua_istable(L, 2)) {
		w->m_clip = true;
		w->m_clipRect = lua::Marshal<Rect>::Get(L, 2, true);
	} else if (lua_isboolean(L, 2)) {
		w->m_clip = lua_toboolean(L, 2) ? true : false;
		if (w->m_clip)
			w->m_clipRect = *w->rect.get();
	} else {
		w->m_clip = false;
	}

	return 0;
}

int TextLabel::LUART_SETFN(Typeface)(lua_State *L) {
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	D_Typeface::Ref f = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 2, false);
	if (f)
		w->BindTypeface(f->asset);
	return 0;
}

} // ui
