// Png.cpp
// Png Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Png.h"

#include "../Stream/MemoryStream.h"
#include "../PushSystemMacros.h"

#undef min
#undef max

#include "lpng1216/png.h"

#include <setjmp.h>


namespace image_codec {
namespace png {

#ifndef png_jmpbuf
#  define png_jmpbuf(png_ptr) ((png_ptr)->jmpbuf)
#endif

namespace
{
	const int SigSize = 8;
	RAD_DECLARE_EXCEPTION(PNGException, exception);

	void PNGAPI PNGErrorHandler(png_structp, png_const_charp msg)
	{
		throw PNGException(msg);
	}

	void PNGAPI PNGWarningHandler(png_structp, png_const_charp)
	{
	}

	png_voidp PNGMalloc(png_structp png, png_size_t size)
	{
		RAD_ASSERT(png);
		RAD_ASSERT(size);
		return safe_zone_malloc(ZImageCodec, size, 0);
	}

	void PNGFree(png_structp png, png_voidp ptr)
	{
		if (ptr)
		{
			zone_free(ptr);
		}
	}

	void PNGAPI PNGRead(png_structp png, png_bytep in, png_size_t size)
	{
		RAD_ASSERT(png);
		RAD_ASSERT(png->io_ptr);

		if (size)
		{
			RAD_ASSERT(size <= std::numeric_limits<stream::SPos>::max());
			stream::InputStream *s = reinterpret_cast<stream::InputStream*>(png->io_ptr);
			if (s->Read(in, (stream::SPos)size, 0) != size)
			{
				png_error(png, "PNGRead: buffer underflow");
			}
		}
	}

	void PNGAPI PNGWrite(png_structp png, png_bytep out, png_size_t size)
	{
		RAD_ASSERT(png);
		RAD_ASSERT(png->io_ptr);
		RAD_ASSERT(out);

		if (size)
		{
			RAD_ASSERT(size <= std::numeric_limits<stream::SPos>::max());
			stream::OutputStream *s = reinterpret_cast<stream::OutputStream*>(png->io_ptr);
			if (s->Write(out, (stream::SPos)size, 0) != size)
			{
				png_error(png, "PNGWrite: buffer write error");
			}
		}
	}

	void PNGAPI PNGFlush(png_structp)
	{
	}

	UReg Format(png_structp png, png_infop info, int bits, int color)
	{
		if (png_get_valid(png, info, PNG_INFO_tRNS)) return Format_RGBA8888;

		switch (color)
		{
		case PNG_COLOR_TYPE_GRAY:
			return Format_A8;
			break;
		case PNG_COLOR_TYPE_PALETTE: // decoder expands paletted to RGB or RGBA
			return Format_RGB888;
			break;
		case PNG_COLOR_TYPE_RGB:
			return Format_RGB888;
			break;
		case PNG_COLOR_TYPE_RGBA:
			return Format_RGBA8888;
			break;
		}

		return InvalidFormat;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// PNG Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(buff&&buffLength);
	RAD_ASSERT(buffLength <= std::numeric_limits<stream::SPos>::max());

	out.Free();
	
	stream::MemInputBuffer ib(buff, (stream::SPos)buffLength);
	stream::InputStream is(ib);

	{
		char sig[SigSize];
		if (is.Read(sig, SigSize, 0) != SigSize) return false;
		// NOTE: png_sig_cmp() returns 0 if the sig matches the PNG sig.
		if (png_sig_cmp((png_bytep)sig, 0, SigSize)) return false;
	}

	png_structp png;
	png_infop   info;
	
	png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 0, PNGErrorHandler, PNGWarningHandler, 0, PNGMalloc, PNGFree);
	if (!png) return false;

	info = png_create_info_struct(png);
	if (!info) 
	{
		png_destroy_read_struct(&png, 0, 0);
		return false;
	}

	png_set_read_fn(png, &is, PNGRead);	
	png_set_sig_bytes(png, SigSize);
	
	try
	{
		png_uint_32 w, h;
		int bd, color, interlace;

		png_read_info(png, info);
		png_get_IHDR(png, info, &w, &h, &bd, &color, &interlace, 0, 0);
		if (!out.AllocateFrames(1)||!out.AllocateMipmaps(0, 1)) return false;
		
		Mipmap &m = out.frames[0].mipmaps[0];

		out.format = Format(png, info, bd, color);

		if (out.format == InvalidFormat)
		{
			out.Free();
			png_destroy_read_struct(&png, &info, 0);
			return false;
		}

		out.bpp = FormatBPP(out.format);

		m.width = w;
		m.height = h;
		m.stride = w * out.bpp;
	}
	catch (PNGException&)
	{
		out.Free();
		png_destroy_read_struct(&png, &info, 0);
		return false;
	}

	png_destroy_read_struct(&png, &info, 0);
	return true;
}

RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(buff&&buffLength);
	RAD_ASSERT(buffLength <= std::numeric_limits<stream::SPos>::max());

	out.Free();

	stream::MemInputBuffer ib(buff, (stream::SPos)buffLength);
	stream::InputStream is(ib);

	{
		char sig[SigSize];
		if (is.Read(sig, SigSize, 0) != SigSize)
			return false;
		// NOTE: png_sig_cmp() returns 0 if the sig matches the PNG sig.
		if (png_sig_cmp((png_bytep)sig, 0, SigSize))
			return false;
	}

	png_structp png;
	png_infop   info;
	
	png = png_create_read_struct_2(PNG_LIBPNG_VER_STRING, 0, PNGErrorHandler, PNGWarningHandler, 0, PNGMalloc, PNGFree);
	if (!png)
		return false;

