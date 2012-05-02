// EditorContentBrowserModel.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../../../Packages/Packages.h"
#include "EditorContentBrowserDef.h"
#include <QtCore/QAbstractItemModel>
#include <QtGui/QIcon>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class ContentBrowserWindow;
class RADENG_CLASS ContentBrowserModel : public QAbstractItemModel
{
	Q_OBJECT
public:

	ContentBrowserModel(bool typesOnly, QObject *parent = 0);
	virtual ~ContentBrowserModel();

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	QModelIndex IndexForAssetId(int id) const;
	QModelIndex IndexForPackage(const pkg::Package::Ref &pkg) const;
	QModelIndex IndexForType(asset::Type type) const;
	QModelIndex IndexForPackageType(const pkg::Package::Ref &pkg, asset::Type type) const;
	
	bool IsPackage(const QModelIndex &index) const;
	bool IsAsset(const QModelIndex &index) const;
	bool IsPackageType(const QModelIndex &index) const;
	bool IsType(const QModelIndex &index) const;
	pkg::Package::Ref PackageForIndex(const QModelIndex &index) const;
	pkg::Package::Entry::Ref PkgEntryForIndex(const QModelIndex &index) const;
	pkg::Package::Ref TypeForIndex(const QModelIndex &index, asset::Type &type) const;
	asset::Type TypeForIndex(const QModelIndex &index) const;
	int IdForIndex(const QModelIndex &index) const;

private:

	friend class ContentBrowserWindow;

	static void NotifyAddRemovePackages();
	static void NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed);
	static void NotifyContentChanged(const ContentChange::Vec &changed);
	void OnNotifyAddRemovePackages();
	void OnNotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed);
	void OnNotifyContentChanged(const ContentChange::Vec &changed);

	struct Entry
	{ // we turn pkg::StringIdMap into this
	  // makes for faster fetching for item model
		int id;
		asset::Type type;
		String name;
	};
	
	typedef zone_vector<Entry, ZEditorT>::type EntryVec;
	typedef zone_vector<int, ZEditorT>::type IdxVec;

	struct Package
	{
		Package() : cached(false) {}
		bool cached;
		EntryVec types[asset::AT_Max];
		IdxVec dir;
		pkg::Package::WRef pkg;
		int NumTypes() const;
	};

	enum
	{
		PkgFlag   = 0x80000000,
		TypeFlag  = 0x40000000,
		ModelFlags = PkgFlag|TypeFlag,
		PkgShift  = 19,
		PkgMask   = 0x3ff80000,
		AssetMask = 0x0007ffff,
		TypeMask  = 0x0fffffff
	};

	quint32 PkgModelId(int idx) const;
	int PkgIdx(qint64 mid) const;
	quint32 AssetModelId(int pkg, int idx) const;
	void AssetIdx(qint64 mid, int &pkg, int &id) const;
	quint32 TypeModelId(int type) const;
	int TypeIdx(qint64 mid) const;

	Package *_PackageForIndex(const QModelIndex &index) const;
	Entry   *EntryForIndex(const QModelIndex &index) const;

	typedef zone_vector<Package, ZEditorT>::type PackageVec;	
	typedef zone_set<ContentBrowserModel*, ZEditorT>::type ModelSet;

	void CachePackage(Package &p) const;
	void LoadPackages(PackageVec &v) const;
	int PackageIdx(const pkg::Package *pkg) const;
	int AssetIdx(const Package &pkg, int id) const;
	void UncacheAssetPackage(const pkg::Package::Entry::Ref &entry);

	bool m_typesOnly;
	QIcon m_ipkg;
	QIcon m_itype[asset::AT_Max];
	mutable PackageVec m_pkgs;
	static ModelSet s_set;
};
	
} // editor
} // tools

#include <Runtime/PopPack.h>
#include "EditorContentBrowserModel.inl"
