// Tga.cpp
// Tga Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Tga.h"
#include "../Endian.h"


namespace image_codec {
namespace tga {

//////////////////////////////////////////////////////////////////////////////////////////
// TGA Decode
//////////////////////////////////////////////////////////////////////////////////////////
// Some of the decode stuff is courtesy of BurgerLib.

static bool UnpackTGAByte(U8 *DestBuffer,const U8 *InputPtr,UReg Pixels, AddrSize buffLength)
{
	UReg Count;
	do {
		if (buffLength<1) 
			return false;
		Count = InputPtr[0];	/* Get the counter */
		if (Count&0x80) {		/* Packed? */
			if (buffLength<2) 
				return false;
			U8 Temp;
			Count = Count-0x7F;	/* Remove the high bit */
			Pixels = Pixels-Count;
			Temp = InputPtr[1];	/* Get the repeater */
			InputPtr+=2;
			buffLength -= 2;
			do {
				DestBuffer[0] = Temp;	/* Fill memory */
				++DestBuffer;
			} while (--Count);
		} else {
			++InputPtr;
			++Count;			/* +1 to the count */
			if (buffLength<Count) 
				return false;
			Pixels = Pixels-Count;	/* Adjust amount */
			buffLength -= Count;
			do {
				DestBuffer[0] = InputPtr[0];	/* Memcpy */
				++InputPtr;
				++DestBuffer;
			} while (--Count);		/* All done? */
		}
	} while (Pixels>0);	/* Is there still more? */

	return true;
}

static bool UnpackTGAShort(U8 *DestBuffer,const U8 *InputPtr,UReg Pixels, AddrSize buffLength) // 565
{
	UReg Count;
	U16 Temp;
	do {
		if (buffLength<1) 
			return false;
		Count = InputPtr[0];	/* Get the counter */
		if (Count&0x80) {		/* Packed? */
			if (buffLength<3) 
				return false;
			Count = Count-0x7F;	/* Remove the high bit and add 1 */
			Pixels = Pixels-Count;
			Temp = ((const U16 *)(InputPtr+1))[0];	/* Get the repeater */
			Temp = ((Temp&31)<<11) | (Temp&(63<<5)) | ((Temp>>11)&31);
			InputPtr = InputPtr+3;
			buffLength -= 3;
			do {
				((U16 *)DestBuffer)[0] = Temp;	/* Fill memory */
				DestBuffer = DestBuffer+2;
			} while (--Count);
		} else {
			++InputPtr;
			++Count;			/* +1 to the count */
			if (buffLength<Count) 
				return false;
			Pixels = Pixels-Count;	/* Adjust amount */
			buffLength -= Count;
			do {
				Temp = ((const U16 *)InputPtr)[0];	/* Memcpy */
				Temp = ((Temp&31)<<11) | (Temp&(63<<5)) | ((Temp>>11)&31);
				((U16 *)DestBuffer)[0] = Temp;	/* Memcpy */
				InputPtr = InputPtr+2;
				DestBuffer = DestBuffer+2;
			} while (--Count);		/* All done? */
		}
	} while (Pixels>0);	/* Is there still more? */

	return true;
}

static bool UnpackTGAByte3(U8 *DestBuffer,const U8 *InputPtr,UReg Pixels, AddrSize buffLength)
{
	UReg Count;
	do {
		if (buffLength<1) 
			return false;
		Count = *InputPtr++;	/* Get the counter */
		if (Count&0x80) {		/* Packed? */
			if (buffLength<3) 
				return false;
			U16 Temp;
			U8 Temp2;
			Count = Count-0x7F;	/* Remove the high bit */
			Pixels = Pixels-Count;
			Temp = endian::Swap(((const U16 *)(InputPtr+1))[0]);	/* Get G,R */
			Temp2 = InputPtr[0];		/* Blue */
			InputPtr = InputPtr+3;
			buffLength -= 3;
			do {
				((U16 *)DestBuffer)[0] = Temp;	/* R and G */
				DestBuffer[2] = Temp2;		/* Blue */
				DestBuffer = DestBuffer+3;
			} while (--Count);
		} else {
			++Count;			/* +1 to the count */
			if (buffLength<Count) 
				return false;
			Pixels = Pixels-Count;	/* Adjust amount */
			buffLength -= Count;
			do {
				DestBuffer[0] = InputPtr[2];	/* Red */
				DestBuffer[1] = InputPtr[1];	/* Green */
				DestBuffer[2] = InputPtr[0];	/* Blue */
				DestBuffer = DestBuffer+3;		/* Next index */
				InputPtr = InputPtr+3;
			} while (--Count);		/* All done? */
		}
	} while (Pixels>0);	/* Is there still more? */

	return true;
}

static bool UnpackTGALong(U8 *DestBuffer,const U8 *InputPtr,UReg Pixels, AddrSize buffLength)
{
	UReg Count;
	do {
		if (buffLength<1) return false;
		Count = *InputPtr++;	/* Get the counter */
		if (Count&0x80) {		/* Packed? */
			if (buffLength<4) return false;
			U32 Temp;
			Count = Count-0x7F;	/* Remove the high bit and add 1 */
			Pixels = Pixels-Count;
			((U8 *)&Temp)[0] = InputPtr[2];	/* Red */
			((U8 *)&Temp)[1] = InputPtr[1];	/* Green */
			((U8 *)&Temp)[2] = InputPtr[0];	/* Blue */
			((U8 *)&Temp)[3] = InputPtr[3];	/* Alpha */
			InputPtr = InputPtr+4;
			buffLength -= 4;
			do {
				((U32 *)DestBuffer)[0] = Temp;	/* Fill memory */
				DestBuffer = DestBuffer+4;
			} while (--Count);
		} else {
			++Count;				/* +1 to the count */
			if (buffLength<Count) return false;
			Pixels = Pixels-Count;	/* Adjust amount */
			buffLength -= Count;
			do {
				DestBuffer[0] = InputPtr[2];	/* Red */
				DestBuffer[1] = InputPtr[1];	/* Green */
				DestBuffer[2] = InputPtr[0];	/* Blue */
				DestBuffer[3] = InputPtr[3];	/* Alpha */
				InputPtr = InputPtr+4;
				DestBuffer = DestBuffer+4;
			} while (--Count);		/* All done? */
		}
	} while (Pixels>0);	/* Is there still more? */

	return true;
}

struct TGAHeader {		/* This is the header for a TGA file */
	U8 imageIdent;	/* Image identification field size in bytes */
	U8 colorMapType;	/* 1 for a color image */
	U8 imageType;		/* Data type present */
	U8 colorMapOrigin[2];	/* Index of the first color map entry */
	U8 colorMapLength[2];	/* Count of color map entries */
	U8 colorMapEntrySize;	/* Size (In bits) for each color entry */
	U16 XOrigin;		/* Lower left x origin of image */
	U16 YOrigin;		/* Lower left y origin of image */
	U16 width;		/* Width of the image in pixels */
	U16 height;		/* Height of the image in pixels */
	U8 bitDepth;		/* Bits per pixels for the image */
	U8 descriptor;	/* Image descriptor bits */
};

enum
{
	TGA_IMAGE_TYPE_NONE         = 0, /* no image data */
	TGA_IMAGE_TYPE_COLORMAP     = 1, /* uncompressed, color-mapped */
	TGA_IMAGE_TYPE_BGR          = 2, /* uncompressed, true-color */
	TGA_IMAGE_TYPE_MONO         = 3, /* uncompressed, black and white */
	TGA_IMAGE_TYPE_COLORMAP_RLE = 9, /* run-length, color-mapped */
	TGA_IMAGE_TYPE_BGR_RLE      = 10, /* run-length, true-color */
	TGA_IMAGE_TYPE_MONO_RLE     = 11 /* run-length, black and white */
};

static int tga_is_colormapped(const TGAHeader *tga)
{
    return (
        tga->imageType == TGA_IMAGE_TYPE_COLORMAP ||
        tga->imageType == TGA_IMAGE_TYPE_COLORMAP_RLE
    );
}

static int tga_is_rle(const TGAHeader *tga)
{
    return (
        tga->imageType == TGA_IMAGE_TYPE_COLORMAP_RLE  ||
        tga->imageType == TGA_IMAGE_TYPE_BGR_RLE       ||
        tga->imageType == TGA_IMAGE_TYPE_MONO_RLE
    );
}

static int tga_is_mono(const TGAHeader *tga)
{
    return (
        tga->imageType == TGA_IMAGE_TYPE_MONO      ||
        tga->imageType == TGA_IMAGE_TYPE_MONO_RLE
    );
}

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(sizeof(TGAHeader)==18);

