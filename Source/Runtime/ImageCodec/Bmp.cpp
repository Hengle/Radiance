// Bmp.cpp
// Bmp Decode/Encode
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Bmp.h"
#include "../Endian.h"


namespace image_codec {
namespace bmp {

// these structs are defined as bytes in some places to fix compiler packing problems.
struct BitmapHeader
{
	U8 sig[2]; // == 'BM'.
	U8 fileSize[4];
	U8 reserved[4];
	U8 dataOfs[4];
};

enum
{
	BM_TYPE_MONOCROME_PAL = 1,
	BM_TYPE_4BIT_PAL      = 4,
	BM_TYPE_8BIT_PAL      = 8,
	BM_TYPE_16BIT         = 16,
	BM_TYPE_24BIT         = 24,

	BM_COMP_RGB = 0,
	BM_COMP_RLE8,
	BM_COMP_RLE4
};

struct BitmapInfo
{
	U32 size;
	U32 width;
	U32 height;
	U16 planes;
	U16 bitCount;
	U32 compression;
	U32 imageSize; // compressed size.
	U32 xpixpm;
	U32 ypixpm;
	U32 usedColors;
	U32 importantColors;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Helpers
//////////////////////////////////////////////////////////////////////////////////////////

static void SwapBitmapInfo(BitmapInfo* bmi)
{
	bmi->size = endian::SwapLittle(bmi->size);
	bmi->width = endian::SwapLittle(bmi->width);
	bmi->height = endian::SwapLittle(bmi->height);
	bmi->planes = endian::SwapLittle(bmi->planes);
	bmi->bitCount = endian::SwapLittle(bmi->bitCount);
	bmi->compression = endian::SwapLittle(bmi->compression);
	bmi->imageSize = endian::SwapLittle(bmi->imageSize);
	bmi->xpixpm = endian::SwapLittle(bmi->xpixpm);
	bmi->ypixpm = endian::SwapLittle(bmi->ypixpm);
	bmi->usedColors      = endian::SwapLittle(bmi->usedColors);
	bmi->importantColors = endian::SwapLittle(bmi->importantColors);
}

static bool BmpDecode8(const U8* bytes, const BitmapInfo* bmi, AddrSize buffLength, U8* out)
{
	UReg srcStride = (bmi->width + 3) & ~3;
	if (buffLength < (srcStride * bmi->height))
		return false;

	srcStride -= bmi->width;

	// read each scanline.
	for (U32 y = 0; y < bmi->height; y++)
	{
		for (U32 x = 0; x < bmi->width; x++) // what the hell is it with formats that swap red/blue?
		{
			*(out++) = *(bytes++);
		}

		bytes += srcStride;
	}

	return true;
}

static bool BmpDecode8RLE(const U8* bytes, const BitmapInfo* bmi, AddrSize buffLength, U8* out)
{
	U8* base = out;

	UReg x = 0;
	for (UReg y = 0; y < bmi->height;)
	{
		U8 vals[2];
				
		if (buffLength < 2) 
			return false;

		vals[0] = bytes[0];
		vals[1] = bytes[1];

		bytes += 2;
		buffLength -= 2;
		
		if (vals[0] > 0)
		{
			// encodes repeat.
			while (vals[0]-- > 0)
			{
				*out++ = vals[1];
			}
		}
		else
		{
			// escape code.
			switch (vals[1])
			{
			case 0: // end of line
				y++;
				x = 0;
				out = base + (y*bmi->width); // next scanline.
			break;
			case 1: // end of bitmap.
				return true;
			break;
			case 2: // delta skip
				if (buffLength < 2) 
					return false;
                x += bytes[0];
				y += bytes[1];
				bytes += 2;
				buffLength -= 2;
				{
					U8* ptr = base + (y*bmi->width) + x;
					memset(out, 0, ptr-out);
					out = ptr;
				}
			break;
			default: // copy literal bytes.
				bool odd = vals[1] & 1;

				UReg count = vals[1];
				if (odd) count++;
				if (buffLength < count) 
					return false;
				buffLength -= count;

				RAD_ASSERT(vals[1]>=3);
				while(vals[1]-- > 0)
				{
					*out++ = *bytes++;
				}
				if (odd) bytes++; // skip zero byte.
			break;
			}
		}
	}

	return true;
}

static bool BmpDecode24(const U8* bytes, const BitmapInfo* bmi, AddrSize buffLength, U8* out)
{
	UReg srcStride = (bmi->width * 3 + 3) & ~3;
	if (buffLength < (srcStride * bmi->height)) 
		return false;

	srcStride -= bmi->width * 3;

	// read each scanline.
	for (U32 y = 0; y < bmi->height; y++)
	{
		for (U32 x = 0; x < bmi->width; x++) // what the hell is it with formats that swap red/blue?
		{
			out[0] = bytes[2];
			out[1] = bytes[1];
			out[2] = bytes[0];

			out += 3;
			bytes += 3;
		}

		bytes += srcStride;
	}

	return true;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BMP Decode
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL DecodeHeader(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(sizeof(BitmapHeader) == 14);
	RAD_ASSERT(sizeof(BitmapInfo) == 40);

	out.Free();
	
	if (buffLength < 54) 
		return false;

	const U8* bytes = (const U8*)buff;

	BitmapHeader bmh;
	BitmapInfo bmi;

	bmh = *((BitmapHeader*)bytes);
	bmi = *((BitmapInfo*)(bytes+14));

	if (bmh.sig[0] != 'B' || bmh.sig[1] != 'M') 
		return false;

	SwapBitmapInfo(&bmi);

	if (bmi.size != 40) 
		return false;
	if (bmi.planes != 1) 
		return false;
	if (bmi.bitCount != BM_TYPE_8BIT_PAL &&
		//bmi.bitCount != BM_TYPE_16BIT && // <-- unsupported!
		bmi.bitCount != BM_TYPE_24BIT) 
		return false;
	
	// not uncompressed, or it's compressed but has the wrong src format.
	if ((bmi.compression != BM_COMP_RLE8 || bmi.bitCount != BM_TYPE_8BIT_PAL) && (bmi.compression != BM_COMP_RGB)) return false;
	if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0,1))) 
		return false;

