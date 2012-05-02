// EditorContentBrowserModel.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorContentBrowserModel.h"
#include "EditorContentBrowserWindow.h"
#include "../EditorUtils.h"
#include <QtGui/QMessageBox>

namespace tools {
namespace editor {

namespace {

const wchar_t *s_typeIcons[asset::AT_Max] =
{
	L"Editor/texture_small.png",
	L"Editor/material_small.png",
	L"Editor/level_small.png",
	L"Editor/skmodel_small.png",
	L"Editor/skanimset_small.png",
	L"Editor/skanimstates_small.png",
	L"Editor/mesh_small.png",
	L"Editor/sound_small.png",
	L"Editor/music_small.png",
	L"Editor/font_small.png",
	L"Editor/typeface_small.png",
	L"Editor/stringtable_small.png"
};

} // namespace

ContentBrowserModel::ModelSet ContentBrowserModel::s_set;

ContentBrowserModel::ContentBrowserModel(bool typesOnly, QObject *parent)
: QAbstractItemModel(parent), m_typesOnly(typesOnly)
{
	s_set.insert(this);
	m_ipkg = LoadIcon(L"Editor/package_small.png");
	for (int i = 0; i < asset::AT_Max; ++i)
	{
		if (s_typeIcons[i])
		{
			m_itype[i] = LoadIcon(s_typeIcons[i]);
		}
		else
		{
			m_itype[i] = LoadIcon(L"Editor/error_small.png");
		}
	}
}

ContentBrowserModel::~ContentBrowserModel()
{
	s_set.erase(this);
}

int ContentBrowserModel::columnCount(const QModelIndex &parent) const
{
	return 1;
}

QVariant ContentBrowserModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid()) 
		return QVariant();

	if (role == Qt::DisplayRole || role == Qt::EditRole)
	{
		if (IsType(index))
		{
			QString str(asset::TypeString((asset::Type)TypeIdx(index.internalId())));
			if (!str.endsWith('s', Qt::CaseInsensitive))
				str += "s";
			return str;
		}
		else if (IsPackage(index))
		{
			const Package *pkg = _PackageForIndex(index);
			return QString(pkg->pkg.lock()->name);
		}
		else if (IsAsset(index))
		{
			const Entry *item = EntryForIndex(index);
			return QString(item->name.c_str());
		}
		else
		{
			int pkg, idx;
			AssetIdx(index.internalId(), pkg, idx);
			QString str(asset::TypeString((asset::Type)idx));
			if (!str.endsWith('s', Qt::CaseInsensitive))
				str += "s";
			return str;
		}
	}
	else if (role == Qt::DecorationRole)
	{
		if (IsType(index))
		{
			int idx = TypeIdx(index.internalId());
			return QVariant(m_itype[idx]);
		}
		else if (IsPackage(index))
		{
			return QVariant(m_ipkg);
		}
		else if (IsPackageType(index))
		{
			int pkg, idx;
			AssetIdx(index.internalId(), pkg, idx);
			return QVariant(m_itype[idx]);
		}
	}

	return QVariant();
}

bool ContentBrowserModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	if (role == Qt::EditRole && value.isValid())
	{
		String name(value.toString().toAscii().constData());

		if (IsPackage(index))
		{
			pkg::Package::Ref pkg = PackageForIndex(index);
			RAD_ASSERT(pkg);

			if (name == String(pkg->name))
				return false;

			if (!pkg->Rename(name.c_str()))
			{
				QMessageBox::critical(
					static_cast<QWidget*>(QObject::parent())->parentWidget(),
					"Error",
					"A package with that name already exists."
				);
			}
			else
			{
				ContentBrowserWindow::NotifyAddRemovePackages();
			}
		}
		else if (IsAsset(index))
		{
			pkg::Package::Entry::Ref asset = PkgEntryForIndex(index);
			RAD_ASSERT(asset);

			if (name == String(asset->name))
				return false;

			if (!asset->Rename(name.c_str()))
			{
				QMessageBox::critical(
					static_cast<QWidget*>(QObject::parent())->parentWidget(),
					"Error",
					"An asset with that name already exists."
				);
			}
			else
			{
				ContentChange::Vec changed;
				changed.push_back(ContentChange(asset));
				ContentBrowserWindow::NotifyContentChanged(changed);
				emit dataChanged(index, index);
			}
		}
	}

	return false;
}

