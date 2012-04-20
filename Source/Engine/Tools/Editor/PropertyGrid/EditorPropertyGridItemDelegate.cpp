// EditorPropertyGridItemDelegate.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorProperty.h"
#include "EditorPropertyGrid.h"
#include "EditorPropertyGridItemDelegate.h"
#include <QtCore/QSignalMapper>
#include <QtGui/QAbstractProxyModel>

namespace tools {
namespace editor {

PropertyGridItemDelegate::PropertyGridItemDelegate(QObject *parent)
: QItemDelegate(parent)
{
	m_sigMap = new QSignalMapper(this);
	connect(m_sigMap, SIGNAL(mapped(QWidget*)), this, SIGNAL(commitData(QWidget*)));
	connect(m_sigMap, SIGNAL(mapped(QWidget*)), this, SIGNAL(closeEditor(QWidget*)));
}

PropertyGridItemDelegate::~PropertyGridItemDelegate()
{
}

QWidget *PropertyGridItemDelegate::MapEditFinished(QWidget *editor) const
{
	if (editor && editor->metaObject()->indexOfSignal("CloseEditor()") != -1)
	{
		connect(editor, SIGNAL(CloseEditor()), m_sigMap, SLOT(map()));
		m_sigMap->setMapping(editor, editor);
	}

	return editor;
}

QModelIndex PropertyGridItemDelegate::MapIndex(const QModelIndex &index) const
{
	PropertyGrid *grid = static_cast<PropertyGrid*>(parent());
	RAD_ASSERT(grid);
	return static_cast<QAbstractProxyModel*>(grid->model())->mapToSource(index);
}

QWidget *PropertyGridItemDelegate::createEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index
) const
{
	QModelIndex _index = MapIndex(index);
	RAD_ASSERT(_index.isValid());
	QWidget *w = reinterpret_cast<Property*>(_index.internalPointer())->CreateEditor(
		parent,
		option,
		index,
		const_cast<PropertyGridItemDelegate&>(*this)
	);
	if (!w)
		w = QItemDelegate::createEditor(parent, option, index);
	return MapEditFinished(w);
}

void PropertyGridItemDelegate::setEditorData(
	QWidget *editor, 
	const QModelIndex &index
) const
{
	QModelIndex _index = MapIndex(index);
	RAD_ASSERT(_index.isValid());

	m_sigMap->blockSignals(true);
	bool r = reinterpret_cast<Property*>(_index.internalPointer())->SetEditorData(
		*editor,
		index,
		false,
		const_cast<PropertyGridItemDelegate&>(*this)
	);
	if (!r)
		QItemDelegate::setEditorData(editor, index);
	m_sigMap->blockSignals(false);
}

void PropertyGridItemDelegate::setModelData(
	QWidget *editor, 
	QAbstractItemModel *model, 
	const QModelIndex &index
) const
{
	QModelIndex _index = MapIndex(index);
	RAD_ASSERT(_index.isValid());

	bool r = reinterpret_cast<Property*>(_index.internalPointer())->Validate(
		*editor,
		*model,
		index,
		const_cast<PropertyGridItemDelegate&>(*this)
	);

	if (!r)
		return;

	r = reinterpret_cast<Property*>(_index.internalPointer())->SetModelData(
		*editor,
		*model,
		index,
		const_cast<PropertyGridItemDelegate&>(*this)
	);

	if (!r)
		QItemDelegate::setModelData(editor, model, index);
}

void PropertyGridItemDelegate::updateEditorGeometry(
	QWidget *editor,
	const QStyleOptionViewItem &option,
	const QModelIndex &index
) const
{
	QModelIndex _index = MapIndex(index);
	RAD_ASSERT(_index.isValid());

	if (!editor)
		return;

	bool r = reinterpret_cast<Property*>(_index.internalPointer())->UpdateEditorGeometry(
		*editor,
		option,
		index,
		const_cast<PropertyGridItemDelegate&>(*this)
	);

	if (!r)
		QItemDelegate::updateEditorGeometry(editor, option, index);
}

} // editor
} // tools

#include "moc_EditorPropertyGridItemDelegate.cc"
