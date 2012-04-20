// Dds.cpp
// DDS Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

// NOTE: This code contains a DXT compressor and decompressor which is covered by an S3TC patent.
// *IF* you ever call these routines with a "true" to compress/decompress DDS files that are DXT encoded
// you are under obligation to seek a patent license from S3TC.
//
// This code is provided as-is with no express or implied warrenty and it is the USERS RESPONSIBILITY
// TO ENSURE THEY ARE NOT VIOLATING ANY LAWS BY USING THIS CODE

#include "Dds.h"
#include "../Utils.h"
#include "../Endian.h"
#include "../StringBase.h"

namespace image_codec {
namespace dds {

struct Caps2
{
	U32 caps1;
	U32 caps2;
	U32 reserved[2];
};

struct PixelFormat
{
	U32 size;
	U32 flags;
	U32 fourCC;
	U32 RGBBitCount;
	U32 RBitMask;
	U32 GBitMask;
	U32 BBitMask;
	U32 ABitMask;
};

struct SurfaceDesc2
{
	U32 size;
	U32 flags;
	U32 height;
	U32 width;
	U32 pitch;
	U32 depth;
	U32 mipmapCount;
	U32 reserved[11];
	PixelFormat pf;
	Caps2 caps;
	U32 reserved2;
};

enum
{
	SDCaps        = 0x00000001,
	SDHeight      = 0x00000002,
	SDWidth       = 0x00000004,
	SDPitch       = 0x00000008,
	SDPixelFormat = 0x00001000,
	SDMipmapCount = 0x00020000,
	SDLinearSize  = 0x00080000,
	SDDepth       = 0x00800000,

	PFAlphaPixels = 0x00000001,
	PFFourCC      = 0x00000004,
	PFRGB         = 0x00000040,

	CapsComplex   = 0x00000008,
	CapsTexture   = 0x00001000,
	CapsMipmap    = 0x00400000,

	Caps2Cubmap           = 0x00000200,
	Caps2CubemapPositiveX = 0x00000400,
	Caps2CubemapNegativeX = 0x00000800,
	Caps2CubemapPositiveY = 0x00001000,
	Caps2CubemapNegativeY = 0x00002000,
	Caps2CubemapPositiveZ = 0x00004000, 
	Caps2CubemapNegativeZ = 0x00008000,
	Caps2Volume            = 0x00200000,

	NativeCubemapFlagShift = 10,
	NumCubemapSides = 6
};

static UReg s_cubemapDirFlags[6] =
{
	Caps2CubemapPositiveX,
	Caps2CubemapNegativeX,
	Caps2CubemapPositiveY,
	Caps2CubemapNegativeY,
	Caps2CubemapPositiveZ,
	Caps2CubemapNegativeZ
};

enum
{
	DDS_S8888 = 63,
	DDS_FP16  = 111,
	DDS_FP416 = 113,
	DDS_FP32  = 114,
	DDS_FP432 = 116
};

#define IS_DXT(f) (((f)==Format_DXT1)||((f)==Format_DXT1A)||((f)==Format_DXT3)||((f)==Format_DXT5))
#define NOT_DXT(f) (!IS_DXT(f))
#define IS_FP(f) (((f)==Format_FP16)||((f)==Format_FP32)||((f)==Format_FP416)||((f)==Format_FP432)||((f)==Format_S8888))
#define NOT_FP(f) (!IS_FP(f))

//////////////////////////////////////////////////////////////////////////////////////////
// DDS Decode
//////////////////////////////////////////////////////////////////////////////////////////

static const U8* RADRT_CALL PrivateDXTDecode(const U8* dxtBuff, AddrSize* io_dxtBuffLength, UReg dxtFormat, UReg width, UReg height, void* outBuff);

static void SwapSurfaceDesc(SurfaceDesc2* sd)
{
	sd->size = endian::SwapLittle(sd->size);
	sd->flags = endian::SwapLittle(sd->flags);
	sd->height = endian::SwapLittle(sd->height);
	sd->width = endian::SwapLittle(sd->width);
	sd->pitch = endian::SwapLittle(sd->pitch);
	sd->depth = endian::SwapLittle(sd->depth);
	sd->mipmapCount = endian::SwapLittle(sd->mipmapCount);

	sd->pf.size = endian::SwapLittle(sd->pf.size);
	sd->pf.flags = endian::SwapLittle(sd->pf.flags);
	sd->pf.fourCC = endian::SwapLittle(sd->pf.fourCC);
	sd->pf.RGBBitCount = endian::SwapLittle(sd->pf.RGBBitCount);
	sd->pf.RBitMask = endian::SwapLittle(sd->pf.RBitMask);
	sd->pf.GBitMask = endian::SwapLittle(sd->pf.GBitMask);
	sd->pf.BBitMask = endian::SwapLittle(sd->pf.BBitMask);
	sd->pf.ABitMask = endian::SwapLittle(sd->pf.ABitMask);

	sd->caps.caps1 = endian::SwapLittle(sd->caps.caps1);
	sd->caps.caps2 = endian::SwapLittle(sd->caps.caps2);
}

static UReg ColorBitsToFormat(UReg bpp, UReg red, UReg green, UReg blue, UReg alpha)
{
	UReg rbc, gbc, bbc, abc;

	rbc = NumBits(red);
	gbc = NumBits(green);
	bbc = NumBits(blue);
	abc = NumBits(alpha);

	switch(bpp)
	{
	case 1:
		RAD_ASSERT(alpha);
		RAD_ASSERT(rbc==0&&gbc==0&&bbc==0&&abc==8);
		return Format_A8;
	break;
	case 2:
		if (alpha)
		{
			if (abc == 1) // 5551
			{
				RAD_ASSERT(rbc==5&&gbc==5&&bbc==5);
				return Format_RGBA5551;
			}
			else
			{
				RAD_ASSERT(abc==4&&rbc==4&&gbc==4&&bbc==4);
				return Format_RGBA4444;
			}
		}
		else
		{
			if (gbc==6) // 565
			{
				RAD_ASSERT(rbc==5&&bbc==5);
				return Format_RGB565;
			}
			else
			{
				RAD_ASSERT(rbc==5&&gbc==5&&bbc==5);
				return Format_RGB555;
			}
		}
	break;
	case 3:
		RAD_ASSERT(rbc==8&&gbc==8&&bbc==8&&abc==0);
		return Format_RGB888;
	break;
	}

	RAD_ASSERT(bpp==4);
	RAD_ASSERT(rbc==8&&gbc==8&&bbc==8&&abc==8);
	return Format_RGBA8888;
}

static void FormatToColorBits(UReg format, U32* redBits, U32* greenBits, U32* blueBits, U32* alphaBits)
{
	RAD_ASSERT(redBits&&greenBits&&blueBits&&alphaBits);

	*redBits = 0;
	*greenBits = 0;
	*blueBits = 0;
	*alphaBits = 0;

	switch(format)
	{
	case Format_A8:
		*alphaBits = 0xFF;
	break;
	case Format_RGBA5551:
		*alphaBits = 1<<15; // fall through to RGB555
	case Format_RGB555:
		*redBits = 31<<10;
		*greenBits = 31<<5;
		*blueBits = 31;
	break;
	case Format_RGB565:
		*redBits = 31<<11;
		*greenBits = 63<<5;
		*blueBits = 31;
	break;
	case Format_RGBA4444:
		*alphaBits = 15<<12;
		*redBits   = 15<<8;
		*greenBits = 15<<4;
		*blueBits  = 15;
	break;
	case Format_RGBA8888:
		*alphaBits = 255<<24; // fall through to RGB888
	case Format_RGB888:
		*redBits   = 255<<16;
		*greenBits = 255<<8;
		*blueBits  = 255;
	break;
	}
}

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(buff);
	RAD_ASSERT(buffLength);
	RAD_ASSERT(sizeof(SurfaceDesc2) == 124); // if this asserts there is an alignment issue.

	out.Free();
	
	const U8* src = (const U8*)buff;
	SurfaceDesc2 sd;
	
	if (buffLength < sizeof(SurfaceDesc2) + 4) 
		return false; // srcvalid length.

	// is it a DDS?
	if (string::ncmp((const char*)src, "DDS ", 4)) 
		return false; // not a DDS.

	src += 4; // skip 4 bytes.

	// header.
	sd = *((const SurfaceDesc2*)src);
	SwapSurfaceDesc(&sd);
	
	if (sd.size != 124) 
		return false; // bad.
	if (sd.pf.size != 32) 
		return false;

	if (sd.width == 0 || sd.height == 0) 
		return false;
	if (sd.width != PowerOf2(sd.width)) 
		return false;
	if (sd.height != PowerOf2(sd.height)) 
		return false;

