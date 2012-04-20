// Texture.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Texture.h"
#include "Files.h"
#include <Runtime/ImageCodec/ImageCodec.h>
#include <Runtime/ImageCodec/Tga.h>
#include <Runtime/ImageCodec/Png.h>
#include <Runtime/ImageCodec/Jpg.h>


using namespace image_codec;

GLTexture *Texture::LoadData(const char *name)
{
	int len;
	void *data = LoadFile(name, len);

	if (!data)
	{
		/*string::string<> s(name);
		 += ".png";
		data = LoadFile(s.c_str(), len);
		if (!data)
		{
			s = name;
			s += ".tga";
			data = LoadFile(s.c_str(), len);
			if (!data)
			{
				s = name;
				s += ".jpg";
				data = LoadFile(s.c_str(), len);
				if (!data)
				{
					FreeFileData(data);
					return 0;
				}
			}
		}*/
		return 0;
	}

	{
		Image img;
		if (tga::Decode(data, len, img) ||
			png::Decode(data, len, img) ||
			jpg::Decode(data, len, true, true, img))
		{
			FreeFileData(data);
			int type = GL_UNSIGNED_BYTE;
			int components = -1;

			switch (img.format)
			{
			case Format_A8:
				components = GL_ALPHA;
				break;
			case Format_RGB888:
				components = GL_RGB;
				break;
			case Format_RGBA8888:
				components = GL_RGBA;
				break;
			}

			if (type == -1 || components == -1)
			{
				FreeFileData(data);
				return 0;
			}

			GLTexture *gltex = new GLTexture(GL_TEXTURE_2D, img.frames[0].mipmaps[0].width, img.frames[0].mipmaps[0].height);
			GLState::SetDriverTMUTexture(0, gltex);
			R_glUploadTexture(
				gltex->Target(), 
				img.frames[0].mipmaps[0].width,
				img.frames[0].mipmaps[0].height,
				0,
				type,
				components,
				(m_mip?MipmapTextureFlag:0)|FilterTextureFlag|(m_wrap?WrapTextureFlag:0),
				img.frames[0].mipmaps[0].data,
				(int)img.frames[0].mipmaps[0].dataSize
			);

			return gltex;
		}
	}

	FreeFileData(data);
	return 0;
}