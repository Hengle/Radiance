// Font.cpp
// Platform Agnostic Font System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Font.h"
#include "../Thread/Interlocked.h"
#include "../Base/ObjectPool.h"
#include "../String/StringBase.h"
#include "../String/utf8.h"
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <algorithm>
#include <math.h>

#define __FT_OPT_DONT_SUPPRESS_WARNINGS__
#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_GLYPH_H
#include FT_MODULE_H

#define FT_FIXED_TO_FLOAT(x) (((float)x) / 64.0f)
#define FT_FLOAT_TO_FIXED(x) ((int)(((float)x) * 64.0f))

#undef max
#undef min

namespace font {

RAD_ZONE_DEF(RADRT_API, ZFonts, "Fonts", ZRuntime);

// private
namespace {
	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	Mutex s_mutex;

	FT_Library s_lib;
	FT_MemoryRec_ s_mem;
	thread::Interlocked<int> s_libRefs;
	ThreadSafeObjectPool<UserGlyph> s_glyphPool;

	void *Allocate(FT_Memory memory, long size) {
		return safe_zone_malloc(ZFonts, size);
	}

	void Free(FT_Memory memory, void *block) {
		zone_free(block);
	}

	void *Realloc(FT_Memory memory, long cur_size, long new_size, void *block) {
		return safe_zone_realloc(ZFonts, block, new_size);
	}

	void InitMemory() {
		s_mem.user = 0;
		s_mem.alloc = Allocate;
		s_mem.free = Free;
		s_mem.realloc = Realloc;
	}

	FT_Library RefLib() {

		if (++s_libRefs == 1) {
			InitMemory();
			s_glyphPool.Create(ZFonts, "free-type glyph pool", 32);
			Lock L(s_mutex);
			RAD_VERIFY(FT_New_Library(&s_mem, &s_lib) == 0);
			FT_Add_Default_Modules(s_lib);
		}
		return s_lib;

	}

	void ReleaseLib() {
		if (--s_libRefs == 0) {
			s_glyphPool.Destroy();
			Lock L(s_mutex);
			FT_Done_Library(s_lib);
		}
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

int Bitmap::RAD_IMPLEMENT_GET(height) {
	RAD_ASSERT(m_bitmap);
	return m_bitmap->rows;
}

int Bitmap::RAD_IMPLEMENT_GET(width) {
	RAD_ASSERT(m_bitmap);
	return m_bitmap->width;
}

AddrSize Bitmap::RAD_IMPLEMENT_GET(pitch) {
	RAD_ASSERT(m_bitmap);
	return (AddrSize)m_bitmap->pitch;
}

const U8* Bitmap::RAD_IMPLEMENT_GET(data) {
	RAD_ASSERT(m_bitmap);
	return m_bitmap->buffer;
}

//////////////////////////////////////////////////////////////////////////////////////////

void Metrics::Copy(const FT_Glyph_Metrics_ *metrics) {
	BOOST_STATIC_ASSERT(sizeof(FT_Glyph_Metrics_)  <= SizeofMetrics);
	RAD_ASSERT(metrics);
	memcpy(m_metrics, metrics, SizeofMetrics);
}

float Metrics::RAD_IMPLEMENT_GET(width) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->width);
}

float Metrics::RAD_IMPLEMENT_GET(height) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->height);
}

float Metrics::RAD_IMPLEMENT_GET(horzBearingX) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->horiBearingX);
}

float Metrics::RAD_IMPLEMENT_GET(horzBearingY) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->horiBearingY);
}

float Metrics::RAD_IMPLEMENT_GET(horzAdvance) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->horiAdvance);
}

float Metrics::RAD_IMPLEMENT_GET(vertBearingX) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->vertBearingX);
}

float Metrics::RAD_IMPLEMENT_GET(vertBearingY) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->vertBearingY);
}

float Metrics::RAD_IMPLEMENT_GET(vertAdvance) {
	RAD_ASSERT(m_metrics);
	return FT_FIXED_TO_FLOAT(((FT_Glyph_Metrics*)m_metrics)->vertAdvance);
}

//////////////////////////////////////////////////////////////////////////////////////////

void Glyph::Setup(FT_GlyphRec_ *glyph,
	FT_GlyphSlotRec_ *slot,
	const FT_Glyph_Metrics_ *metrics
) {
	RAD_ASSERT(glyph||slot);
	RAD_ASSERT(metrics);
	m_glyph = glyph;
	m_slot = slot;
	m_metrics.Copy(metrics);
}