	info = png_create_info_struct(png);
	if (!info) 
	{
		png_destroy_read_struct(&png, 0, 0);
		return false;
	}
	
	png_set_read_fn(png, &is, PNGRead);	
	png_set_sig_bytes(png, SigSize);
	
	try
	{
		png_uint_32 w, h;
		int bd, color, interlace;

		png_read_info(png, info);
		png_get_IHDR(png, info, &w, &h, &bd, &color, &interlace, 0, 0);
		if (!out.AllocateFrames(1)||!out.AllocateMipmaps(0, 1)) 
		{
			png_destroy_read_struct(&png, &info, 0);
			return false;
		}
		
		Mipmap &m = out.frames[0].mipmaps[0];
				
		out.format = Format(png, info, bd, color);

		if (out.format == InvalidFormat)
		{
			out.Free();
			png_destroy_read_struct(&png, &info, 0);
			return false;
		}

		out.bpp = FormatBPP(out.format);

		if (!out.AllocateMipmap(0, 0, w, h, w * out.bpp, w * h * out.bpp))
		{
			out.Free();
			png_destroy_read_struct(&png, &info, 0);
			return false;
		}

		png_set_strip_16(png);

		if (color == PNG_COLOR_TYPE_PALETTE)
		{
			png_set_palette_to_rgb(png);
		}

		if (color == PNG_COLOR_TYPE_GRAY && bd < 8)
		{
			png_set_gray_1_2_4_to_8(png);
		}

		if (png_get_valid(png, info, PNG_INFO_tRNS))
		{
			png_set_tRNS_to_alpha(png);
		}

		png_set_swap(png);
		int passes = png_set_interlace_handling(png);

		png_bytep dstRow = 0;
		png_bytep imgRow = 0;
		png_uint_32 stride = png_get_rowbytes(png, info);

		RAD_ASSERT(stride >= m.stride);
		if (stride != m.stride) 
		{
			dstRow = (png_bytep)safe_zone_malloc(ZImageCodec, stride, 0);
		}
		
		for (int pass = 0; pass < passes; ++pass)
		{
			if (stride == m.stride)
			{
				dstRow = (png_bytep)m.data; // reset.
			}

			imgRow = (png_bytep)m.data;

			for (png_uint_32 y = 0; y < h; ++y)
			{
				if (pass > 0 && stride != m.stride)
				{
					RAD_ASSERT(imgRow != dstRow);
					// interlaced image, copy the result of the last pass.
					memcpy(dstRow, imgRow, m.stride);
				}

				png_read_rows(png, &dstRow, 0, 1);

				if (stride != m.stride)
				{
					// imgRow/dstRow are correct, we are copying from dstRow to imgRow.
					memcpy(imgRow, dstRow, m.stride);
				}
				else
				{
					dstRow += m.stride;
				}

				imgRow += m.stride;
			}
		}

		if (stride != m.stride) 
		{
			RAD_ASSERT(dstRow);
			zone_free(dstRow);
		}

		png_read_end(png, info);
	}
	catch (PNGException&)
	{
		out.Free();
		png_destroy_read_struct(&png, &info, 0);
		return false;
	}

	png_destroy_read_struct(&png, &info, 0);
	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// PNG Encode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, bool interlaced, void *&outData, AddrSize &outSize)
{
	RAD_ASSERT(in.format == Format_A8 || in.format == Format_RGB888 || in.format == Format_RGBA8888);

	if (in.format != Format_A8 &&
		in.format != Format_RGB888 &&
		in.format != Format_RGBA8888) return false;

	png_structp png;
	png_infop   info;

	png = png_create_write_struct_2(PNG_LIBPNG_VER_STRING, 0, PNGErrorHandler, PNGWarningHandler, 0,
		PNGMalloc, PNGFree);

	if (!png) return false;

	info = png_create_info_struct(png);

	if (!info)
	{
		png_destroy_write_struct(&png, 0);
		return false;
	}

	stream::DynamicMemOutputBuffer ob(ZImageCodec);
	stream::OutputStream os(ob);

	png_set_write_fn(png, &os, PNGWrite, PNGFlush);
	png_set_IHDR(png, info, mip.width, mip.height, 8,
		(in.format == Format_A8) ? PNG_COLOR_TYPE_GRAY :
		(in.format == Format_RGB888) ? PNG_COLOR_TYPE_RGB : PNG_COLOR_TYPE_RGBA,
		interlaced ? PNG_INTERLACE_ADAM7 : PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);

	png_color_8 sig;
	memset(&sig, 0, sizeof(sig));
	
	if (in.format == Format_A8)
	{
		sig.gray = 8;
	}
	else
	{
		sig.red = 8;
		sig.green = 8;
		sig.blue = 8;
		sig.alpha = (in.format == Format_RGBA8888) ? 8 : 0;
	}

	png_set_sBIT(png, info, &sig);

	try
	{
		png_write_info(png, info);
	}
	catch (PNGException&)
	{
		png_destroy_write_struct(&png, &info);
		return false;
	}

	png_set_shift(png, &sig);

	int passes = 1;

	if (interlaced)
	{
		passes = png_set_interlace_handling(png);
	}

	for (int pass = 0; pass < passes; ++pass)
	{
		png_bytep src = (png_bytep)mip.data;

		for (UReg y = 0; y < mip.height; ++y)
		{
			png_write_rows(png, (png_bytepp)&src, 1);
			src += mip.stride;
		}
	}

	png_write_end(png, info);
	png_destroy_write_struct(&png, &info);

	outData = ob.OutputBuffer().Ptr();
	outSize = ob.OutputBuffer().Size();
	ob.OutputBuffer().Set(0, 0); // don't let the output buffer release the memory.

	return true;

}

} // png
} // image_codec