	out.bpp = bmi.bitCount / 8;
	RAD_ASSERT(out.bpp > 0);

	out.format = (out.bpp==1) ? Format_PAL8_RGB888 : (out.bpp==2) ? Format_RGB565 : Format_RGB888;
	out.frames[0].mipmaps[0].width = (UReg)bmi.width;
	out.frames[0].mipmaps[0].height = (UReg)bmi.height;
	out.frames[0].mipmaps[0].stride = (UReg)(bmi.width * out.bpp);

	return true;
}

RADRT_API bool RADRT_CALL Decode(const void *buff, AddrSize buffLength, Image &out)
{
	RAD_ASSERT(sizeof(BitmapHeader) == 14);
	RAD_ASSERT(sizeof(BitmapInfo) == 40);

	out.Free();
	
	if (buffLength < 54) 
		return false;

	const U8* bytes = (const U8*)buff;

	BitmapHeader bmh;
	BitmapInfo bmi;

	bmh = *((BitmapHeader*)bytes);
	bmi = *((BitmapInfo*)(bytes+14));
	bytes += 54; // skip.

	AddrSize orgBuffLength = buffLength;
	buffLength -= 54;
	
	if (bmh.sig[0] != 'B' || bmh.sig[1] != 'M') 
		return false;

	SwapBitmapInfo(&bmi);

	if (bmi.size != 40) 
		return false;
	if (bmi.planes != 1) 
		return false;
	if (bmi.bitCount != BM_TYPE_8BIT_PAL &&
		//bmi.bitCount != BM_TYPE_16BIT && // <-- unsupported!
		bmi.bitCount != BM_TYPE_24BIT) 
		return false;
	
	// not uncompressed, or it's compressed but has the wrong src format.
	if ((bmi.compression == BM_COMP_RLE8 && bmi.bitCount != BM_TYPE_8BIT_PAL) && (bmi.compression != BM_COMP_RGB)) 
		return false;
	if (!(out.AllocateFrames(1)&&out.AllocateMipmaps(0,1))) 
		return false;

	out.bpp = bmi.bitCount / 8;
	RAD_ASSERT(out.bpp > 0);

	out.format = (out.bpp==1) ? Format_PAL8_RGB888 : (out.bpp==2) ? Format_RGB565 : Format_RGB888;
	
	if (!out.AllocateMipmap(0, 0, (UReg)bmi.width, (UReg)bmi.height, (UReg)bmi.width*out.bpp, (AddrSize)(bmi.width*bmi.height*out.bpp))) 
	{
		out.Free();
		return false;
	}
	
	if (out.bpp == 1) // there is a color table
	{
		RAD_ASSERT(PaletteSize(out.format) == 256*3);

		if (buffLength < 256*4)
		{
			out.Free();
			return false;
		}

		if (!out.AllocatePalette(out.format))
		{
			out.Free();
			return false;
		}
		U8* palB = (U8*)out.pal;

		// read the palette.
		for (UReg i = 0; i < 256; i++)
		{
			palB[0] = bytes[2];
			palB[1] = bytes[1];
			palB[2] = bytes[0];

			palB += 3;
			bytes += 4;
		}

	}

	{
		// load raster data offset.
		U32 rasterOfs = endian::SwapLittle(((const U32*)bmh.dataOfs)[0]);
		bytes = ((const U8*)buff) + rasterOfs;
		buffLength = orgBuffLength - rasterOfs;
	}

	bool s = true;

	if (bmi.bitCount == BM_TYPE_24BIT)
	{
		RAD_ASSERT(bmi.compression == BM_COMP_RGB);
		s = BmpDecode24(bytes, &bmi, buffLength, (U8*)out.frames[0].mipmaps[0].data);
	}
	else if (bmi.bitCount == BM_TYPE_8BIT_PAL)
	{
		if (bmi.compression == BM_COMP_RLE8)
		{
			s = BmpDecode8RLE(bytes, &bmi, buffLength, (U8*)out.frames[0].mipmaps[0].data);
		}
		else
		{
			RAD_ASSERT(bmi.compression == BM_COMP_RGB);
			s = BmpDecode8(bytes, &bmi, buffLength, (U8*)out.frames[0].mipmaps[0].data);
		}
	}
	else
	{
		s = false;
	}

	if (s) // what is it with formats storing things bottom up?
	{
		VerticalFlip(out.frames[0].mipmaps[0].data,
			out.frames[0].mipmaps[0].width,
			out.frames[0].mipmaps[0].height,
			out.bpp,
			out.frames[0].mipmaps[0].stride);
	}
	else
	{
		out.Free();
	}

	return s;
}

//////////////////////////////////////////////////////////////////////////////////////////
// BMP Encode
//////////////////////////////////////////////////////////////////////////////////////////

struct OUTBUFF
{
	void *m_buff;
	AddrSize m_ofs;
	AddrSize m_size;