	out.Free();
	
	TGAHeader header;

	if (buffLength < sizeof(TGAHeader)) 
		return false;

	memcpy(&header, buff, sizeof(TGAHeader));
	
	if (tga_is_colormapped(&header) && !header.colorMapType) 
		return false;
	if (!tga_is_colormapped(&header) && header.colorMapType) 
		return false;

	U16 colorMapOrigin = endian::SwapLittle(((U16*)header.colorMapOrigin)[0]);
	U16 colorMapLength = endian::SwapLittle(((U16*)header.colorMapLength)[0]);

	if (header.colorMapType)
	{
		if (colorMapOrigin) 
			return false; // must be zero.
		if (colorMapLength != 256) 
			return false; // only 256 colors supported.
		if (!((header.colorMapEntrySize==16)||(header.colorMapEntrySize==24)||(header.colorMapEntrySize==32))) 
			return false;
		if ((header.imageType != TGA_IMAGE_TYPE_COLORMAP) && (header.imageType != TGA_IMAGE_TYPE_COLORMAP_RLE)) 
			return false;
	}

	header.width  = endian::SwapLittle(header.width);
	header.height = endian::SwapLittle(header.height);

	if (header.width==0||header.height==0) return false;
	if (!((header.bitDepth==8)||(header.bitDepth==16)||(header.bitDepth==24)||(header.bitDepth==32))) 
		return false;
	
