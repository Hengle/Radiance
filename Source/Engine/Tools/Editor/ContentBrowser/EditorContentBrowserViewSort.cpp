// EditorContentBrowserViewSort.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentBrowserView.h"
#include "../EditorUtils.h"
#include <Runtime/Container/ZoneMap.h>
#include <algorithm>

namespace tools {
namespace editor {

void ContentBrowserView::BuildAssetList()
{
	switch (m_filter.sort)
	{
	case SortName:
		_SortName(); break;
	case SortSize:
	case SortSizeName:
		_SortSize(m_filter.sort == SortSizeName); break;
	}
}

pkg::Package::Entry::Ref ContentBrowserView::Filter::Begin()
{
	char sz[256];

	aIt = assets.begin();

	if (pkgs.empty())
	{
		mpkgs  = &Packages()->packages.get();
		mpkgIt = mpkgs->begin();
		if (mpkgIt != mpkgs->end())
		{
			do
			{
				mids = &mpkgIt->second->dir.get();
				for (midIt = mids->begin(); midIt != mids->end(); ++midIt)
				{
					pkg::Package::Entry::Ref asset = mpkgIt->second->FindEntry(midIt->second);
					if (types[asset->type])
					{
						if (!filter.empty)
						{
							string::cpy(sz, asset->path.get());
							string::tolower(sz);
							if (!string::strstr(sz, filter.c_str.get()))
								continue;
						}
						return asset;
					}
				}
				++mpkgIt;
			} while (mpkgIt != mpkgs->end());
		}
	}
	else
	{
		vpkgIt = pkgs.begin();
		if (vpkgIt != pkgs.end())
		{
			do
			{
				mids = &(*vpkgIt).pkg->dir.get();
				for (midIt = mids->begin(); midIt != mids->end(); ++midIt)
				{
					pkg::Package::Entry::Ref asset = (*vpkgIt).pkg->FindEntry(midIt->second);
					if ((*vpkgIt).types[asset->type] && types[asset->type])
					{
						if (!filter.empty)
						{
							string::cpy(sz, asset->path.get());
							string::tolower(sz);
							if (!string::strstr(sz, filter.c_str.get()))
								continue;
						}
						return asset;
					}
				}
				++vpkgIt;
			} while (vpkgIt != pkgs.end());
		}
		else
		{
			if (aIt != assets.end())
			{
				pkg::Package::Entry::Ref asset = Packages()->FindEntry(*aIt);
				++aIt;
				return asset;
			}
		}
	}

	return pkg::Package::Entry::Ref();
}

pkg::Package::Entry::Ref ContentBrowserView::Filter::Next()
{
	char sz[256];

	if (pkgs.empty())
	{
		if (mpkgIt != mpkgs->end())
		{
			++midIt;

			do
			{
				for (; midIt != mids->end(); ++midIt)
				{
					pkg::Package::Entry::Ref asset = mpkgIt->second->FindEntry(midIt->second);
					if (types[asset->type])
					{
						if (!filter.empty)
						{
							string::cpy(sz, asset->path.get());
							string::tolower(sz);
							if (!string::strstr(sz, filter.c_str.get()))
								continue;
						}
						return asset;
					}
				}

				++mpkgIt;
				if (mpkgIt != mpkgs->end())
				{
					mids = &mpkgIt->second->dir.get();
					midIt = mids->begin();
				}

			} while (mpkgIt != mpkgs->end());
		}
	}
	else
	{
		if (vpkgIt != pkgs.end())
		{
			++midIt;

			do
			{
				for (; midIt != mids->end(); ++midIt)
				{
					pkg::Package::Entry::Ref asset = (*vpkgIt).pkg->FindEntry(midIt->second);
					if ((*vpkgIt).types[asset->type] && types[asset->type])
					{
						if (!filter.empty)
						{
							string::cpy(sz, asset->path.get());
							string::tolower(sz);
							if (!string::strstr(sz, filter.c_str.get()))
								continue;
						}
						return asset;
					}
				}

				++vpkgIt;
				if (vpkgIt != pkgs.end())
				{
					mids = &(*vpkgIt).pkg->dir.get();
					midIt = mids->begin();
				}

			} while (vpkgIt != pkgs.end());
		}

		if (aIt != assets.end())
		{
			pkg::Package::Entry::Ref asset = Packages()->FindEntry(*aIt);
			++aIt;
			return asset;
		}
	}

	return pkg::Package::Entry::Ref();
}

struct _name_less
{
	bool operator() (int ida, int idb) const
	{
		pkg::Package::Entry::Ref a = Packages()->FindEntry(ida);
		pkg::Package::Entry::Ref b = Packages()->FindEntry(idb);
		return string::cmp<char>(a->path, b->path) < 0;
	}
};

void ContentBrowserView::_SortName()
{
	m_assets.clear();
	m_assets.reserve(1024);
	for (pkg::Package::Entry::Ref asset = m_filter.Begin(); asset; asset = m_filter.Next())
	{
		m_assets.push_back(asset->id);
	}

	std::sort(m_assets.begin(), m_assets.end(), _name_less());
}

struct _size_less
{
	bool operator() (const pkg::Package::Entry::Ref &a, const pkg::Package::Entry::Ref &b) const
	{
		return string::cmp<char>(a->path, b->path) < 0;
	}
};

void ContentBrowserView::_SortSize(bool sortName)
{
	m_assets.clear();
	typedef zone_map<int, pkg::Package::Entry::Vec, ZEditorT>::type SizeXMap;
	typedef zone_map<int, SizeXMap, ZEditorT>::type SizeYMap;

	SizeYMap ymap;
	for (pkg::Package::Entry::Ref entry = m_filter.Begin(); entry; entry = m_filter.Next())
	{
		int w = 256; 
		int h = 256;
		ContentAssetThumb *thumb = ThumbForType(entry->type);
		if (thumb)
		{
			thumb->Dimensions(entry, w, h);
		}
		if (w <=0 || h <=0) continue;
		SizeXMap &xmap = ymap[h];
		xmap[w].push_back(entry);
	}

	if (sortName)
	{
		for (SizeYMap::iterator ys = ymap.begin(); ys != ymap.end(); ++ys)
		{
			for (SizeXMap::iterator xs = ys->second.begin(); xs != ys->second.end(); ++xs)
			{
				std::sort(xs->second.begin(), xs->second.end(), _size_less());
			}
		}
	}

	for (SizeYMap::const_iterator ys = ymap.begin(); ys != ymap.end(); ++ys)
	{
		for (SizeXMap::const_iterator xs = ys->second.begin(); xs != ys->second.end(); ++xs)
		{
			m_assets.reserve(m_assets.size()+xs->second.size());
			for (pkg::Package::Entry::Vec::const_iterator it = xs->second.begin(); it != xs->second.end(); ++it)
			{
				m_assets.push_back((*it)->id);
			}
		}
	}
}

} // editor
} // tools
