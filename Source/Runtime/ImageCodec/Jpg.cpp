// Jpg.cpp
// Jpg Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Jpg.h"
#include <libjpeg/jpeglib.h>
#include <libjpeg/jerror.h>
#include <libjpeg/jversion.h>

#define MAKE_JERROR_TABLE


namespace image_codec {
namespace jpg {

//////////////////////////////////////////////////////////////////////////////////////////
// Decode Functions
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	const void* buffer;
	U8 eoibuf[4];
	AddrSize buffer_length;
} decode_locals_t;

static boolean my_jpeg_fill_input_buffer( j_decompress_ptr cinfo )
{
	decode_locals_t *locals = (decode_locals_t*)cinfo->client_data;
	
	locals->eoibuf[0] = 0xff;
	locals->eoibuf[1] = JPEG_EOI;
	cinfo->src->next_input_byte = locals->eoibuf;
	cinfo->src->bytes_in_buffer = 2;
	
	return 1;
}

static void my_jpeg_init_source( j_decompress_ptr cinfo )
{
	decode_locals_t *locals = (decode_locals_t*)cinfo->client_data;
	cinfo->src->next_input_byte = (JOCTET*)locals->buffer;
	cinfo->src->bytes_in_buffer = locals->buffer_length;
}

static void my_jpeg_skip_input_data( j_decompress_ptr cinfo, long num_bytes )
{
	if( num_bytes >= (int)cinfo->src->bytes_in_buffer )
		my_jpeg_fill_input_buffer( cinfo );
	else
	{
		cinfo->src->next_input_byte += num_bytes;
		cinfo->src->bytes_in_buffer -= num_bytes;
	}
}

static void my_jpeg_term_source( j_decompress_ptr cinfo )
{
}

//////////////////////////////////////////////////////////////////////////////////////////
// Encode Functions
//////////////////////////////////////////////////////////////////////////////////////////

typedef struct
{
	void* buffer;
	AddrSize buffer_length;
} encode_locals_t;

#define JPEG_BUFFER_GROW	(1024*32)

static void my_jpeg_init_destination( j_compress_ptr cinfo )
{
	encode_locals_t* locals = (encode_locals_t*)cinfo->client_data;
	locals->buffer = zone_malloc(ZImageCodec, JPEG_BUFFER_GROW, 0);
	locals->buffer_length = JPEG_BUFFER_GROW;
	cinfo->dest->next_output_byte = (JOCTET*)locals->buffer;
	cinfo->dest->free_in_buffer = (locals->buffer)?JPEG_BUFFER_GROW:0;
}


static U8 my_jpeg_empty_output_buffer( j_compress_ptr cinfo )
{
	//
	// flush the whole buffer.
	//
	encode_locals_t* locals = (encode_locals_t*)cinfo->client_data;

	// we're at the end of the buffer, so grow it, and keep going.
	locals->buffer = zone_realloc(ZImageCodec, locals->buffer, locals->buffer_length+JPEG_BUFFER_GROW, 0);

	if (locals->buffer)
	{
		cinfo->dest->next_output_byte = (JOCTET*)&(((U8*)locals->buffer)[locals->buffer_length]);
		cinfo->dest->free_in_buffer = JPEG_BUFFER_GROW;

		locals->buffer_length += JPEG_BUFFER_GROW;
	}
	else
	{
		cinfo->dest->next_output_byte = 0;
		cinfo->dest->free_in_buffer = 0;
	}
	
	return (cinfo->dest->free_in_buffer>0)?TRUE:FALSE;
}

static void my_jpeg_term_destination( j_compress_ptr cinfo )
{
	encode_locals_t* locals = (encode_locals_t*)cinfo->client_data;
	AddrSize left = (AddrSize)cinfo->dest->free_in_buffer;
	if( left > 0 )
	{
		locals->buffer = zone_realloc(ZImageCodec, locals->buffer, locals->buffer_length-left, 0);
		locals->buffer_length -= left;
	}
}

//////////////////////////////////////////////////////////////////////////////////////////
// Shared Functions
//////////////////////////////////////////////////////////////////////////////////////////

static void my_jpeg_discard_error (j_common_ptr cinfo)
{
}

static void my_jpeg_discard_emit_message (j_common_ptr cinfo, int msg_level)
{
}

static void my_jpeg_discard_format_message (j_common_ptr cinfo, char * buffer)
{
}

static void my_jpeg_reset_error_mgr (j_common_ptr cinfo)
{
	jpeg_error_mgr *ptr;
	ptr = cinfo->err;
	ptr->num_warnings = 0;
	/* trace_level is not reset since it is an application-supplied parameter */
	ptr->msg_code = 0;	/* may be useful as a flag for "no error" */
}

#if defined(MAKE_JERROR_TABLE)

#undef JMESSAGE
#define JMESSAGE(code, string) string,
static const char * const jpeg_std_message_table[] = {
#include <libjpeg/jerror.h>
  0
};

#endif

static void my_jpeg_fatal_error( j_common_ptr cinfo )
{
#if defined(MAKE_JERROR_TABLE)
	if( cinfo->err->msg_code < JMSG_LASTMSGCODE )
	{
		RAD_FAIL("JPEG6B-LIB FATAL ERROR!");
	}
	else
#endif
	{
		RAD_FAIL("JPEG6B-LIB FATAL ERROR!");
	}
}

static void my_jpeg_std_error(struct jpeg_error_mgr * err)
{
	err->error_exit = my_jpeg_fatal_error;
	err->emit_message = my_jpeg_discard_emit_message;
	err->output_message = my_jpeg_discard_error;
	err->format_message = my_jpeg_discard_format_message;
	err->reset_error_mgr = my_jpeg_reset_error_mgr;

	err->trace_level = 0;		/* default = no tracing */
	err->num_warnings = 0;	/* no warnings emitted yet */
	err->msg_code = 0;		/* may be useful as a flag for "no error" */

	/* Initialize message table pointers */
	err->jpeg_message_table = jpeg_std_message_table;
	err->last_jpeg_message = (int) JMSG_LASTMSGCODE - 1;

	err->addon_message_table = 0;
	err->first_addon_message = 0;	/* for safety */
	err->last_addon_message = 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// JPEG Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(buff);
	RAD_ASSERT(buffLength);

	out.Free();
	
	jpeg_source_mgr jsrc = {
	0,		// next_input_byte
	0,		// bytes_in_buffer
	my_jpeg_init_source,
	my_jpeg_fill_input_buffer,
	my_jpeg_skip_input_data,
	jpeg_resync_to_restart,	// use the default
	my_jpeg_term_source
	};

	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	decode_locals_t jlocals;

	memset(&cinfo, 0, sizeof(jpeg_decompress_struct));
	memset(&jerr, 0,  sizeof(jpeg_error_mgr));
	memset(&jlocals, 0, sizeof(decode_locals_t));

	my_jpeg_std_error(&jerr);
	cinfo.err = &jerr;

	jpeg_create_decompress(&cinfo);

	cinfo.src = &jsrc;
	cinfo.client_data = &jlocals;

	jlocals.buffer = buff;
	jlocals.buffer_length = buffLength;

	bool s = jpeg_read_header(&cinfo, 1) == JPEG_HEADER_OK;
	
	if (s)
	{
		if (out.AllocateFrames(1)&&out.AllocateMipmaps(0, 1))
		{
			switch (cinfo.num_components)
			{
			case 1:
				out.bpp = 1;
				out.format = Format_A8;
				
			break;
			case 3:
				out.bpp = 3;
				out.format = Format_RGB888;
			break;

			default:
				s = false;
			}

			if (s)
			{
				out.frames[0].mipmaps[0].stride = cinfo.image_width * out.bpp;
				out.frames[0].mipmaps[0].width  = cinfo.image_width;
				out.frames[0].mipmaps[0].height = cinfo.image_height;
			}
		}
		else
		{
			s = false;
		}
	}

	if (!s) out.Free();

	jpeg_destroy_decompress(&cinfo);

	return s;
}

RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, bool highQualityUpsampling, bool interBlockSmoothing, Image &out)
{
	RAD_ASSERT(buff);
	RAD_ASSERT(buffLength);

	out.Free();

	jpeg_source_mgr jsrc = {
	0,		// next_input_byte
	0,		// bytes_in_buffer
	my_jpeg_init_source,
	my_jpeg_fill_input_buffer,
	my_jpeg_skip_input_data,
	jpeg_resync_to_restart,	// use the default
	my_jpeg_term_source
	};

	jpeg_decompress_struct cinfo;
	jpeg_error_mgr jerr;
	decode_locals_t jlocals;

	memset(&cinfo, 0, sizeof(jpeg_decompress_struct));
	memset(&jerr, 0,  sizeof(jpeg_error_mgr));
	memset(&jlocals, 0, sizeof(decode_locals_t));

	my_jpeg_std_error(&jerr);
	cinfo.err = &jerr;

	jpeg_create_decompress(&cinfo);

	cinfo.src = &jsrc;
	cinfo.client_data = &jlocals;

	jlocals.buffer = buff;
	jlocals.buffer_length = buffLength;

	if (jpeg_read_header(&cinfo, 1) != JPEG_HEADER_OK)
	{
		jpeg_destroy_decompress(&cinfo);
		return false;
	}
	
	if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0, 1)))
	{
		jpeg_destroy_decompress(&cinfo);
		return false;
	}
	

	switch (cinfo.num_components)
	{
	case 1:
		out.bpp = 1;
		out.format = Format_A8;
	break;
	case 3:
		out.bpp = 3;
		out.format = Format_RGB888;
	break;

	default:
		jpeg_destroy_decompress(&cinfo);
		return false;
	}
	
	// allocate scanline data.
	if (!out.AllocateMipmap(0, 0, cinfo.image_width, cinfo.image_height, cinfo.image_width*out.bpp, cinfo.image_height*cinfo.image_width*out.bpp))
	{
		out.Free();
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	U8* bytes = (U8*)out.frames[0].mipmaps[0].data;

	JSAMPROW* scanlines = (JSAMPROW*)zone_malloc(ZImageCodec, sizeof(JSAMPROW)*cinfo.image_height, 0);
	if (!scanlines)
	{
		out.Free();
		jpeg_destroy_decompress(&cinfo);
		return false;
	}

	{
		JSAMPROW* row = scanlines;
		U8* cur = bytes;

		for(JDIMENSION i = 0; i < cinfo.image_height; i++)
		{
			*(row++) = cur;
			cur += out.frames[0].mipmaps[0].stride;
		}
	}

	cinfo.dct_method = JDCT_FLOAT;
	cinfo.do_fancy_upsampling = (highQualityUpsampling)?TRUE:FALSE;
	cinfo.do_block_smoothing  = (interBlockSmoothing)?TRUE:FALSE;

	jpeg_start_decompress(&cinfo);
	while (cinfo.output_scanline < cinfo.image_height)
	{
		if (jpeg_read_scanlines(&cinfo, &scanlines[cinfo.output_scanline], cinfo.image_height - cinfo.output_scanline)<1)
		{
			out.Free();
			zone_free(scanlines);
			jpeg_finish_decompress(&cinfo);
			jpeg_destroy_decompress(&cinfo);
			return false;
		}
	}

	zone_free(scanlines);
	jpeg_finish_decompress(&cinfo);
	jpeg_destroy_decompress(&cinfo);

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// JPEG Encode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, float quality_ZeroToOne, void *&outData, AddrSize &outSize)
{
	RAD_ASSERT((in.format==Format_A8&&in.bpp==1)||(in.format==Format_RGB888&&in.bpp==3));
	RAD_ASSERT(mip.width&&mip.height);
	RAD_ASSERT(mip.stride);
	RAD_ASSERT(mip.data&&mip.dataSize);
	RAD_ASSERT(quality_ZeroToOne>=0.0f&&quality_ZeroToOne<=1.0f);

	struct jpeg_compress_struct cinfo;
	struct jpeg_error_mgr jerr;
	struct jpeg_destination_mgr jdst = {
		0,		// next_output_byte
		0,		// free_in_buffer
		my_jpeg_init_destination,
		my_jpeg_empty_output_buffer,
		my_jpeg_term_destination
	};
	encode_locals_t locals;

	JSAMPROW* scanlines = (JSAMPROW*)zone_malloc(ZImageCodec, sizeof(JSAMPROW)*mip.height, 0);
	if (scanlines)
	{
		const U8* row = (const U8*)mip.data;
		for(UReg y = 0; y < mip.height; y++)
		{
			scanlines[y] = ((const JSAMPROW)row) + y * mip.stride;
		}
	}
	else
	{
		return false;
	}

	my_jpeg_std_error(&jerr);
	cinfo.err = &jerr;
	jpeg_create_compress(&cinfo);

	memset(&locals, 0, sizeof(encode_locals_t));

	cinfo.dest = &jdst;
	cinfo.client_data = &locals;

	cinfo.image_width  = (U32)mip.width;
	cinfo.image_height = (U32)mip.height;
	cinfo.input_components = (U32)in.bpp;
	cinfo.in_color_space = (in.format==Format_RGB888)?JCS_RGB:JCS_GRAYSCALE;
	jpeg_set_defaults(&cinfo);

	cinfo.dct_method = JDCT_FLOAT;
	cinfo.optimize_coding = TRUE;

	jpeg_set_quality(&cinfo, (int)(quality_ZeroToOne*100.0f), true);
	jpeg_start_compress(&cinfo, true);

	while (cinfo.next_scanline < cinfo.image_height)
	{
		if (jpeg_write_scanlines(&cinfo, scanlines + cinfo.next_scanline, cinfo.image_height - cinfo.next_scanline) < 1)
		{
			jpeg_finish_compress(&cinfo);
			jpeg_destroy_compress(&cinfo);
			if (locals.buffer) zone_free(locals.buffer);
			zone_free(scanlines);
			return false;
		}
	}

	zone_free(scanlines);

	jpeg_finish_compress(&cinfo);
	jpeg_destroy_compress(&cinfo);

	if (locals.buffer&&locals.buffer_length)
	{
		outData = locals.buffer;
		outSize = locals.buffer_length;
	}
	else
	{
		if (locals.buffer)
		{
			zone_free(locals.buffer);
		}
	}

	return (locals.buffer&&locals.buffer_length);
}

} // jpg
} // image_codec