	if (sd.caps.caps1 & CapsMipmap)
	{
		sd.flags |= SDMipmapCount;
	}

	if (!((sd.flags&SDMipmapCount) && sd.mipmapCount > 0))
	{
		sd.flags &= ~SDMipmapCount;
		sd.mipmapCount = 1;
	}

	if (sd.flags & SDMipmapCount)
	{
		if (!(sd.caps.caps1&CapsMipmap)) 
			return false;
		if (!(sd.caps.caps1&CapsComplex)) 
			return false;
		U32 maxEdge = (U32)std::max(sd.width, sd.height);
		if (sd.mipmapCount > LowBitVal(maxEdge)) 
			return false;
	}

	if (sd.depth != 0) 
		return false;

	// is it compressed?
	if (sd.pf.flags & PFFourCC)
	{
		switch (sd.pf.fourCC)
		{ // note: the RAD_FOURCC's are not the byte swapped versions because we swapped the surface desc above.
		case RAD_FOURCC('D', 'X', 'T', '1'):
			out.bpp = 0;
			out.format = Format_DXT1;
		break;
		case RAD_FOURCC('D', 'X', 'T', '3'):
			out.bpp = 0;
			out.format = Format_DXT3;
		break;
		case RAD_FOURCC('D', 'X', 'T', '5'):
			out.bpp = 0;
			out.format = Format_DXT5;
		break;
		case DDS_S8888:
			out.bpp = 4;
			out.format = Format_S8888;
		break;
		case DDS_FP16:
			out.bpp = 2;
			out.format = Format_FP16;
		break;
		case DDS_FP416:
			out.bpp = 8;
			out.format = Format_FP416;
		break;
		case DDS_FP32:
			out.bpp = 4;
			out.format = Format_FP32;
		break;
		case DDS_FP432:
			out.bpp = 16;
			out.format = Format_FP432;
		break;
		default: 
			return false; // bad format.
		};
	}
	else
	{
		// RGB/RGBA uncompressed.
		out.bpp = sd.pf.RGBBitCount >> 3;
		out.format = ColorBitsToFormat(out.bpp, sd.pf.RBitMask, sd.pf.GBitMask, sd.pf.BBitMask, (sd.pf.flags&PFAlphaPixels) ? sd.pf.ABitMask : 0);
	}

	// cubemaps?
	if ((sd.caps.caps1&CapsComplex) && (sd.caps.caps2&Caps2Cubmap))
	{
		// how many?
		UReg numSides = 0;
		for (UReg i = 0; i < NumCubemapSides; i++)
		{
			if (sd.caps.caps2 & s_cubemapDirFlags[i]) numSides++;
		}

		if (numSides == 0) 
			return false; // no sides.

		if (!out.AllocateFrames(numSides)) 
			return false; // number of frames.
		for (UReg i = 0; i < numSides; i++)
		{
			Frame* f = &out.frames[i];
			if (!out.AllocateMipmaps(i, sd.mipmapCount))
			{
				out.Free(); 
				return false;
			}

			UReg w = sd.width;
			UReg h = sd.height;
			
			for (UReg k = 0; k < sd.mipmapCount; k++)
			{
				f->mipmaps[k].width = w;
				f->mipmaps[k].height = h;
				f->mipmaps[k].stride = w * out.bpp; // note: will be ZERO for a DXT format.

				w >>= 1;
				h >>= 1;
			}
		}
	}
	else if(sd.caps.caps1 & CapsTexture) // not a complex surface. (i.e. a single image)
	{
		if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0,sd.mipmapCount))) return false;

		UReg w = sd.width;
		UReg h = sd.height;

		for (UReg i = 0; i < sd.mipmapCount; i++)
		{
			out.frames[0].mipmaps[i].width = w;
			out.frames[0].mipmaps[i].height = h;
			out.frames[0].mipmaps[i].stride = (w * out.bpp); // note: will be ZERO for a DXT format.

			w >>= 1;
			h >>= 1;
		}
	}
	else
	{
		return false;
	}

	return true;
}

static void SwapRedAndBlue(U8* src, UReg bpp, AddrSize srcPixelCount, const SurfaceDesc2* sd)
{
	RAD_ASSERT(src);
	RAD_ASSERT(sd);
	RAD_ASSERT(bpp==1||bpp==2||bpp==3||bpp==4);

	switch (bpp)
	{
	case 1:
	break;
	case 2:
		{
			UReg rbits = sd->pf.RBitMask;
			UReg bbits = sd->pf.BBitMask;
			UReg rbmask = rbits | bbits;

			// in DDS files, red is always the high bits (aside from alpha).
			UReg redBitNum = LowBit(sd->pf.RBitMask);
			if (redBitNum) { redBitNum -= 1; }

			U16 p, r, b;
			while(srcPixelCount-- > 0)
			{
				p = endian::SwapLittle(((U16*)src)[0]);
				r = (U16)((p & rbits) >> redBitNum);
				b = (U16)((p & bbits) << redBitNum);
				((U16*)src)[0] = (U16)((p & ~rbmask) | r | b);
				src += 2;
			}
		}
	break;
	case 3:
	case 4:
		{
			U8 t;
			while(srcPixelCount-- > 0)
			{
				t = src[0];
				src[0] = src[2];
				src[2] = t;
				src += bpp;
			}
		}
	break;
	}
}

static const U8* DecodePixels(
	Zone &zone,
	const U8* src, 
	AddrSize* io_srcLength, 
	const SurfaceDesc2* sd, 
	UReg format, 
	UReg width, 
	UReg height, 
	UReg bpp, 
	AddrSize lumpSize, 
	bool refSrcData, 
	bool decompress, 
	void** out, 
	AddrSize* outSize, 
	UReg* outFormat, 
	UReg* outBPP, 
	bool *isRef)
{
	RAD_ASSERT(src);
	RAD_ASSERT(io_srcLength);
	RAD_ASSERT(lumpSize <= *io_srcLength);
	RAD_ASSERT(sd);
	RAD_ASSERT(out&&outSize&&isRef);

	if (!width || !height) 
		return 0;

	U8* dst = 0;
	bool swaprb = false;

	switch(format)
	{
	case Format_DXT1:
	case Format_DXT3:
	case Format_DXT5:

		if (decompress)
		{
			// we have to duplicate the image data. all DXT formats become RGBA8888
			dst = (U8*)zone_malloc(ZImageCodec, width*height*4, 0);
			if (dst)
			{
				src = PrivateDXTDecode(src, io_srcLength, format, width, height, dst);
				if (!src)
				{
					zone_free(dst);
					dst = 0;
				}
				else
				{
					*outSize = width*height*4;
					*outFormat = Format_RGBA8888;
					*outBPP = 4;
					*isRef = false;
				}
			}
			else
			{
				src = 0;
			}
		}
		else
		{
			lumpSize = std::max<UReg>(lumpSize, (format==Format_DXT1)?8:16);
			*outSize = lumpSize;
			*outFormat = format;
			*outBPP = 0;
			RAD_ASSERT(lumpSize <= *io_srcLength);
			*io_srcLength -= lumpSize;

			if (refSrcData)
			{
				// easy enough.
				*isRef = true;
				dst = const_cast<U8*>(src);
				src += lumpSize;
			}
			else
			{
				// copy it.
				dst = (U8*)zone_malloc(zone, lumpSize, 0);
				if (dst)
				{
					memcpy(dst, src, lumpSize);
					src += lumpSize;
					*isRef = false;
				}
				else
				{
					src = 0;
				}
			}
		}

	break;

	case Format_RGB555:
	case Format_RGB565:
	case Format_RGB888:
	case Format_RGBA4444:
	case Format_RGBA5551:
	case Format_RGBA8888:
		swaprb = true;
	case Format_A8:
	case Format_FP16:
	case Format_FP32:
	case Format_FP416:
	case Format_FP432:
	case Format_S8888:

		RAD_ASSERT(lumpSize);

		// we never ref the src data in an uncompressed file.
		dst = (U8*)zone_malloc(zone, lumpSize, 0);
		if (!dst) 
			return 0;

		*outSize = lumpSize;

		if (*io_srcLength < lumpSize) 
			return 0;

		memcpy(dst, src, lumpSize);
		src += lumpSize;
		*io_srcLength -= lumpSize;

		if (swaprb) 
			SwapRedAndBlue(dst, bpp, lumpSize / bpp, sd);

		*isRef = false;
		*outFormat = format;
		*outBPP = bpp;

	break;
	}

	*out = dst;

	return src;
}

