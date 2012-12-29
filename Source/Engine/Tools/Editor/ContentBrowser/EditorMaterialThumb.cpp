// EditorMaterialThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMaterialThumb.h"
#include "EditorContentAssetThumbDimensionCache.h"
#include "../EditorUtils.h"
#include "../../../Assets/MaterialParser.h"
#include "../../../Assets/TextureParser.h"
#include "../../../Renderer/Material.h"
#include <algorithm>
#undef min
#undef max

using namespace r;
using namespace asset;
using namespace pkg;

namespace tools {
namespace editor {

enum {
	MaxSize = 16*Meg,
	MaxLoading = 8,
	OverlayDiv = 8
};

struct OverlayVert {
	float xy[2];
	float st[2];
};

MaterialThumb::MaterialThumb(ContentBrowserView &view) :
ContentAssetThumb(view), m_cache(MaxLoading, MaxSize), m_lastDraw(0) {
}

MaterialThumb::~MaterialThumb() {
	m_cache.EvictAll();
}

void MaterialThumb::Dimensions(const pkg::Package::Entry::Ref &entry, int &w, int &h) {
	ContentAssetThumb::Dimensions(entry, w, h);

	Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);
	if (!asset)
		return;

	xtime::TimeDate modified;
	MaterialParser::Ref parser = MaterialParser::Cast(asset);
	if (!parser)
		return;

	if (parser->SourceModifiedTime(
		*App::Get()->engine,
		asset,
		0,
		modified
	) != SR_Success) {
		return;
	}

	const ContentAssetThumbDimensionCache::Info *pinfo = 
		ContentAssetThumbDimensionCache::Get()->Find(entry->id);

	if (pinfo) {
		if (modified == pinfo->modified) {
			w = pinfo->width;
			h = pinfo->height;
			return;
		}
	}

	MaterialLoader::Ref mL = MaterialLoader::Cast(asset);
	if (!mL->info) {
		int r = asset->Process(
			xtime::TimeSlice::Infinite,
			P_Info|P_Unformatted|P_NoDefaultMedia
		);

		if (r != SR_Success)
			return;

		if (!mL->info)
			return;
	}

	int mx = 0;
	int my = 0;

	// find the largest texture.
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		TextureParser::Ref t = TextureParser::Cast(mL->Texture(i));
		if (!t)
			break;
		if (!t->headerValid)
			break;
		mx = std::max(mx, t->header->width);
		my = std::max(my, t->header->height);
	}
	
	if (mx > 0 && my > 0) {
		w = mx;
		h = my;
	}

	ContentAssetThumbDimensionCache::Info info;
	info.width = w;
	info.height = h;
	info.modified = modified;
	ContentAssetThumbDimensionCache::Get()->Update(entry->id, info);
	ContentAssetThumbDimensionCache::Get()->Save();
}

inline float Wrap(float r) {
	float z = floorf(r);
	int side = (int)z;

	r -= z;

	if (side&1) {
		return -1.0f + r*2;
	}

	return 1.0f - r*2;
}

bool MaterialThumb::RenderIcon(const Thumbnail::Ref &t, int x, int y, int w, int h) {
	GLTexture::Ref icon = t->icon;
	if (!icon)
		return false;

	gls.BindBuffer(GL_ARRAY_BUFFER_ARB, GLVertexBufferRef());
	gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GLVertexBufferRef());
	gls.DisableTextures();
	gls.Set(kDepthTest_Disable|kDepthWriteMask_Disable|kCullFaceMode_None|kColorWriteMask_RGBA, kBlendMode_Off);
	gls.SetTexture(0, icon);
	gls.Commit();

	gl.MatrixMode(GL_MODELVIEW);
	gl.PushMatrix();
	gl.LoadIdentity();

	int tw = w / 3 * 2;
	int th = h / 3 * 2;

	gl.Translatef(x + w/2, y + h/2, 0.0f);

	if (t->active)
		gl.Rotatef(Wrap(t->rot)*20.0f, 0.0f, 0.0f, 1.0f);
	
	x = -tw/2;
	y = -th/2;

	glBegin(GL_QUADS);
		glTexCoord2i(0, 0);
		glVertex2i(x, y);
		glTexCoord2i(1, 0);
		glVertex2i(x+tw, y);
		glTexCoord2i(1, 1);
		glVertex2i(x+tw, y+th);
		glTexCoord2i(0, 1);
		glVertex2i(x, y+th);
	glEnd();

	gl.PopMatrix();

	return true;
}

