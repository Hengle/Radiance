// EditorWidgetPropertyEditors.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <QtGui/QComboBox>
#include "EditorProperty.h"
#include <Runtime/PushPack.h>

class QFocusEvent;

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS NullPropertyHints
{
	void ApplyHints(QWidget &, const Property &) {}
};

///////////////////////////////////////////////////////////////////////////////

class PropertyComboBox : public QComboBox
{
	Q_OBJECT
public:
	PropertyComboBox(QWidget *parent);

signals:

	void CloseEditor();

private slots:
	
	void OnSelectionChanged();
};

///////////////////////////////////////////////////////////////////////////////

template <typename TWidget, typename THints, typename T>
struct SpinBoxPropertyTraits : public QVariantPropertyTraits<T>
{
	typedef T Type;

	QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS FilePathFieldWidgetPropertyTraits : public QVariantPropertyTraits<QString>
{
	typedef QString Type;
	
	QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS BoolComboBoxPropertyTraits : public QVariantPropertyTraits<bool>
{
	typedef bool Type;

	QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

template <typename TWidget, typename TExtractor, typename T>
struct WidgetPropertyTraits : public QVariantPropertyTraits<T>
{
	typedef T Type;
	
	QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS LineEditPropertyTraits : public QVariantPropertyTraits<QString>
{
	typedef QString Type;

	QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

} // editor
} // tools

#include <Runtime/PopPack.h>
#include "EditorWidgetPropertyEditors.inl"