	bool cmap = false;

	// supported format?
	switch (header.imageType)
	{
	case TGA_IMAGE_TYPE_COLORMAP: // unpacked indexed (pal).
	case TGA_IMAGE_TYPE_COLORMAP_RLE: // RLE indexed (pal).
		if (header.bitDepth!=8) 
			return false;
		cmap = true;
	break;
	case TGA_IMAGE_TYPE_BGR: // unpacked true color.
	case TGA_IMAGE_TYPE_BGR_RLE: // RLE true color.
		if (header.bitDepth==8) 
			return false;
	break;
	case TGA_IMAGE_TYPE_MONO: // unpacked gray scale.
	case TGA_IMAGE_TYPE_MONO_RLE: // RLE gray scale.
		if (header.bitDepth!=8) 
			return false;
	break;
	default:
		return false;
	}

	if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0,1))) 
		return false;

	out.bpp = header.bitDepth / 8;
	
	if (cmap)
	{
		out.format = (header.colorMapEntrySize==15) ? Format_PAL8_RGB555 :
			(header.colorMapEntrySize==16) ? Format_PAL8_RGB565 : 
			(header.colorMapEntrySize==24) ? Format_PAL8_RGB888 : Format_PAL8_RGBA8888;
		RAD_ASSERT(out.bpp==1);
	}
	else
	{
		out.format = (out.bpp==1) ? Format_A8 : (out.bpp==2) ? Format_RGB555 : (out.bpp==3) ? Format_RGB888 : Format_RGBA8888;
	}

	out.frames[0].mipmaps[0].width = header.width;
	out.frames[0].mipmaps[0].height = header.height;
	out.frames[0].mipmaps[0].stride = header.width * out.bpp;

	return true;
}

RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(sizeof(TGAHeader)==18);

	out.Free();
	
	TGAHeader header;
	const U8* bytes = (const U8*)buff;

	if (buffLength < sizeof(TGAHeader)) 
		return false;

	memcpy(&header, bytes, sizeof(TGAHeader));
	bytes += sizeof(TGAHeader);
	buffLength -= sizeof(TGAHeader);
	
	if (tga_is_colormapped(&header) && !header.colorMapType) 
		return false;
	if (!tga_is_colormapped(&header) && header.colorMapType) 
		return false;

	U16 colorMapOrigin = endian::SwapLittle(((U16*)header.colorMapOrigin)[0]);
	U16 colorMapLength = endian::SwapLittle(((U16*)header.colorMapLength)[0]);

	if (header.colorMapType)
	{
		if (colorMapOrigin) 
			return false; // must be zero.
		if (colorMapLength != 256) 
			return false; // only 256 colors supported.
		if (!((header.colorMapEntrySize==16)||(header.colorMapEntrySize==24)||(header.colorMapEntrySize==32))) 
			return false;
		if ((header.imageType != TGA_IMAGE_TYPE_COLORMAP) && (header.imageType != TGA_IMAGE_TYPE_COLORMAP_RLE)) 
			return false;
	}

	header.width  = endian::SwapLittle(header.width);
	header.height = endian::SwapLittle(header.height);

	if (header.width==0||header.height==0) return false;
	if (!((header.bitDepth==8)||(header.bitDepth==16)||(header.bitDepth==24)||(header.bitDepth==32))) 
		return false;
	
	bool cmap = false;
	UReg cmapbpp, cmaplen;

	// supported format?
	switch (header.imageType)
	{
	case TGA_IMAGE_TYPE_COLORMAP: // unpacked indexed (pal).
	case TGA_IMAGE_TYPE_COLORMAP_RLE: // RLE indexed (pal).
		if (header.bitDepth!=8) 
			return false;
		cmap = true;
	break;
	case TGA_IMAGE_TYPE_BGR: // unpacked true color.
	case TGA_IMAGE_TYPE_BGR_RLE: // RLE true color.
		if (header.bitDepth==8) 
			return false;
	break;
	case TGA_IMAGE_TYPE_MONO: // unpacked gray scale.
	case TGA_IMAGE_TYPE_MONO_RLE: // RLE gray scale.
		if (header.bitDepth!=8) 
			return false;
	break;
	default:
		return false;
	}

	if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0,1))) 
		return false;
	
	out.bpp = header.bitDepth / 8;

	if (cmap)
	{
		out.format = (header.colorMapEntrySize==15) ? Format_PAL8_RGB555 :
			(header.colorMapEntrySize==16) ? Format_PAL8_RGB565 : 
			(header.colorMapEntrySize==24) ? Format_PAL8_RGB888 : Format_PAL8_RGBA8888;
		RAD_ASSERT(out.bpp==1);
		cmapbpp = PaletteBPP(out.format);
		cmaplen = PaletteSize(out.format);
	}
	else
	{
		out.format = (out.bpp==1) ? Format_A8 : (out.bpp==2) ? Format_RGB555 : (out.bpp==3) ? Format_RGB888 : Format_RGBA8888;
	}

	if (!out.AllocateMipmap(0, 0, header.width, header.height, header.width * out.bpp, header.height * header.width * out.bpp))
	{
		out.Free();
		return false;
	}

	bytes += header.imageIdent; // skip ident.

	// cmap.

	if (cmap)
	{
		if (!out.AllocatePalette(out.format))
		{
			out.Free();
			return false;
		}
		memcpy(out.pal, bytes, cmaplen);

		U8* palB = (U8*)out.pal;

		RAD_ASSERT((cmapbpp==2)||(cmapbpp==3)||(cmapbpp==4));

		// swap.
		switch (cmapbpp)
		{
		case 2:
			{
				for (U16 i = 0; i < colorMapLength; i++) // BGR -> RGB
				{
					U16 Temp = ((U16*)(&palB[i*2]))[0];
                    ((U16*)(&palB[i*2]))[0] = ((Temp&31)<<11) | (Temp&(63<<5)) | ((Temp>>11)&31);
				}
			}
		break;
		case 3:
		case 4: // alpha is not swapped
			{
				for (UReg i = 0; i < cmaplen; i+= cmapbpp) // BGR -> RGB
				{
					std::swap(palB[i+0], palB[i+2]);
				}
			}
		break;
		}

		bytes += cmaplen;
	}

	// decode.

	UReg length = header.width * header.height;
	U8* dst = (U8*)out.frames[0].mipmaps[0].data;

	bool s = true;

	// the unpack complexity arises because we swap BGR to RGB on decode (instead of doing a second pass after RLE decompression).
	if (header.imageType == TGA_IMAGE_TYPE_BGR || header.imageType == TGA_IMAGE_TYPE_BGR_RLE)
	{
		// 16 bit?
		if (out.bpp == 2)
		{
			// unpacked?
			if (header.imageType == TGA_IMAGE_TYPE_BGR)
			{
				if (buffLength >= length*2)
				{
					while(length-- > 0)
					{
						U16 t = ((const U16*)bytes)[0];
                        ((U16*)dst)[0] = ((t&31)<<11) | (t&(63<<5)) | ((t>>11)&31);
						dst += 2;
						bytes += 2;
					}
				}
				else
				{
					s = false;
				}
			}
			else
			{
				s = UnpackTGAShort(dst, bytes, length, buffLength);
			}
		}
		else if (out.bpp == 3)
		{
			// unpacked?
			if (header.imageType == TGA_IMAGE_TYPE_BGR)
			{
				if (buffLength >= length*3)
				{
					while(length-- > 0)
					{
						dst[0] = bytes[2];
						dst[1] = bytes[1];
						dst[2] = bytes[0];
						dst += 3;
						bytes += 3;
					}
				}
				else
				{
					s = false;
				}	
			}
			else
			{
				s = UnpackTGAByte3(dst, bytes, length, buffLength);
			}
		}
		else if (out.bpp == 4) // 4 bytes
		{
			// unpacked?
			if (header.imageType == TGA_IMAGE_TYPE_BGR)
			{
				if (buffLength >= length*4)
				{
					while(length-- > 0)
					{
						dst[0] = bytes[2];
						dst[1] = bytes[1];
						dst[2] = bytes[0];
						dst[3] = bytes[3];
						dst += 4;
						bytes += 4;
					}
				}
				else
				{
					s = false;
				}
			}
			else
			{
				s = UnpackTGALong(dst, bytes, length, buffLength);
			}
		}
		else
		{
			s = false;
		}
	}
	else if ((header.imageType == TGA_IMAGE_TYPE_MONO) || (header.imageType == TGA_IMAGE_TYPE_COLORMAP))
	{
		if (buffLength >= length)
		{
			memcpy(dst, bytes, length);
		}
		else
		{
			s = false;
		}
	}
	else if (header.imageType == TGA_IMAGE_TYPE_MONO_RLE || (header.imageType == TGA_IMAGE_TYPE_COLORMAP_RLE))
	{
		s = UnpackTGAByte(dst, bytes, length, buffLength);
	}
	else
	{
		s = false;
	}

	if (s)
	{
		//if ((header.descriptor & 16) == 0) // right to left
		//{
		//	HorizontalFlip(out.frames[0].mipmaps[0].data, 
		//		out.frames[0].mipmaps[0].width, 
		//		out.frames[0].mipmaps[0].height, 
		//		out.bpp,
		//		out.frames[0].mipmaps[0].stride);
		//}
		if ((header.descriptor & 32) == 0) // bottom to top
		{
			VerticalFlip(out.frames[0].mipmaps[0].data, 
				out.frames[0].mipmaps[0].width, 
				out.frames[0].mipmaps[0].height, 
				out.bpp,
				out.frames[0].mipmaps[0].stride);
		}
	}
	else
	{
		out.Free();
	}

	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////
