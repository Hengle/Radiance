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

	void Clear();

	bool BindTypeface(const pkg::AssetRef &typeface);
	void Dimensions(float &w, float &h);

protected:

	virtual void OnDraw(const Rect *clip);
	virtual void AddedToRoot();
	virtual void PushCallTable(lua_State *L);
	virtual void CreateFromTable(lua_State *L);
	
private:

	void SetText(
		lua_State *L,
		const r::TextModel::String *strings,
		int numStrings
	);

	void SetText(
		lua_State *L,
		const char *utf8String, 
		float x = 0.0f, 
		float y = 0.0f, 
		float z = 0.0f, 
		bool kern = true, 
		float kernScale = 1.0f, 
		float scaleX = 1.0f, 
		float scaleY = 1.0f
	);

	UIW_DECL_GETSET(ClipRect);
	UIW_DECL_SET(Typeface);

	static int lua_SetText(lua_State *L);
	static int lua_Dimensions(lua_State *L);

	pkg::Asset::Ref m_typeface;
	asset::TypefaceParser::Ref m_parser;
	r::TextModel::Ref m_textModel;
	Vec4 m_color;
	Rect m_clipRect;
	bool m_clip;
	bool m_multiline;
};

} // ui
