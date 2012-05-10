// Font.inl
// Platform Agnostic Font System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace font {

//////////////////////////////////////////////////////////////////////////////////////////

inline Bitmap::Bitmap() : m_bitmap(0) {
}

inline Bitmap::~Bitmap() {
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Metrics::Metrics() {
}

inline Metrics::~Metrics() {
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Glyph::Glyph() : m_glyph(0), m_slot(0) {
}

inline Glyph::~Glyph() {
}

//////////////////////////////////////////////////////////////////////////////////////////

inline UserGlyph::UserGlyph() {
}

inline UserGlyph::~UserGlyph() {
}

//////////////////////////////////////////////////////////////////////////////////////////

inline Font::Font() : m_lib(0), m_face(0), m_width(0), m_height(0) {
}

inline Font::~Font() {
	Destroy();
}

//////////////////////////////////////////////////////////////////////////////////////////

inline int GlyphCache::RAD_IMPLEMENT_GET(fontPixelWidth) {
	return m_fontWidth;
}

inline int GlyphCache::RAD_IMPLEMENT_GET(fontPixelHeight) {
	return m_fontHeight;
}

//////////////////////////////////////////////////////////////////////////////////////////

inline float GlyphCache::RAD_IMPLEMENT_GET(ascenderPixels) {
	m_font->SetPixelSize(m_fontWidth, m_fontHeight);
	return m_font->ascenderPixels;
}

inline float GlyphCache::RAD_IMPLEMENT_GET(descenderPixels) {
	m_font->SetPixelSize(m_fontWidth, m_fontHeight);
	return m_font->descenderPixels;
}

inline Font &GlyphCache::RAD_IMPLEMENT_GET(font) {
	return *m_font;
}

//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
inline void GlyphCache::MoveToList(T **old, T *item, T **_new) {
	RemoveFromList(item, old);
	AddToList(item, _new);
}

template <typename T>
inline void GlyphCache::AddToList(T *item, T **_new) {
	RAD_ASSERT(item);
	RAD_ASSERT(_new);

	item->prev = 0;
	item->next = *_new;
	if (*_new) (*_new)->prev = item;
	*_new = item;
}

template <typename T>
inline void GlyphCache::RemoveFromList(T *item, T **old) {
	RAD_ASSERT(item);
	RAD_ASSERT(old);

	if (item->prev) item->prev->next = item->next;
	if (item->next) item->next->prev = item->prev;
	if (*old == item) *old = item->next;
}

inline bool GlyphCache::RAD_IMPLEMENT_GET(allowEvict) {
	return m_allowEvict;
}

inline void GlyphCache::RAD_IMPLEMENT_SET(allowEvict) (bool b) {
	m_allowEvict = b;
}

} // font
