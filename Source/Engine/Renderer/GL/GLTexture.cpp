// GLTexture.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLTexture.h"
#include "GLState.h"
#include "../../Engine.h"
#include <Runtime/ImageCodec/Dds.h>
#include "../../Assets/TextureParser.h"

#if defined(RAD_OPT_OGLES)
// NOTE: there will be some build breaks when I port back to ES1 but
// I'm leaving it this way because I want to unify the ES1/ES2 headers
#define GL_RGBA8 GL_RGBA
#define GL_RGB8 GL_RGB
#define GL_ALPHA8 GL_ALPHA
#define GL_LUMINANCE8 GL_LUMINANCE
#define GL_LUMINANCE8_ALPHA8 GL_LUMINANCE_ALPHA
#define GL_TEXTURE_CUBE_MAP_ARB GL_TEXTURE_CUBE_MAP
#define GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB GL_TEXTURE_CUBE_MAP_POSITIVE_X
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB GL_TEXTURE_CUBE_MAP_NEGATIVE_X
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB GL_TEXTURE_CUBE_MAP_POSITIVE_Y
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB GL_TEXTURE_CUBE_MAP_NEGATIVE_Y
#define GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB GL_TEXTURE_CUBE_MAP_POSITIVE_Z
#define GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB GL_TEXTURE_CUBE_MAP_NEGATIVE_Z
#define GL_MAX_CUBE_MAP_TEXTURE_SIZE_ARB GL_MAX_CUBE_MAP_TEXTURE_SIZE
#endif

using namespace pkg;