bool MaterialThumb::Render(const pkg::Package::Entry::Ref &entry, int x, int y, int w, int h) {
	Thumbnail::Ref t = RequestThumbnail(entry->id, w, h);
	if (t) {
		if (RenderIcon(t, x, y, w, h))
			return true;

		gl.Color4f(1.f, 1.f, 1.f, 1.f, true); // force=true, who knows what Qt does behind our backs.

		t->material->BindStates(kDepthTest_Disable|kDepthWriteMask_Disable|kCullFaceMode_None, 0);

		gls.DisableAllMTSources();
		gls.DisableAllMGSources();

		// bind our thumbnail textures.
		for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
			GLTexture::Ref tex = t->Texture(i);
			if (!tex)
				break;
			gls.SetMTSource(kMaterialTextureSource_Texture, i, tex);
		}
		

		gls.SetMGSource(
			kMaterialGeometrySource_Vertices,
			0,
			t->vb,
			2,
			GL_FLOAT,
			GL_FALSE,
			sizeof(OverlayVert),
			0
		);

		gls.SetMGSource(
			kMaterialGeometrySource_TexCoords,
			0,
			t->vb,
			2, 
			GL_FLOAT,
			GL_FALSE,
			sizeof(OverlayVert),
			sizeof(float)*2
		);

		gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, t->ib);

		gl.MatrixMode(GL_MODELVIEW);
		gl.PushMatrix();
		gl.LoadIdentity();
		gl.Translatef(x + w/2, y + h/2, 0.0f);

		t->material->shader->Begin(Shader::kPass_Preview, *t->material.get());
		t->material->shader->BindStates();
		gls.Commit();
		
		gl.DrawElements(GL_TRIANGLES, (OverlayDiv-1)*(OverlayDiv-1)*6, GL_UNSIGNED_SHORT, 0);

		gl.PopMatrix();

		t->material->shader->End();
		gls.DisableTextures();
		gls.DisableVertexAttribArrays();
		gls.Set(0, kBlendMode_Off);
		gls.Commit();
		gls.BindBuffer(GL_ARRAY_BUFFER_ARB, GLVertexBufferRef());
		gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GLVertexBufferRef());
		gls.SetActiveTexture(0);
		gls.SetActiveTexCoord(0);
	}

	return true;
}

void MaterialThumb::NotifyAddRemovePackages() {
}

void MaterialThumb::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed) {
}

void MaterialThumb::NotifyContentChanged(const ContentChange::Vec &changed) {
	for (ContentChange::Vec::const_iterator it = changed.begin(); it != changed.end(); ++it) {
		const ContentChange &c = *it;
		if (c.entry->type != asset::AT_Material)
			continue;

		Thumbnail::Ref t = FindThumbnail(c.entry->id);
		if (t)
			t->Evict();

		ContentAssetThumbDimensionCache::Get()->Delete(c.entry->id);
	}
}

void MaterialThumb::Begin() {
	m_cache.BeginRefresh();
}

void MaterialThumb::End() {
	m_cache.EndRefresh();
}

void MaterialThumb::Tick(float t, const xtime::TimeSlice &time) {
	bool vis = false;
	bool notify = m_cache.Tick(t, time, vis);
	xtime::TimeVal now = xtime::ReadMilliseconds();
	if (notify || (vis && (now-m_lastDraw) > 1000/5))
	{
		m_lastDraw = now;
		ThumbChanged();
	}
}

MaterialThumb::Thumbnail::Ref MaterialThumb::RequestThumbnail(int id, int w, int h) {
	Thumbnail::Ref t = FindThumbnail(id);
	if (!t) {
		t.reset(new Thumbnail(id, this));
		m_cache.Insert(boost::static_pointer_cast<ContentThumbCache::Item>(t));
	}
	t->Request(w, h);
	t->MarkVisible();
	return t;
}

MaterialThumb::Thumbnail::Ref MaterialThumb::FindThumbnail(int id) {
	return boost::static_pointer_cast<Thumbnail>(m_cache.Find(id));
}

void MaterialThumb::New(ContentBrowserView &view) {
	MaterialThumb *t = new (ZEditor) MaterialThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Material);
}

void CreateMaterialThumb(ContentBrowserView &view) {
	MaterialThumb::New(view);
}