int Glyph::RAD_IMPLEMENT_GET(left) {
	if (m_slot) {
		RAD_ASSERT(m_slot->format == FT_GLYPH_FORMAT_BITMAP);
		return m_slot->bitmap_left;
	} else {
		RAD_ASSERT(m_glyph && m_glyph->format == FT_GLYPH_FORMAT_BITMAP);
		return ((const FT_BitmapGlyph)m_glyph)->left;
	}
}

int Glyph::RAD_IMPLEMENT_GET(top) {
	if (m_slot)	{
		RAD_ASSERT(m_slot->format == FT_GLYPH_FORMAT_BITMAP);
		return m_slot->bitmap_top;
	} else {
		RAD_ASSERT(m_glyph && m_glyph->format == FT_GLYPH_FORMAT_BITMAP);
		return ((const FT_BitmapGlyph)m_glyph)->top;
	}
}

const Bitmap *Glyph::RAD_IMPLEMENT_GET(bitmap) {
	if (m_slot) {
		RAD_ASSERT(m_slot->format == FT_GLYPH_FORMAT_BITMAP);
		m_bitmap.m_bitmap = &m_slot->bitmap;
	} else {
		RAD_ASSERT(m_glyph && m_glyph->format == FT_GLYPH_FORMAT_BITMAP);
		m_bitmap.m_bitmap = &reinterpret_cast<const FT_BitmapGlyph>(const_cast<const FT_Glyph>(m_glyph))->bitmap;
	}

	return &m_bitmap;
}

const Metrics *Glyph::RAD_IMPLEMENT_GET(metrics) {
	RAD_ASSERT(m_glyph||m_slot);
	return &m_metrics;
}