QModelIndex ContentBrowserModel::index(int row, int column, const QModelIndex &parent) const
{
	if (column != 0) 
		return QModelIndex();

	if (parent.isValid())
	{
		if (IsPackage(parent))
		{
			if (row<0||row>=(int)asset::AT_Max)
			{
				return QModelIndex();
			}

			return createIndex(
				row, 
				column, 
				AssetModelId(
					PkgIdx(parent.internalId()),
					row
				) | PkgFlag | TypeFlag
			);
		}
	
		RAD_ASSERT(IsPackageType(parent));
		
		return createIndex(
			row, 
			column, 
			AssetModelId(
				PkgIdx(parent.internalId()),
				row
			)
		);
	}
	else
	{
		if (row < asset::AT_Max)
		{
			return createIndex(
				row,
				column,
				TypeModelId(row)
			);
		}
		
		return createIndex(
			row,
			column,
			PkgModelId(row-asset::AT_Max)
		);
	}
}

QModelIndex ContentBrowserModel::parent(const QModelIndex &index) const
{
	if (index.isValid() && !IsPackage(index) && !IsType(index))
	{
		int pkg, idx;
		AssetIdx(index.internalId(), pkg, idx);
		RAD_ASSERT(pkg>=0&&idx>=0);

		if (IsPackageType(index))
		{
			quint32 flags = PkgModelId(pkg);
			return createIndex(pkg, 0, flags);
		}
		else
		{
			Entry *e = EntryForIndex(index);
			quint32 flags = AssetModelId(pkg, e->type);
			return createIndex(e->type, 0, flags | PkgFlag | TypeFlag);
		}
	}

	return QModelIndex();
}

Qt::ItemFlags ContentBrowserModel::flags(const QModelIndex &index) const
{
	Qt::ItemFlags f = QAbstractItemModel::flags(index);
	if (IsPackage(index) || IsAsset(index))
	{
		f |= Qt::ItemIsEditable;
	}
	return f;
}

int ContentBrowserModel::rowCount(const QModelIndex &parent) const
{
	if (parent.isValid())
	{
		if (IsPackage(parent))
		{
			return asset::AT_Max;
		}
		else if (IsPackageType(parent) && !m_typesOnly)
		{
			int pkg, idx;
			AssetIdx(parent.internalId(), pkg, idx);
			RAD_ASSERT(pkg >= 0 && pkg < (int)m_pkgs.size());
			RAD_ASSERT(idx >= 0 && idx < asset::AT_Max);
			return (int)m_pkgs[pkg].types[idx].size();
		}
		return 0;
	}
	else
	{
		if (m_pkgs.empty())
		{
			LoadPackages(m_pkgs);
		}

		return (int)m_pkgs.size() + asset::AT_Max;
	}
}

QModelIndex ContentBrowserModel::IndexForAssetId(int id) const
{
	pkg::Package::Entry::Ref ref = Packages()->FindEntry(id);
	if (!ref)
		return QModelIndex();

	pkg::Package::Ref pkg = ref->pkg;
	RAD_ASSERT(pkg);
	int pkgIdx = PackageIdx(pkg.get());
	RAD_ASSERT(pkgIdx>=0);
	CachePackage(m_pkgs[pkgIdx]);
	int row = AssetIdx(m_pkgs[pkgIdx], id);
	RAD_ASSERT(row>=0);
	quint32 flags = AssetModelId(pkgIdx, row);

	return createIndex(row, 0, flags);
}

QModelIndex ContentBrowserModel::IndexForPackage(const pkg::Package::Ref &pkg) const
{
	int idx = PackageIdx(pkg.get());
	RAD_ASSERT(idx>=0);
	return createIndex(idx+asset::AT_Max, 0, PkgModelId(idx));
}

QModelIndex ContentBrowserModel::IndexForType(asset::Type type) const
{
	return createIndex(type, 0, TypeModelId(type));
}

QModelIndex ContentBrowserModel::IndexForPackageType(const pkg::Package::Ref &pkg, asset::Type type) const
{
	int idx = PackageIdx(pkg.get());
	RAD_ASSERT(idx>=0);
	return createIndex(
		type, 
		0, 
		AssetModelId(
			idx,
			type
		) | PkgFlag | TypeFlag
	);
}

void ContentBrowserModel::CachePackage(Package &p) const
{
	if (!p.cached)
	{
		p.cached = true;
		pkg::Package::Ref pkg = p.pkg.lock();
		if (pkg)
		{
			const pkg::StringIdMap &dir = pkg->dir;
			for (pkg::StringIdMap::const_iterator it = dir.begin(); it != dir.end(); ++it)
			{
				Entry x;
				x.id = it->second;
				x.name = it->first;
				pkg::Asset::Ref asset = Packages()->Asset(x.id, pkg::Z_ContentBrowser);
				if (asset)
				{
					x.type = asset->type;
					p.types[x.type].push_back(x);
					int idx = (int)p.types[x.type].size()-1;
					p.dir.push_back(idx | (x.type<<24));
				}
			}
		}
	}
}