void MaterialThumb::ScaleSize(int &w, int &h, int rw, int rh) {
	if (w > h) {
		float s = rw / ((float)w);
		h = (int)(s * h);
		w = rw;
	} else {
		float s = rh / ((float)h);
		w = (int)(s * w);
		h = rh;
	}
}

MaterialThumb::Thumbnail::Thumbnail(int id, MaterialThumb *outer) :
ContentThumbCache::Item(id), 
m_outer(outer), 
m_w(0), 
m_h(0), 
m_idx(0),
m_time(0),
m_rot(0.f),
m_load(false),
m_icon(true),
m_mat(0) {
}

MaterialThumb::Thumbnail::~Thumbnail() {
}

void MaterialThumb::Thumbnail::Request(int w, int h) {
	if (m_w == w && m_h == h)
		return;
	Cancel();
	Resize(w*h*4);
	m_w = w;
	m_h = h;

	m_mat = 0;
	m_asset.reset();
	m_icon = true;

	m_vb.reset(
		new GLVertexBuffer(
			GL_ARRAY_BUFFER_ARB, 
			GL_STATIC_DRAW_ARB, 
			sizeof(OverlayVert)*OverlayDiv*OverlayDiv
		)
	);

	RAD_ASSERT(m_vb);

	GLVertexBuffer::Ptr::Ref vb = m_vb->Map();
	RAD_ASSERT(vb);
	OverlayVert *verts = (OverlayVert*)vb->ptr.get();

	float xInc = w / ((float)OverlayDiv-1);
	float yInc = h / ((float)OverlayDiv-1);

	int x, y;
	float xf, yf;

	for (y = 0, yf = 0.f; y < OverlayDiv; ++y, yf += yInc) {
		for (x = 0, xf = 0.f; x < OverlayDiv; ++x, xf += xInc) {
			OverlayVert &v = verts[y*OverlayDiv+x];
			v.xy[0] = xf-w/2;
			v.xy[1] = yf-h/2;
			v.st[0] = xf / w;
			v.st[1] = yf / h;
		}
	}

	vb.reset(); // unmap

	// setup triangle indices

	m_ib.reset(
		new GLVertexBuffer(
			GL_ELEMENT_ARRAY_BUFFER_ARB,
			GL_STATIC_DRAW_ARB,
			sizeof(U16)*(OverlayDiv-1)*(OverlayDiv-1)*6
		)
	);

	vb = m_ib->Map();
	RAD_ASSERT(vb);
	U16 *indices = (U16*)vb->ptr.get();

	for (y = 0; y < OverlayDiv-1; ++y) {
		for (x = 0; x < OverlayDiv-1; ++x) {
			U16 *idx = &indices[y*(OverlayDiv-1)*6+x*6];

			// glOrtho() inverts the +Z axis (or -Z can't recall), inverting the 
			// dot product sign and culling CCW faces (i.e. we're looking from the 
			// back instead of from the front).
			// I'm correcting this in the indices.

			idx[2] = (U16)(y*OverlayDiv+x);
			idx[1] = (U16)((y+1)*OverlayDiv+x);
			idx[0] = (U16)((y+1)*OverlayDiv+x+1);
			idx[5] = (U16)(y*OverlayDiv+x);
			idx[4] = (U16)((y+1)*OverlayDiv+x+1);
			idx[3] = (U16)(y*OverlayDiv+x+1);
		}
	}

	vb.reset(); // unmap

	m_load = true;
}

GLTexture::Ref MaterialThumb::Thumbnail::RAD_IMPLEMENT_GET(icon) {
	return m_icon ? m_outer->View().GetIcon((ContentBrowserView::Icon)m_idx) : GLTexture::Ref();
}

GLTexture::Ref MaterialThumb::Thumbnail::Texture(int index) {
	const GLTexture::Vec &texs = m_texs[index];
	if (texs.empty())
		return GLTexture::Ref();

	int image = 0;
	float fps = 0.f;

	if (m_mat)
		fps = m_mat->TextureFPS(index);

	if ((texs.size() > 1) && (fps > 0.f)) {
		image = FloatToInt(m_mat->time.get()*fps);
		image = image % (int)texs.size();
	}

	return texs[image];
}

