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
#include <Runtime/Container/StackVector.h>

using world::D_Typeface;

namespace ui {

TextLabel::TextLabel() : m_parser(0) {
}

TextLabel::TextLabel(const Rect &r) : MatWidget(r), m_parser(0) {
}

void TextLabel::SetText(
	const r::TextModel::String *strings,
	int numStrings
) {
	RAD_ASSERT(m_textModel);

	if (m_textModel) {
		m_textModel->SetText(strings, numStrings);
	}
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

void TextLabel::AllocateText(
	const char **utf8Strings,
	int numStrings) {
	RAD_ASSERT(m_textModel);
	if (m_textModel) {
		m_textModel->AllocateText(utf8Strings, numStrings);
	}
}

void TextLabel::Clear() {
	if (m_textModel)
		m_textModel->Clear();
}

bool TextLabel::BindTypeface(
	const pkg::AssetRef &typeface
) {
	if (typeface && m_typeface && typeface.get() == m_typeface.get())
		return true;
	if (!typeface) {
		m_typeface.reset();
		m_textModel.reset();
		m_parser = 0;
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

	Root::Ref r = this->root;
	if (r)
		r->AddTickMaterial(m_parser->materialAsset);

	return true;
}

Vec4 TextLabel::RAD_IMPLEMENT_GET(dimensions) {
	RAD_ASSERT(m_textModel);
	float x;
	float y;
	float w = 0.f;
	float h = 0.f;

	if (m_textModel)
		m_textModel->Dimensions(x, y, w, h);

	return Vec4(x, y, w, h);
}

void TextLabel::CreateFromTable(lua_State *L) {
	MatWidget::CreateFromTable(L);
	
	lua_getfield(L, -1, "typeface");
	D_Typeface::Ref f = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", -1, false);
	if (f)
		BindTypeface(f->asset);
	lua_pop(L, 1);
}

int TextLabel::lua_SetText(lua_State *L) {
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);

	if (lua_type(L, 2) == LUA_TTABLE) {
		typedef zone_vector<r::TextModel::String, ZUIT>::type TextModelStringVec;
		TextModelStringVec modelStrings;
		StringVec strings;

		for (int i = 0; ; ++i) {
			lua_pushinteger(L, i+1);
			lua_gettable(L, 2);
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				break;
			}
			luaL_checktype(L, -1, LUA_TTABLE);

			r::TextModel::String modelString(0);
			
			lua_pushstring(L, "x");
			lua_gettable(L, -2);
			modelString.x = (float)luaL_checknumber(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "y");
			lua_gettable(L, -2);
			modelString.y = (float)luaL_checknumber(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "text");
			lua_gettable(L, -2);
			strings.push_back(String(luaL_checkstring(L, -1)));
			lua_pop(L, 1);
			
			lua_pushstring(L, "kern");
			lua_gettable(L, -2);
			if (!lua_isnil(L, -1))
				modelString.kern = lua_toboolean(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "kernScale");
			lua_gettable(L, -2);
			if (!lua_isnil(L, -1))
				modelString.kernScale = (float)luaL_checknumber(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "scaleX");
			lua_gettable(L, -2);
			if (!lua_isnil(L, -1))
				modelString.scaleX = (float)luaL_checknumber(L, -1);
			lua_pop(L, 1);

			lua_pushstring(L, "scaleY");
			lua_gettable(L, -2);
			if (!lua_isnil(L, -1))
				modelString.scaleY = (float)luaL_checknumber(L, -1);
			lua_pop(L, 1);

			modelStrings.push_back(modelString);
			lua_pop(L, 1);
		}

		for (int i = 0; i < (int)modelStrings.size(); ++i) {
			modelStrings[i].utf8String = strings[i].c_str;
		}

		if (!modelStrings.empty())
			w->SetText(&modelStrings[0], (int)modelStrings.size());

	} else {

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
	}

	return 0;
}

int TextLabel::lua_AllocateText(lua_State *L) {
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);

	if (lua_type(L, 2) == LUA_TTABLE) {

		typedef stackify<zone_vector<const char*, ZUIT>::type, 64> StringPtrVec;
		StringPtrVec v;

		for (int i = 0; ; ++i) {
			lua_pushinteger(L, i+1);
			lua_gettable(L, 2);
			if (lua_isnil(L, -1)) {
				lua_pop(L, 1);
				break;
			}
			v->push_back(luaL_checkstring(L, -1));
			lua_pop(L, 1);
		}

		if (!v->empty())
			w->AllocateText(&v[0], (int)v->size());

	} else {
		const char *sz = luaL_checkstring(L, 2);
		w->AllocateText(&sz, 1);
	}

	return 0;
}

void TextLabel::OnDraw(const Rect *clip) {
	MatWidget::OnDraw(clip);

	if (!m_textModel || !m_parser)
		return;

	Rect screen = this->screenRect;
	
	rbDraw->DrawTextModel(
		screen,
		clip,
		this->zRotScreen,
		*m_parser->material.get(),
		*m_textModel,
		true,
		this->blendedColor
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
	lua_pushcfunction(L, lua_AllocateText);
	lua_setfield(L, -2, "AllocateText");
	LUART_REGISTER_GET(L, Dimensions);
	LUART_REGISTER_GETSET(L, Typeface);
}

int TextLabel::LUART_SETFN(Typeface)(lua_State *L) {
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	D_Typeface::Ref f = lua::SharedPtr::Get<D_Typeface>(L, "D_Typeface", 2, false);
	if (f)
		w->BindTypeface(f->asset);
	return 0;
}

int TextLabel::LUART_GETFN(Typeface)(lua_State *L) {
	Ref w = GetRef<TextLabel>(L, "TextLabel", 1, true);
	if (!w->m_typeface)
		return 0;
	D_Typeface::Ref f = D_Typeface::New(w->m_typeface);
	f->Push(L);
	return 1;
}

UIW_GET(TextLabel, Dimensions, Vec4, dimensions);

} // ui
