/*! \file UITextLabel.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#pragma once

#include "UIMatWidget.h"
#include "../Renderer/TextModel.h"
#include "../Assets/TypefaceParser.h"
#include <Runtime/PushPack.h>

namespace ui {

class RADENG_CLASS TextLabel : public MatWidget {
public:
	typedef boost::shared_ptr<TextLabel> Ref;
	typedef boost::shared_ptr<TextLabel> WRef;

	TextLabel();
	TextLabel(const Rect &r);

	void SetText(
		const r::TextModel::String *strings,
		int numStrings
	);

	void SetText(
		const char *utf8String, 
		float x = 0.0f, 
		float y = 0.0f, 
		float z = 0.0f, 
		bool kern = true, 
		float kernScale = 1.0f, 
		float scaleX = 1.0f, 
		float scaleY = 1.0f
	);

	void AllocateText(
		const char **utf8Strings,
		int numStrings
	);

	void Clear();

	bool BindTypeface(const pkg::AssetRef &typeface);
	

	RAD_DECLARE_READONLY_PROPERTY(TextLabel, dimensions, Vec2);

protected:

	virtual void OnDraw(const Rect *clip);
	virtual void AddedToRoot();
	virtual void PushCallTable(lua_State *L);
	virtual void CreateFromTable(lua_State *L);
	
private:

	RAD_DECLARE_GET(dimensions, Vec2);

	UIW_DECL_GETSET(Typeface);
	UIW_DECL_GET(Dimensions);

	static int lua_SetText(lua_State *L);
	static int lua_AllocateText(lua_State *L);

	pkg::Asset::Ref m_typeface;
	asset::TypefaceParser::Ref m_parser;
	r::TextModel::Ref m_textModel;
	Vec4 m_color;
	bool m_multiline;
};

} // ui

#include <Runtime/PopPack.h>
