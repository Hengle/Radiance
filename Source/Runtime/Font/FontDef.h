// FontDef.h
// Platform Agnostic Font System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntFont.h"

namespace font {

enum
{
	BadGlyphIndex = 0
};

enum SizeMode
{
	Pixels,
	Points
};

//////////////////////////////////////////////////////////////////////////////////////////
// Forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

class Bitmap;
class Metrics;
class Glyph;
class UserGlyph;
class Font;
class IGlyphPage;
class IGlyphPageFactory;
class GlyphCache;

typedef boost::shared_ptr<IGlyphPage> IGlyphPageRef;
typedef boost::shared_ptr<IGlyphPageFactory> IGlyphPageFactoryRef;

} // font

//////////////////////////////////////////////////////////////////////////////////////////

struct FT_FaceRec_;
struct FT_Bitmap_;
struct FT_GlyphRec_;
struct FT_LibraryRec_;
struct FT_GlyphSlotRec_;
struct FT_Glyph_Metrics_;
