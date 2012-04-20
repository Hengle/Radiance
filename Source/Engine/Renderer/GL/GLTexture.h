// GLTexture.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Packages/Packages.h"
#include "GLTextureDef.h"
#include "GLTable.h"
#include "../RendererDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace image_codec {
struct Image;
} // image_codec

namespace r {

enum
{
	RAD_FLAG(TX_WrapS),
	RAD_FLAG(TX_WrapT),
	RAD_FLAG(TX_WrapR),
	RAD_FLAG(TX_Filter),
	RAD_FLAG(TX_Mipmap),
	TX_WrapAll = TX_WrapS|TX_WrapT|TX_WrapR
};

RADENG_API GLenum RADENG_CALL GLInternalFormat(GLenum format, GLenum type);
RADENG_API bool RADENG_CALL GLImageFormat(UReg imgType, GLenum &format, GLenum &type, GLenum alpha);
RADENG_API bool RADENG_CALL GLImageFormat(UReg imgType, GLenum &internal, GLenum &format, GLenum &type, GLenum alpha);
RADENG_API int RADENG_CALL GLCubeFace(int flags);

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS GLTexture
{
public:
	typedef boost::shared_ptr<GLTexture> Ref;
	typedef zone_vector<Ref, ZEngineT>::type Vec;
	GLTexture(int size=0);
	GLTexture(
		GLenum target, 
		GLenum format, 
		GLsizei width, 
		GLsizei height, 
		GLsizei depth, 
		GLsizei size
	);
	virtual ~GLTexture();

	static void SetFlags(const Ref &tex, int flags, int numMips=0, bool generateMips=true);
	static void GenerateMipmaps(const Ref &tex);

	static Ref UploadImage(const image_codec::Image &img, int flags, int width, int height);

	GLenum target;
	GLenum format;
	GLuint id;
	GLsizei width;
	GLsizei height;
	GLsizei depth;
	GLsizei size;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS GLTextureAsset : public pkg::Sink<GLTextureAsset>
{
public:

	static void Register(Engine &engine);

	enum
	{
		SinkStage = pkg::SS_Process,
		AssetType = asset::AT_Texture
	};

	typedef boost::shared_ptr<GLTextureAsset> Ref;

	GLTextureAsset();
	virtual ~GLTextureAsset();

	const GLTexture::Ref &Texture(int index);

	RAD_DECLARE_READONLY_PROPERTY(GLTextureAsset, numTextures, int);

#if defined(RAD_OPT_PC_TOOLS)
	static GLTexture::Ref CreateThumbnail(const pkg::Asset::Ref &asset, int index, int flags, int width, int height);
#endif

protected:

	int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	RAD_DECLARE_GET(numTextures, int) { return (int)m_texs.size(); }

	GLTexture::Vec m_texs;
};

} // r

#include <Runtime/PopPack.h>
#include "GLTexture.inl"
