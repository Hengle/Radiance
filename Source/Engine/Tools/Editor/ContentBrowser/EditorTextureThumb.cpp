// EditorTextureThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorTextureThumb.h"
#include "EditorContentPropertyGrid.h"
#include "../EditorUtils.h"
#include "../../../Renderer/GL/GLState.h"
#include "../../../Assets/TextureParser.h"
#include "../../../FileSystem/FileSystem.h"
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Thread/Thread.h>
#include <Runtime/Thread/Locks.h>
#include <Runtime/Math/Math.h>
#include <algorithm>
#undef min
#undef max

using namespace tools::editor;
using namespace r;
using namespace asset;

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

TextureThumb::Thumbnail::Thumbnail(int id, TextureThumb *outer) :
ContentThumbCache::Item(id), 
m_outer(outer), 
m_icon(true),
m_load(false),
m_time(0),
m_rot(0.f),
m_idx(0),
m_w(0),
m_h(0)
{
	m_tex = m_outer->View().GetIcon(ContentBrowserView::I_OneDisk);
}

TextureThumb::Thumbnail::~Thumbnail()
{
	m_outer->m_sizes.erase(this->id);
}

void TextureThumb::Thumbnail::Request(int w, int h)
{
	if (m_w != w || m_h != h)
	{
		Cancel();
		Resize(w*h*4);
		m_w = w;
		m_h = h;
		m_load = true;
	}
}

ThumbResult TextureThumb::Thumbnail::Tick(float dt, const xtime::TimeSlice &time)
{
	RAD_ASSERT(m_load && active);
	
	if (!m_asset)
		m_asset = Packages()->Asset(this->id, pkg::Z_ContentBrowser);

	if (!m_asset)
	{
		m_load = false;
		m_icon = true;
		m_tex = m_outer->View().errorIcon;
		return TR_Complete;
	}

	if (m_icon)
	{
		m_rot += dt;

		xtime::TimeVal now = xtime::ReadMilliseconds();
		if (m_time == 0)
			m_time = now;

		if (now-m_time > 750)
		{
			m_time = now;
			m_idx = (m_idx+1)%ContentBrowserView::I_AnimRange;
		}

		m_tex = m_outer->View().GetIcon((ContentBrowserView::Icon)m_idx);
	}

	if (!time.remaining)
		return TR_Pending;

	int r = m_asset->Process(
		time,
		pkg::P_Unformatted|pkg::P_Parse|pkg::P_NoDefaultMedia
	);

	if (r == pkg::SR_Pending)
		return TR_Pending;

	m_load = false;

	if (r != pkg::SR_Success)
	{
		m_icon = true;
		m_tex = m_outer->View().errorIcon;
		return TR_Complete;
	}

	m_tex = r::GLTextureAsset::CreateThumbnail(
		m_asset, 
		0,
		r::TX_WrapAll|r::TX_Filter,
		m_w,
		m_h
	);

	m_asset.reset();

	if (m_tex)
	{
		m_icon = false;
	}
	else
	{
		m_icon = true;
		m_tex = m_outer->View().errorIcon;
	}

	return TR_Complete;
}

bool TextureThumb::Thumbnail::CheckActivate() const
{
	return m_load;
}

void TextureThumb::Thumbnail::Cancel()
{
	if (m_asset)
	{
		m_asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Cancel
		);
	}
}

///////////////////////////////////////////////////////////////////////////////

void TextureThumb::New(ContentBrowserView &view)
{
	TextureThumb *t = new (ZEditor) TextureThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, AT_Texture);
}

void TextureThumb::NotifyAddRemovePackages()
{
//	CancelThumbs();
}

void TextureThumb::NotifyAddRemoveContent(const pkg::IdVec &, const pkg::IdVec &)
{
//	CancelThumbs();
}

void TextureThumb::NotifyContentChanged(const ContentChange::Vec &changed)
{
	for (ContentChange::Vec::const_iterator it = changed.begin(); it != changed.end(); ++it)
	{
		const ContentChange &c = *it;
		if (c.entry->type != asset::AT_Texture)
			continue;

		// I could actually do a Process(P_PropChange) but that's more involved
		// than just reloading the entire file and binding the new properties
		// that way.
		CancelThumb(c.entry->id);

		// they changed a file, rebuild the property grid.
		if (c.key && c.key->def && (c.key->def->style&pkg::K_File))
			ContentPropertyGrid::Rebuild();
	}
}