	OUTBUFF() : m_buff(0), m_ofs(0), m_size(0)
	{
	}

	~OUTBUFF()
	{
		if (m_buff)
		{
			free(m_buff);
		}
	}

	bool Write(const void* src, AddrSize size)
	{
		if (m_ofs+size > m_size)
		{
			AddrSize growSize = std::max((AddrSize)(Kilo*4), size);
			// grow.
			void *newBuff = realloc(m_buff, m_size+growSize);
			if (!newBuff) 
				return false;
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
			m_buff = realloc(m_buff, m_ofs);
			RAD_ASSERT(m_buff);
			m_size = m_ofs;
		}
	}
};

static bool BmpEncode24(const Mipmap &mip, OUTBUFF &outBuff)
{
	// the only trick here is to DWORD align each scanline.
	UReg scanlineFix = ((mip.width * 3 + 3) & ~3) - (mip.width * 3);
	U8 scanpad[3] = {0, 0, 0};
	U8 pix[3];

	RAD_ASSERT(scanlineFix <= 3);

	for (UReg y = mip.height; y > 0; y--)
	{
		const U8* src = ((const U8*)mip.data) + ((y-1)*mip.width*3);
		for (UReg x = 0; x < mip.width; x++)
		{
			pix[0] = src[2];
			pix[1] = src[1];
			pix[2] = src[0];
			if (!outBuff.Write(pix, 3)) 
				return false;
			src += 3;
		}

		if (scanlineFix > 0)
		{
			if (!outBuff.Write(scanpad, scanlineFix))
				return false; // align.
		}
	}

	return true;
}

static bool BmpEncode8(const Mipmap &mip, OUTBUFF &outBuff)
{
	// the only trick here is to DWORD align each scanline.
	UReg scanlineFix = ((mip.width + 3) & ~3) - mip.width;
	U8 scanpad[3] = {0, 0, 0};

	RAD_ASSERT(scanlineFix <= 3);

	for (UReg y = mip.height; y > 0; y--)
	{
		const U8* src = ((const U8*)mip.data) + ((y-1)*mip.width);
		if (!outBuff.Write(src, mip.width)) return false;
		if (scanlineFix > 0)
		{
			if (!outBuff.Write(scanpad, scanlineFix)) 
				return false; // align.
		}
	}

	return true;
}

const int SCANMODE_RAW = 0;
const int SCANMODE_RLE = 1;

static bool BmpEncodeRLE8Scanline(const U8* scanline, UReg width, OUTBUFF &outBuff)
{
	int scanmode = SCANMODE_RAW;
	UReg scanlen = 0;
	U8 scanpix = 0;
	const U8* rawline = scanline;

	for (UReg x = 0; x < width;)
	{
		int nextmode = scanmode;

		if (scanmode == SCANMODE_RAW)
		{
			// should we switch to RLE mode?
			U8 pix = *scanline;
			UReg runlen = 0;
			while (((x+runlen) < width) && (pix == scanline[runlen]))
			{
				runlen++;
			}

			if (runlen >=4 || (runlen >= 2 && scanlen==0)) // can't emit a RAW packet if scanlen < 3.
			{
				// it's worth it to change modes now.
				nextmode = SCANMODE_RLE;
				scanpix = pix;
			}
			else
			{
				// just move one pixel forward.
				x++;
				scanline++;
				scanlen++;
			}
		}
		else
		{
			RAD_ASSERT(scanmode == SCANMODE_RLE);
			// are these pixels the same?
			if (scanpix == *scanline)
			{
				scanlen++;
				x++;
				scanline++;
			}
			else
			{
				// change modes!
				nextmode = SCANMODE_RAW;
			}
		}

		if ((nextmode != scanmode) || (scanlen == 255) || (x==width)) // emit what we were doing...
		{
			U8 codes[2];

			if (scanmode == SCANMODE_RAW)
			{
				if (scanlen > 3) // don't emit if it's zero length
				{
					RAD_ASSERT(scanlen >= 3);
					codes[0] = 0;
					codes[1] = scanlen; // literal bytes.
					if (!(outBuff.Write(codes, 2)&&outBuff.Write(rawline, scanlen))) 
						return false;
					if (scanlen&1)
					{
						U8 zero = 0;
						if (!outBuff.Write(&zero, 1)) 
							return false;
					}
				}
				else if (scanlen > 0)
				{
					// have to emit in RLE packets because RAW packets encode a minimum of 3 pixels.
					for (UReg i = 0; i < scanlen; i++)
					{
						codes[0] = 1;
						codes[1] = rawline[i];
						if (!outBuff.Write(codes, 2)) 
							return false;
					}
				}
			}
			else
			{
				RAD_ASSERT(scanmode == SCANMODE_RLE);
				if (scanlen > 0)
				{
					codes[0] = scanlen;
					codes[1] = scanpix;
					if (!outBuff.Write(codes, 2)) 
						return false;
				}
			}

			scanlen = 0;
			rawline = scanline;
			scanmode = nextmode;
		}
	}

	return true;
}

static bool BmpEncodeRLE8(const Mipmap &mip, OUTBUFF &outBuff)
{
	U8 codes[2];

	// write end of line.
	codes[0] = 0;
	codes[1] = 0;

	for (UReg y = mip.height; y > 0; y--)
	{
		const U8* src = ((const U8*)mip.data) + ((y-1)*mip.width);
		if (!BmpEncodeRLE8Scanline(src, mip.width, outBuff)) 
			return false;
		if (y > 1)
		{
			if (!outBuff.Write(codes, 2)) 
				return false;
		}
	}

	 codes[0] = 0;
	 codes[1] = 1; // End of bitmap.

	return outBuff.Write(codes, 2);
}

RADRT_API bool RADRT_CALL Encode(const Image &in, const Mipmap &mip, bool compressRLE, void *&outData, AddrSize &outSize)
{
	RAD_ASSERT(sizeof(BitmapHeader)==14);
	RAD_ASSERT(sizeof(BitmapInfo)==40);
	RAD_ASSERT(mip.width&&mip.height&&mip.stride&&mip.data&&mip.dataSize);
	RAD_ASSERT(in.bpp==1||in.bpp==3);
	RAD_ASSERT(in.format == Format_PAL8_RGB888 || in.format == Format_RGB888);

	if (in.format != Format_PAL8_RGB888 && in.format != Format_RGB888) 
		return false; // unsupported.

	OUTBUFF outBuff;
	BitmapHeader bmh;
	BitmapInfo   bmi;

	outSize = 0;

	memset(&bmh, 0, sizeof(BitmapHeader));
	bmh.sig[0] = 'B';
	bmh.sig[1] = 'M';

	bool hasPal = in.bpp == 1;
	RAD_ASSERT(hasPal == IsPalettedFormat(in.format));
	U32 dataOfs = 54;
	if (hasPal) dataOfs += 256*4;
	*((U32*)bmh.dataOfs) = endian::SwapLittle(dataOfs);

	if (!outBuff.Write(&bmh, sizeof(BitmapHeader))) 
		return false;

	memset(&bmi, 0, sizeof(BitmapInfo));
	bmi.size = sizeof(BitmapInfo);
	bmi.bitCount = in.bpp * 8;
	bmi.compression = (hasPal && compressRLE) ? BM_COMP_RLE8 : BM_COMP_RGB;
	bmi.width = (U32)mip.width;
	bmi.height = (U32)mip.height;
	bmi.planes = 1;
	bmi.xpixpm = 2834; // from photoshop.
	bmi.ypixpm = 2834;

	SwapBitmapInfo(&bmi);

	if (!outBuff.Write(&bmi, sizeof(BitmapInfo))) 
		return false;

	if (hasPal)
	{
		// write the palette.
		const U8* palB = (const U8*)in.pal;
		U8 vals[4];

		for (UReg i = 0; i < 256; i++)
		{
			vals[0] = palB[2];
			vals[1] = palB[1];
			vals[2] = palB[0];
			vals[3] = 0;
			palB += 3;

			if (!outBuff.Write(vals, 4)) 
				return false;
		}
	}

	bool s = true;

	if (in.bpp == 3)
	{
		// write the image.
		s = BmpEncode24(mip, outBuff);
	}
	else
	{
		RAD_ASSERT(hasPal);

		if (compressRLE)
		{
			s = BmpEncodeRLE8(mip, outBuff);
		}
		else
		{
			s = BmpEncode8(mip, outBuff);
		}
	}

	if (s)
	{
		outBuff.Trim();
		RAD_ASSERT(outBuff.m_buff);
		outData = outBuff.m_buff;
		outBuff.m_buff = 0;
		outSize = outBuff.m_size;

		// hack: update the filesize of the header.
		*((U32*)(((U8*)outData) + 2)) = endian::SwapLittle((U32)(outSize));

		if (hasPal && compressRLE)
		{
			// hack-update the image size.
			*((U32*)(((U8*)outData) + 34)) = endian::SwapLittle((U32)((outSize) - (54+256*4)));
		}
	}
		
	return s;
}

} // bmp
} // image_codec