// TGA Encode
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
			AddrSize growSize = std::max<AddrSize>(kKilo*4, size);
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
			m_buff = safe_zone_realloc(ZImageCodec, m_buff, m_ofs, 0);
			m_size = m_ofs;
		}
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// I swiped this code off the internet. Thanks to Emil Mikulic. I modified his code to 
// work in our system. Here's the source license.
//
/* ---------------------------------------------------------------------------
 * Truevision Targa Reader/Writer
 * $Id: targa.h,v 1.8 2004/10/09 09:30:26 emikulic Exp $
 *
 * Copyright (C) 2001-2003 Emil Mikulic.
 *
 * Source and binary redistribution of this code, with or without
 * changes, for free or for profit, is allowed as long as this copyright
 * notice is kept intact.  Modified versions have to be clearly marked
 * as modified.
 *
 * This code is provided without any warranty.  The copyright holder is
 * not liable for anything bad that might happen as a result of the
 * code.
 * -------------------------------------------------------------------------*/
//////////////////////////////////////////////////////////////////////////////////////////

typedef int packet_type;
#define RAW 0
#define RLE 1
#define BIT(x) (1<<(x))

static const char tga_id[] =
    "\0\0\0\0" /* extension area offset */
    "\0\0\0\0" /* developer directory offset */
    "TRUEVISION-XFILE.";

