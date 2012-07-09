// EditorPropertyGrid.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorPropertyGrid.h"
#include "EditorPropertyGridModel.h"
#include "EditorPropertyGridItemDelegate.h"
#include <QtGui/QHeaderView>
#include <QtGui/QSortFilterProxyModel>

namespace tools {
namespace editor { 

PropertyGrid::PropertyGrid(bool editable, QWidget *parent)
: QTreeView(parent)
{
	setEditTriggers(
		editable ?
		QAbstractItemView::DoubleClicked : 
		QAbstractItemView::NoEditTriggers
	);
	setSelectionBehavior(QAbstractItemView::SelectRows);
	setSelectionMode(QAbstractItemView::SingleSelection);
	setAllColumnsShowFocus(true);
	setIndentation(10);
	setSortingEnabled(true);
	header()->setSortIndicator(0, Qt::AscendingOrder);
	
	QSortFilterProxyModel *proxy = new QSortFilterProxyModel(this);
	proxy->setSourceModel(new PropertyGridModel(this));
	setModel(proxy);

	setItemDelegate(new PropertyGridItemDelegate(this));
}

inline PropertyGridModel *PropertyGrid::Model() const
{
	return static_cast<PropertyGridModel*>(static_cast<QSortFilterProxyModel*>(model())->sourceModel());
}

void PropertyGrid::Add(Property *p)
{
	Model()->Add(p);
}

void PropertyGrid::Remove(Property *p)
{
	Model()->Remove(p);
}

void PropertyGrid::Add(const PropertyList &list)
{
	foreach(Property *p, list)
	{
		Add(p);
	}
}

void PropertyGrid::Remove(const PropertyList &list)
{
	foreach(Property *p, list)
	{
		Remove(p);
	}
}

void PropertyGrid::Clear()
{
	Model()->Clear();
}

Property *PropertyGrid::PropertyForIndex(const QModelIndex &index) const
{
	return Model()->PropertyForIndex(index);
}

Property *PropertyGrid::Selection() const
{
	QModelIndexList list = selectedIndexes();
	if (list.isEmpty())
		return 0;
	return PropertyForIndex(static_cast<QSortFilterProxyModel*>(model())->mapToSource(list.first()));
}

QModelIndex PropertyGrid::FindIndex(const QString &name) const
{
	return Model()->FindIndex(name);
}

void PropertyGrid::SetSelection(const QModelIndex &index)
{
	setCurrentIndex(static_cast<QSortFilterProxyModel*>(model())->mapFromSource(index));
}

} // editor
} // tools

#include "moc_EditorPropertyGrid.cc"