bool Glyph::Render() {
	if (m_slot) {
		if (m_slot->format != FT_GLYPH_FORMAT_BITMAP) {
			Lock L(s_mutex);
			if (FT_Render_Glyph(m_slot, FT_RENDER_MODE_NORMAL) != 0)
				return false;
		}
	} else {
		RAD_ASSERT(m_glyph);
		if (m_glyph->format != FT_GLYPH_FORMAT_BITMAP) {
			Lock L(s_mutex);
			if (FT_Glyph_To_Bitmap(&m_glyph, FT_RENDER_MODE_NORMAL, 0, 1) != 0)
				return false;
		}
	}
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

void UserGlyph::Release() {
	if (m_glyph) {
		{
			Lock L(s_mutex);
			FT_Done_Glyph(m_glyph);
		}
		m_glyph = 0;
		s_glyphPool.Destroy(this);
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

bool Font::Create(const void *data, AddrSize size) {
	m_lib = RefLib();
	int error;
	{
		Lock L(s_mutex);
		error = FT_New_Memory_Face(m_lib, (const FT_Byte*)data, (FT_Long)size, 0, &m_face);
	}
	if (error)
		Destroy();
	return error == 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

void Font::Destroy() {
	if (m_face) {
		Lock L(s_mutex);
		FT_Done_Face(m_face);
		m_face = 0;
	}
	if (m_lib) {
		ReleaseLib();
		m_lib = 0;
	}
}

int Font::GlyphIndex(U32 utf32Char) const {
	RAD_ASSERT(m_face);
	Lock L(s_mutex);
	return (int)FT_Get_Char_Index(m_face, utf32Char);
}

void Font::SetTransform(float *_2x2, float *translate2d) {
	RAD_ASSERT(m_face);
	FT_Matrix m;
	FT_Vector d;

	m.xx = (FT_Fixed)(_2x2[0] * (float)(1<<16));
	m.xy = (FT_Fixed)(_2x2[1] * (float)(1<<16));
	m.yx = (FT_Fixed)(_2x2[2] * (float)(1<<16));
	m.yy = (FT_Fixed)(_2x2[3] * (float)(1<<16));

	d.x = (FT_Pos)(translate2d[0] * 64.0f);
	d.y = (FT_Pos)(translate2d[1] * 64.0f);

	Lock L(s_mutex);
	FT_Set_Transform(m_face, &m, &d);
}

bool Font::SetPointSize(int width, int height, int horzDPI, int vertDPI) {
	if (!width) 
		width = height;
	if (!height)
		height = width;
	RAD_ASSERT(width||height);

	if (!horzDPI) 
		horzDPI = 72;
	if (!vertDPI) 
		vertDPI = 72;
	width  = (int)(((float)width / (float)horzDPI) * 72.0f);
	height = (int)(((float)height / (float)vertDPI) * 72.0f);
	
	return SetPixelSize(width, height);
}

bool Font::SetPixelSize(int width, int height) {
	RAD_ASSERT(m_face);
	if (!width) 
		width = height;
	if (!height) 
		height = width;
	RAD_ASSERT(width||height);
	if (width != m_width || height != m_height) {
		m_width = width;
		m_height = height;
		Lock L(s_mutex);
		return FT_Set_Pixel_Sizes(m_face, width, height) == 0;
	}
	return true;
}

bool Font::LoadGlyphFromIndex(int index) {
	RAD_ASSERT(m_face);
	int error;
	
	{
		Lock L(s_mutex);
		error = FT_Load_Glyph(m_face, index, FT_LOAD_DEFAULT);
	}

	if (!error)
		m_glyph.Setup(0, m_face->glyph, &m_face->glyph->metrics);
	return error == 0;
}

bool Font::LoadGlyphFromChar(U32 character) {
	RAD_ASSERT(m_face);
	int error;
	
	{
		Lock L(s_mutex);
		error = FT_Load_Char(m_face, character, FT_LOAD_DEFAULT);
	}

	if (!error)
		m_glyph.Setup(0, m_face->glyph, &m_face->glyph->metrics);

	return error == 0;
}

UserGlyph *Font::CopyGlyph() const {
	RAD_ASSERT(m_face);
	int error;
	FT_Glyph copy;

	{
		Lock L(s_mutex);
		error = FT_Get_Glyph(m_face->glyph, &copy);
	}

	UserGlyph *user = 0;

	if (!error) {
		user = s_glyphPool.SafeConstruct();
		user->Setup(copy, 0, &m_face->glyph->metrics);
	}

	return user;
}

Glyph *Font::RAD_IMPLEMENT_GET(glyph) {
	RAD_ASSERT(m_face);
	return const_cast<Glyph*>(&m_glyph);
}

float Font::Kerning(U32 utf32Left, U32 utf32Right) {
	return Kerning(GlyphIndex(utf32Left), GlyphIndex(utf32Right));
}

float Font::Kerning(int glyphLeft, int glyphRight) {
	FT_Vector kern;
	Lock L(s_mutex);
	FT_Get_Kerning(m_face, glyphLeft, glyphRight, FT_KERNING_DEFAULT, &kern);
	return (float)kern.x / 64.0f;
}

void Font::StringDimensions(
		const char *utf8String, 
		float &width, 
		float &height, 
		bool kern,
		float kernScale
) {
	RAD_ASSERT(utf8String);
	RAD_ASSERT(utf8::is_valid(utf8String, utf8String+string::len(utf8String)));

	width = 0;
	height = 0;

	U32 last = 0;

	while (*utf8String) {
		U32 cp = utf8::unchecked::next(utf8String);

		if (LoadGlyphFromChar(cp)) {
			width += glyph->metrics->horzAdvance;
			height = std::max<float>(height, glyph->metrics->height);
			
			if (kern && last)
				width += Kerning(last, cp) * kernScale;
		}

		last = cp;
	}
}

StringVec Font::WordWrapString(
	const char *utf8String,
	float maxWidth,
	bool kern,
	float kernScale
) {
	StringVec strings;
	String text(CStr(utf8String));
	
	string::UTF32Buf utf = text.ToUTF32();

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

			StringDimensions(
				test.c_str,
				w,
				h,
				kern,
				kernScale
			);

			if (w > maxWidth) {
				strings.push_back(cur);
				cur.Clear();
				start = end+1;
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

		strings.push_back(cur);
	}

	return strings;
}

float Font::RAD_IMPLEMENT_GET(ascenderPixels) {
	RAD_ASSERT(m_face);
	return floorf(0.5f + ((float)m_face->ascender) * m_face->size->metrics.y_ppem / m_face->units_per_EM);
}

float Font::RAD_IMPLEMENT_GET(descenderPixels) {
	RAD_ASSERT(m_face);
	return floorf(0.5f + ((float)m_face->descender) * m_face->size->metrics.y_ppem / m_face->units_per_EM);
}

//////////////////////////////////////////////////////////////////////////////////////////

GlyphCache::GlyphCache() :
m_fontWidth(0),
m_fontHeight(0),
m_cellWidth(0),
m_cellHeight(0),
m_pageWidth(0),
m_pageHeight(0),
m_cellsPerPage(0),
m_maxPages(0),
m_numPages(0),
m_font(0),
m_pages(0) {
}

GlyphCache::GlyphCache(
	Font &font,
	int fontWidth,
	int fontHeight,
	int fontHorzDPI,
	int fontVertDPI,
	SizeMode fontSizeMode,
	int pageWidth,
	int pageHeight,
	int initialPages,
	int maxPages,
	const IGlyphPageFactory::Ref &pageFactory,
	bool allowEvict
) {
	Create(
		font, 
		fontWidth, 
		fontHeight, 
		fontHorzDPI,
		fontVertDPI,
		fontSizeMode, 
		pageWidth, 
		pageHeight, 
		initialPages, 
		maxPages, 
		pageFactory,
		allowEvict
	);
}

GlyphCache::~GlyphCache() {
	Destroy();
}

void GlyphCache::Create(Font &font, 
	int fontWidth, 
	int fontHeight, 
	int fontHorzDPI,
	int fontVertDPI,
	SizeMode fontSizeMode, 
	int pageWidth, 
	int pageHeight, 
	int initialPages, 
	int maxPages, 
	const IGlyphPageFactory::Ref &pageFactory,
	bool allowEvict
) {
	RAD_ASSERT(fontWidth || fontHeight);
	RAD_ASSERT(maxPages > 0);
	RAD_DEBUG_ONLY(m_inDraw = false);

	m_allowEvict = allowEvict;
	m_font = &font;
	m_active = 0;
	m_pageFactory = pageFactory;

	if (!fontWidth)
		fontWidth = fontHeight;
	if (!fontHeight)
		fontHeight = fontWidth;

	if (fontSizeMode == Points) {
		if (!fontHorzDPI)
			fontHorzDPI = 72;
		if (!fontVertDPI)
			fontVertDPI = 72;
		fontWidth  = (int)(((float)fontWidth / (float)fontHorzDPI) * 72.0f);
		fontHeight = (int)(((float)fontHeight / (float)fontVertDPI) * 72.0f);
	}

	m_fontWidth = fontWidth;
	m_fontHeight = fontHeight;
	m_cellWidth = fontWidth + (CellBorder<<1);
	m_cellHeight = fontHeight + (CellBorder<<1);

	RAD_ASSERT(pageWidth >= m_cellWidth);
	RAD_ASSERT(pageHeight >= m_cellHeight);

	m_pageWidth = pageWidth;
	m_pageHeight = pageHeight;
	m_cellsPerPage = (pageWidth / m_cellWidth) * (pageHeight / m_cellHeight);
	m_maxPages = maxPages;
	m_numPages = 0;

	m_bankPool.Create(ZFonts, "glyph-cache-ch-bnk-pool", 32);
	m_charMapPool.Create(ZFonts, "glyph-cache-ch-map-pool", 32);
	m_pagePool.Create(ZFonts, "glyph-cache-page-pool", 8);
	m_drawPool.Create(ZFonts, "glyph-cache-draw-pool", MaxBatchSize);
	m_cellPool.Create(ZFonts, "glyph-cache-cell-pool", sizeof(Cell) * m_cellsPerPage, 1);

	for (int i = 0; i < initialPages; ++i)
		CreatePage();

	memset(&m_charMap.banks[0], 0, sizeof(CharMap2*) * CharMapSize);
}

void GlyphCache::Destroy() {
	for (Page *page = m_pages; page;) {
		Page *next = page->next;
		m_pagePool.Destroy(page);
		page = next;
	}

	m_pages = 0;
	m_numPages = 0;
	m_active = 0;

	for (int i = 0; i < CharMapSize; ++i) {
		if (m_charMap.banks[i]) {
			CharMap2 *m2 = m_charMap.banks[i];
			for (int k = 0; k < CharMapSize; ++k) {
				if (m2->banks[k])
					m_bankPool.Destroy(m2->banks[k]);
			}
			m_charMapPool.Destroy(m2);
			m_charMap.banks[i] = 0;
		}
	}

	m_bankPool.Destroy();
	m_pagePool.Destroy();
	m_drawPool.Destroy();
	m_cellPool.Destroy();
	m_drawList = 0;
	m_drawStart = 0;
	m_precacheStart = 0;
	m_pageFactory.reset();
}

void GlyphCache::Clear() {
	for (int i = 0; i < CharMapSize; ++i) {
		if (m_charMap.banks[i]) {
			CharMap2 *m2 = m_charMap.banks[i];
			for (int k = 0; k < CharMapSize; ++k) {
				if (m2->banks[k])
					m_bankPool.Destroy(m2->banks[k]);
			}
			m_charMapPool.Destroy(m2);
			m_charMap.banks[i] = 0;
		}
	}

	for (Page *page = m_pages; page;) {
		Page *next = page->next;
		m_pagePool.Destroy(page);
		page = next;
	}

	m_pages = 0;
	m_numPages = 0;
	m_active = 0;
	m_drawList = 0;
	m_drawStart = 0;
	m_precacheStart = 0;
	m_batch.numChars = 0;
}

void GlyphCache::SetupBatch() {
	m_batch.numChars = 0;

	if (!m_active) {
		m_precacheStart = Precache();
		if (!m_precacheStart)
			return; // nothing to do.
		// goto first active page in this precache batch.
		for (m_active = m_pages; m_active && !m_active->mark; m_active = m_active->next) {}
		RAD_ASSERT(m_active);
	}
	
	while (m_active && m_batch.numChars == 0) {
		m_batch.page = m_active->page.get();
		// there is an active page (i.e. a batch overflow or page switch occured).
		for (CharDraw *draw = m_precacheStart; draw && draw->item; draw = draw->next) {
			if (!draw->drawn && draw->item->cell->page == m_active) {
				draw->drawn = true;
				m_batch.chars[m_batch.numChars++] = &draw->metrics;
				if (m_batch.numChars == MaxBatchSize)
					return; // done.
			}
		}

		// goto next active page.
		for (m_active = m_active->next; m_active && !m_active->mark; m_active = m_active->next) {}
	}
}

GlyphCache::CharDraw *GlyphCache::Precache() {
	RAD_ASSERT(m_inDraw);

	CharDraw *draw = 0;
	float x = 0;
	float y = 0;
	int lastGlyph = 0;

	m_active = 0;
	for (Page *page = m_pages; page; page = page->next)
		page->mark = false;

	if (m_drawStart) {
		RAD_ASSERT(m_drawStart->item);
		
		// unlock all rendered chars
		for (draw = m_drawList; draw != m_drawStart; draw = draw->next) {
			if (draw->item) {
				draw->item->locked = false;
				draw->item = 0;
			}
		}

		m_drawStart->item->locked = false;
		m_drawStart->item = 0;
		draw = m_drawStart->next;
		lastGlyph = m_drawStart->glyph;
	} else {
		draw = m_drawList;
	}

	x = m_orgX;
	y = m_orgY;

	int cached = 0;

	CharDraw *firstDraw = draw;

	while (draw) {
		draw->drawn = false;
		draw->glyph = m_font->GlyphIndex(draw->ch);
		draw->item = CacheChar(draw->ch, draw->glyph);

		if (!draw->item) {
			if (!m_allowEvict || !Evict()) {
				// unable to evict anything from the cache. we're done here.
				// this is an error.
				RAD_ASSERT_MSG(cached, "Font page overflow.");
				break;
			}
			draw->item = CacheChar(draw->ch, draw->glyph);
			RAD_ASSERT(draw->item);
		}

		const Cell *cell = draw->item->cell;
		Metrics *metrics = &draw->metrics;
		metrics->bitmap.x1 = (float)(cell->x * m_cellWidth + CellBorder);
		metrics->bitmap.y1 = (float)(cell->y * m_cellHeight + CellBorder);
		metrics->bitmap.x2 = metrics->bitmap.x1 + (float)cell->bmWidth;
		metrics->bitmap.y2 = metrics->bitmap.y1 + (float)cell->bmHeight;

		if (m_kern && lastGlyph != 0)
			x += m_font->Kerning(lastGlyph, draw->glyph) * m_kernScale;

		metrics->draw.x1 = FloorFastFloat(x + draw->item->bearingX + 0.5f);
		metrics->draw.x2 = FloorFastFloat(metrics->draw.x1 + (float)cell->bmWidth + 0.5f);
		metrics->draw.y1 = FloorFastFloat(y + m_tallest - draw->item->bearingY + 0.5f);
		metrics->draw.y2 = FloorFastFloat(metrics->draw.y1 + (float)cell->bmHeight + 0.5f);

		x += draw->item->advance;
		m_orgX = x;

		draw->item->locked = true;
		draw->item->cell->page->mark = true; // used this cache cycle.
		m_drawStart = draw;
		lastGlyph = draw->glyph;
		draw = draw->next;
		++cached;
	}

	for (Page *page = m_pages; page; page = page->next) {
		if (page->locked)  {
			page->page->Unlock();
			page->locked = false;
		}
	}

	return (cached > 0) ? firstDraw : 0;
}

GlyphCache::Batch *GlyphCache::BeginStringBatch(
	const char *utf8String, 
	float orgX, 
	float orgY,
	bool kerning, 
	float kerningScale
) {
	RAD_ASSERT(utf8String);
	RAD_ASSERT(utf8::is_valid(utf8String, utf8String+string::len(utf8String)));
	RAD_ASSERT(!m_inDraw);
	RAD_DEBUG_ONLY(m_inDraw=true);

	m_kern = kerning;
	m_kernScale = kerningScale;

	m_font->SetPixelSize(m_fontWidth, m_fontHeight);

	m_orgX = orgX;
	m_orgY = orgY;
	m_drawStart = 0;
	m_batch.numChars = 0;

	CharDraw *last = 0;
	m_drawList = 0;

	m_tallest = m_font->ascenderPixels;

	// Load glyph indices for all strings.
	while (*utf8String) {
		U32 cp = utf8::unchecked::next(utf8String);
		CharDraw *draw = m_drawPool.SafeConstruct();
		draw->item = 0;
		draw->next = 0;
		draw->ch = cp;
		draw->glyph = m_font->GlyphIndex(draw->ch);
		if (!m_drawList)
			m_drawList = draw;
		if (last)
			last->next = draw;
		last = draw;
	}
	
	if (0 != last)
		SetupBatch();

	return (m_batch.numChars > 0 ) ? &m_batch : 0;
}

GlyphCache::Batch *GlyphCache::NextStringBatch() {
	SetupBatch();
	return (m_batch.numChars > 0 ) ? &m_batch : 0;
}

void GlyphCache::EndStringBatch() {
	RAD_DEBUG_ONLY(m_inDraw=false);
	m_active = 0;
	m_drawStart = 0;
	m_precacheStart = 0;
	m_batch.numChars = 0;

	while (m_drawList) {
		CharDraw *next = m_drawList->next;
		m_drawPool.Destroy(m_drawList);
		m_drawList = next;
	}
}

void GlyphCache::StringDimensions(
		const char *utf8String, 
		float &width, 
		float &height, 
		bool kern,
		float kernScale
) {
	RAD_ASSERT(utf8String);
	RAD_ASSERT(utf8::is_valid(utf8String, utf8String+string::len(utf8String)));

	width = 0;
	height = 0;

	Batch *batch = BeginStringBatch(utf8String, 0.0f, 0.0f, kern, kernScale);
	while (batch) {
		for (int i = 0; i < batch->numChars; ++i) {
			Metrics *m = batch->chars[i];
			height = std::max(m->draw.y2, height);
			width  = std::max(m->draw.x2, width);
		}

		batch = NextStringBatch();
	}
	EndStringBatch();
}

bool GlyphCache::CreatePage() {
	IGlyphPage::Ref glyphPage = m_pageFactory->AllocatePage(m_pageWidth, m_pageHeight);
	if (!glyphPage)
		return false;

	Page *page = m_pagePool.SafeConstruct();
	page->prev = 0;
	page->next = 0;
	page->used = 0;
	page->free = 0;
	page->mark = false;
	page->locked = false;
	page->stride = 0;
	page->cells = (Cell*)m_cellPool.SafeGetChunk();
	
	for (int i = 0; i < m_cellsPerPage; ++i) {
		AddToList(&page->cells[i], &page->free);
		page->cells[i].page = page;
		page->cells[i].item = 0;
	}

	int numHCells = m_pageWidth / m_cellWidth;
	int numVCells = m_pageHeight / m_cellHeight;

	for (int y = 0; y < numVCells; ++y) {
		for (int x = 0; x < numHCells; ++x) {
			Cell *cell = &page->cells[y*numHCells+x];
			cell->x = x;
			cell->y = y;
		}
	}

	page->page = glyphPage;
	RAD_ASSERT(page->page);
	AddToList(page, &m_pages);

	++m_numPages;
	return true;
}

GlyphCache::CharItem *GlyphCache::CacheChar(U32 ch, int glyph) {
	int idx0 = (ch & 0xFF0000) >> 16;
	int idx1 = (ch & 0xFF00) >> 8;
	int idx2 = (ch & 0xFF);

	CharMap2 *m2 = m_charMap.banks[idx0];
	if (!m2) {
		m2 = m_charMapPool.SafeConstruct();
		m_charMap.banks[idx0] = m2;
		memset(&m2->banks[0], 0, sizeof(CharBank*) * CharMapSize);
	}

	CharBank *bank = m2->banks[idx1];

	if (!bank) {
		bank = m_bankPool.SafeConstruct();
		m2->banks[idx1] = bank;
		memset(&bank->items[0], 0, sizeof(CharItem) * CharMapSize);
	}

	if (!bank->items[idx2].cell) {
		m_font->LoadGlyphFromIndex(glyph);

		for (;;) {
			// character has not been cached.
			Page *page;
			for (page = m_pages; page; page = page->next) {
				if (page->free) {
					Cell *cell = page->free;
					MoveToList(&page->free, cell, &page->used);
					// NOTE: SetupBatch() calls Precache() which loads the glyph
					// into the font object before this is called.
					UploadGlyph(cell);
					cell->item = &bank->items[idx2];

					cell->item->cell = cell;
					
					// NOTE: this will need to change for vertically oriented fonts.
					cell->item->bearingX = m_font->glyph->metrics->horzBearingX;
					cell->item->bearingY = m_font->glyph->metrics->horzBearingY;
					cell->item->advance  = m_font->glyph->metrics->horzAdvance;
#if defined(RAD_OPT_DEBUG)
#if defined(RAD_OPT_4BYTE_WCHAR)
					cell->item->ch = (wchar_t)ch;
#else
					char utf8[4];
					utf8::unchecked::utf32to8(&ch, (&ch)+1, utf8);
					utf8::unchecked::utf8to16(utf8, utf8+1, (U16*)&cell->item->ch);
#endif
#endif
					break;
				}
			}

			if (page)
				break; // found a slot.

			if ((m_numPages >= m_maxPages) || !CreatePage())
				break; // failed to make a new page.
		}
	}
	
	return (bank->items[idx2].cell) ? &bank->items[idx2] : 0;
}

bool GlyphCache::Evict() {
	for (Page *page = m_pages; page; page = page->next) {
		RAD_ASSERT(!page->free);
		for (Cell *cell = page->used; cell; cell = cell->next) {
			if (!cell->item->locked) { // not being used, we can free this slot.
				MoveToList(&page->used, cell, &page->free);
				cell->item->cell = 0;
				cell->item = 0;
				return true;
			}
		}
	}

	return false;
}

void GlyphCache::UploadGlyph(Cell *cell) {
	RAD_ASSERT(cell);
	Page *page = cell->page;
	RAD_ASSERT(page);
	RAD_ASSERT(page->page);

	if (!page->locked) {
		page->data = page->page->Lock(page->stride);
		page->locked = true;
	}

	U8 *dst = (U8*)page->data;
	AddrSize dstPitch = page->stride;

	if (m_font->glyph->Render()) {
		const Bitmap *bitmap = m_font->glyph->bitmap;
		cell->bmWidth = bitmap->width;
		cell->bmHeight = bitmap->height;
		const U8 *src = bitmap->data;
		AddrSize srcPitch = bitmap->pitch; 

		int x = cell->x * m_cellWidth;
		int y = cell->y * m_cellHeight;

		dst = dst + (y * dstPitch + x);

		// set top border.
		for (int i = 0; i < CellBorder;  ++i) {
			memset(dst, 0, m_cellWidth);
			dst += dstPitch;
		}
		
		for (int y = 0; y < cell->bmHeight; ++y) {
			for (int i = 0; i < CellBorder; ++i)
				dst[i] = 0;
			
			memcpy(dst + CellBorder, src, cell->bmWidth);
			
			for (int i = 0; i < CellBorder; ++i)
				dst[i+CellBorder+cell->bmWidth] = 0;

			src += srcPitch;
			dst += dstPitch;
		}

		// set bottom border.
		for (int i = 0; i < CellBorder;  ++i) {
			memset(dst, 0, m_cellWidth);
			dst += dstPitch;
		}
	}
	else {
		for (int i = 0; i < m_cellHeight; ++i) {
			memset(dst, 0, m_cellWidth);
			dst += dstPitch;
		}
	}
}

} // font
