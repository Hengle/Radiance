// ImageCodecDef.h
// Image Codec System Definitions.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntImageCodec.h"
#include "../PushPack.h"


namespace image_codec {

//////////////////////////////////////////////////////////////////////////////////////////
// Shared Formats
//////////////////////////////////////////////////////////////////////////////////////////

enum
{
	FormatFamilyShift = 0,
	FormatTypeShift   = 16,
	FormatFamilyMask  = 0x0000FFFF,
	FormatTypeMask    = 0xFFFF0000,

	// family format styles
	SharedFamily       = 0,
	DDSFamily,
	// new format families should go here
	LastFamily
};

enum
{
	SharedFrameFlagRef = 0x1,
	FirstUserFrameBit = 7 // first bit.
};

#define RAD_IMAGECODEC_IMAGEFORMAT(family, type) (((family)<<image_codec::FormatFamilyShift) | ((type)<<image_codec::FormatTypeShift))
#define RAD_IMAGECODEC_SHAREDFORMAT(type) RAD_IMAGECODEC_IMAGEFORMAT(image_codec::SharedFamily, type)
#define RAD_IMAGECODEC_FAMILY(value) ((value&image_codec::FormatFamilyMask)>>image_codec::FormatFamilyShift)
#define RAD_IMAGECODEC_TYPE(value) ((value&image_codec::FormatTypeMask)>>image_codec::FormatTypeShift)

enum
{
	_Format_A8 = 1,
	_Format_Pal8_RGB555,
	_Format_Pal8_RGB565,
	_Format_Pal8_RGB888,
	_Format_Pal8_RGBA8888,
	_Format_RGB555,
	_Format_BGR555,
	_Format_RGB565,
	_Format_BGR565,
	_Format_RGBA4444,
	_Format_BGRA4444,
	_Format_RGBA5551,
	_Format_BGRA5551,
	_Format_RGBA8888,
	_Format_BGRA8888,
	_Format_RGB888,
	_Format_BGR888,

	Format_A8            = RAD_IMAGECODEC_SHAREDFORMAT(_Format_A8),
	Format_PAL8_RGB555   = RAD_IMAGECODEC_SHAREDFORMAT(_Format_Pal8_RGB555),
	Format_PAL8_RGB565   = RAD_IMAGECODEC_SHAREDFORMAT(_Format_Pal8_RGB565),
	Format_PAL8_RGB888   = RAD_IMAGECODEC_SHAREDFORMAT(_Format_Pal8_RGB888),
	Format_PAL8_RGBA8888 = RAD_IMAGECODEC_SHAREDFORMAT(_Format_Pal8_RGBA8888),
	Format_RGB555        = RAD_IMAGECODEC_SHAREDFORMAT(_Format_RGB555),
	Format_BGR555        = RAD_IMAGECODEC_SHAREDFORMAT(_Format_BGR555),
	Format_RGB565        = RAD_IMAGECODEC_SHAREDFORMAT(_Format_RGB565),
	Format_BGR565        = RAD_IMAGECODEC_SHAREDFORMAT(_Format_BGR565),
	Format_RGB888        = RAD_IMAGECODEC_SHAREDFORMAT(_Format_RGB888),
	Format_BGR888        = RAD_IMAGECODEC_SHAREDFORMAT(_Format_BGR888),
	Format_RGBA4444      = RAD_IMAGECODEC_SHAREDFORMAT(_Format_RGBA4444),
	Format_BGRA4444      = RAD_IMAGECODEC_SHAREDFORMAT(_Format_BGRA4444),
	Format_RGBA5551      = RAD_IMAGECODEC_SHAREDFORMAT(_Format_RGBA5551),
	Format_BGRA5551      = RAD_IMAGECODEC_SHAREDFORMAT(_Format_BGRA5551),
	Format_RGBA8888      = RAD_IMAGECODEC_SHAREDFORMAT(_Format_RGBA8888),
	Format_BGRA8888      = RAD_IMAGECODEC_SHAREDFORMAT(_Format_BGRA8888),
	InvalidFormat       = 0
};

struct Image;
struct Mipmap;
struct Frame;

typedef boost::shared_ptr<Image> ImageRef;

} // image_codec


#include "../PopPack.h"
