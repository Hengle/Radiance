// EditorPropertyGridModel.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <QtCore/QAbstractItemModel>
#include <QtCore/QObjectList>
#include <QtCore/QHash>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class Property;
class PropertyGridModel : public QAbstractItemModel
{
	Q_OBJECT
public:

	PropertyGridModel(QObject *parent = 0);
	virtual ~PropertyGridModel();

	virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
	virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
	virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;
	virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
	virtual QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const;
	virtual QModelIndex parent(const QModelIndex &index) const;
	virtual Qt::ItemFlags flags(const QModelIndex &index) const;
	virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;

	void Add(Property *property);
	void Remove(Property *property);
	void Clear();

	Property *PropertyForIndex(const QModelIndex &index) const;
	QModelIndex FindIndex(const QString &name) const;

private:

	typedef QHash<QString, Property*> PHash;
	PHash m_props;
	QObject *m_root;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
