// Font.h
// Platform Agnostic Font System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "FontDef.h"
#include "../Base/ObjectPool.h"
#include "../PushPack.h"

namespace font {

RAD_ZONE_DEC(RADRT_API, ZFont);

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Bitmap : public boost::noncopyable {
public:

	RAD_DECLARE_READONLY_PROPERTY(Bitmap, height, int);
	RAD_DECLARE_READONLY_PROPERTY(Bitmap, width, int);
	RAD_DECLARE_READONLY_PROPERTY(Bitmap, pitch, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Bitmap, data, const U8*);

private:

	RAD_DECLARE_GET(height, int);
	RAD_DECLARE_GET(width, int);
	RAD_DECLARE_GET(pitch, AddrSize);
	RAD_DECLARE_GET(data, const U8*);

	Bitmap();
	~Bitmap();

	const FT_Bitmap_ *m_bitmap;

	friend class Glyph;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Metrics : public boost::noncopyable {
public:

	RAD_DECLARE_READONLY_PROPERTY(Metrics, width, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, height, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, horzBearingX, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, horzBearingY, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, horzAdvance, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, vertBearingX, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, vertBearingY, float);
	RAD_DECLARE_READONLY_PROPERTY(Metrics, vertAdvance, float);
	
private:

	RAD_DECLARE_GET(width, float);
	RAD_DECLARE_GET(height, float);
	RAD_DECLARE_GET(horzBearingX, float);
	RAD_DECLARE_GET(horzBearingY, float);
	RAD_DECLARE_GET(horzAdvance, float);
	RAD_DECLARE_GET(vertBearingX, float);
	RAD_DECLARE_GET(vertBearingY, float);
	RAD_DECLARE_GET(vertAdvance, float);

	Metrics();
	~Metrics();

	void Copy(const FT_Glyph_Metrics_ *metrics);

#if defined(RAD_OPT_MACHINE_SIZE_64)
	enum { SizeofMetrics = 64 };
#else
	enum { SizeofMetrics = 32 };
#endif

	U8 m_metrics[SizeofMetrics];

	friend class Glyph;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Glyph : public boost::noncopyable {
public:

	bool Render();
	RAD_DECLARE_READONLY_PROPERTY(Glyph, left, int);
	RAD_DECLARE_READONLY_PROPERTY(Glyph, top,  int);
	RAD_DECLARE_READONLY_PROPERTY(Glyph, bitmap, const Bitmap*);
	RAD_DECLARE_READONLY_PROPERTY(Glyph, metrics, const Metrics*);

private:

	RAD_DECLARE_GET(left, int);
	RAD_DECLARE_GET(top, int);
	RAD_DECLARE_GET(bitmap, const Bitmap*);
	RAD_DECLARE_GET(metrics, const Metrics*);

	Glyph();
	virtual ~Glyph();

	void Setup(FT_GlyphRec_ *glyph,
		FT_GlyphSlotRec_ *slot,
		const FT_Glyph_Metrics_ *metrics
	);

	FT_GlyphRec_ *m_glyph;
	FT_GlyphSlotRec_ *m_slot;

	mutable Bitmap m_bitmap;
	Metrics m_metrics;

	friend class UserGlyph;
	friend class Font;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS UserGlyph : public Glyph {
public:

	void Release();

private:

	UserGlyph();
	virtual ~UserGlyph();

	friend class Font;
	friend class ThreadSafeObjectPool<UserGlyph>;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Font : public boost::noncopyable {
public:

	Font();
	~Font();

	// NOTE: Caller must keep the data around, it is not copied by the font object.
	bool Create(const void *data, AddrSize size);
	void Destroy();
	int GlyphIndex(U32 utf32Char) const;
	void SetTransform(float *_2x2, float *translate2d);
	bool SetPointSize(int width, int height, int horzDPI, int vertDPI);
	bool SetPixelSize(int width, int height);
	bool LoadGlyphFromIndex(int index);
	bool LoadGlyphFromChar(U32 utf32Char);
	float Kerning(U32 utf32Left, U32 utf32Right);
	float Kerning(int glyphLeft, int glyphRight);

	void StringDimensions(
		const char *utf8String, 
		float &width, 
		float &height, 
		bool kern,
		float kernScale
	);

	UserGlyph *CopyGlyph() const;

	RAD_DECLARE_READONLY_PROPERTY(Font, glyph, Glyph*);
	RAD_DECLARE_READONLY_PROPERTY(Font, ascenderPixels, float);
	RAD_DECLARE_READONLY_PROPERTY(Font, descenderPixels, float);

private:

	RAD_DECLARE_GET(glyph, Glyph*);
	RAD_DECLARE_GET(ascenderPixels, float);
	RAD_DECLARE_GET(descenderPixels, float);

	int m_width;
	int m_height;
	FT_LibraryRec_ *m_lib;
	FT_FaceRec_ *m_face;
	Glyph m_glyph;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RAD_NOVTABLE IGlyphPage {
public:
	typedef IGlyphPageRef Ref;
	virtual ~IGlyphPage() {}
	virtual void *Lock(AddrSize &stride) = 0;
	virtual void Unlock() = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RAD_NOVTABLE IGlyphPageFactory {
public:
	typedef IGlyphPageFactoryRef Ref;
	virtual ~IGlyphPageFactory() {}
	virtual IGlyphPage::Ref AllocatePage(int width, int height) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS GlyphCache : public boost::noncopyable {
public:

	struct Rect {
		float x1, y1;
		float x2, y2;
	};

	struct Metrics {
		Rect bitmap; // Positions in the glyphPage.
		Rect draw; // Pen position. Draw origin is Upper Left Relative.
	};

	enum {
		MaxBatchSize = 256
	};

	struct Batch {
		boost::array<Metrics*, MaxBatchSize> chars;
		IGlyphPage *page;
		int numChars;
	};

	GlyphCache();

	GlyphCache(
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
		bool allowEvict = false
	);

	~GlyphCache();

	void Create(Font &font, 
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
		bool allowEvict = false
	);

	void Destroy();
	void Clear();
	
	Batch *BeginStringBatch(
		const char *utf8String, 
		float orgX, 
		float orgY, 
		bool kerning, 
		float kerningScale = 1.0f
	);

	Batch *NextStringBatch();
	void EndStringBatch();

	void StringDimensions(
		const char *utf8String, 
		float &width, 
		float &height, 
		bool kern,
		float kernScale
	);

	RAD_DECLARE_PROPERTY(GlyphCache, allowEvict, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(GlyphCache, fontPixelWidth, int);
	RAD_DECLARE_READONLY_PROPERTY(GlyphCache, fontPixelHeight, int);
	RAD_DECLARE_READONLY_PROPERTY(GlyphCache, ascenderPixels, float);
	RAD_DECLARE_READONLY_PROPERTY(GlyphCache, descenderPixels, float);
	RAD_DECLARE_READONLY_PROPERTY(GlyphCache, font, Font&);

private:

	RAD_DECLARE_GETSET(allowEvict, bool, bool);
	RAD_DECLARE_GET(fontPixelWidth, int);
	RAD_DECLARE_GET(fontPixelHeight, int);
	RAD_DECLARE_GET(ascenderPixels, float);
	RAD_DECLARE_GET(descenderPixels, float);
	RAD_DECLARE_GET(font, Font&);

	struct Page;
	struct CharItem;

	enum {
		CellBorder = 1,
		CharMapSize = 256
	};

	struct Cell {
		int x, y; // in cells not pixels.
		int bmWidth;
		int bmHeight;
		Cell *prev, *next;
		Page *page;
		CharItem *item;
	};

	struct CharDraw {
		Metrics metrics;
		CharItem *item;
		CharDraw *next;
		U32 ch;
		int glyph;
		bool drawn;
	};

	struct CharItem {
		Cell    *cell;
		float    bearingX;
		float    bearingY;
		float    advance;
		RAD_DEBUG_ONLY(wchar_t  ch);
		bool     locked;
	};

	struct CharBank {
		boost::array<CharItem, CharMapSize> items;
	};

	struct CharMap2 {
		boost::array<CharBank*, CharMapSize> banks;
	};

	struct CharMap {
		boost::array<CharMap2*, CharMapSize> banks;
	};

	struct Page {
		IGlyphPage::Ref page;
		Page *prev, *next;
		Cell *free;
		Cell *used;
		Cell *cells;
		void *data;
		AddrSize stride;
		bool  mark;
		bool  locked;
	};

	bool CreatePage();
	CharItem *CacheChar(U32 ch, int glyph);
	bool Evict();
	CharDraw *Precache();
	void SetupBatch();
	void UploadGlyph(Cell *cell);

	template <typename T>
	void MoveToList(T **old, T *item, T **_new);
	template <typename T>
	void AddToList(T *item, T **_new);
	template <typename T>
	void RemoveFromList(T *item, T **old);

	Batch m_batch;
	CharMap m_charMap;
	int m_fontWidth;
	int m_fontHeight;
	int m_cellWidth;
	int m_cellHeight;
	int m_pageWidth;
	int m_pageHeight;
	int m_cellsPerPage;
	int m_maxPages;
	int m_numPages;
	IGlyphPageFactory::Ref m_pageFactory;
	Font *m_font;
	Page *m_pages;
	Page *m_active;
	CharDraw *m_drawList;
	CharDraw *m_drawStart;
	CharDraw *m_precacheStart;
	float m_orgX, m_orgY;
	float m_tallest;
	float m_kernScale;
	ObjectPool<CharBank> m_bankPool;
	ObjectPool<CharMap2> m_charMapPool;
	ObjectPool<Page> m_pagePool;
	ObjectPool<CharDraw> m_drawPool;
	MemoryPool m_cellPool;
	RAD_DEBUG_ONLY(bool m_inDraw);
	bool m_kern;
	bool m_allowEvict;
};

} // font

#include "../PopPack.h"
#include "Font.inl"
