// EditorMaterialThumb.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorContentBrowserView.h"
#include "EditorContentThumbCache.h"
#include "../../../Renderer/GL/GLTexture.h"
#include "../../../Renderer/GL/GLVertexBuffer.h"
#include "../../../Renderer/Material.h"
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class MaterialThumbCache;
typedef boost::shared_ptr<MaterialThumbCache> MaterialThumbCacheRef;

class RADENG_CLASS MaterialThumb : public ContentAssetThumb
{
	Q_OBJECT
public:

	MaterialThumb(ContentBrowserView &view);
	virtual ~MaterialThumb();
	virtual void Dimensions(const pkg::Package::Entry::Ref &entry, int &w, int &h);
	virtual bool Render(const pkg::Package::Entry::Ref &entry, int x, int y, int w, int h);
	virtual void NotifyAddRemovePackages();
	virtual void NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed);
	virtual void NotifyContentChanged(const ContentChange::Vec &changed);
	virtual void Begin();
	virtual void End();
	virtual void Tick(float t, const xtime::TimeSlice &time);

	static void New(ContentBrowserView &view);

private:

	static void ScaleSize(int &w, int &h, int rw, int rh);

	class Thumbnail : public ContentThumbCache::Item
	{
	public:
		typedef boost::shared_ptr<Thumbnail> Ref;
		Thumbnail(int id, MaterialThumb *outer);
		virtual ~Thumbnail();
		void Request(int w, int h);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, icon, r::GLTexture::Ref);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, material, r::Material*);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, rot, float);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, vb, r::GLVertexBuffer::Ref);
		RAD_DECLARE_READONLY_PROPERTY(Thumbnail, ib, r::GLVertexBuffer::Ref);
		r::GLTexture::Ref Texture(r::MTSource source, int index);
	protected:
		virtual RAD_DECLARE_GET(size, int)  { return m_w*m_h*4; }
		virtual ThumbResult Tick(float dt, const xtime::TimeSlice &time);
		virtual void InactiveTick(float dt);
		virtual void Cancel();
		virtual bool CheckActivate() const;
	private:
		RAD_DECLARE_GET(icon, r::GLTexture::Ref);
		RAD_DECLARE_GET(material, r::Material*) { return m_mat; }
		RAD_DECLARE_GET(rot, float) { return m_rot; }
		RAD_DECLARE_GET(vb, r::GLVertexBuffer::Ref) { return m_vb; }
		RAD_DECLARE_GET(ib, r::GLVertexBuffer::Ref) { return m_ib; }
		int m_w, m_h;
		int m_idx;
		float m_rot;
		bool m_load;
		bool m_icon;
		xtime::TimeVal m_time;
		pkg::Asset::Ref m_asset;
		r::Material *m_mat;
		MaterialThumb *m_outer;
		r::GLTexture::Vec m_texs[r::MTS_Max][r::MTS_MaxIndices];
		r::GLVertexBuffer::Ref m_vb;
		r::GLVertexBuffer::Ref m_ib;
	};

	Thumbnail::Ref RequestThumbnail(int id, int w, int h);
	Thumbnail::Ref FindThumbnail(int id);
	bool RenderIcon(const Thumbnail::Ref &t, int x, int y, int w, int h);

	friend class Thumbnail;
	
	int m_lastDraw;
	ContentThumbCache m_cache;
};

} // editor
} // tools

#include <Runtime/PopPack.h>

