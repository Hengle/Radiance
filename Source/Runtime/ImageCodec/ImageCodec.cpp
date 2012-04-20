// ImageCodec.cpp
// Image Codec System.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "ImageCodec.h"
#include <algorithm>

namespace image_codec {

RAD_ZONE_DEF(RADRT_API, ZImageCodec, "Image Codec", ZRuntime);

//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL ConvertPixel(const void* voidsrcPix, void* voiddstPix, const void* pal, UReg srcFormat, UReg dstFormat)
{
	RAD_ASSERT(voidsrcPix);
	RAD_ASSERT(voiddstPix);
	RAD_ASSERT(FormatBPP(srcFormat)!=0);
	RAD_ASSERT(FormatBPP(dstFormat)!=0);
	RAD_ASSERT((!IsPalettedFormat(srcFormat)) || pal); // if it's palettized must supply the palette.

	const U8* srcPix = (const U8*)voidsrcPix;
	U8* dstPix = (U8*)voiddstPix;

	switch(srcFormat)
	{
	case Format_A8:
		{
			switch(dstFormat)
			{
			case Format_A8:
				*dstPix = *srcPix;
			break;
			case Format_RGB555:
			case Format_BGR555:
				{
					U16 a = (srcPix[0]>>3) & 31;
					((U16*)dstPix)[0] = (a | (a<<5) | (a<<10));
				}
			break;
			case Format_RGB565:
			case Format_BGR565:
				{
					U16 a = (srcPix[0]>>3) & 31;
					((U16*)dstPix)[0] = (a | (a<<5) | (a<<11));
				}
			break;
			case Format_RGBA4444:
			case Format_BGRA4444:
				{
					U16 a = (srcPix[0]>>4) & 15;
					((U16*)dstPix)[0] = (a | (a<<4) | (a<<8) | 0xF000);
				}
			break;
			case Format_RGBA5551:
			case Format_BGRA5551:
				{
					U16 a = (srcPix[0]>>3) & 31;
					((U16*)dstPix)[0] = (a | (a<<5) | (a<<10) | 0x8000);
				}
			break;
			case Format_RGBA8888:
			case Format_BGRA8888:
				dstPix[3] = 0xFF; // fallthrough
			case Format_RGB888:
			case Format_BGR888:
				{
					dstPix[0] = srcPix[0];
					dstPix[1] = srcPix[0];
					dstPix[2] = srcPix[0];
				}
			break;
			}
		}
	break;
	case Format_RGB555:
	case Format_BGR555:
	case Format_PAL8_RGB555:
		{
			U16 p;
			
			if (srcFormat == Format_RGB555)
			{
				p = ((U16*)srcPix)[0];
			}
			else
			{
				RAD_ASSERT(pal);
				p = ((U16*)pal)[((U8*)srcPix)[0]];
			}

			U16 r, g, b;

			if (srcFormat == Format_BGR555)
			{
				r = p & 31;
				g = (p>>5) & 31;
				b = (p>>10) & 31;
			}
			else
			{
				b = p & 31;
				g = (p>>5) & 31;
				r = (p>>10) & 31;
			}

			switch(dstFormat)
			{
			case Format_A8:
				{
					dstPix[0] = (U8)(((r+g+b)/3)<<3);
				}
			break;
			case Format_RGB555:
				{
					((U16*)dstPix)[0] = r | (g<<5) | (b<<10);
				}
			break;
			case Format_BGR555:
				{
					((U16*)dstPix)[0] = b | (g<<5) | (r<<10);
				}
			break;
			case Format_RGB565:
				{
					((U16*)dstPix)[0] = r | (g<<6) | (b<<11);
				}
			break;
			case Format_BGR565:
				{
					((U16*)dstPix)[0] = b | (g<<6) | (r<<11);
				}
			break;
			case Format_RGBA4444:
				{
					((U16*)dstPix)[0] = (r>>1) | ((g>>1)<<4) | ((b>>1)<<8) | 0xF000;
				}
			break;
			case Format_BGRA4444:
				{
					((U16*)dstPix)[0] = (b>>1) | ((g>>1)<<4) | ((r>>1)<<8) | 0xF000;
				}
			break;
			case Format_RGBA5551:
				{
					((U16*)dstPix)[0] = r | (g<<6) || (b<<11) | 0x8000;
				}
			break;
			case Format_BGRA5551:
				{
					((U16*)dstPix)[0] = b | (g<<6) | (r<<11) | 0x8000;
				}
			break;
			case Format_RGBA8888: // fall through:
				dstPix[3] = 0xFF;
			case Format_RGB888:
				{
					dstPix[0] = r << 3;
					dstPix[1] = g << 3;
					dstPix[2] = b << 3;
				}
			break;
			case Format_BGRA8888: // fall through:
				dstPix[3] = 0xFF;
			case Format_BGR888:
				{
					dstPix[0] = b << 3;
					dstPix[1] = g << 3;
					dstPix[2] = r << 3;
				}
			break;
			}
		}
	break;
	case Format_RGB565:
	case Format_BGR565:
	case Format_PAL8_RGB565:
		{
			U16 p;
			
			if (srcFormat == Format_RGB565)
			{
				p = ((U16*)srcPix)[0];
			}
			else
			{
				RAD_ASSERT(pal);
				p = ((U16*)pal)[((U16*)srcPix)[0]];
			}

			U16 r, g, b;

			if (srcFormat == Format_BGR565)
			{
				b = p & 31;
				g = (p>>5) & 63;
				r = (p>>11) & 31;
			}
			else
			{
				r = p & 31;
				g = (p>>5) & 63;
				b = (p>>11) & 31;
			}

			switch(dstFormat)
			{
			case Format_A8:
				{
					dstPix[0] = (U8)(((r+(g>>1)+b)/3)<<3);
				}
			break;
			case Format_RGB555:
				{
					((U16*)dstPix)[0] = r | ((g>>1)<<5) | (b<<10);
				}
			break;
			case Format_BGR555:
				{
					((U16*)dstPix)[0] = b | ((g>>1)<<5) | (r<<10);
				}
			break;
			case Format_RGB565:
				{
					((U16*)dstPix)[0] = r | (g<<5) | (b<<11);
				}
			break;
			case Format_BGR565:
				{
					((U16*)dstPix)[0] = b | (g<<5) | (r<<11);
				}
			break;
			case Format_RGBA4444:
				{
					((U16*)dstPix)[0] = (r>>1) | ((g>>2)<<4) | ((b>>1)<<8) | 0xF000;
				}
			break;
			case Format_BGRA4444:
				{
					((U16*)dstPix)[0] = (b>>1) | ((g>>2)<<4) | ((r>>1)<<8) | 0xF000;
				}
			break;
			case Format_RGBA5551:
				{
					((U16*)dstPix)[0] = r | ((g>>1)<<5) | (b<<10);
				}
			break;
			case Format_BGRA5551:
				{
					((U16*)dstPix)[0] = b | ((g>>1)<<5) | (r<<10);
				}
			break;
			case Format_RGBA8888: // fall through:
				dstPix[3] = 0xFF;
			case Format_RGB888:
				{
					dstPix[0] = r << 3;
					dstPix[1] = (g>>1) << 3;
					dstPix[2] = b << 3;
				}
			break;
			case Format_BGRA8888: // fall through:
				dstPix[3] = 0xFF;
			case Format_BGR888:
				{
					dstPix[0] = b << 3;
					dstPix[1] = (g>>1) << 3;
					dstPix[2] = r << 3;
				}
			break;
			}
		}
	break;
	case Format_RGBA4444:
	case Format_BGRA4444:
		{
			U16 p = ((U16*)srcPix)[0];
			U16 r, g, b, a;

			if (srcFormat == Format_BGRA4444)
			{
				b = p & 15;
				g = (p>>4) & 15;
				r = (p>>8) & 15;
				a = (p>>12) & 15;
			}
			else
			{
				r = p & 15;
				g = (p>>4) & 15;
				b = (p>>8) & 15;
				a = (p>>12) & 15;
			}

			switch(dstFormat)
			{
			case Format_A8:
				{
					dstPix[0] = (U8)(a);
				}
			break;
			case Format_RGB555:
				{
					((U16*)dstPix)[0] = (r<<1) | (g<<6) | (b<<11);
				}
			break;
			case Format_BGR555:
				{
					((U16*)dstPix)[0] = (b<<1) | (g<<6) | (r<<11);
				}
			break;
			case Format_RGB565:
				{
					((U16*)dstPix)[0] = (r<<1) | (g<<7) | (b<<12);
				}
			break;
			case Format_BGR565:
				{
					((U16*)dstPix)[0] = (b<<1) | (g<<7) | (r<<12);
				}
			break;
			case Format_RGBA4444:
				{
					((U16*)dstPix)[0] = r | (g<<4) | (b<<8) | (a<<12);
				}
			break;
			case Format_BGRA4444:
				{
					((U16*)dstPix)[0] = b | (g<<4) | (r<<8) | (a<<12);
				}
			break;
			case Format_RGBA5551:
				{
					((U16*)dstPix)[0] = (r<<1) | (g<<6) | (b<<11) | ((a>=8) ? 0x8000 : 0x0);
				}
			break;
			case Format_BGRA5551:
				{
					((U16*)dstPix)[0] = (b<<1) | (g<<6) | (r<<11) | ((a>=8) ? 0x8000 : 0x0);
				}
			break;
			case Format_RGBA8888: // fall through:
				dstPix[3] = a << 4;
			case Format_RGB888:
				{
					dstPix[0] = r << 4;
					dstPix[1] = g << 4;
					dstPix[2] = b << 4;
				}
			break;
			case Format_BGRA8888: // fall through:
				dstPix[3] = a << 4;
			case Format_BGR888:
				{
					dstPix[0] = b << 4;
					dstPix[1] = g << 4;
					dstPix[2] = r << 4;
				}
			break;
			}
		}
	break;
	case Format_RGBA5551:
	case Format_BGRA5551:
		{
			U16 p = ((U16*)srcPix)[0];
			U16 r, g, b, a;

			if (srcFormat == Format_BGRA5551)
			{
				b = p & 31;
				g = (p>>5) & 31;
				r = (p>>10) & 31;
				a = p & 0x8000;
			}
			else
			{
				r = p & 31;
				g = (p>>5) & 31;
				b = (p>>10) & 31;
				a = p & 0x8000;
			}

			switch(dstFormat)
			{
			case Format_A8:
				{
					dstPix[0] = (U8)((a) ? 0xFF : 0x0);
				}
			break;
			case Format_RGB555:
				{
					((U16*)dstPix)[0] = r | (g<<5) || (b<<10);
				}
			break;
			case Format_BGR555:
				{
					((U16*)dstPix)[0] = b | (g<<5) || (r<<10);
				}
			break;
			case Format_RGB565:
				{
					((U16*)dstPix)[0] = r | (g<<6) | (b<<11);
				}
			break;
			case Format_BGR565:
				{
					((U16*)dstPix)[0] = b | (g<<6) | (r<<11);
				}
			break;
			case Format_RGBA4444:
				{
					((U16*)dstPix)[0] = (r>>1) | ((g>>1)<<4) | ((b>>1)<<8) | ((a) ? 0xF000 : 0x0);
				}
			break;
			case Format_BGRA4444:
				{
					((U16*)dstPix)[0] = (b>>1) | ((g>>1)<<4) | ((r>>1)<<8) | ((a) ? 0xF000 : 0x0);
				}
			break;
			case Format_RGBA5551:
				{
					((U16*)dstPix)[0] = r | (g<<5) | (b<<10) | a;
				}
			break;
			case Format_BGRA5551:
				{
					((U16*)dstPix)[0] = b | (g<<5) | (r<<10) | a;
				}
			break;
			case Format_RGBA8888: // fall through:
				dstPix[3] = (a) ? 0xFF : 0x0;
			case Format_RGB888:
				{
					dstPix[0] = r << 3;
					dstPix[1] = g << 3;
					dstPix[2] = b << 3;
				}
			break;
			case Format_BGRA8888: // fall through:
				dstPix[3] = (a) ? 0xFF : 0x0;
			case Format_BGR888:
				{
					dstPix[0] = b << 3;
					dstPix[1] = g << 3;
					dstPix[2] = r << 3;
				}
			break;
			}
		}
	break;
	case Format_RGB888:
	case Format_BGR888:
	case Format_PAL8_RGB888:
		{
			U16 r, g, b;

			if (srcFormat == Format_RGB888)
			{
				r = (U16)srcPix[0];
				g = (U16)srcPix[1];
				b = (U16)srcPix[2];
			}
			else if (srcFormat == Format_BGR888)
			{
				b = (U16)srcPix[0];
				g = (U16)srcPix[1];
				r = (U16)srcPix[2];
			}
			else // palettized.
			{
				RAD_ASSERT(pal);
				const U8* palB = (const U8*)pal;
				AddrSize ofs = ((AddrSize)(*srcPix)) * 3; // 3 bytes per pixel.

				r = (U16)palB[ofs+0];
				g = (U16)palB[ofs+1];
				b = (U16)palB[ofs+2];
			}
			

			switch(dstFormat)
			{
			case Format_A8:
				{
					dstPix[0] = (U8)((r + g + b) / 3);
				}
			break;
			case Format_RGB555:
				{
					((U16*)dstPix)[0] = ((r>>3)) | ((g>>3)<<5) | ((b>>3)<<10);
				}
			break;
			case Format_BGR555:
				{
					((U16*)dstPix)[0] = ((b>>3)) | ((g>>3)<<5) | ((r>>3)<<10);
				}
			break;
			case Format_RGB565:
				{
					((U16*)dstPix)[0] = ((r>>3)) | ((g>>2)<<5) | ((b>>3)<<11);
				}
			break;
			case Format_BGR565:
				{
					((U16*)dstPix)[0] = ((b>>3)) | ((g>>2)<<5) | ((r>>3)<<11);
				}
			break;
			case Format_RGBA4444:
				{
					((U16*)dstPix)[0] = ((r>>4)) | ((g>>4)<<4) | ((b>>4)<<8) | 0xF000;
				}
			break;
			case Format_BGRA4444:
				{
					((U16*)dstPix)[0] = ((b>>4)) | ((g>>4)<<4) | ((r>>4)<<8) | 0xF000;
				}
			break;
			case Format_RGBA5551:
				{
					((U16*)dstPix)[0] = ((r>>3)) | ((g>>3)<<5) | ((b>>3)<<10) | 0x8000;
				}
			break;
			case Format_BGRA5551:
				{
					((U16*)dstPix)[0] = ((b>>3)) | ((g>>3)<<5) | ((r>>3)<<10) | 0x8000;
				}
			break;
			case Format_RGBA8888: // fall through:
				dstPix[3] = 0xFF;
			case Format_RGB888:
				{
					dstPix[0] = (U8)r;
					dstPix[1] = (U8)g;
					dstPix[2] = (U8)b;
				}
			break;
			case Format_BGRA8888: // fall through:
				dstPix[3] = 0xFF;
			case Format_BGR888:
				{
					dstPix[0] = (U8)b;
					dstPix[1] = (U8)g;
					dstPix[2] = (U8)r;
				}
			break;
			}
		}
	break;
	case Format_RGBA8888:
	case Format_BGRA8888:
	case Format_PAL8_RGBA8888:
		{
			U16 r, g, b, a;

			if (srcFormat == Format_RGBA8888)
			{
				r = srcPix[0];
				g = srcPix[1];
				b = srcPix[2];
				a = srcPix[3];
			}
			else if (srcFormat == Format_BGRA8888)
			{
				b = srcPix[0];
				g = srcPix[1];
				r = srcPix[2];
				a = srcPix[3];
			}
			else
			{
				RAD_ASSERT(pal);
				const U8* palB = (const U8*)pal;
				AddrSize ofs = ((AddrSize)(*srcPix)) * 4; // 4 bytes per pixel.

				r = (U16)palB[ofs+0];
				g = (U16)palB[ofs+1];
				b = (U16)palB[ofs+2];
				a = (U16)palB[ofs+3];
			}
	
			switch(dstFormat)
			{
			case Format_A8:
				{
					dstPix[0] = (U8)(a);
				}
			break;
			case Format_RGB555:
				{
					((U16*)dstPix)[0] = ((r>>3)) | ((g>>3)<<5) | ((b>>3)<<10);
				}
			break;
			case Format_BGR555:
				{
					((U16*)dstPix)[0] = ((b>>3)) | ((g>>3)<<5) | ((r>>3)<<10);
				}
			break;
			case Format_RGB565:
				{
					((U16*)dstPix)[0] = ((r>>3)) | ((g>>2)<<5) | ((b>>3)<<11);
				}
			break;
			case Format_BGR565:
				{
					((U16*)dstPix)[0] = ((b>>3)) | ((g>>2)<<5) | ((r>>3)<<11);
				}
			break;
			case Format_RGBA4444:
				{
					((U16*)dstPix)[0] = ((r>>4)) | ((g>>4)<<4) | ((b>>4)<<8) | ((a>>4)<<12);
				}
			break;
			case Format_BGRA4444:
				{
					((U16*)dstPix)[0] = ((b>>4)) | ((g>>4)<<4) | ((r>>4)<<8) | ((a>>4)<<12);
				}
			break;
			case Format_RGBA5551:
				{
					((U16*)dstPix)[0] = ((r>>3)) | ((g>>3)<<5) | ((b>>3)<<10) | ((a>=128) ? 0x8000 : 0x0);
				}
			break;
			case Format_BGRA5551:
				{
					((U16*)dstPix)[0] = ((b>>3)) | ((g>>3)<<5) | ((r>>3)<<10) | ((a>=128) ? 0x8000 : 0x0);
				}
			break;
			case Format_RGBA8888: // fall through:
				dstPix[3] = (U8)a;
			case Format_RGB888:
				{
					dstPix[0] = (U8)r;
					dstPix[1] = (U8)g;
					dstPix[2] = (U8)b;
				}
			break;
			case Format_BGRA8888: // fall through:
				dstPix[3] = (U8)a;
			case Format_BGR888:
				{
					dstPix[0] = (U8)b;
					dstPix[1] = (U8)g;
					dstPix[2] = (U8)r;
				}
			break;
			}
		}
	break;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// image_codec::FormatBPP()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API UReg RADRT_CALL FormatBPP(UReg format)
{
	switch(format)
	{
	case Format_A8: 
	case Format_PAL8_RGB555:
	case Format_PAL8_RGB565:
	case Format_PAL8_RGB888:
	case Format_PAL8_RGBA8888:
		return 1;
	case Format_RGB555:
	case Format_BGR555:
	case Format_RGB565:
	case Format_BGR565:
	case Format_RGBA4444:
	case Format_BGRA4444:
	case Format_RGBA5551:
	case Format_BGRA5551:
		return 2;
	case Format_RGB888:
	case Format_BGR888:
		return 3;
	case Format_RGBA8888:
	case Format_BGRA8888:
		return 4;
	}

	RAD_FAIL("Bad pixel format!");
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API UReg RADRT_CALL PaletteSize(UReg format)
{
	RAD_ASSERT(IsPalettedFormat(format));
	UReg size = 0;

	switch (format)
	{
	case Format_PAL8_RGB555:
	case Format_PAL8_RGB565:
	case Format_PAL8_RGB888:
	case Format_PAL8_RGBA8888:
		size = PaletteBPP(format) * 256; // 8 bits of palette entry.
	break;
	}

	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////

UReg RADRT_CALL PaletteBPP(UReg format)
{
	RAD_ASSERT(IsPalettedFormat(format));
	UReg size = 0;

	switch (format)
	{
	case Format_PAL8_RGB555:
		size = FormatBPP(Format_RGB555);
	break;
	case Format_PAL8_RGB565:
		size = FormatBPP(Format_RGB565);
	break;

	case Format_PAL8_RGB888:
		size = FormatBPP(Format_RGB888);
	break;

	case Format_PAL8_RGBA8888:
		size = FormatBPP(Format_RGBA8888);
	break;
	}

	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL ConvertPixelData(const void* src, AddrSize srcByteCount, void* dst, const void* pal, UReg srcFormat, UReg dstFormat)
{
	RAD_ASSERT(src&&dst&&srcByteCount);
	RAD_ASSERT(FormatBPP(srcFormat)!=0);
	RAD_ASSERT(FormatBPP(dstFormat)!=0);
	RAD_ASSERT(!IsPalettedFormat(dstFormat)); // we don't quantize.
	RAD_ASSERT((!IsPalettedFormat(srcFormat)) || pal); // if it's palettized must supply the palette.

	UReg srcBPP = FormatBPP(srcFormat);
	UReg dstBPP = FormatBPP(dstFormat);

	if (!(srcBPP&&dstBPP)) 
		return false;

	U8* dstB = (U8*)dst;
	const U8* srcB = (const U8*)src;

	AddrSize pixCount = srcByteCount / srcBPP;

	while (pixCount-- > 0)
	{
		ConvertPixel(srcB, dstB, pal, srcFormat, dstFormat);
		srcB += srcBPP;
		dstB += dstBPP;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL ConvertImageFormat(const Image &src, Image &dst, UReg dstFormat)
{
	RAD_ASSERT(src.bpp!=0);
	RAD_ASSERT(FormatBPP(dstFormat)!=0);
	RAD_ASSERT(!IsPalettedFormat(dstFormat)); // we don't quantize.
	RAD_ASSERT((!IsPalettedFormat(src.format)) || src.pal); // if it's palettized must supply the palette.
	dst.Free();

	if (src.frameCount && (FormatBPP(dstFormat)!=0))
	{
		dst.autoFree = true;
		dst.format = dstFormat;
		dst.bpp = FormatBPP(dstFormat);
		dst.AllocateFrames(src.frameCount);

		for (UReg i = 0; i < src.frameCount; i++)
		{
			Frame* srcF = &src.frames[i];

			if (!srcF->mipCount)
			{
				dst.Free(); 
				return false;
			}

			Frame* dstF = &dst.frames[i];

			if (!dst.AllocateMipmaps(i, srcF->mipCount))
			{
				dst.Free();
				return false;
			}

			for (UReg k = 0; k < srcF->mipCount; k++)
			{
				Mipmap* srcM = &srcF->mipmaps[k];

				if (!dst.AllocateMipmap(i, k, srcM->width, srcM->height, srcM->width * dst.bpp, srcM->width * srcM->height * dst.bpp))
				{
					dst.Free();
					return false;
				}

				Mipmap* dstM = &dstF->mipmaps[k];

				if (!ConvertPixelData(srcM->data, srcM->dataSize, dstM->data, src.pal, src.format, dst.format))
				{
					dst.Free();
					return false;
				}
			}
		}

		return true;
	}
	else
	{
		return false;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL VerticalFlip(void* pix, UReg width, UReg height, UReg bpp, UReg stride)
{
	RAD_ASSERT(pix);
	RAD_ASSERT(height&&stride);

	U8* top = (U8*)pix;
	U8* bottom = top + ((height-1) * stride);

	UReg scanwidth = width * bpp;

	while (top < bottom)
	{
		// swap the scanlines.
		for (UReg ofs = 0; ofs < scanwidth; ofs++)
		{
			std::swap(top[ofs], bottom[ofs]);
		}

		top += stride;
		bottom -= stride;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL HorizontalFlip(void* pix, UReg width, UReg height, UReg bpp, UReg stride)
{
	RAD_ASSERT(pix);
	RAD_ASSERT(width&&bpp&&stride);

	U8* left = (U8*)pix;
	U8* right = left + ((width-1) * bpp);

	while (left < right)
	{
		U8* swapleft = left;
		U8* swapright = right;

		// swap the scanlines.
		for (UReg ofs = 0; ofs < height; ofs++)
		{
			for (UReg bppc = 0; bppc < bpp; bppc++)
			{
				std::swap(swapleft[bppc], swapright[bppc]);
			}

			swapleft += stride;
			swapright += stride;
		}

		left += bpp;
		right -= bpp;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////

Image::Image() : format(0), bpp(0), frameCount(0), frames(0), pal(0), autoFree(true)
{
	zone = &ZImageCodecT::Get();
}

Image::Image(Zone &_zone) : format(0), bpp(0), frameCount(0), frames(0), pal(0), autoFree(true)
{
	zone = &_zone;
}

Image::Image(const Image& i) : format(0), bpp(0), frameCount(0), frames(0), pal(0), autoFree(true)
{
	operator = (i);
}

Image::~Image()
{
	Free(autoFree);
}

void Image::VerticalFlip()
{
	if (frameCount&&frames)
	{
		for (UReg i = 0; i < frameCount; i++)
		{
			for (UReg k = 0; k < frames[i].mipCount; k++)
			{
				image_codec::VerticalFlip(frames[i].mipmaps[k].data,
					frames[i].mipmaps[k].width,
					frames[i].mipmaps[k].height, 
					bpp,
					frames[i].mipmaps[k].stride);
			}
		}
	}
}

void Image::HorizontalFlip()
{
	if (frameCount&&frames)
	{
		for (UReg i = 0; i < frameCount; i++)
		{
			for (UReg k = 0; k < frames[i].mipCount; k++)
			{
				image_codec::HorizontalFlip(frames[i].mipmaps[k].data,
					frames[i].mipmaps[k].width,
					frames[i].mipmaps[k].height, 
					bpp,
					frames[i].mipmaps[k].stride);
			}
		}
	}
}

Image& Image::operator = (const Image& im)
{
	Free(autoFree);
	zone       = im.zone;
	format     = im.format;
	bpp        = im.bpp;
	autoFree   = im.autoFree;

	if (AllocateFrames(im.frameCount))
	{
		for (UReg i = 0; i < im.frameCount; i++)
		{
			RAD_ASSERT(im.frames[i].mipCount > 0);
			AllocateMipmaps(i, im.frames[i].mipCount);

			for (UReg k = 0; k < im.frames[i].mipCount; k++)
			{
				frames[i].flags = im.frames[i].flags;

				if (im.frames[i].flags & SharedFrameFlagRef)
				{
					frames[i].mipmaps[k].data = im.frames[i].mipmaps[k].data;
					frames[i].mipmaps[k].dataSize = im.frames[i].mipmaps[k].dataSize;
					frames[i].mipmaps[k].width = im.frames[i].mipmaps[k].width;
					frames[i].mipmaps[k].height = im.frames[i].mipmaps[k].height;
					frames[i].mipmaps[k].stride = im.frames[i].mipmaps[k].stride;
				}
				else
				{
					if (!AllocateMipmap(i, k, im.frames[i].mipmaps[k].width, im.frames[i].mipmaps[k].height, im.frames[i].mipmaps[k].stride, im.frames[i].mipmaps[k].dataSize))
					{
						Free(true);
						return *this;
					}

					memcpy(frames[i].mipmaps[k].data, im.frames[i].mipmaps[k].data, im.frames[i].mipmaps[k].dataSize);
				}

				frames[i].flags = im.frames[i].flags;
			}
		}
	}

	if (IsPalettedFormat(format))
	{
		AddrSize size = PaletteSize(format);
		RAD_ASSERT(size > 0);
		pal = zone_malloc(*zone, size, 0);
		
		if (pal)
		{
			memcpy(pal, im.pal, size); // copy the palette.
		}
		else
		{
			Free(true);
		}
	}

	return *this;
}

void Image::Free(bool all)
{
	if (frameCount&&frames)
	{
		for (UReg i = 0; i < frameCount; i++)
		{
			if (frames[i].mipmaps)
			{
				for (UReg k = 0; k < frames[i].mipCount; k++)
				{
					if (frames[i].mipmaps[k].data && !(frames[i].flags&SharedFrameFlagRef) && all) 
					{
						zone_free(frames[i].mipmaps[k].data);
					}
				}

				zone_free(frames[i].mipmaps);
			}
		}

		zone_free(frames);
	}
	if (pal)
	{
		zone_free(pal);
	}
	
	frameCount = 0;
	frames = 0;
	pal = 0;
}

bool Image::AllocateFrames(UReg fc)
{
	Free();
	if (fc)
	{
		frames = (Frame*)zone_calloc(*zone, fc, sizeof(Frame), 0);
	}
	frameCount = fc;

	return frames != 0;
}

bool Image::AllocateMipmaps(UReg frameNum, UReg mipmapCount)
{
	RAD_ASSERT(frameNum < frameCount);
	RAD_ASSERT(mipmapCount > 0);
	Frame* f = &frames[frameNum];
	
	f->mipmaps = (Mipmap*)zone_calloc(*zone, mipmapCount, sizeof(Mipmap), 0);
	f->mipCount = mipmapCount;
	f->flags = 0;
	
	return f->mipmaps != 0;
}

bool Image::AllocateMipmap(UReg frameNum, UReg mipmapNum, UReg width, UReg height, UReg stride, AddrSize dataSize)
{
	RAD_ASSERT(frameNum < frameCount);
	Frame* f = &frames[frameNum];
	RAD_ASSERT(mipmapNum < f->mipCount);
	Mipmap* m = &f->mipmaps[mipmapNum];

	m->width = width;
	m->height = height;
	m->stride = stride;
	m->dataSize = dataSize;
	m->data = zone_malloc(*zone, dataSize, 0);
	
	return m->data != 0;
}

bool Image::AllocatePalette(UReg format)
{
	RAD_ASSERT(format);
	RAD_ASSERT(bpp);
	RAD_ASSERT(PaletteSize(format)!=0);

	pal = zone_malloc(*zone, PaletteSize(format), 0);
	return pal != 0;
}

void Image::Swap(Image &i)
{
	std::swap(format, i.format);
	std::swap(bpp, i.bpp);
	std::swap(frameCount, i.frameCount);
	std::swap(frames, i.frames);
	std::swap(pal, i.pal);
	std::swap(zone, i.zone);
	std::swap(autoFree, i.autoFree);
}

} // image_codec