static const size_t tga_id_length = 26; /* tga_id + \0 */

static packet_type rle_packet_type(const U8 *row, const U16 pos, const U16 width, const U16 bpp);
static U8 rle_packet_len(const U8 *row, const U16 pos, const U16 width, const U16 bpp, const packet_type type);

/* ---------------------------------------------------------------------------
 * Write one row of an image to <fp> using RLE.  This is a helper function
 * called from tga_write_to_FILE().  It assumes that <src> has its header
 * fields set up correctly.
 */

#define PIXEL(ofs) ( row + (ofs)*bpp )
static bool tga_write_row_RLE(OUTBUFF &fp,
    const Image &src, const Mipmap &mip, const U8 *row)
{
    #define WRITE(src, size) \
        if (fp.Write(src, size) == false) return false

    U16 pos = 0;
    U16 bpp = src.bpp;

    while (pos < mip.width)
    {
        packet_type type = rle_packet_type(row, pos, mip.width, bpp);
        U8 len = rle_packet_len(row, pos, mip.width, bpp, type);
        U8 packet_header;

        packet_header = len - 1;
        if (type == RLE) packet_header |= BIT(7);

        WRITE(&packet_header, 1);
        if (type == RLE)
        {
            WRITE(PIXEL(pos), bpp);
        }
        else /* type == RAW */
        {
            WRITE(PIXEL(pos), bpp*len);
        }

        pos += len;
    }

    return true;
    #undef WRITE
}

/* ---------------------------------------------------------------------------
 * Determine whether the next packet should be RAW or RLE for maximum
 * efficiency.  This is a helper function called from rle_packet_len() and
 * tga_write_row_RLE().
 */
#define SAME(ofs1, ofs2) (memcmp(PIXEL(ofs1), PIXEL(ofs2), bpp) == 0)

static packet_type rle_packet_type(const U8 *row, const U16 pos, const U16 width, const U16 bpp)
{
    if (pos == width - 1) return RAW; /* one pixel */
    if (SAME(pos,pos+1)) /* dupe pixel */
    {
        if (bpp > 1) return RLE; /* inefficient for bpp=1 */

        /* three repeats makes the bpp=1 case efficient enough */
        if ((pos < width - 2) && SAME(pos+1,pos+2)) return RLE;
    }
    return RAW;
}

/* ---------------------------------------------------------------------------
 * Find the length of the current RLE packet.  This is a helper function
 * called from tga_write_row_RLE().
 */
static U8 rle_packet_len(const U8 *row, const U16 pos, const U16 width, const U16 bpp, const packet_type type)
{
    U8 len = 2;

    if (pos == width - 1) return 1;
    if (pos == width - 2) return 2;

    if (type == RLE)
    {
        while (pos + len < width)
        {
            if (SAME(pos, pos+len))
                len++;
            else
                return len;

            if (len == 128) return 128;
        }
    }
    else /* type == RAW */
    {
        while (pos + len < width)
        {
            if (rle_packet_type(row, pos+len, width, bpp) == RAW)
                len++;
            else
                return len;
            if (len == 128) return 128;
        }
    }
    return len; /* hit end of row (width) */
}
#undef SAME
#undef PIXEL

