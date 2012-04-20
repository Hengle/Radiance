// EditorContentBrowserModel.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace tools {
namespace editor {

inline bool ContentBrowserModel::IsPackage(const QModelIndex &index) const
{
	return index.isValid() && ((index.internalId()&ModelFlags)==PkgFlag);
}

inline bool ContentBrowserModel::IsAsset(const QModelIndex &index) const
{
	return index.isValid() && ((index.internalId()&ModelFlags)==0);
}

inline bool ContentBrowserModel::IsPackageType(const QModelIndex &index) const
{
	return index.isValid() && ((index.internalId()&ModelFlags)==(TypeFlag|PkgFlag));
}

inline bool ContentBrowserModel::IsType(const QModelIndex &index) const
{
	return index.isValid() && ((index.internalId()&ModelFlags)==TypeFlag);
}

inline pkg::Package::Ref ContentBrowserModel::PackageForIndex(const QModelIndex &index) const
{
	Package *pkg = _PackageForIndex(index);
	return pkg ? pkg->pkg.lock() : pkg::Package::Ref();
}

inline int ContentBrowserModel::IdForIndex(const QModelIndex &index) const
{
	Entry *e = EntryForIndex(index);
	return e ? e->id : -1;
}

inline pkg::Package::Ref ContentBrowserModel::TypeForIndex(const QModelIndex &index, asset::Type &type) const
{
	if (!IsPackageType(index))
		return pkg::Package::Ref();

	int pkg, idx;
	AssetIdx(index.internalId(), pkg, idx);
	RAD_ASSERT(pkg >= 0 && pkg < (int)m_pkgs.size());
	RAD_ASSERT(idx >= 0 && idx < asset::AT_Max);
	pkg::Package::Ref ref = m_pkgs[pkg].pkg.lock();
	type = (asset::Type)idx;
	return ref;
}

inline asset::Type ContentBrowserModel::TypeForIndex(const QModelIndex &index) const
{
	return (asset::Type)TypeIdx(index.internalId());
}

inline quint32 ContentBrowserModel::PkgModelId(int idx) const
{
	return static_cast<quint32>(idx<<PkgShift) | PkgFlag;
}

inline int ContentBrowserModel::PkgIdx(qint64 mid) const
{
	return static_cast<int>((mid&PkgMask)>>PkgShift);
}

inline quint32 ContentBrowserModel::AssetModelId(int pkg, int idx) const
{
	return (static_cast<quint32>(pkg)<<PkgShift) | static_cast<quint32>(idx);
}

inline void ContentBrowserModel::AssetIdx(qint64 mid, int &pkg, int &id) const
{
	pkg = PkgIdx(mid);
	id  = static_cast<int>(mid&AssetMask);
}

inline quint32 ContentBrowserModel::TypeModelId(int type) const
{
	return static_cast<quint32>(type) | TypeFlag;
}

inline int ContentBrowserModel::TypeIdx(qint64 mid) const
{
	return static_cast<int>(mid&TypeMask);
}
	
} // editor
} // tools