static const U8* DecodeMipMaps(
	Zone &zone,
	const U8* src, 
	AddrSize* io_srcLength, 
	const SurfaceDesc2* sd, 
	Frame* f, 
	UReg format, 
	UReg width,
	UReg height, 
	UReg bpp, 
	AddrSize lumpSize, 
	bool refSrcData, 
	bool decompress, 
	UReg* outFormat, 
	UReg* outBPP)
{
	RAD_ASSERT(src);
	RAD_ASSERT(io_srcLength);
	RAD_ASSERT(f);
	RAD_ASSERT(width&&height&&lumpSize);
	RAD_ASSERT(outFormat);
	RAD_ASSERT(f->mipCount == sd->mipmapCount);

	UReg w = width;
	UReg h = height;
	UReg newFormat = format;
	
	for (UReg i = 0; i < sd->mipmapCount; i++)
	{
		Mipmap* m = &f->mipmaps[i];

		m->width = w;
		m->height = h;
		m->stride = w * bpp; // note: will be ZERO for a DXT format.

		// decode pixels.
		{
			bool isRef = false;
			src = DecodePixels(zone, src, io_srcLength, sd, format, w, h, bpp, lumpSize, refSrcData, decompress, &(m->data), &(m->dataSize), &newFormat, outBPP, &isRef);
			if (!src)
			{
				return 0;
			}
			RAD_ASSERT((((bool)(f->flags&SharedFrameFlagRef)) == isRef) || (i==0)); // make sure all mips are the same ref type.
			if (isRef) f->flags |= SharedFrameFlagRef;
		}

		w = (UReg)std::max<UReg>(1, w >> 1);
		h = (UReg)std::max<UReg>(1, h >> 1);
		lumpSize = w * h * bpp;
	}

	*outFormat = newFormat;

	return src;
}

// note that SurfaceDesc2.pitch is used as the size of the main image, not a scanline as per the documentation on MSDN.
// (i.e. this appears to be a misdocumented thing, or there are alot of .DDS tools and plugins that do bad things).

RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out, bool refSrcData, bool decompress)
{
	RAD_ASSERT(buff);
	RAD_ASSERT(buffLength);
	RAD_ASSERT(sizeof(SurfaceDesc2) == 124); // if this asserts there is an alignment issue.

	out.Free();

	const U8* src = (const U8*)buff;
	SurfaceDesc2 sd;
	
	if (buffLength < sizeof(SurfaceDesc2) + 4) 
		return false; // srcvalid length.

	// is it a DDS?
	if (string::ncmp((const char*)src, "DDS ", 4)) 
		return false; // not a DDS.

	src += 4; // skip 4 bytes.

	// header.
	sd = *((const SurfaceDesc2*)src);
	SwapSurfaceDesc(&sd);

	src += sizeof(SurfaceDesc2);
	buffLength -= sizeof(SurfaceDesc2)+4;
	
	if (sd.size != 124) 
		return false; // bad.
	if (sd.pf.size != 32) 
		return false;

	if (sd.width == 0 || sd.height == 0) 
		return false;
	if (sd.width != PowerOf2(sd.width)) 
		return false;
	if (sd.height != PowerOf2(sd.height)) 
		return false;

	if (sd.caps.caps1 & CapsMipmap)
	{
		sd.flags |= SDMipmapCount;
	}

	if (!((sd.flags&SDMipmapCount) && sd.mipmapCount > 0))
	{
		sd.flags &= ~SDMipmapCount;
		sd.mipmapCount = 1;
	}

	if (sd.flags & SDMipmapCount)
	{
		if (!(sd.caps.caps1&CapsMipmap)) 
			return false;
		if (!(sd.caps.caps1&CapsComplex)) 
			return false;
		UReg maxEdge = (UReg)std::max(sd.width, sd.height);
		if (sd.mipmapCount > LowBitVal(maxEdge)) 
			return false;
	}

	if (sd.depth != 0) 
		return false;

	// is it compressed?
	if (sd.pf.flags & PFFourCC)
	{ // note: the RAD_FOURCC's are not the byte swapped versions _LE / _BE because we swapped the surface desc above.
		switch (sd.pf.fourCC)
		{
		case RAD_FOURCC('D', 'X', 'T', '1'):
			out.bpp = 0;
			out.format = Format_DXT1;
		break;
		case RAD_FOURCC('D', 'X', 'T', '3'):
			out.bpp = 0;
			out.format = Format_DXT3;
		break;
		case RAD_FOURCC('D', 'X', 'T', '5'):
			out.bpp = 0;
			out.format = Format_DXT5;
		break;
		case DDS_S8888:
			out.bpp = 4;
			out.format = Format_S8888;
		break;
		case DDS_FP16:
			out.bpp = 2;
			out.format = Format_FP16;
		break;
		case DDS_FP416:
			out.bpp = 8;
			out.format = Format_FP416;
		break;
		case DDS_FP32:
			out.bpp = 4;
			out.format = Format_FP32;
		break;
		case DDS_FP432:
			out.bpp = 16;
			out.format = Format_FP432;
		break;
		default: 
			return false; // bad format.
		};
	}
	else
	{
		// RGB/RGBA uncompressed.
		out.bpp = sd.pf.RGBBitCount >> 3;
		if (out.bpp == 1) // we don't support this.
		{
			return false;
		}
		out.format = ColorBitsToFormat(out.bpp, sd.pf.RBitMask, sd.pf.GBitMask, sd.pf.BBitMask, ((sd.pf.flags&PFAlphaPixels)||out.bpp==1) ? sd.pf.ABitMask : 0);
	}

	// cubemaps?
	if ((sd.caps.caps1&CapsComplex) && (sd.caps.caps2&Caps2Cubmap))
	{
		// how many?
		UReg numSides = 0;
		for (UReg i = 0; i < NumCubemapSides; i++)
		{
			if (sd.caps.caps2 & s_cubemapDirFlags[i]) numSides++;
		}

		if (numSides == 0) 
			return false; // no sides.

		if (!out.AllocateFrames(numSides)) 
			return false; // number of frames.
		UReg format;
		UReg bpp;
		for (UReg i = 0; i < numSides; i++)
		{
			Frame* f = &out.frames[i];
			if (!out.AllocateMipmaps(i, sd.mipmapCount))
			{
				out.Free(); 
				return false;
			}

			src = DecodeMipMaps(*out.zone, src, &buffLength, &sd, f, out.format, sd.width, sd.height, out.bpp, sd.pitch, refSrcData, decompress, &format, &bpp);
			if (!src)
			{
				out.Free();
				return false;
			}
		}
		out.format = format;
		out.bpp = bpp;
	}
	else if(sd.caps.caps1 & CapsTexture) // not a complex surface. (i.e. a single image)
	{
		if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0,sd.mipmapCount))) 
			return false;

		src = DecodeMipMaps(*out.zone, src, &buffLength, &sd, &(out.frames[0]), out.format, sd.width, sd.height, out.bpp, sd.pitch, refSrcData, decompress, &(out.format), &(out.bpp));
		if (!src)
		{
			out.Free();
			return false;
		}
	}
	else
	{
		return false;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// DDS Encode
//////////////////////////////////////////////////////////////////////////////////////////


struct OUTBUFF
{
	void* m_buff;
	AddrSize m_ofs;
	AddrSize m_size;

	OUTBUFF() : m_buff(0), m_ofs(0), m_size(0)
	{
	}

	~OUTBUFF()
	{
		if (m_buff)
		{
			zone_free(m_buff);
		}
	}

	bool Write(const void* src, AddrSize size)
	{
		if (m_ofs+size > m_size)
		{
			AddrSize growSize = std::max<AddrSize>(Kilo*4, size);
			// grow.
			void* newBuff = zone_realloc(ZImageCodec, m_buff, m_size+growSize, 0);
			if (!newBuff) return false;
			m_buff = newBuff;
			m_size += growSize;
		}

		memcpy(&(((U8*)m_buff)[m_ofs]), src, size);
		m_ofs += size;

		return true;
	}

	void Trim()
	{
		if (m_ofs != m_size)
		{
			m_buff = zone_realloc(ZImageCodec, m_buff, m_ofs, 0);
			RAD_ASSERT(m_buff);
			m_size = m_ofs;
		}
	}
};

static bool EncodeMipMaps(
	const SurfaceDesc2* sd, 
	const Frame* f, 
	UReg srcBPP, 
	UReg srcFormat, 
	UReg dstFormat, 
	OUTBUFF* outBuff)
{
	RAD_ASSERT(sd);
	RAD_ASSERT(f);
	RAD_ASSERT(outBuff);

	if (!f->mipCount) return false;

	// do we have to convert?
	if (IS_DXT(srcFormat))
	{
		RAD_ASSERT(srcFormat != dstFormat); // give em an RAD_ASSERT in debug.
		if (srcFormat != dstFormat) return false;
	}

	void* temp = 0, *dxtBuff = 0;

	UReg expWidth = f->mipmaps[0].width;
	UReg expHeight = f->mipmaps[0].height;
	UReg dstBPP;
	AddrSize dstSize;

	RAD_ASSERT(expWidth&&expHeight);

	if (NOT_DXT(srcFormat))
	{
		temp = zone_malloc(ZImageCodec, expWidth*expHeight*srcBPP, 0); // biggest we'll need.
		if (!temp) return false;

		if (IS_DXT(dstFormat))
		{
			AddrSize dxtSize = DXTEncodeSize(dstFormat, expWidth, expHeight);
			RAD_ASSERT(dxtSize);
			dxtBuff = zone_malloc(ZImageCodec, dxtSize, 0);
			if (!dxtBuff)
			{
				zone_free(temp);
				return false;
			}
		}
	}
	
	for (UReg i = 0; i < f->mipCount; i++)
	{
		Mipmap* m = &f->mipmaps[i];

		if (!expWidth || !expHeight)
		{
			if (temp) zone_free(temp);
			if (dxtBuff) zone_free(dxtBuff);
			return false;
		}

		if ((expWidth != m->width) || (expHeight != m->height))
		{
			if (temp) zone_free(temp);
			if (dxtBuff) zone_free(dxtBuff);
			return false;
		}

		// convert?
		if ((srcFormat != dstFormat) && NOT_DXT(dstFormat))
		{
			if (!ConvertPixelData(m->data, m->dataSize, temp, 0, srcFormat, dstFormat))
			{
				zone_free(temp);
				return false;
			}

			dstBPP = FormatBPP(dstFormat);
			dstSize = dstBPP * m->width * m->height;
		}
		else
		{
			if (NOT_DXT(srcFormat))
			{
				RAD_ASSERT(temp);
				// copy it.
				memcpy(temp, m->data, m->dataSize);
				dstSize = m->dataSize;
				dstBPP  = srcBPP;
			}
			// else it's DXT already.
		}

		// swap red and blue
		if (NOT_DXT(srcFormat) && NOT_FP(srcFormat))
		{
			RAD_ASSERT(temp);
			SwapRedAndBlue((U8*)temp, dstBPP, dstSize / dstBPP, sd);

			if(IS_DXT(dstFormat))
			{
				RAD_ASSERT(dxtBuff);
				// compress DXT!
				if (!DXTEncode(temp, dstSize, srcFormat, dstFormat, m->width, m->height, dxtBuff))
				{
					zone_free(temp);
					zone_free(dxtBuff);
					return false;
				}

				dstSize = DXTEncodeSize(dstFormat, m->width, m->height);
			}
		}

		if (IS_DXT(dstFormat))
		{
			if (NOT_DXT(srcFormat))
			{
				RAD_ASSERT(dxtBuff);
				if (!outBuff->Write(dxtBuff, dstSize))
				{
					RAD_ASSERT(temp);
					RAD_ASSERT(dxtBuff);
					zone_free(temp);
					zone_free(dxtBuff);
					return false;
				}
			}
			else
			{
				if (!outBuff->Write(m->data, m->dataSize))
				{
					RAD_ASSERT(!temp);
					RAD_ASSERT(!dxtBuff);
					return false;
				}
			}
		}
		else
		{
			if (!outBuff->Write(temp, dstSize))
			{
				zone_free(temp);
				return false;
			}
		}

		expWidth = (UReg)std::max<UReg>(1, expWidth >> 1);
		expHeight = (UReg)std::max<UReg>(1, expHeight >> 1);
	}

	if (temp) zone_free(temp);
	if (dxtBuff) zone_free(dxtBuff);
	return true;
}

RADRT_API bool RADRT_CALL Encode(const Image &in, UReg encodeFormat, UReg encodeFlags, void *&outData, AddrSize &outSize)
{
	RAD_ASSERT(in.frameCount>0);

	RAD_ASSERT(sizeof(SurfaceDesc2) == 124); // if this asserts there is an alignment issue.

	OUTBUFF outBuff;
	SurfaceDesc2 sd;

#if defined(RAD_OPT_DEBUG)
	if (encodeFlags & EncodeFlagCubemap)
	{
		// better have 6 frames!
		RAD_ASSERT_MSG(in.frameCount==6, "Need 6 frames for a cubemap!");

		UReg c = in.frames[0].mipCount;
		UReg w = in.frames[0].mipmaps[0].width;
		UReg h = in.frames[0].mipmaps[0].width;
		AddrSize d = in.frames[0].mipmaps[0].dataSize;

		for (UReg i = 1; i < 6; i++)
		{
			RAD_ASSERT_MSG(in.frames[i].mipCount==c, "All cubemaps sides must have the same mipmap count!");
			RAD_ASSERT_MSG((in.frames[i].mipmaps[0].width==w)&&
				(in.frames[i].mipmaps[0].height==h)&&
				(in.frames[i].mipmaps[0].dataSize==d), "All cubemaps sides must have the same dimensions!");
		}
	}
#endif

	bool fpFormat = IS_FP(in.format);

	if (fpFormat)
	{
		RAD_ASSERT_MSG(in.format==encodeFormat, "DDSEncode: cannot convert floating point pixel formats!");
		if (in.format != encodeFormat) return false;
	}

	Frame* frame = &in.frames[0];
	Mipmap* mip  = &frame->mipmaps[0];

	RAD_ASSERT(mip->dataSize);
	RAD_ASSERT(mip->width&&mip->height);
	RAD_ASSERT(mip->data);
	RAD_ASSERT(PowerOf2(mip->width)==mip->width);
	RAD_ASSERT(PowerOf2(mip->height)==mip->height);

	if (!outBuff.Write("DDS ", 4)) return false;

	memset(&sd, 0, sizeof(SurfaceDesc2));
	sd.size = sizeof(SurfaceDesc2);
	sd.pf.size = sizeof(PixelFormat);
	sd.width = mip->width;
	sd.height = mip->height;

	if (NOT_DXT(encodeFormat))
	{
		if (fpFormat)
		{
			sd.pitch = mip->width * mip->height * in.bpp;
		}
		else
		{
			sd.pitch = mip->width * mip->height * FormatBPP(encodeFormat);
		}
	}
	else
	{
		sd.pitch = (U32)DXTEncodeSize(encodeFormat, mip->width, mip->height);
#if defined(RAD_OPT_DEBUG)
		if (IS_DXT(in.format))
		{
			RAD_ASSERT(mip->dataSize==sd.pitch);
		}
#endif
	}
	
	if (NOT_DXT(encodeFormat))
	{
		FormatToColorBits(encodeFormat, &sd.pf.RBitMask, &sd.pf.GBitMask, &sd.pf.BBitMask, &sd.pf.ABitMask);
	}
	else
	{
		// this may be bogus if the input is DXT, but it's ignored anyway.
		FormatToColorBits(Format_RGBA8888, &sd.pf.RBitMask, &sd.pf.GBitMask, &sd.pf.BBitMask, &sd.pf.ABitMask);
	}

	switch(encodeFormat)
	{
	case Format_DXT1:
	case Format_DXT1A:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = RAD_FOURCC_LE('D', 'X', 'T', '1');
	break;
	case Format_DXT3:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = RAD_FOURCC_LE('D', 'X', 'T', '3');
	break;
	case Format_DXT5:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = RAD_FOURCC_LE('D', 'X', 'T', '5');
	break;
	case Format_FP16:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = DDS_FP16;
	break;
	case Format_FP32:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = DDS_FP32;
	break;
	case Format_FP416:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = DDS_FP416;
	break;
	case Format_FP432:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = DDS_FP432;
	break;
	case Format_S8888:
		sd.pf.flags = PFFourCC;
		sd.pf.fourCC = DDS_S8888;
	break;

	case Format_RGB555:
	case Format_RGB565:
	case Format_RGB888:
		sd.pf.flags = PFRGB;
	break;

	case Format_A8:
	case Format_RGBA4444:
	case Format_RGBA5551:
	case Format_RGBA8888:
		sd.pf.flags = PFRGB|PFAlphaPixels;
	break;
	}
	
	if (encodeFlags & EncodeFlagMipmaps)
	{
		sd.flags = SDMipmapCount;
		sd.caps.caps1 = CapsMipmap | CapsComplex;
		sd.mipmapCount = frame->mipCount;
	}
	if (encodeFlags & EncodeFlagCubemap)
	{
		sd.caps.caps2 |= Caps2Cubmap;
	}
	else
	{
		sd.caps.caps1 |= CapsTexture;
	}

	sd.flags |= SDCaps | SDWidth | SDHeight | SDLinearSize;

	if (IS_DXT(encodeFormat) || fpFormat)
	{
		sd.pf.RGBBitCount = 0;
	}
	else
	{
		sd.pf.RGBBitCount = FormatBPP(encodeFormat) * 8;
	}

	SwapSurfaceDesc(&sd); // swap for write.
	if (!outBuff.Write(&sd, sizeof(SurfaceDesc2)))
	{
		return false;
	}
	SwapSurfaceDesc(&sd); // swap to use again.

	FormatToColorBits((NOT_DXT(encodeFormat)) ? encodeFormat : in.format, &sd.pf.RBitMask, &sd.pf.GBitMask, &sd.pf.BBitMask, &sd.pf.ABitMask);

	bool s = false;

	if (encodeFlags & EncodeFlagCubemap)
	{
		UReg i;
		for (i = 0; i < in.frameCount; i++)
		{
			if (!EncodeMipMaps(&sd, &in.frames[i], in.bpp, in.format, encodeFormat, &outBuff))
			{
				break;
			}
		}

		s = i == in.frameCount;
	}
	else
	{
		s = EncodeMipMaps(&sd, &in.frames[0], in.bpp, in.format, encodeFormat, &outBuff);
	}

	if (s)
	{
		outBuff.Trim();
		RAD_ASSERT(outBuff.m_buff);
		outData = outBuff.m_buff;
		outBuff.m_buff = 0;
		outSize = outBuff.m_size;
	}


	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////
// DXT Encode/Decode
//////////////////////////////////////////////////////////////////////////////////////////

static const U8* DecodeDXT1(const U8* dxtBuff, AddrSize* io_dxtBuffLength, void* outBuff, UReg width, UReg height)
{
	RAD_ASSERT(dxtBuff&&io_dxtBuffLength);
	RAD_ASSERT(outBuff);
	RAD_ASSERT(width&&height);

	U32 colors[4][4];
	U16 colors565[2]; // 565
	U32 bitmask;
	const AddrSize stride = width * 4;

	const U32 rshift = 8;
	const U32 rmask  = 0xF800;
	const U32 gshift = 3;
	const U32 gmask  = 0x7E0;
	const U32 bmask  = 0x1F;
	const U32 bshift = 3;

	U8* dst = (U8*)outBuff;
	
	// check size. (8 bytes per 16 pixels).
	UReg lumpSize = (UReg)std::max<UReg>(1, (width*height/16)) * 8;
	if (*io_dxtBuffLength < lumpSize) return 0;

	for (UReg y = 0; y < height; y+=4)
	{
		for (UReg x = 0; x < width; x+=4)
		{
			colors565[0] = endian::SwapLittle(((U16*)(dxtBuff+0))[0]);
			colors565[1] = endian::SwapLittle(((U16*)(dxtBuff+2))[0]);
			bitmask      = endian::SwapLittle(((U32*)(dxtBuff+4))[0]);
			dxtBuff += 8; // next block.

			colors[0][0] = (colors565[0]&rmask)>>rshift;
			colors[0][1] = (colors565[0]&gmask)>>gshift;
			colors[0][2] = (colors565[0]&bmask)<<bshift;
			colors[0][3] = 0xFF;

			colors[1][0] = (colors565[1]&rmask)>>rshift;
			colors[1][1] = (colors565[1]&gmask)>>gshift;
			colors[1][2] = (colors565[1]&bmask)<<bshift;
			colors[1][3] = 0xFF;

			// block type?
			if (colors565[0] > colors565[1]) // 4 colors
			{
				colors[2][0] = (2 * colors[0][0] + colors[1][0] + 1) / 3;
				colors[2][1] = (2 * colors[0][1] + colors[1][1] + 1) / 3;
				colors[2][2] = (2 * colors[0][2] + colors[1][2] + 1) / 3;
				colors[2][3] = 0xFF;

				colors[3][0] = (2 * colors[1][0] + colors[0][0] + 1) / 3;
				colors[3][1] = (2 * colors[1][1] + colors[0][1] + 1) / 3;
				colors[3][2] = (2 * colors[1][2] + colors[0][2] + 1) / 3;
				colors[3][3] = 0xFF;
			}
			else // 3 colors and 1 alpha
			{
				colors[2][0] = (colors[0][0] + colors[1][0]) / 2;
				colors[2][1] = (colors[0][1] + colors[1][1]) / 2;
				colors[2][2] = (colors[0][2] + colors[1][2]) / 2;
				colors[2][3] = 0xFF;

				colors[3][0] = (2 * colors[1][0] + colors[0][0] + 1) / 3;
				colors[3][1] = (2 * colors[1][1] + colors[0][1] + 1) / 3;
				colors[3][2] = (2 * colors[1][2] + colors[0][2] + 1) / 3;
				colors[3][3] = 0x00;
			}

			// use the bitmask to select the colors.
			U32* pcl;
			UReg sel;
			AddrSize ofs;

			for (UReg by = 0, bit = 0; by < 4; by++)
			{
				for (UReg bx = 0; bx < 4; bx++, bit++)
				{
					if (((x+bx)<width) && ((y+by)<height))
					{
						sel = (bitmask & (0x03 << (bit<<1))) >> (bit<<1);
						pcl = colors[sel];

						ofs = (by+y) * stride + ((bx+x)<<2);
						dst[ofs + 0] = (U8)pcl[0];
						dst[ofs + 1] = (U8)pcl[1];
						dst[ofs + 2] = (U8)pcl[2];
						dst[ofs + 3] = (U8)pcl[3];
					}
				}
			}
		}
	}

	*io_dxtBuffLength -= lumpSize;

	return dxtBuff;
}

static const U8* DecodeDXT3(const U8* dxtBuff, AddrSize* io_dxtBuffLength, void* outBuff, UReg width, UReg height)
{
	RAD_ASSERT(dxtBuff&&io_dxtBuffLength);
	RAD_ASSERT(outBuff);
	RAD_ASSERT(width&&height);

	U32 colors[4][4];
	U16 colors565[2]; // 565
	U32 bitmask;
	U16 alphaBlock[4];
	const AddrSize stride = width * 4;

	const U32 rshift = 8;
	const U32 rmask  = 0xF800;
	const U32 gshift = 3;
	const U32 gmask  = 0x7E0;
	const U32 bmask  = 0x1F;
	const U32 bshift = 3;

	U8* dst = (U8*)outBuff;
	
	// check size. (8 bytes per 16 pixels).
	UReg lumpSize = (UReg)std::max<UReg>(1, (width*height/16)) * 16;
	if (*io_dxtBuffLength < lumpSize) return 0;

	for (UReg y = 0; y < height; y+=4)
	{
		for (UReg x = 0; x < width; x+=4)
		{
			alphaBlock[0] = endian::SwapLittle(((U16*)(dxtBuff))[0]);
			alphaBlock[1] = endian::SwapLittle(((U16*)(dxtBuff))[1]);
			alphaBlock[2] = endian::SwapLittle(((U16*)(dxtBuff))[2]);
			alphaBlock[3] = endian::SwapLittle(((U16*)(dxtBuff))[3]);
			dxtBuff += 8;

			colors565[0] = endian::SwapLittle(((U16*)(dxtBuff+0))[0]);
			colors565[1] = endian::SwapLittle(((U16*)(dxtBuff+2))[0]);
			bitmask      = endian::SwapLittle(((U32*)(dxtBuff+4))[0]);
			dxtBuff += 8; // next block.

			colors[0][0] = (colors565[0]&rmask)>>rshift;
			colors[0][1] = (colors565[0]&gmask)>>gshift;
			colors[0][2] = (colors565[0]&bmask)<<bshift;
			colors[0][3] = 0xFF;

			colors[1][0] = (colors565[1]&rmask)>>rshift;
			colors[1][1] = (colors565[1]&gmask)>>gshift;
			colors[1][2] = (colors565[1]&bmask)<<bshift;
			colors[1][3] = 0xFF;

			colors[2][0] = (2 * colors[0][0] + colors[1][0] + 1) / 3;
			colors[2][1] = (2 * colors[0][1] + colors[1][1] + 1) / 3;
			colors[2][2] = (2 * colors[0][2] + colors[1][2] + 1) / 3;
			colors[2][3] = 0xFF;

			colors[3][0] = (2 * colors[1][0] + colors[0][0] + 1) / 3;
			colors[3][1] = (2 * colors[1][1] + colors[0][1] + 1) / 3;
			colors[3][2] = (2 * colors[1][2] + colors[0][2] + 1) / 3;
			colors[3][3] = 0xFF;
						
			// use the bitmask to select the colors.
			U32* pcl;
			UReg sel;
			U16 alpha;
			AddrSize ofs;

			for (UReg by = 0, bit = 0; by < 4; by++)
			{
				alpha = alphaBlock[by];
				for (UReg bx = 0; bx < 4; bx++, bit++)
				{
					if (((x+bx)<width) && ((y+by)<height))
					{
						sel = (bitmask & (0x03 << (bit<<1))) >> (bit<<1);
						pcl = colors[sel];

						ofs = (by+y) * stride + ((bx+x)<<2);
						dst[ofs + 0] = (U8)pcl[0];
						dst[ofs + 1] = (U8)pcl[1];
						dst[ofs + 2] = (U8)pcl[2];
#pragma message("check this")
						dst[ofs + 3] = (U8)((alpha&0xF) | ((alpha&0xF)<<4));
					}

					alpha >>= 4;
				}
			}
		}
	}

	*io_dxtBuffLength -= lumpSize;

	return dxtBuff;
}

static const U8* DecodeDXT5(const U8* dxtBuff, AddrSize* io_dxtBuffLength, void* outBuff, UReg width, UReg height)
{
	RAD_ASSERT(dxtBuff&&io_dxtBuffLength);
	RAD_ASSERT(outBuff);
	RAD_ASSERT(width&&height);

	U32 colors[4][4];
	U16 colors565[2]; // 565
	U32 bitmask;
	UReg alphas[8];
	U8 alphamask[7];
	const AddrSize stride = width * 4;

	const U32 rshift = 8;
	const U32 rmask  = 0xF800;
	const U32 gshift = 3;
	const U32 gmask  = 0x7E0;
	const U32 bmask  = 0x1F;
	const U32 bshift = 3;

	U8* dst = (U8*)outBuff;
	
	// check size. (8 bytes per 16 pixels).
	UReg lumpSize = (UReg)std::max<UReg>(1, (width*height/16)) * 16;
	if (*io_dxtBuffLength < lumpSize) return 0;

	for (UReg y = 0; y < height; y+=4)
	{
		for (UReg x = 0; x < width; x+=4)
		{
			alphas[0]    = dxtBuff[0];
			alphas[1]    = dxtBuff[1];
			alphamask[0] = dxtBuff[2];
			alphamask[1] = dxtBuff[3];
			alphamask[2] = dxtBuff[4];
			alphamask[3] = dxtBuff[5];
			alphamask[4] = dxtBuff[6];
			alphamask[5] = dxtBuff[7];
			dxtBuff += 8;

			colors565[0] = endian::SwapLittle(((U16*)(dxtBuff+0))[0]);
			colors565[1] = endian::SwapLittle(((U16*)(dxtBuff+2))[0]);
			bitmask      = endian::SwapLittle(((U32*)(dxtBuff+4))[0]);
			dxtBuff += 8; // next block.

			colors[0][0] = (colors565[0]&rmask)>>rshift;
			colors[0][1] = (colors565[0]&gmask)>>gshift;
			colors[0][2] = (colors565[0]&bmask)<<bshift;
			colors[0][3] = 0xFF;

			colors[1][0] = (colors565[1]&rmask)>>rshift;
			colors[1][1] = (colors565[1]&gmask)>>gshift;
			colors[1][2] = (colors565[1]&bmask)<<bshift;
			colors[1][3] = 0xFF;

			colors[2][0] = (2 * colors[0][0] + colors[1][0] + 1) / 3;
			colors[2][1] = (2 * colors[0][1] + colors[1][1] + 1) / 3;
			colors[2][2] = (2 * colors[0][2] + colors[1][2] + 1) / 3;
			colors[2][3] = 0xFF;

			colors[3][0] = (2 * colors[1][0] + colors[0][0] + 1) / 3;
			colors[3][1] = (2 * colors[1][1] + colors[0][1] + 1) / 3;
			colors[3][2] = (2 * colors[1][2] + colors[0][2] + 1) / 3;
			colors[3][3] = 0xFF;

			// 8 or 6 alpha block?
			if (alphas[0] > alphas[1])
			{
				// 8-alpha block:  derive the other six alphas.    
				// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
				alphas[2] = (6 * alphas[0] + 1 * alphas[1] + 3) / 7;	// bit code 010
				alphas[3] = (5 * alphas[0] + 2 * alphas[1] + 3) / 7;	// bit code 011
				alphas[4] = (4 * alphas[0] + 3 * alphas[1] + 3) / 7;	// bit code 100
				alphas[5] = (3 * alphas[0] + 4 * alphas[1] + 3) / 7;	// bit code 101
				alphas[6] = (2 * alphas[0] + 5 * alphas[1] + 3) / 7;	// bit code 110
				alphas[7] = (1 * alphas[0] + 6 * alphas[1] + 3) / 7;	// bit code 111  
			}
			else
			{
				// 6-alpha block.    
				// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
				alphas[2] = (4 * alphas[0] + 1 * alphas[1] + 2) / 5;	// Bit code 010
				alphas[3] = (3 * alphas[0] + 2 * alphas[1] + 2) / 5;	// Bit code 011
				alphas[4] = (2 * alphas[0] + 3 * alphas[1] + 2) / 5;	// Bit code 100
				alphas[5] = (1 * alphas[0] + 4 * alphas[1] + 2) / 5;	// Bit code 101
				alphas[6] = 0x00;										// Bit code 110
				alphas[7] = 0xFF;										// Bit code 111
			}
						
			// use the bitmask to select the colors.
			U32* pcl;
			UReg sel;
			AddrSize ofs;

			for (UReg by = 0, bit = 0; by < 4; by++)
			{
				for (UReg bx = 0; bx < 4; bx++, bit++)
				{
					if (((x+bx)<width) && ((y+by)<height))
					{
						sel = (bitmask & (0x03 << (bit<<1))) >> (bit<<1);
						pcl = colors[sel];

						ofs = (by+y) * stride + ((bx+x)<<2);
						dst[ofs + 0] = (U8)pcl[0];
						dst[ofs + 1] = (U8)pcl[1];
						dst[ofs + 2] = (U8)pcl[2];
					}
				}
			}

			U32 abits = endian::SwapLittle(((U32*)&alphamask[0])[0]);

			for (UReg by = 0; by < 2; by++)
			{
				for (UReg bx = 0; bx < 4; bx++)
				{
					if (((x+bx)<width) && ((y+by)<height))
					{
						ofs = (by+y) * stride + ((bx+x)<<2) + 3;
						dst[ofs] = (U8)alphas[abits & 0x07];
					}
					abits >>= 3;
				}
			}

			abits = endian::SwapLittle(((U32*)&alphamask[3])[0]);

			for (UReg by = 2; by < 4; by++)
			{
				for (UReg bx = 0; bx < 4; bx++)
				{
					if (((x+bx)<width) && ((y+by)<height))
					{
						ofs = (by+y) * stride + ((bx+x)<<2) + 3;
						dst[ofs] = (U8)alphas[abits & 0x07];
					}
					abits >>= 3;
				}
			}
		}
	}

	*io_dxtBuffLength -= lumpSize;

	return dxtBuff;
}

static const U8* RADRT_CALL PrivateDXTDecode(const U8* dxtBuff, AddrSize* io_dxtBuffLength, UReg dxtFormat, UReg width, UReg height, void* outBuff)
{
	RAD_ASSERT(dxtBuff&&io_dxtBuffLength);
	RAD_ASSERT(outBuff);
	RAD_ASSERT(width&&height);

	const U8* r = 0;

	switch (dxtFormat)
	{
	case Format_DXT1:
		r = DecodeDXT1(dxtBuff, io_dxtBuffLength, outBuff, width, height);
	break;
	case Format_DXT3:
		r = DecodeDXT3(dxtBuff, io_dxtBuffLength, outBuff, width, height);
	break;
	case Format_DXT5:
		r = DecodeDXT5(dxtBuff, io_dxtBuffLength, outBuff, width, height);
	break;
	}

	return r;
}

RADRT_API bool RADRT_CALL DXTDecode(const void* dxtBuff, AddrSize dxtBuffLength, UReg dxtFormat, UReg width, UReg height, void* outBuff)
{
	RAD_ASSERT(dxtBuff&&dxtBuffLength);
	RAD_ASSERT(outBuff);
	RAD_ASSERT(width&&height);

	return PrivateDXTDecode((const U8*)dxtBuff, &dxtBuffLength, dxtFormat, width, height, outBuff) != 0;
}

static void GetBlock(U16* block, U16* pixels, UReg xp, UReg yp, UReg width, UReg height)
{
	UReg i = 0;

	for (UReg y = 0; y < 4; y++)
	{
		for (UReg x = 0; x < 4; x++)
		{
			UReg ofs = std::min(height, y+yp) * width + std::min(width, x+xp);
			block[i++] = pixels[ofs];
		}
	}
}

static void GetAlphaBlock(U8* block, U8* alpha, UReg xp, UReg yp, UReg width, UReg height)
{
	UReg i = 0;

	for (UReg y = 0; y < 4; y++)
	{
		for (UReg x = 0; x < 4; x++)
		{
			UReg ofs = std::min(height, y+yp) * width + std::min(width, x+xp);
			block[i++] = alpha[ofs];
		}
	}
}

static void CorrectDXTEndPoints(U16* color0, U16* color1, bool alpha)
{
	U16 t;

	if (alpha)
	{
		if (*color0 > *color1)
		{
			t = *color0;
			*color0 = *color1;
			*color1 = t;
		}
	}
	else
	{
		if (*color0 < *color1)
		{
			t = *color0;
			*color0 = *color1;
			*color1 = t;
		}
	}
}

static SReg Distance(const U8* c0, const U8* c1)
{
	SReg r0, g0, b0;
	SReg r1, g1, b1;

	r0 = c0[0];
	g0 = c0[1];
	b0 = c0[2];

	r1 = c1[0];
	g1 = c1[1];
	b1 = c1[2];

	return ((r0 - r1) * (r0 - r1) + (g0 - g1) * (g0 - g1) + (b0 - b1) * (b0 - b1));
}

static void ChooseEndPoints(const U16* block, U16* c0, U16* c1)
{
	U8 colors[16][3];
	SReg best = -1, d;
	
	for (UReg i = 0; i < 16; i++)
	{
		ConvertPixel((U8*)&block[i], colors[i], 0, Format_RGB565, Format_RGB888);
	}

	for (UReg i = 0; i < 16; i++)
	{
		for (UReg k = i+1; k < 16; k++)
		{
			d = Distance(colors[i], colors[k]);
			if (d > best)
			{
				best = d;
				*c0 = block[i];
				*c1 = block[k];
			}
		}
	}
}

static void ChooseAlphaEndPoints(const U8* block, U8* a0, U8* a1)
{
	UReg low = 0xFF, high = 0;

	for (UReg i = 0; i < 16; i++)
	{
		if (block[i] < low)
		{
			*a1 = block[i];
			low = block[i];
		}
		else if (block[i] > high)
		{
			*a0 = block[i];
			high = block[i];
		}
	}
}

static U32 GenBitMask(U16 ex0, U16 ex1, UReg numColors, const U16* srcColors, const U8* alpha)
{
	RAD_ASSERT(srcColors);
	RAD_ASSERT(numColors==3||numColors==4);
	U8 mask[16];
	U8 colors[4][3];
	
	ConvertPixel((const U8*)&ex0, colors[0], 0, Format_RGB565, Format_RGB888);
	ConvertPixel((const U8*)&ex1, colors[1], 0, Format_RGB565, Format_RGB888);

	if (numColors == 3)
	{
		colors[2][0] = (U8)(((UReg)colors[0][0] + (UReg)colors[1][0]) / 2);
		colors[2][1] = (U8)(((UReg)colors[0][1] + (UReg)colors[1][1]) / 2);
		colors[2][2] = (U8)(((UReg)colors[0][2] + (UReg)colors[1][2]) / 2);
		colors[3][0] = colors[2][0];
		colors[3][1] = colors[2][1];
		colors[3][2] = colors[2][2];
	}
	else
	{
		colors[2][0] = (U8)((2 * (UReg)colors[0][0] + (UReg)colors[1][0] + 1) / 3);
		colors[2][1] = (U8)((2 * (UReg)colors[0][1] + (UReg)colors[1][1] + 1) / 3);
		colors[2][2] = (U8)((2 * (UReg)colors[0][2] + (UReg)colors[1][2] + 1) / 3);
		colors[3][0] = (U8)((2 * (UReg)colors[1][0] + (UReg)colors[0][0] + 1) / 3);
		colors[3][1] = (U8)((2 * (UReg)colors[1][1] + (UReg)colors[0][1] + 1) / 3);
		colors[3][2] = (U8)((2 * (UReg)colors[1][2] + (UReg)colors[0][2] + 1) / 3);
	}

	for (UReg i = 0; i < 16; i++)
	{
		if (alpha && (alpha[i] < 128)) // alpha test?
		{
			mask[i] = 3; // transparent pixel.
		}
		else
		{
			SReg best = std::numeric_limits<SReg>::max();
			U8 c[3];
			ConvertPixel((const U8*)&srcColors[i], c, 0, Format_RGB565, Format_RGB888);
			for (UReg k = 0; k < numColors; k++)
			{
				SReg d = Distance(c, colors[k]);
				if (d < best)
				{
					best = d;
					mask[i] = k;
				}
			}
		}
	}

	U32 bitMask = 0;
	for (UReg i = 0; i < 16; i++) bitMask  |= (mask[i] << (i<<1));

	return bitMask;
}

static void GenAlphaBitMask(U8 a0, U8 a1, UReg numColors, U8* srcAlpha, U8* mask, U8* alpha)
{
	RAD_ASSERT(numColors==8||numColors==6);
	RAD_ASSERT(srcAlpha);
	RAD_ASSERT(mask);
	RAD_ASSERT(alpha);

	SReg workAlpha[8], m[16];

	workAlpha[0] = a0;
	workAlpha[1] = a1;

	if (numColors==8)
	{
		// 8-alpha block:  derive the other six alphas.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		workAlpha[2] = (6 * workAlpha[0] + 1 * workAlpha[1] + 3) / 7;	// bit code 010
		workAlpha[3] = (5 * workAlpha[0] + 2 * workAlpha[1] + 3) / 7;	// bit code 011
		workAlpha[4] = (4 * workAlpha[0] + 3 * workAlpha[1] + 3) / 7;	// bit code 100
		workAlpha[5] = (3 * workAlpha[0] + 4 * workAlpha[1] + 3) / 7;	// bit code 101
		workAlpha[6] = (2 * workAlpha[0] + 5 * workAlpha[1] + 3) / 7;	// bit code 110
		workAlpha[7] = (1 * workAlpha[0] + 6 * workAlpha[1] + 3) / 7;	// bit code 111  
	}
	else
	{
		// 6-alpha block.    
		// Bit code 000 = alpha_0, 001 = alpha_1, others are interpolated.
		workAlpha[2] = (4 * workAlpha[0] + 1 * workAlpha[1] + 2) / 5;	// Bit code 010
		workAlpha[3] = (3 * workAlpha[0] + 2 * workAlpha[1] + 2) / 5;	// Bit code 011
		workAlpha[4] = (2 * workAlpha[0] + 3 * workAlpha[1] + 2) / 5;	// Bit code 100
		workAlpha[5] = (1 * workAlpha[0] + 4 * workAlpha[1] + 2) / 5;	// Bit code 101
		workAlpha[6] = 0x00;											// Bit code 110
		workAlpha[7] = 0xFF;											// Bit code 111
	}

	for (UReg i = 0; i < 16; i++)
	{
		SReg best = std::numeric_limits<SReg>::max();
		for (UReg j = 0; j < 8; j++)
		{
			SReg d = (SReg)srcAlpha[i] - workAlpha[j];
			if (d < 0) d = -d;
			if (d < best)
			{
				best = d;
				m[i] = j;
			}
		}
	}

	// build mask (6 bytes).
	mask[0] = (U8)((m[0]) | (m[1]<<3) | ((m[2]&0x3)<<6));
	mask[1] = (U8)((m[2]&0x4) | (m[3]<<1) | (m[4]<<4) | ((m[5]&0x1)<<7));
	mask[2] = (U8)((m[5]&0x6) | (m[6]<<2) | (m[7]<<5));
	mask[3] = (U8)((m[8]) | (m[9]<<3) | ((m[10]&0x3)<<6));
	mask[4] = (U8)((m[10]&0x4) | (m[11]<<1) | (m[12]<<4) | ((m[13]&0x1)<<7));
	mask[5] = (U8)((m[13]&0x6) | (m[14]<<2) | (m[15]<<5));
}

static U8* ExtractAlpha(const void* src, AddrSize srcLength, UReg srcFormat, UReg width, UReg height)
{
	RAD_ASSERT(src);
	RAD_ASSERT(srcLength);
	RAD_ASSERT(width&&height);

	U8* rgb = (U8*)src;
	
	if (srcFormat != Format_RGBA8888)
	{
		rgb = (U8*)zone_malloc(ZImageCodec, width*height*4, 0);
		if (!rgb) return 0;
		if (!ConvertPixelData(src, srcLength, rgb, 0, srcFormat, Format_RGBA8888))
		{
			zone_free(rgb);
			return 0;
		}
		srcLength = width*height*4;
	}

	U8* alpha, *wrkAlpha;
	wrkAlpha = alpha = (U8*)zone_malloc(ZImageCodec, width*height, 0);
	if (!alpha)
	{
		zone_free(rgb);
		return 0;
	}

	for (AddrSize ofs = 3; ofs < srcLength; ofs += 4)
	{
		*(wrkAlpha++) = rgb[ofs];
	}

	if (src != (void*)rgb)
	{
		zone_free(rgb);
	}

	return alpha;
}

static bool PrivateEncodeDXT1(const void* src, AddrSize srcLength, UReg srcFormat, UReg width, UReg height, void* outBuff)
{
    U8* dstB = (U8*)outBuff;
	U16* wrkSrc = (U16*)src;

	if (srcFormat != Format_RGB565)
	{
		// convert.
		wrkSrc = (U16*)zone_malloc(ZImageCodec, width*height*2, 0);
		if (!wrkSrc) return false;

		if (!ConvertPixelData(src, srcLength, (void*)wrkSrc, 0, srcFormat, Format_RGB565))
		{
			zone_free(wrkSrc);
			return false;
		}
	}

	U8* alpha = ExtractAlpha(src, srcLength, srcFormat, width, height);
	if (!alpha)
	{
		if (src != (void*)wrkSrc)
		{
			zone_free(wrkSrc);
		}

		return false;
	}

	U32 bitMask;
	U16 block[16], c0, c1;
	U8  alphaBlock[16];
    
	for (UReg y = 0; y < height; y+=4)
	{
		for (UReg x = 0; x < width; x+=4)
		{
			GetAlphaBlock(alphaBlock, alpha, x, y, width, height);
			bool hasAlpha = false;

			// do we need to encode alpha?
			for (UReg i = 0; i < 16; i++)
			{
				if (alphaBlock[i] < 128)
				{
					hasAlpha = true; break;
				}
			}

			GetBlock(block, wrkSrc, x, y, width, height);
			ChooseEndPoints(block, &c0, &c1);
			CorrectDXTEndPoints(&c0, &c1, hasAlpha);
			((U16*)dstB)[0] = endian::SwapLittle(c0);
			((U16*)dstB)[1] = endian::SwapLittle(c1);
			dstB += 4;
			if (hasAlpha)
			{
				bitMask = GenBitMask(c0, c1, 3, block, alphaBlock);
			}
			else
			{
				bitMask = GenBitMask(c0, c1, 4, block, 0);
			}
			((U32*)dstB)[0] = endian::SwapLittle(bitMask);
			dstB += 4;
		}
	}

	if (src != (void*)wrkSrc)
	{
		zone_free(wrkSrc);
	}

	zone_free(alpha);

	return true;
}

static bool PrivateEncodeDXT3(const void* src, AddrSize srcLength, UReg srcFormat, UReg width, UReg height, void* outBuff)
{
    U8* dstB = (U8*)outBuff;
	U16* wrkSrc = (U16*)src;

	if (srcFormat != Format_RGB565)
	{
		// convert.
		wrkSrc = (U16*)zone_malloc(ZImageCodec, width*height*2, 0);
		if (!wrkSrc) return false;

		if (!ConvertPixelData(src, srcLength, (void*)wrkSrc, 0, srcFormat, Format_RGB565))
		{
			zone_free(wrkSrc);
			return false;
		}
	}

	U8* alpha = ExtractAlpha(src, srcLength, srcFormat, width, height);
	if (!alpha)
	{
		if (src != (void*)wrkSrc)
		{
			zone_free(wrkSrc);
		}

		return false;
	}

	U32 bitMask;
	U16 block[16], c0, c1;
	U8  alphaBlock[16];
    
	for (UReg y = 0; y < height; y+=4)
	{
		for (UReg x = 0; x < width; x+=4)
		{
			GetAlphaBlock(alphaBlock, alpha, x, y, width, height);
			for (UReg i = 0; i < 16; i+=2)
			{
				*(dstB++) = ((alphaBlock[i]>>4)<<4) | (alphaBlock[i+1]>>4);
			}

			GetBlock(block, wrkSrc, x, y, width, height);
			ChooseEndPoints(block, &c0, &c1);
			CorrectDXTEndPoints(&c0, &c1, false);
			((U16*)dstB)[0] = endian::SwapLittle(c0);
			((U16*)dstB)[1] = endian::SwapLittle(c1);
			dstB += 4;
			bitMask = GenBitMask(c0, c1, 4, block, 0);
			((U32*)dstB)[0] = endian::SwapLittle(bitMask);
			dstB += 4;
		}
	}

	if (src != (void*)wrkSrc)
	{
		zone_free(wrkSrc);
	}

	zone_free(alpha);

	return true;
}

static bool PrivateEncodeDXT5(const void* src, AddrSize srcLength, UReg srcFormat, UReg width, UReg height, void* outBuff)
{
    U8* dstB = (U8*)outBuff;
	U16* wrkSrc = (U16*)src;

	if (srcFormat != Format_RGB565)
	{
		// convert.
		wrkSrc = (U16*)zone_malloc(ZImageCodec, width*height*2, 0);
		if (!wrkSrc) return false;

		if (!ConvertPixelData(src, srcLength, (void*)wrkSrc, 0, srcFormat, Format_RGB565))
		{
			zone_free(wrkSrc);
			return false;
		}
	}

	U8* alpha = ExtractAlpha(src, srcLength, srcFormat, width, height);
	if (!alpha)
	{
		if (src != (void*)wrkSrc)
		{
			zone_free(wrkSrc);
		}

		return false;
	}

	U32 bitMask;
	U16 block[16], c0, c1;
	U8  alphaBlock[16], alphaMask[6], alphaOut[16], a0, a1;
    
	for (UReg y = 0; y < height; y+=4)
	{
		for (UReg x = 0; x < width; x+=4)
		{
			GetAlphaBlock(alphaBlock, alpha, x, y, width, height);
			ChooseAlphaEndPoints(alphaBlock, &a0, &a1);
			GenAlphaBitMask(a0, a1, 6, alphaBlock, alphaMask, alphaOut);
			dstB[0] = a0;
			dstB[1] = a1;
			dstB[2] = alphaMask[0];
			dstB[3] = alphaMask[1];
			dstB[4] = alphaMask[2];
			dstB[5] = alphaMask[3];
			dstB[6] = alphaMask[4];
			dstB[7] = alphaMask[5];
			dstB += 8;

			GetBlock(block, wrkSrc, x, y, width, height);
			ChooseEndPoints(block, &c0, &c1);
			CorrectDXTEndPoints(&c0, &c1, false);
			((U16*)dstB)[0] = endian::SwapLittle(c0);
			((U16*)dstB)[1] = endian::SwapLittle(c1);
			dstB += 4;
			bitMask = GenBitMask(c0, c1, 4, block, 0);
			((U32*)dstB)[0] = endian::SwapLittle(bitMask);
			dstB += 4;
		}
	}

	if (src != (void*)wrkSrc)
	{
		zone_free(wrkSrc);
	}

	zone_free(alpha);

	return true;
}

RADRT_API bool RADRT_CALL DXTEncode(const void* src, AddrSize srcLength, UReg srcFormat, UReg dstFormat, UReg width, UReg height, void* outBuff)
{
	RAD_ASSERT(src&&srcLength);
	RAD_ASSERT(NOT_DXT(srcFormat));
	RAD_ASSERT(IS_DXT(dstFormat));
	RAD_ASSERT(outBuff);
	RAD_ASSERT(width&&height);
	RAD_ASSERT(PowerOf2(width)==width);
	RAD_ASSERT(PowerOf2(height)==height);

	bool s = false;

	switch (dstFormat)
	{
	case Format_DXT1:
	case Format_DXT1A:
		s = PrivateEncodeDXT1(src, srcLength, srcFormat, width, height, outBuff);
	break;
	case Format_DXT3:
		s = PrivateEncodeDXT3(src, srcLength, srcFormat, width, height, outBuff);
	break;
	case Format_DXT5:
		s = PrivateEncodeDXT5(src, srcLength, srcFormat, width, height, outBuff);
	break;
	}

	return s;
}

RADRT_API AddrSize RADRT_CALL DXTEncodeSize(UReg format, UReg width, UReg height)
{
	switch(format)
	{
	case Format_DXT1:
	case Format_DXT1A:
		return std::max<UReg>((width*height/16) * 8, 8);
	case Format_DXT3:
	case Format_DXT5:
		return std::max<UReg>((width*height/16) * 16, 16);
	}

	RAD_FAIL("Bad dxt pixel format!");
	return 0;
}

} // dds
} // image_codec

