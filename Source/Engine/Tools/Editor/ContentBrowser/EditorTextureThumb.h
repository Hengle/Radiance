// EditorTextureThumb.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorContentBrowserView.h"
#include "EditorContentThumbCache.h"
#include "../../../Renderer/GL/GLTexture.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class TextureThumb : public ContentAssetThumb {
	Q_OBJECT
public:

	enum {
		kMaxCacheSize = 16*kMeg,
		kMaxLoading = 16
	};

	TextureThumb(ContentBrowserView &view) :
	  ContentAssetThumb(view),
	  m_lastDraw(0),
	  m_cache(kMaxLoading, kMaxCacheSize) {
	}

	~TextureThumb() {
		Release();
	}

	virtual void Dimensions(const pkg::Package::Entry::Ref &asset, int &w, int &h);
	virtual bool Render(const pkg::Package::Entry::Ref &asset, int x, int y, int w, int h);
	virtual void OpenEditor(const pkg::Package::Entry::Ref &asset, bool editable, bool modal);
	virtual void NotifyAddRemovePackages();
	virtual void NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed);
	virtual void NotifyContentChanged(const ContentChange::Vec &changed);
	virtual void Begin();
	virtual void End();
	virtual void Tick(float t, const xtime::TimeSlice &time);

	void Release();
	void CancelThumbs();
	void CancelThumb(int id);

	static void New(ContentBrowserView &view);

private:

	class Thumbnail : public ContentThumbCache::Item {
	public:
		typedef boost::shared_ptr<Thumbnail> Ref;
		Thumbnail(int id, TextureThumb *outer);
		virtual ~Thumbnail();

		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, texture, r::GLTexture::Ref);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, rot, float);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, loading, bool);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, isIcon, bool);

		void Request(int w, int h);

	protected:

		virtual ThumbResult Tick(float dt, const xtime::TimeSlice &time);
		virtual void Cancel();
		virtual bool CheckActivate() const;

	private:

		RAD_DECLARE_GET(texture, r::GLTexture::Ref) { return m_tex; }
		RAD_DECLARE_GET(rot, float) { return m_rot; }
		RAD_DECLARE_GET(loading, bool) { return m_load; }
		RAD_DECLARE_GET(isIcon, bool) { return m_icon; }
		RAD_DECLARE_GET(size, int) { return m_w*m_h*4; }

		pkg::Asset::Ref m_asset;
		r::GLTexture::Ref m_tex;
		TextureThumb *m_outer;
		bool m_icon;
		bool m_load;
		float m_rot;
		xtime::TimeVal m_time;
		int m_idx;
		int m_w, m_h;
	};

	Thumbnail::Ref FindThumbnail(int id);
	Thumbnail::Ref RequestThumb(int id, int w, int h);

	friend class Thumbnail;

	ContentThumbCache m_cache;
	xtime::TimeVal m_lastDraw;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
