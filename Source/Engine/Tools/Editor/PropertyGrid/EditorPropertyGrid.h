// EditorPropertyGrid.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "EditorProperty.h"
#include <QtGui/QTreeView>
#include <QtCore/QList>
#include <QtCore/QModelIndex>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor { 

class PropertyGridModel;

class RADENG_CLASS PropertyGrid : public QTreeView
{
	Q_OBJECT

public:

	PropertyGrid(bool editable, QWidget *parent = 0);

	// property will be owned by the control after this call.
	void Add(Property *p);
	// remove will delete the property object.
	void Remove(Property *p);
	void Add(const PropertyList &list);
	void Remove(const PropertyList &list);
	void Clear();

	Property *Selection() const;
	QModelIndex FindIndex(const QString &name) const;
	void SetSelection(const QModelIndex &index);

protected:

	PropertyGridModel *Model() const;
	Property *PropertyForIndex(const QModelIndex &index) const;

};

} // editor
} // tools

#include <Runtime/PopPack.h>
