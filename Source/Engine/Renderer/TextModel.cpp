// TextModel.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "TextModel.h"
#include "../Assets/FontParser.h"
#include "../App.h"
#include "../Engine.h"
#include <Runtime/Font/Font.h>
#include <Runtime/StringBase.h>
#include <algorithm>
#include <limits>
#undef min
#undef max

namespace {

enum {
	PageSize = 256
};

} // namespace
namespace r {

TextModel::FontInstanceHash TextModel::s_fontHash;
TextModel::GlyphPageFactory TextModel::s_factory;

TextModel::TextModel() :
m_orgX(0.f),
m_orgY(0.f),
m_width(0.f),
m_height(0.f),
m_fontWidth(10),
m_fontHeight(10),
m_curPass(0) {
}

TextModel::TextModel(
	const pkg::AssetRef &font, 
	int fontWidth, 
	int fontHeight
) :
m_orgX(0.f),
m_orgY(0.f),
m_width(0.f),
m_height(0.f),
m_fontWidth(10),
m_fontHeight(10),
m_curPass(0) {
	SetFont(font, fontWidth, fontHeight);
}

TextModel::~TextModel() {
}

void TextModel::SetFont(const pkg::AssetRef &font, int fontWidth, int fontHeight) {
	RAD_ASSERT(font);
	m_font = font;
	if (fontWidth == 0)
		fontWidth = m_fontWidth;
	if (fontHeight == 0)
		fontHeight = m_fontHeight;
	BindFont(fontWidth, fontHeight);
}

void TextModel::SetSize(int fontWidth, int fontHeight) {
	if (m_fontWidth != fontWidth || m_fontHeight != fontHeight)
		BindFont(fontWidth, fontHeight);
}

void TextModel::Clear() {
	m_passes.clear();
	m_width = 0;
	m_height = 0;
}

void TextModel::Dimensions(float &x, float &y, float &w, float &h) {
	x = m_orgX;
	y = m_orgY;
	w = m_width;
	h = m_height;
}

void TextModel::SetText(
	const String &string
) {
	BuildTextVerts(&string, 1);
}

void TextModel::SetText(
	const String *strings,
	int numStrings
) {
	BuildTextVerts(strings, numStrings);
}

void TextModel::SetText(
	const char *utf8String, 
	float x,
	float y,
	float z,
	bool kern, 
	float kernScale, 
	float scaleX, 
	float scaleY
) {
	String s(utf8String, x, y, z, kern, kernScale, scaleX, scaleY);
	BuildTextVerts(&s, 1);
}

void TextModel::AllocateText(
	const char **utf8Strings,
	int numStrings
) {
	int len = 0;
	for (int i = 0; i < numStrings ; ++i) {
		if (!utf8Strings[i])
			continue;
		len += (int)::string::len(utf8Strings[i]);
	}

	if (len > 0)
		ReserveVerts(len*6);
}

void TextModel::BuildTextVerts(const String *strings, int numStrings) {
	BOOST_STATIC_ASSERT(sizeof(VertexType)==16);
	RAD_ASSERT(m_fontInstance);
	m_passes.clear();

	m_orgX = std::numeric_limits<float>::max();
	m_orgY = std::numeric_limits<float>::max();
	m_width = 0;
	m_height = 0;

	if (numStrings) {
		int len = 0;
		for (int i = 0; i < numStrings ; ++i) {
			if (!strings[i].utf8String)
				continue;
			len += (int)::string::len(strings[i].utf8String);
		}

		if (len < 1)
			return; // null string.

		VertexType *ptr = LockVerts(len*6);
		if (!ptr) {
			RAD_OUT_OF_MEM(ptr);
			return;
		}

		font::GlyphCache &cache = m_fontInstance->cache;

		int ofs = 0;
		
		for (int i = 0; i < numStrings ; ++i) {
			const String &string = strings[i];
			if (!string.utf8String || string.utf8String[0] == 0)
				continue;

			font::GlyphCache::Batch *b = cache.BeginStringBatch(
				string.utf8String, 
				0, 
				0, 
				string.kern, 
				string.kernScale
			);

			float height = 0;

			while (b) {
				RAD_ASSERT(ptr);
				Pass pass;

				pass.ofs = ofs;
				pass.page = b->page;

				for (int i = 0; i < (int)b->numChars; ++i) {
					const ::font::GlyphCache::Metrics *m = b->chars[i];

					// two triangles per character.
					// CCW

					VertexType *v = ptr;

					// TRI 1

					float x = (m->draw.x1 * string.scaleX);
					float y = (m->draw.y1 * string.scaleY);

					m_width = std::max(m_width, x);
					height  = std::max(height, y);

					v->x = x + string.x;
					v->y = y + string.y;

					m_orgX = std::min(m_orgX, v->x);
					m_orgY = std::min(m_orgY, v->y);

					v->s = m->bitmap.x1 / PageSize;
					v->t = m->bitmap.y1 / PageSize;
					
					++v;

					x = (m->draw.x1 * string.scaleX);
					y = (m->draw.y2 * string.scaleY);
					
					m_width = std::max(m_width, x);
					height  = std::max(height, y);

					v->x = x + string.x;
					v->y = y + string.y;

					m_orgX = std::min(m_orgX, v->x);
					m_orgY = std::min(m_orgY, v->y);

					v->s = m->bitmap.x1 / PageSize;
					v->t = m->bitmap.y2 / PageSize + (0.5f / PageSize);
					
					++v;

					x = (m->draw.x2 * string.scaleX);
					y = (m->draw.y1 * string.scaleY);
					
					m_width = std::max(m_width, x);
					height  = std::max(height, y);

					v->x = x + string.x;
					v->y = y + string.y;

					m_orgX = std::min(m_orgX, v->x);
					m_orgY = std::min(m_orgY, v->y);

					v->s = m->bitmap.x2 / PageSize + (0.5f / PageSize);
					v->t = m->bitmap.y1 / PageSize;
					
					++v;

					// TRI 2

					*v = ptr[2];
					++v;
					*v = ptr[1];
					++v;

					x = (m->draw.x2 * string.scaleX);
					y = (m->draw.y2 * string.scaleY);
					
					m_width = std::max(m_width, x);
					height  = std::max(height, y);

					v->x = x + string.x;
					v->y = y + string.y;

					m_orgX = std::min(m_orgX, v->x);
					m_orgY = std::min(m_orgY, v->y);

					v->s = m->bitmap.x2 / PageSize + (0.5f / PageSize);
					v->t = m->bitmap.y2 / PageSize + (0.5f / PageSize);
					
					ptr += 6;
				}

				ofs += b->numChars;
				pass.num = b->numChars;
				m_passes.push_back(pass);
				b = cache.NextStringBatch();
			}

			m_height = std::max(m_height, height+string.y);

			cache.EndStringBatch();
		}

		UnlockVerts();
	}
}

TextModel::FontInstance::~FontInstance() {
	TextModel::s_fontHash.erase(it);
}

font::IGlyphPage::Ref TextModel::GlyphPageFactory::AllocatePage(int width, int height) {
	return TextModel::AllocatePage(width, height);
}

void TextModel::GlyphPageFactory::NoopDelete(font::IGlyphPageFactory*) {
}

void TextModel::BindFont(int fontWidth, int fontHeight) {
	Clear();

	if (m_fontInstance)
		m_fontInstance.reset();

	m_fontWidth = fontWidth;
	m_fontHeight = fontHeight;

	FontKey key(m_font->id, fontWidth, fontHeight);
	FontInstanceHash::iterator it = s_fontHash.find(key);
	if (it != s_fontHash.end()) {
		m_fontInstance = it->second.lock();
		if (m_fontInstance)
			return;
	}

	asset::FontParser *parser = asset::FontParser::Cast(m_font);
	RAD_VERIFY(parser && parser->font.get());

	m_fontInstance.reset(new (ZFonts) FontInstance());
	m_fontInstance->font = m_font;
	m_fontInstance->cache.Create(
		*parser->font.get(),
		fontWidth,
		fontHeight,
		0,
		0,
		::font::Pixels,
		PageSize,
		PageSize,
		1,
		std::numeric_limits<int>::max(),
		font::IGlyphPageFactory::Ref(&s_factory, GlyphPageFactory::NoopDelete)
	);

	m_fontInstance->it = s_fontHash.insert(FontInstanceHash::value_type(key, m_fontInstance)).first;
}

void TextModel::BeginPass(const r::Material &material, int i) {
	RAD_ASSERT(i < (int)m_passes.size());
	Pass &p = m_passes[i];
	RAD_ASSERT(p.page);
	BindStates(material, p.ofs, p.num, *p.page);
	m_curPass = &p;
}

void TextModel::DrawVerts(const r::Material &material) {
	RAD_ASSERT(m_curPass);
	DrawVerts(material, m_curPass->ofs, m_curPass->num);
}

void TextModel::EndPass() {
	m_curPass = 0;
}

void TextModel::Draw(r::Material &material, bool sampleMaterialColor, const Vec4 &rgba, int flags, int blends) {
	material.BindStates(flags, blends);
	BatchDraw(material, sampleMaterialColor, rgba);
}

void TextModel::BatchDraw(r::Material &material, bool sampleMaterialColor, const Vec4 &rgba) {
	Shader::Uniforms u(rgba);

	for (int i = 0; i < numPasses; ++i) {
		BeginPass(material, i);
		material.shader->Begin(Shader::kPass_Default, material);
		material.shader->BindStates(u, sampleMaterialColor);
		App::Get()->engine->sys->r->CommitStates();
		DrawVerts(material);
		material.shader->End();
	}
}

} // r
