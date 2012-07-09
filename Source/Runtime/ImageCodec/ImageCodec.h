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
	AddrSize stride; // horizontal scanline stride in bytes.
	int width, height;
};

struct RADRT_CLASS Frame
{
	Mipmap *mipmaps;
	int mipCount;
	int flags;
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
	bool AllocateFrames(int frameCount);
	bool AllocateMipmaps(int frameNum, int mipmapCount);
	bool AllocateMipmap(int frameNum, int mipmapNum, int width, int height, AddrSize stride, AddrSize dataSize);
	bool AllocatePalette(int format);
	void VerticalFlip();
	void HorizontalFlip();
	void Swap(Image &i);

	int format;
	int bpp; // bytes per pixel.
	int frameCount;
	Frame *frames;
	void *pal;
	Zone *zone;

	// if true the destructor will Free() the image. This should not be set on images who's members reference
	// shared image data.
	bool autoFree;
};

bool IsPalettedFormat(UReg format);
RADRT_API void RADRT_CALL ConvertPixel(const void* srcPix, void* dstPix, const void* pal, int srcFormat, int dstFormat);
RADRT_API int RADRT_CALL FormatBPP(int format);
RADRT_API bool RADRT_CALL ConvertPixelData(const void* src, AddrSize srcByteCount, void* dst, const void* pal, int srcFormat, int dstFormat);
RADRT_API bool RADRT_CALL ConvertImageFormat(const Image &src, Image &dst, int dstFormat);
RADRT_API int RADRT_CALL PaletteSize(int format);
RADRT_API int RADRT_CALL PaletteBPP(int format);
RADRT_API void RADRT_CALL VerticalFlip(void* pix, int width, int height, int bpp, AddrSize stride);
RADRT_API void RADRT_CALL HorizontalFlip(void* pix, int width, int height, int bpp, AddrSize stride);

} // image_codec


#include "../PopPack.h"
#include "ImageCodec.inl"
