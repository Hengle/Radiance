// EditorPropertyGridItemDelegate.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtCore/QAbstractItemModel>
#include <QtGui/QItemDelegate>
#include <Runtime/PushPack.h>

class QSignalMapper;

namespace tools {
namespace editor {

class RADENG_CLASS PropertyGridItemDelegate : public QItemDelegate
{
	Q_OBJECT

public:

	PropertyGridItemDelegate(QObject *parent = 0);
	virtual ~PropertyGridItemDelegate();

	virtual QWidget *createEditor(
		QWidget * parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index
	) const;

	virtual void setEditorData(
		QWidget *editor, 
		const QModelIndex &index
	) const;

	virtual void setModelData(
		QWidget *editor, 
		QAbstractItemModel *model, 
		const QModelIndex &index
	) const;

	void updateEditorGeometry(
		QWidget *editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index
	) const;

	QSignalMapper *SignalMapper() const { return m_sigMap; }

private:

	QModelIndex MapIndex(const QModelIndex &index) const;

	QWidget *MapEditFinished(QWidget *editor) const;
	QSignalMapper *m_sigMap;

};

} // editor
} // tools

#include <Runtime/PopPack.h>
