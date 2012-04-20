// Texture.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "RGL.h"
#include "Asset.h"

class Texture : public Asset<Texture, GLTexture>
{
public:

	typedef Asset<Texture, GLTexture> SuperType;

	explicit Texture(bool mipmap=false, bool wrap=true) : m_mip(mipmap), m_wrap(wrap) {}
	Texture(const Texture &tex) : SuperType(tex), m_mip(tex.m_mip), m_wrap(tex.m_wrap) { if (tex.IsLoaded()) { Load(); } }
	explicit Texture(const char *filename, bool mipmap=false, bool wrap=true) : SuperType(filename), m_mip(mipmap), m_wrap(wrap) {}
	virtual ~Texture() { Release(); }

	void Bind(const std::string &filename, bool mipmap = false, bool wrap = true)
	{
		Bind(filename.c_str(), mipmap, wrap);
	}

	void Bind(const char *filename, bool mipmap=false, bool wrap=true)
	{
		m_mip = mipmap;
		m_wrap = wrap;
		SuperType::Bind(filename);
	}
	
	GLTexture *GLTex() const { return GetAsset(); }

protected:

	virtual GLTexture *LoadData(const char *name);
	virtual GLTexture *CreateType(GLTexture *data) { return data; }

private:

	bool m_wrap, m_mip;
};