ThumbResult MaterialThumb::Thumbnail::Tick(float dt, const xtime::TimeSlice &time) {
	RAD_ASSERT(m_load && active);

	if (!m_asset)
		m_asset = Packages()->Asset(this->id, Z_Unique);

	if (!m_asset) {
		m_load = false;
		m_icon = true;
		m_idx = ContentBrowserView::I_Error;
		return TR_Complete;
	}

	xtime::TimeVal now = xtime::ReadMilliseconds();

	if (m_icon) {
		m_rot += dt;

		if (m_time == 0)
			m_time = now;

		if (now-m_time > 750) {
			m_time = now;
			m_idx = (m_idx+1)%ContentBrowserView::I_AnimRange;
		}
	}

	if (m_mat && m_mat->animated) {
		m_mat->Sample(App::Get()->time, dt);
		if (!m_icon)
			m_outer->ThumbChanged();
	}

	if (!time.remaining)
		return TR_Pending;
	
	int r = m_asset->Process(
		time,
		P_Unformatted|P_Parse|P_NoDefaultMedia
	);

	if (r == SR_Pending)
		return TR_Pending;

	m_load = false;

	if (r != SR_Success) {
		m_icon = true;
		m_idx = ContentBrowserView::I_Error;
		return TR_Complete;
	}

	m_mat = 0;

	MaterialParser::Ref mP = MaterialParser::Cast(m_asset);
	MaterialLoader::Ref mL = MaterialLoader::Cast(m_asset);

	if (!mP || !mL || !mP->valid || !mL->parsed) {
		m_icon = true;
		m_idx = ContentBrowserView::I_Error;
		return TR_Complete;
	}

	m_mat = mP->material;

	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		m_texs[i].clear();
	}
	
	// make the thumbnails.
	// NOTE: I should make this honor the TimeSlice
	// at some point...
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		Asset::Ref a = mL->Texture(i);
		if (!a)
			break;

		TextureParser::Ref t = TextureParser::Cast(a);
		const bool *wrapS = a->entry->KeyValue<bool>("Wrap.S", P_TARGET_PLATFORM);
		const bool *wrapT = a->entry->KeyValue<bool>("Wrap.T", P_TARGET_PLATFORM);
		const bool *wrapR = a->entry->KeyValue<bool>("Wrap.R", P_TARGET_PLATFORM);
		const bool *filter = a->entry->KeyValue<bool>("Filter", P_TARGET_PLATFORM);
		const bool *mipmap = a->entry->KeyValue<bool>("Mipmap", P_TARGET_PLATFORM);

		if (!t || !t->imgValid || !wrapS || !wrapT || !wrapR || !filter || !mipmap) {
			for (int k = 0; k < kMaterialTextureSource_MaxIndices; ++k) {
				m_texs[k].clear();
			}

			m_asset.reset();
			m_mat = 0;
			m_icon = true;
			m_idx = ContentBrowserView::I_Error;
			return TR_Complete;
		}

		int flags = 0;

		if (*wrapS) 
			flags |= TX_WrapS;
		if (*wrapS) 
			flags |= TX_WrapT;
		if (*wrapS) 
			flags |= TX_WrapR;
		if (*filter) 
			flags |= TX_Filter;
		if (*mipmap) 
			flags |= TX_Mipmap;

		int w = t->header->width;
		int h = t->header->height;

		MaterialThumb::ScaleSize(w, h, m_w, m_h);

		const int numImages = t->numImages;
		for (int j = 0; j < numImages; ++j) {
			GLTexture::Ref tex = GLTextureAsset::CreateThumbnail(a, j, flags, w, h);
			RAD_ASSERT(tex);
			m_texs[i].push_back(tex);
		}
	}

	// release textures...
	m_asset->Process(
		xtime::TimeSlice::Infinite,
		P_Trim
	);

	m_icon = false;
	return TR_Complete;
}

void MaterialThumb::Thumbnail::InactiveTick(float dt) {
	if (m_mat && m_mat->animated) {
		m_mat->Sample(App::Get()->time, dt);
		if (!m_icon)
			m_outer->ThumbChanged();
	}
}

void MaterialThumb::Thumbnail::Cancel() {
	if (m_asset) {
		m_asset->Process(
			xtime::TimeSlice::Infinite,
			P_Cancel
		);
	}
}

bool MaterialThumb::Thumbnail::CheckActivate() const {
	return m_load;
}

} // editor
} // tools

#include "moc_EditorMaterialThumb.cc"
