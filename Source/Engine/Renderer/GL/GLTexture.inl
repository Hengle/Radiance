// RBTexture.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {

inline GLTexture::GLTexture(int _size)
: target(0),
format(0),
width(0),
height(0),
depth(0),
size(_size)
{
	glGenTextures(1, &id);
	RAD_ASSERT(id>0);
	CHECK_GL_ERRORS();
	ZTextures.Get().Inc(_size, 0);
}

inline GLTexture::GLTexture(
	GLenum _target, 
	GLenum _format, 
	GLsizei _width, 
	GLsizei _height, 
	GLsizei _depth, 
	GLsizei _size
) : target(_target),
format(_format),
width(_width),
height(_height),
depth(_depth),
size(_size)
{
	glGenTextures(1, &id);
	RAD_ASSERT(id>0);
	CHECK_GL_ERRORS();
	ZTextures.Get().Inc(_size, 0);
}

inline GLTexture::~GLTexture()
{
	if (id>0)
	{
		glDeleteTextures(1, &id);
		CHECK_GL_ERRORS();
		ZTextures.Get().Dec(size, 0);
	}
}

inline const GLTexture::Ref &GLTextureAsset::Texture(int index)
{
	RAD_ASSERT(index < (int)m_texs.size());
	return m_texs[index];
}

} // r