void TextureThumb::CancelThumb(int id)
{
	Thumbnail::Ref t = FindThumbnail(id);
	if (t)
		t->Evict();
}

void TextureThumb::Tick(float t, const xtime::TimeSlice &time)
{
	bool vis = false;
	bool notify = m_cache.Tick(t, time, vis);
	xtime::TimeVal now = xtime::ReadMilliseconds();
	if (notify || (vis && (now-m_lastDraw) > 1000/5))
	{
		m_lastDraw = now;
		ThumbChanged();
	}
}

void TextureThumb::Begin()
{
	m_cache.BeginRefresh();
}

void TextureThumb::End()
{
	m_cache.EndRefresh();
}

TextureThumb::Thumbnail::Ref TextureThumb::FindThumbnail(int id)
{
	ContentThumbCache::Item::Ref i = m_cache.Find(id);
	if (i)
		return boost::static_pointer_cast<Thumbnail>(i);
	return Thumbnail::Ref();
}

TextureThumb::Thumbnail::Ref TextureThumb::RequestThumb(int id, int w, int h)
{
	Thumbnail::Ref t = FindThumbnail(id);
	if (!t)
	{
		t.reset(new Thumbnail(id, this));
		m_cache.Insert(boost::static_pointer_cast<ContentThumbCache::Item>(t));
	}

	t->Request(w, h);
	t->MarkVisible();
	return t;
}

void TextureThumb::Release()
{
	CancelThumbs();
}

void TextureThumb::CancelThumbs()
{
	m_cache.EvictAll();
	m_sizes.clear();
}

void TextureThumb::Dimensions(const pkg::Package::Entry::Ref &entry, int &w, int &h)
{
	ContentAssetThumb::Dimensions(entry, w, h); // default

	ItemSizeMap::const_iterator it = m_sizes.find(entry->id);
	if (it != m_sizes.end())
	{
		w = it->second.w;
		h = it->second.h;
	}
	else
	{
		pkg::Asset::Ref asset = Packages()->Asset(entry->id, pkg::Z_ContentBrowser);
		TextureParser::Ref t = TextureParser::Cast(asset);
		if (t)
		{
			if (!t->headerValid)
			{
				// cache image header for dimensions.
				asset->Process(
					xtime::TimeSlice::Infinite,
					pkg::P_Info|pkg::P_Unformatted|pkg::P_NoDefaultMedia
				);
			}

			if (t->headerValid)
			{
				w = t->header->width;
				h = t->header->height;
				ItemSize s;
				s.w = w;
				s.h = h;
				RAD_VERIFY(m_sizes.insert(ItemSizeMap::value_type(asset->id, s)).second);
			}
		}
	}
}

inline float Wrap(float r)
{
	float z = floorf(r);
	int side = (int)z;

	r -= z;

	if (side&1)
	{
		return -1.0f + r*2;
	}

	return 1.0f - r*2;
}

bool TextureThumb::Render(const pkg::Package::Entry::Ref &asset, int x, int y, int w, int h)
{
	Thumbnail::Ref t = RequestThumb(asset->id, w, h);
	if (t)
	{
		gl.Color4f(1.f, 1.f, 1.f, 1.f, true); // force=true, who knows what Qt does behind our backs.
		gls.DisableTextures();
		gls.Set(DT_Disable|DWM_Disable|CFM_None|CWM_RGBA, BMS_SrcAlpha|BMD_InvSrcAlpha);
		gls.SetTexture(0, t->texture);
		gls.Commit();

		gl.MatrixMode(GL_MODELVIEW);
		gl.PushMatrix();
		gl.LoadIdentity();

		int tw;
		int th;

		if (t->isIcon)
		{
			tw = w / 3 * 2;
			th = h / 3 * 2;
		}
		else
		{
			tw = w; // scale.
			th = h;
		}

		gl.Translatef(x + w/2, y + h/2, 0.0f);
		if (t->active && t->isIcon)
		{
			gl.Rotatef(Wrap(t->rot)*20.0f, 0.0f, 0.0f, 1.0f);
		}

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

		gls.Set(0, BM_Off);
		gls.Commit();
	}

	return true;
}

void TextureThumb::OpenEditor(const pkg::Package::Entry::Ref &asset)
{
}

void CreateTextureThumb(ContentBrowserView &view)
{
	TextureThumb::New(view);
}

} // editor
} // tools

