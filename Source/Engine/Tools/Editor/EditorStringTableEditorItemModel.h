// EditorStringTableEditorItemModel.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../StringTable.h"
#include <QtCore/QAbstractItemModel>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

class QIcon;

namespace tools {
namespace editor {

class StringTableEditorItemModel : public QAbstractItemModel {
	Q_OBJECT
public:

	StringTableEditorItemModel(
		StringTable &stringTable,
		StringTable::LangId langId,
		bool editable,
		const QIcon *icons,
		QObject *parent = 0
	);

	void Load();

	virtual QModelIndex index(int row, int column, const QModelIndex &parent) const {
		return createIndex(row, column);
	}

	virtual QModelIndex parent(const QModelIndex &index) const {
		return QModelIndex();
	}

	virtual int rowCount(const QModelIndex &parent) const {
		return (int)m_stringVec.size();
	}

	virtual int columnCount(const QModelIndex &parent) const {
		return 3;
	}

	virtual QVariant data(const QModelIndex &index, int role) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role);
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role) const;
	
	const StringTable::Entry::Map::const_iterator *IteratorForIndex(const QModelIndex &index) const;
	int RowForId(const char *id) const;
	QModelIndex IndexForRow(int row) const;
	void DeleteItem(const QModelIndex &index);
	void DeleteItems(const QModelIndexList &indices);

	RAD_DECLARE_PROPERTY(StringTableEditorItemModel, langId, StringTable::LangId, StringTable::LangId);

signals:

	void ItemRenamed(const QModelIndex &index);

private:

	typedef zone_vector<StringTable::Entry::Map::const_iterator, ZEditorT>::type Vec;
	typedef zone_map<String, int, ZEditorT>::type Map;

	RAD_DECLARE_GET(langId, StringTable::LangId) {
		return m_langId;
	}

	RAD_DECLARE_SET(langId, StringTable::LangId);

	Vec m_stringVec;
	Map m_stringMap;
	StringTable::LangId m_langId;
	StringTable &m_stringTable;
	const QIcon *m_icons;
	bool m_editable;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
