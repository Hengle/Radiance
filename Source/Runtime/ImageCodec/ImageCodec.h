// ImageCodec.h
// Image Codec System.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntImageCodec.h"
#include "ImageCodecDef.h"
#include "../PushPack.h"


namespace image_codec {

RAD_ZONE_DEC(RADRT_API, ZImageCodec);

//////////////////////////////////////////////////////////////////////////////////////////
// Shared Types
//////////////////////////////////////////////////////////////////////////////////////////

struct RADRT_CLASS Mipmap
{
	void *data;
	AddrSize dataSize;
	UReg width, height;
	UReg stride; // horizontal scanline stride in bytes.
};

struct RADRT_CLASS Frame
{
	Mipmap *mipmaps;
	UReg mipCount;
	UReg flags;
};

struct RADRT_CLASS Image
{
	typedef ImageRef Ref;

	Image();
	Image(const Image& i);
	explicit Image(Zone &zone);
	~Image();

	Image &operator=(const Image& i);

	void Free(bool all=true);
	bool AllocateFrames(UReg frameCount);
	bool AllocateMipmaps(UReg frameNum, UReg mipmapCount);
	bool AllocateMipmap(UReg frameNum, UReg mipmapNum, UReg width, UReg height, UReg stride, AddrSize dataSize);
	bool AllocatePalette(UReg format);
	void VerticalFlip();
	void HorizontalFlip();
	void Swap(Image &i);

	UReg format;
	UReg bpp; // bytes per pixel.
	UReg frameCount;
	Frame *frames;
	void *pal;
	Zone *zone;

	// if true the destructor will Free() the image. This should not be set on images who's members reference
	// shared image data.
	bool autoFree;
};

bool IsPalettedFormat(UReg format);
RADRT_API void RADRT_CALL ConvertPixel(const void* srcPix, void* dstPix, const void* pal, UReg srcFormat, UReg dstFormat);
RADRT_API UReg RADRT_CALL FormatBPP(UReg format);
RADRT_API bool RADRT_CALL ConvertPixelData(const void* src, AddrSize srcByteCount, void* dst, const void* pal, UReg srcFormat, UReg dstFormat);
RADRT_API bool RADRT_CALL ConvertImageFormat(const Image &src, Image &dst, UReg dstFormat);
RADRT_API UReg RADRT_CALL PaletteSize(UReg format);
RADRT_API UReg RADRT_CALL PaletteBPP(UReg format);
RADRT_API void RADRT_CALL VerticalFlip(void* pix, UReg width, UReg height, UReg bpp, UReg stride);
RADRT_API void RADRT_CALL HorizontalFlip(void* pix, UReg width, UReg height, UReg bpp, UReg stride);

} // image_codec


#include "../PopPack.h"
#include "ImageCodec.inl"