static void PrepForEncode(void* image, AddrSize len, int width, int height, int bpp, AddrSize stride)
{
	VerticalFlip(image, width, height, bpp, stride);

	// RGB to BGR
	if (bpp == 4)
	{
		U8* dst = (U8*)image;
		U8 t;
		for(AddrSize i = 0; i < len; i+=4)
		{
			// swap R&B
			t = dst[i+0];
			dst[i+0] = dst[i+2];
			dst[i+2] = t;
		}
	}
	else if (bpp == 3)
	{
		U8* dst = (U8*)image;
		U8 t;
		for(AddrSize i = 0; i < len; i+=3)
		{
			// swap R&B
			t = dst[i+0];
			dst[i+0] = dst[i+2];
			dst[i+2] = t;
		}
	}
	else if (bpp == 2)
	{
		U8* dst = (U8*)image;
		U16 t;
		for(AddrSize i = 0; i < len; i+=2)
		{
			// swap R&B
			t = ((U16*)(&dst[i]))[0]&endian::SwapLittle(0x7FFF);
			((U16*)(&dst[i]))[0] = ((t&31)<<10) | (t&(31<<5)) | ((t>>10)&31);
		}
	}
}

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, bool compressRLE, void *&outData, AddrSize &outSize)
{
	RAD_ASSERT(sizeof(TGAHeader)==18);
	RAD_ASSERT(mip.width&&mip.height&&mip.stride&&mip.data&&mip.dataSize);
	RAD_ASSERT(outData&&outSize);
	RAD_ASSERT(in.bpp==1||in.bpp==2||in.bpp==3||in.bpp==4);
		
	OUTBUFF outBuff;
	TGAHeader header;

	outSize = 0;

	memset(&header, 0, sizeof(TGAHeader));
	header.bitDepth = in.bpp * 8;
	header.width  = endian::SwapLittle(mip.width);
	header.height = endian::SwapLittle(mip.height);

	if (IsPalettedFormat(in.format))
	{
		if (in.format == Format_PAL8_RGB555) return false; // unsupported!

		header.colorMapType = 1;
		header.colorMapLength[1] = 1; // yay. (lowest bit of high byte == 256).
		header.colorMapEntrySize = PaletteBPP(in.format) * 8;
		header.imageType = TGA_IMAGE_TYPE_COLORMAP;
	}
	else
	{
		header.imageType = (in.bpp==1) ? TGA_IMAGE_TYPE_MONO : TGA_IMAGE_TYPE_BGR;
	}

	if (compressRLE) header.imageType |= 8;
	
	header.descriptor = 0;//(in.bpp==2) ? 1 : (in.bpp==4) ? 8 : 0;
	
	// copy the image so we can flip it and reverse the color channels.
	void* image_copy = zone_malloc(ZImageCodec, mip.dataSize, 0);
	if (!image_copy) return false;

	memcpy(image_copy, mip.data, mip.dataSize);
	PrepForEncode(image_copy, mip.dataSize, mip.width, mip.height, in.bpp, mip.stride);

	if (!outBuff.Write(&header, sizeof(TGAHeader)))
	{
		zone_free(image_copy);
		return false;	
	}

	if (IsPalettedFormat(in.format))
	{
		UReg cmaplen = PaletteSize(in.format);
		UReg cmapbpp = PaletteBPP(in.format);
		U8* palB = (U8*)safe_zone_malloc(ZImageCodec, cmaplen, 0);
		memcpy(palB, in.pal, cmaplen);

		RAD_ASSERT((cmapbpp==2)||(cmapbpp==3)||(cmapbpp==4));

		// swap.
		switch (cmapbpp)
		{
		case 2:
			{
				for (U16 i = 0; i < 256; i++) // BGR -> RGB
				{
					U16 Temp = ((U16*)(&palB[i*2]))[0];
                    ((U16*)(&palB[i*2]))[0] = ((Temp&31)<<11) | (Temp&(63<<5)) | ((Temp>>11)&31);
				}
			}
		break;
		case 3:
		case 4: // alpha is not swapped
			{
				UReg count = 256 * cmapbpp;
				for (UReg i = 0; i < count; i+= cmapbpp) // BGR -> RGB
				{
					std::swap(palB[i+0], palB[i+2]);
				}
			}
		break;
		}

		bool good = outBuff.Write(palB, cmaplen);
		zone_free(palB);
		if (!good)
		{
			return false;
		}
	}

	bool s = true;

	if (compressRLE)
	{
		for (int row = 0; row < mip.height; row++)
		{
			if (!tga_write_row_RLE(outBuff, in, mip, ((U8*)image_copy) + row * mip.stride))
			{
				s = false; break;
			}
		}
	}
	else
	{
		if (!outBuff.Write(image_copy, mip.dataSize))
		{
			s = false;
		}
	}

	zone_free(image_copy);

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

} // tga
} // image_codec
