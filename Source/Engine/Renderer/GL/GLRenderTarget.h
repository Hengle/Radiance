// GLRenderTarget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "GLTable.h"
#include "GLTexture.h"
#include "../RendererDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS GLRenderTarget {
public:

	typedef boost::shared_ptr<GLRenderTarget> Ref;

	RAD_BEGIN_FLAGS
		RAD_FLAG(kDiscard_Color),
		RAD_FLAG(kDiscard_Depth),
		kDiscard_None = 0,
		kDiscard_All = kDiscard_Color|kDiscard_Depth
	RAD_END_FLAGS(DiscardFlags);

	GLRenderTarget(
		GLenum target, 
		GLenum format, 
		GLenum type,
		GLsizei width, 
		GLsizei height, 
		GLsizei depthFormat, 
		GLsizei size,
		int flags,
		bool autoGenMips
	);

	GLRenderTarget(
		const GLTexture::Ref &tex
	);

	~GLRenderTarget();

	void BindTexture(int index);
	void BindFramebuffer(DiscardFlags flags);
	void CreateDepthBufferTexture();
	void AttachDepthBuffer(const GLTexture::Ref &tex);

	static void DiscardFramebuffer(DiscardFlags flags);

	GLTexture::Ref tex;
	GLTexture::Ref depthTex;
	GLenum depthFormat;
	int depthSize;
	boost::array<GLuint, 2> id;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS GLRenderTargetCache {
public:

	typedef boost::shared_ptr<GLRenderTargetCache> Ref;
	
	enum DepthInstanceMode {
		kDepthInstanceMode_None,
		kDepthInstanceMode_Shared,
		kDepthInstanceMode_Unique
	};
	
	GLRenderTargetCache();

	GLRenderTargetCache(
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLenum depth,
		DepthInstanceMode depthMode,
		int flags,
		int colorBytesPP,
		int depthBytesPP
	);

	void SetFormats(
		GLsizei width,
		GLsizei height,
		GLenum format,
		GLenum type,
		GLenum depth,
		DepthInstanceMode depthMode,
		int flags,
		int colorBytesPP,
		int depthBytesPP
	);

	void CreateRenderTargets(int num);
	GLRenderTarget::Ref NextRenderTarget(bool wrap);

	void Reset();
	void Clear();

	RAD_DECLARE_READONLY_PROPERTY(GLRenderTargetCache, width, GLsizei);
	RAD_DECLARE_READONLY_PROPERTY(GLRenderTargetCache, height, GLsizei);

private:
	friend class GLRenderTargetMultiCache;
	typedef zone_vector<GLRenderTarget::Ref, ZRenderT>::type Vec;

	RAD_DECLARE_GET(width, GLsizei) {
		return m_width;
	}

	RAD_DECLARE_GET(height, GLsizei) {
		return m_height;
	}

	Vec m_vec;
	int m_flags;
	int m_next;
	int m_colorSize;
	int m_depthSize;
	GLsizei m_width;
	GLsizei m_height;
	GLenum m_format;
	GLenum m_type;
	GLenum m_depth;
	DepthInstanceMode m_depthMode;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS GLRenderTargetMultiCache {
public:
	typedef boost::shared_ptr<GLRenderTargetMultiCache> Ref;

	GLRenderTargetMultiCache();
	
	GLRenderTargetMultiCache(
		int numRenderTargets,
		GLenum format,
		GLenum type,
		GLenum depth,
		GLRenderTargetCache::DepthInstanceMode depthMode,
		int flags,
		int colorBytesPP,
		int depthBytesPP
	);

	void SetFormats(
		int numRenderTargets,
		GLenum format,
		GLenum type,
		GLenum depth,
		GLRenderTargetCache::DepthInstanceMode depthMode,
		int flags,
		int colorBytesPP,
		int depthBytesPP
	);

	GLRenderTarget::Ref NextRenderTarget(
		GLsizei width,
		GLsizei height,
		bool wrap
	);

	void Clear();

private:

	struct Key {
		Key(GLsizei w, GLsizei h) : width(w), height(h) {}

		GLsizei width;
		GLsizei height;

		int Compare(const Key &other) const {
			if (width < other.width)
				return -1;
			if (width > other.width)
				return 1;
			if (height < other.height)
				return -1;
			if (height > other.height)
				return 1;
			return 0;
		}

		bool operator == (const Key &other) const {
			return Compare(other) == 0;
		}

		bool operator != (const Key &other) const {
			return Compare(other) != 0;
		}

		bool operator > (const Key &other) const {
			return Compare(other) > 0;
		}

		bool operator < (const Key &other) const {
			return Compare(other) < 0;
		}

		bool operator >= (const Key &other) const {
			return Compare(other) >= 0;
		}

		bool operator <= (const Key &other) const {
			return Compare(other) <= 0;
		}
	};

	typedef zone_map<Key, GLRenderTargetCache::Ref, ZRenderT>::type Map;

	Map m_map;
	int m_colorBytesPP;
	int m_depthBytesPP;
	int m_flags;
	int m_numRenderTargets;
	GLenum m_format;
	GLenum m_type;
	GLenum m_depth;
	GLRenderTargetCache::DepthInstanceMode m_depthMode;
	GLRenderTargetCache::Ref m_lastCache;
};

} // r

RAD_IMPLEMENT_FLAGS(r::GLRenderTarget::DiscardFlags);

#include <Runtime/PopPack.h>