void ContentBrowserModel::LoadPackages(PackageVec &v) const
{
	v.clear();
	const pkg::Package::Map &pkgs = Packages()->packages;
	for (pkg::Package::Map::const_iterator it = pkgs.begin(); it != pkgs.end(); ++it)
	{
		Package p;
		p.pkg = it->second;
		v.push_back(p);
	}
}

int ContentBrowserModel::PackageIdx(const pkg::Package *pkg) const
{
	int i = 0;
	for (PackageVec::const_iterator it = m_pkgs.begin(); it != m_pkgs.end(); ++it, ++i)
	{
		const Package &x = *it;
		pkg::Package::Ref ref = x.pkg.lock();
		if (ref && ref.get() == pkg)
			return i;
	}
	
	return -1;
}

int ContentBrowserModel::AssetIdx(const Package &pkg, int id) const
{
	int i = 0;
	for (IdxVec::const_iterator it = pkg.dir.begin(); it != pkg.dir.end(); ++it, ++i)
	{
		int idx = *it;
		int type = idx >> 24;
		idx &= 0x00ffffff;

		if (pkg.types[type][idx].id == id) 
			return i;
	}
	return -1;
}

ContentBrowserModel::Package *ContentBrowserModel::_PackageForIndex(const QModelIndex &index) const
{
	if (!IsPackage(index))
		return 0;

	int pkg = PkgIdx(index.internalId());
	RAD_ASSERT(pkg>=0&&pkg<(int)m_pkgs.size());
	return &m_pkgs[pkg];
}

ContentBrowserModel::Entry *ContentBrowserModel::EntryForIndex(const QModelIndex &index) const
{
	if (!IsAsset(index))
		return 0;

	int pkg, idx;
	AssetIdx(index.internalId(), pkg, idx);
	RAD_ASSERT(pkg>=0&&idx>=0);
	CachePackage(m_pkgs[pkg]);
	RAD_ASSERT(pkg<(int)m_pkgs.size());
	RAD_ASSERT(idx<(int)m_pkgs[pkg].dir.size());
	int tidx = m_pkgs[pkg].dir[idx];
	int type = tidx >> 24;
	tidx &= 0x00ffffff;

	return &m_pkgs[pkg].types[type][tidx];
}

pkg::Package::Entry::Ref ContentBrowserModel::PkgEntryForIndex(const QModelIndex &index) const
{
	Entry *e = EntryForIndex(index);
	return e ? Packages()->FindEntry(e->id) : pkg::Package::Entry::Ref();
}

void ContentBrowserModel::OnNotifyAddRemovePackages()
{
	m_pkgs.clear();
	reset();
}

void ContentBrowserModel::OnNotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
	if (m_typesOnly)
		return;
	m_pkgs.clear();
	reset();
}

void ContentBrowserModel::OnNotifyContentChanged(const ContentChange::Vec &changed)
{
	// flag packages for recache.
	for (ContentChange::Vec::const_iterator it = changed.begin(); it != changed.end(); ++it)
	{
		UncacheAssetPackage((*it).entry);
	}

	for (ContentChange::Vec::const_iterator it = changed.begin(); it != changed.end(); ++it)
	{
		QModelIndex index = IndexForAssetId((*it).entry->id);
		emit dataChanged(index, index);
	}
}

void ContentBrowserModel::UncacheAssetPackage(const pkg::Package::Entry::Ref &ref)
{
	RAD_ASSERT(ref);
	pkg::Package::Ref pkg = ref->pkg;
	RAD_ASSERT(pkg);
	int pkgIdx = PackageIdx(pkg.get());
	RAD_ASSERT(pkgIdx>=0);

	int row = AssetIdx(m_pkgs[pkgIdx], ref->id);
	if (row < 0)
		m_pkgs[pkgIdx].cached = false;
}

void ContentBrowserModel::NotifyAddRemovePackages()
{
	for (ModelSet::const_iterator it = s_set.begin(); it != s_set.end(); ++it)
	{
		(*it)->OnNotifyAddRemovePackages();
	}
}

void ContentBrowserModel::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
	for (ModelSet::const_iterator it = s_set.begin(); it != s_set.end(); ++it)
	{
		(*it)->OnNotifyAddRemoveContent(added, removed);
	}
}

void ContentBrowserModel::NotifyContentChanged(const ContentChange::Vec &changed)
{
	for (ModelSet::const_iterator it = s_set.begin(); it != s_set.end(); ++it)
	{
		(*it)->OnNotifyContentChanged(changed);
	}
}

int ContentBrowserModel::Package::NumTypes() const
{
	int n = 0;
	for (int i = 0; i < asset::AT_Max; ++i)
	{
		n += types[i].empty() ? 0 : 1;
	}
	return n;
}

} // editor
} // tools

#include "moc_EditorContentBrowserModel.cc"