namespace r {

RADENG_API GLenum RADENG_CALL GLInternalFormat(GLenum format, GLenum type) {
	switch( type ) {
		case GL_FLOAT:
			return format;
			
		case GL_UNSIGNED_BYTE: {
			switch(format) {
				case GL_RGBA:
				case GL_BGRA:
					return GL_RGBA8;
				case GL_RGB:
#if !defined(RAD_OPT_OGLES)
				case GL_BGR:
#endif
					return GL_RGB8;
				case GL_LUMINANCE:
					return GL_LUMINANCE8;
				case GL_LUMINANCE_ALPHA:
					return GL_LUMINANCE8_ALPHA8;
				case GL_ALPHA:
					return GL_ALPHA8;
#if !defined(RAD_OPT_OGLES)
				case GL_COLOR_INDEX:
					return GL_COLOR_INDEX8_EXT;
#endif
			}
		}		
		break;

		case GL_UNSIGNED_SHORT: {
			switch(format) {
#if !defined(RAD_OPT_OGLES)
				case GL_RGB:
				case GL_BGR:
					return GL_RGB16;
				case GL_RGBA:
				case GL_BGRA:
					return GL_RGBA16;
				case GL_LUMINANCE:
					return GL_LUMINANCE16;
				case GL_LUMINANCE_ALPHA:
					return GL_LUMINANCE16_ALPHA16;
				case GL_ALPHA:
					return GL_ALPHA16;
#endif
				case GL_DEPTH_COMPONENT:
					return GL_DEPTH_COMPONENT16_ARB;
			}
		}	
		break;
		
#if !defined(RAD_OPT_OGLES)
		case GL_UNSIGNED_BYTE_3_3_2:
		case GL_UNSIGNED_BYTE_2_3_3_REV:
			return GL_R3_G3_B2;
#endif
			
		case GL_UNSIGNED_SHORT_5_6_5:
#if defined(RAD_OPT_OGLES)
			return GL_RGB;
#else
		case GL_UNSIGNED_SHORT_5_6_5_REV:
			return GL_RGB5;
#endif

		case GL_UNSIGNED_SHORT_4_4_4_4:
#if defined(RAD_OPT_OGLES)
			return GL_RGBA;
#else
		case GL_UNSIGNED_SHORT_4_4_4_4_REV:
			return GL_RGBA4;
#endif
			
		case GL_UNSIGNED_SHORT_5_5_5_1:
#if defined(RAD_OPT_OGLES)
			return GL_RGBA;
#else
		case GL_UNSIGNED_SHORT_1_5_5_5_REV:
			return GL_RGB5_A1;

		case GL_UNSIGNED_INT: {
			switch (format) {
			case GL_DEPTH_COMPONENT:
				return GL_DEPTH_COMPONENT32_ARB;
			}
		} break;
			
		case GL_UNSIGNED_INT_8_8_8_8:
		case GL_UNSIGNED_INT_8_8_8_8_REV:
			return GL_RGBA8;
		
		case GL_UNSIGNED_INT_10_10_10_2:
		case GL_UNSIGNED_INT_2_10_10_10_REV:
			return GL_RGB10_A2;

		case GL_COMPRESSED_RGB_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT1_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT3_EXT:
		case GL_COMPRESSED_RGBA_S3TC_DXT5_EXT:
			return type;
#endif
#if defined(RAD_OPT_OGLES)
		case GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG:
		case GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG:
		case GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG:
		case GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG:
			return type;
#endif
	}

	return (GLenum)0;
}

RADENG_API bool RADENG_CALL GLImageFormat(int imgType, GLenum &format, GLenum &type, GLenum alpha) {
	switch (imgType) {
	case image_codec::Format_A8:
		format = alpha;
		type = GL_UNSIGNED_BYTE;
		return true;
	case image_codec::Format_RGB565:
		format = GL_RGB;
		type = GL_UNSIGNED_SHORT_5_6_5;
		return true;
	case image_codec::Format_RGB888:
		format = GL_RGB;
		type = GL_UNSIGNED_BYTE;
		return true;
	case image_codec::Format_RGBA4444:
		format = GL_RGBA;
		type = GL_UNSIGNED_SHORT_4_4_4_4;
		return true;
	case image_codec::Format_RGBA8888:
		format = GL_RGBA;
		type = GL_UNSIGNED_BYTE;
		return true;
#if defined(RAD_OPT_OGLES)
	case image_codec::dds::Format_PVR2:
		format = GL_RGB;
		type = GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG;
		return true;
	case image_codec::dds::Format_PVR2A:
		format = GL_RGBA;
		type = GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG;
		return true;
	case image_codec::dds::Format_PVR4:
		format = GL_RGB;
		type = GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG;
		return true;
	case image_codec::dds::Format_PVR4A:
		format = GL_RGBA;
		type = GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
		return true;
#else
	case image_codec::Format_BGR565:
		format = GL_BGR;
		type = GL_UNSIGNED_SHORT_5_6_5_REV;
		return true;
	case image_codec::Format_BGR888:
		format = GL_BGR;
		type = GL_UNSIGNED_BYTE;
		return true;
	case image_codec::Format_BGRA4444:
		format = GL_BGRA;
		type = GL_UNSIGNED_SHORT_4_4_4_4_REV;
		return true;
	case image_codec::Format_BGRA8888:
		format = GL_BGRA;
		type = GL_UNSIGNED_BYTE;
		return true;
	case image_codec::dds::Format_DXT1:
		format = GL_RGB;
		type = GL_COMPRESSED_RGB_S3TC_DXT1_EXT;
		return true;
	case image_codec::dds::Format_DXT1A:
		format = GL_RGBA;
		type = GL_COMPRESSED_RGBA_S3TC_DXT1_EXT;
		return true;
	case image_codec::dds::Format_DXT3:
		format = GL_RGBA;
		type = GL_COMPRESSED_RGBA_S3TC_DXT3_EXT;
		return true;
	case image_codec::dds::Format_DXT5:
		format = GL_RGBA;
		type = GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;
		return true;
#endif
	}

	return false;
}

RADENG_API bool RADENG_CALL GLImageFormat(
	int imgType, 
	GLenum &internal, 
	GLenum &format, 
	GLenum &type, 
	GLenum alpha
) {
	if (!GLImageFormat(imgType, format, type, alpha)) {
		return false;
	}

	return (internal=GLInternalFormat(format, type)) != (GLenum)0;
}

RADENG_API int RADENG_CALL GLCubeFace(int flags) {
	if (flags&image_codec::dds::FrameFlagCubemapPositiveX)
		return GL_TEXTURE_CUBE_MAP_POSITIVE_X_ARB;
	if (flags&image_codec::dds::FrameFlagCubemapNegativeX)
		return GL_TEXTURE_CUBE_MAP_NEGATIVE_X_ARB;
	if (flags&image_codec::dds::FrameFlagCubemapPositiveY)
		return GL_TEXTURE_CUBE_MAP_POSITIVE_Y_ARB;
	if (flags&image_codec::dds::FrameFlagCubemapNegativeY)
		return GL_TEXTURE_CUBE_MAP_NEGATIVE_Y_ARB;
	if (flags&image_codec::dds::FrameFlagCubemapPositiveZ)
		return GL_TEXTURE_CUBE_MAP_POSITIVE_Z_ARB;
	
	return GL_TEXTURE_CUBE_MAP_NEGATIVE_Z_ARB;
}

void GLTexture::SetFlags(const Ref &tex, int flags, int numMips, bool generateMips) {
	gls.SetTexture(0, tex, true);

	glTexParameteri(tex->target, GL_TEXTURE_WRAP_S, (flags&TX_WrapS) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	CHECK_GL_ERRORS();

	glTexParameteri(tex->target, GL_TEXTURE_WRAP_T, (flags&TX_WrapT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	CHECK_GL_ERRORS();

#if !defined(RAD_OPT_OGLES)
	glTexParameteri(tex->target, GL_TEXTURE_WRAP_R, (flags&TX_WrapR) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	CHECK_GL_ERRORS();
#endif

	bool mipmap = flags&TX_Mipmap ? true : false;

	GLint filter = GL_NEAREST;
	
	if (flags&TX_FilterBilinear) {
		filter = mipmap ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;
	} else if (flags&TX_FilterTrilinear) {
		filter = mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	}
	
	glTexParameteri(
		tex->target, 
		GL_TEXTURE_MIN_FILTER, 
		filter
	);

	CHECK_GL_ERRORS();

	glTexParameteri(
		tex->target,
		GL_TEXTURE_MAG_FILTER,
		(flags&(TX_FilterBilinear|TX_FilterTrilinear)) ? GL_LINEAR : GL_NEAREST
	);

	CHECK_GL_ERRORS();

#if !defined(RAD_OPT_OGLES)
	// sgis auto-gen mipmaps?
	if (mipmap && !numMips && gl.SGIS_generate_mipmap && !gl.EXT_framebuffer_object) {
		glTexParameteri(tex->target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
		CHECK_GL_ERRORS();
	}

	if (mipmap && numMips) {
		glTexParameteri(tex->target, GL_TEXTURE_MAX_LEVEL, numMips-1);
		CHECK_GL_ERRORS();
	}
#endif

	// EXT_frame_buffer generate mips?
	if (mipmap && !numMips && generateMips && gl.EXT_framebuffer_object) {
		GenerateMipmaps(tex);
	}
}

void GLTexture::GenerateMipmaps(const Ref &tex) {
	RAD_ASSERT(tex);
	if (gl.EXT_framebuffer_object) {
		gls.SetTexture(0, tex, true);
		gl.GenerateMipmapEXT(tex->target);
		CHECK_GL_ERRORS();
	}
}

namespace {

int UploadTexture(
	const image_codec::Image &image,
	GLTexture::Ref &out,
	int flags,
	int width,
	int height
) {
	if (image.frameCount < 1 || image.frames[0].mipCount < 1) {
		return SR_ErrorGeneric;
	}

	if (image.frameCount > 1) {
		if (image.frameCount != 6) {
			return SR_ErrorGeneric; // invalid cube-map.
		}

		UReg numMips = image.frames[0].mipCount;

		for (int i = 1; i < 6; ++i) {
			if (image.frames[i].mipCount != numMips) {
				return SR_ErrorGeneric; // invalid cube-map
			}
		}
	}

	GLenum internal, format, type;

	// find out what kind of pixel data this is.
	if (!GLImageFormat(image.format, internal, format, type, GL_ALPHA)) {
		return SR_InvalidFormat;
	}

#if defined(RAD_OPT_OGLES)
	bool compressed = 
		internal == GL_COMPRESSED_RGB_PVRTC_2BPPV1_IMG ||
		internal == GL_COMPRESSED_RGB_PVRTC_4BPPV1_IMG ||
		internal == GL_COMPRESSED_RGBA_PVRTC_2BPPV1_IMG ||
		internal == GL_COMPRESSED_RGBA_PVRTC_4BPPV1_IMG;
#else
	bool compressed = 
		internal == GL_COMPRESSED_RGB_S3TC_DXT1_EXT ||
		internal == GL_COMPRESSED_RGBA_S3TC_DXT1_EXT ||
		internal == GL_COMPRESSED_RGBA_S3TC_DXT3_EXT ||
		internal == GL_COMPRESSED_RGBA_S3TC_DXT5_EXT;

	if (compressed && (!gl.EXT_texture_compression_s3tc || !gl.ARB_texture_compression)) {
		return SR_InvalidFormat;
	}
#endif
	
	CHECK_GL_ERRORS();

	GLTexture::Ref tex(new (ZRender) GLTexture());

	int iwidth = (int)image.frames[0].mipmaps[0].width;
	int iheight = (int)image.frames[0].mipmaps[0].height;

	tex->target = image.frameCount > 1 ? GL_TEXTURE_CUBE_MAP_ARB : GL_TEXTURE_2D;
	tex->format = internal;
	tex->width = width&&!compressed ? width : iwidth;
	tex->height = height&&!compressed ? height : iheight;

	gls.SetTexture(0, tex, true);

	glTexParameteri(tex->target, GL_TEXTURE_WRAP_S, (flags&TX_WrapS) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	CHECK_GL_ERRORS();

	glTexParameteri(tex->target, GL_TEXTURE_WRAP_T, (flags&TX_WrapT) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	CHECK_GL_ERRORS();

#if !defined(RAD_OPT_OGLES)
	glTexParameteri(tex->target, GL_TEXTURE_WRAP_R, (flags&TX_WrapR) ? GL_REPEAT : GL_CLAMP_TO_EDGE);
	CHECK_GL_ERRORS();
#endif

	bool mipmap = flags&TX_Mipmap ? true : false;
	bool hasMips = image.frames[0].mipCount > 1;
	
	GLint filter = GL_NEAREST;
	
	if (flags&TX_FilterBilinear) {
		filter = mipmap ? GL_LINEAR_MIPMAP_NEAREST : GL_LINEAR;
	} else if (flags&TX_FilterTrilinear) {
		filter = mipmap ? GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR;
	}
	
	glTexParameteri(
		tex->target, 
		GL_TEXTURE_MIN_FILTER, 
		filter
	);

	CHECK_GL_ERRORS();

	glTexParameteri(
		tex->target,
		GL_TEXTURE_MAG_FILTER,
		(flags&(TX_FilterBilinear|TX_FilterTrilinear)) ? GL_LINEAR : GL_NEAREST
	);

	CHECK_GL_ERRORS();

#if !defined(RAD_OPT_OGLES)
	// sgis auto-gen mipmaps?
	if (mipmap && !hasMips && gl.SGIS_generate_mipmap && !gl.EXT_framebuffer_object) {
		glTexParameteri(tex->target, GL_GENERATE_MIPMAP_SGIS, GL_TRUE);
		CHECK_GL_ERRORS();
	}

	if (mipmap && hasMips) {
		glTexParameteri(tex->target, GL_TEXTURE_MAX_LEVEL, (int)image.frames[0].mipCount-1);
		CHECK_GL_ERRORS();
	}
#endif
	
	if (tex->target == GL_TEXTURE_CUBE_MAP_ARB) {
		for (int i = 0; i < 6; ++i) {
			const image_codec::Frame &f = image.frames[i];
			for (int k = 0; k < f.mipCount; ++k) {
				const image_codec::Mipmap &m = f.mipmaps[k];
				GLuint msize = (GLuint)m.dataSize;

				if (compressed) {
#if defined(RAD_OPT_OGLES)
					glCompressedTexImage2D(
						GLCubeFace(f.flags),
						k,
						internal,
						m.width,
						m.height,
						0,
						(GLsizei)m.dataSize,
						m.data
					);
#else
					gl.CompressedTexImage2DARB(
						GLCubeFace(f.flags),
						(GLint)k,
						internal,
						(GLint)m.width,
						(GLint)m.height,
						0,
						(GLsizei)m.dataSize,
						m.data
					);
#endif
				} else {
					void *mdata = m.data;
					GLint  mw = (GLint)m.width;
					GLint  mh = (GLint)m.height;

#if !defined(RAD_OPT_OGLES)
					if ((width && width != iwidth) || (height && height != iheight)) {
						GLuint bpp = (GLuint)(m.dataSize / (m.width*m.height));
						mw = (GLuint)width>>k;
						mh = (GLuint)height>>k;
						msize = PowerOf2(mw)*(PowerOf2(mh)+2)*bpp;
						U8 *data = (U8*)safe_zone_malloc(ZEngine, msize);

						gluScaleImage(
							format,
							(GLint)m.width,
							(GLint)m.height,
							type,
							m.data,
							mw,
							mh,
							type,
							data
						);

						mdata = data;
					}
#endif

					glTexImage2D(
						GLCubeFace(f.flags), 
						(GLint)k, 
						internal, 
						mw, 
						mh,
						0,
						format,
						type,
						mdata
					);

					if (mdata != m.data) {
						zone_free(mdata);
					}
				}
				CHECK_GL_ERRORS();
				tex->size += msize;
				if (!mipmap) 
					break; // that's it
			}
		}
	} else {
		const image_codec::Frame &f = image.frames[0];
		for (int i = 0; i < f.mipCount; ++i) {
			const image_codec::Mipmap &m = f.mipmaps[i];
			GLuint msize = (GLuint)m.dataSize;

			if (compressed) {
#if defined(RAD_OPT_OGLES)
				glCompressedTexImage2D(
					tex->target,
					i,
					internal,
					m.width,
					m.height,
					0,
					(GLsizei)m.dataSize,
					m.data
				);
#else
				gl.CompressedTexImage2DARB(
					tex->target,
					(GLint)i,
					internal,
					(GLint)m.width,
					(GLint)m.height,
					0,
					(GLsizei)m.dataSize,
					m.data
				);
#endif
			} else {
				void *mdata = m.data;
				GLint  mw = (GLint)m.width;
				GLint  mh = (GLint)m.height;

#if !defined(RAD_OPT_OGLES)
				if ((width && width != iwidth) || (height && height != iheight)) {
					GLuint bpp = (GLuint)(m.dataSize / (m.width*m.height));
					mw = (GLuint)width>>i;
					mh = (GLuint)height>>i;
					msize = PowerOf2(mw)*(PowerOf2(mh)+2)*bpp;
					U8 *data = (U8*)safe_zone_malloc(ZEngine, msize);

					gluScaleImage(
						format,
						(GLint)m.width,
						(GLint)m.height,
						type,
						m.data,
						mw,
						mh,
						type,
						data
					);

					mdata = data;
				}
#endif

				glTexImage2D(
					tex->target, 
					(GLint)i, 
					internal, 
					mw, 
					mh,
					0,
					format,
					type,
					mdata
				);

				if (mdata != m.data) {
					zone_free(mdata);
				}
			}
			CHECK_GL_ERRORS();
			tex->size += msize;
			if (!mipmap) 
				break;
		}
	}

	// EXT_frame_buffer generate mips?
	if (mipmap && !hasMips && gl.EXT_framebuffer_object) {
		gl.GenerateMipmapEXT(tex->target);
		CHECK_GL_ERRORS();
	}

	out = tex;
	ZTextures.Get().Inc(tex->size, 0);
	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

int UploadTexture(
	const pkg::Asset::Ref &asset,
	const image_codec::Image &image,
	GLTexture::Ref &out,
	int flags,
	int width,
	int height
) {
	int tflags = 0;

	const bool *b = asset->entry->KeyValue<bool>("Wrap.S", P_TARGET_FLAGS(flags));
	if (!b) 
		return SR_MetaError;
	if (*b) 
		tflags |= TX_WrapS;

	b = asset->entry->KeyValue<bool>("Wrap.T", P_TARGET_FLAGS(flags));
	if (!b) 
		return SR_MetaError;
	if (*b) 
		tflags |= TX_WrapT;

	b = asset->entry->KeyValue<bool>("Wrap.R", P_TARGET_FLAGS(flags));
	if (!b) 
		return SR_MetaError;
	if (*b) 
		tflags |= TX_WrapR;

	const String *s = asset->entry->KeyValue<String>("Filter", P_TARGET_FLAGS(flags));
	if (s) {
		if (*s == "Bilinear") {
			tflags |= TX_FilterBilinear;
		} else if (*s == "Trilinear") {
			tflags |= TX_FilterTrilinear;
		}
	} else {
		const bool *filter = asset->entry->KeyValue<bool>("Filter", P_TARGET_FLAGS(flags));
		if (!filter) 
			return SR_MetaError;
		if (*filter) 
			tflags |= TX_FilterBilinear;
	}

	const bool *mipmap = asset->entry->KeyValue<bool>("Mipmap", P_TARGET_FLAGS(flags));
	if (!mipmap) 
		return SR_MetaError;

	if (*mipmap) 
		tflags |= TX_Mipmap;

	return UploadTexture(image, out, tflags, width, height);
}

#endif

} // namespace

GLTexture::Ref GLTexture::UploadImage(const image_codec::Image &img, int flags, int width, int height) {
	Ref t;
	if (UploadTexture(img, t, flags, width, height) == SR_Success)
		return t;
	return GLTexture::Ref();
}

#if defined(RAD_OPT_PC_TOOLS)

GLTexture::Ref GLTextureAsset::CreateThumbnail(const pkg::Asset::Ref &asset, int index, int flags, int width, int height) {
	GLTexture::Ref t;

	if (!asset->cooked) {
		asset::TextureParser *parser(asset::TextureParser::Cast(asset));
		if (parser && parser->imgValid) {
			UploadTexture(*parser->Image(index), t, flags, width, height);
		}
	}

	return t;
}

#endif

GLTextureAsset::GLTextureAsset() {
}

GLTextureAsset::~GLTextureAsset() {
}

int GLTextureAsset::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (flags & (P_Unload|P_VidReset)) {
		m_texs.clear();
		STLContainerShrinkToSize(m_texs);
		return SR_Success;
	}

	int r = SR_Success;

	if (flags&(P_Load|P_VidBind) && m_texs.empty()) {
		asset::TextureParser *parser(asset::TextureParser::Cast(asset));
		if (!parser || !parser->imgValid)
			return SR_ParseError;
		

		// This method must be called on the main thread (where GL is bound)
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {
			const int numImages = parser->numImages;
			for (int i = 0; i < numImages; ++i) {
				GLTexture::Ref tex;

				const image_codec::Image *image = parser->Image(i);

				r = UploadTexture(
					asset,
					*image,
					tex,
					flags,
					0,
					0
				);

				if (r != SR_Success)
					break;

				m_texs.push_back(tex);
			}
		}
		else
#endif
		{
			int tflags = 0;
			if (parser->tag->flags&asset::TextureTag::WrapS)
				tflags |= TX_WrapS;
			if (parser->tag->flags&asset::TextureTag::WrapT)
				tflags |= TX_WrapT;
			if (parser->tag->flags&asset::TextureTag::WrapR)
				tflags |= TX_WrapR;
			if (parser->tag->flags&asset::TextureTag::Mipmap)
				tflags |= TX_Mipmap;

			if (parser->tag->flags&asset::TextureTag::FilterBilinear) {
				tflags |= TX_FilterBilinear;
			} else if (parser->tag->flags&asset::TextureTag::FilterTrilinear) {
				tflags |= TX_FilterTrilinear;
			}

			const int numImages = parser->numImages;
			for (int i = 0; i < numImages; ++i) {
				GLTexture::Ref tex;

				const image_codec::Image *image = parser->Image(i);

				r = UploadTexture(
					*image,
					tex,
					tflags,
					0,
					0
				);

				if (r != SR_Success)
					break;

				m_texs.push_back(tex);
			}
		}
	}

	return r;
}

void GLTextureAsset::Register(Engine &engine) {
	static pkg::Binding::Ref r = engine.sys->packages->Bind<GLTextureAsset>();
}

} // r
