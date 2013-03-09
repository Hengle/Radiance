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

struct RADENG_CLASS NullPropertyHints {
	static void ApplyHints(QWidget &, const Property &) {}
};

///////////////////////////////////////////////////////////////////////////////

class PropertyComboBox : public QComboBox {
	Q_OBJECT
public:
	PropertyComboBox(QWidget *parent);

signals:

	void CloseEditor();

private slots:
	
	void OnSelectionChanged();
};

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct ReadOnlyTraits : public QVariantPropertyTraits<T> {
	typedef T Type;
	typedef QVariantPropertyTraits<T> Super;
	typedef ReadOnlyTraits<T> Self;
	typedef TProperty<Self> PropertyType;

	static Qt::ItemFlags Flags(const Property &p) {
		return Super().Flags(p) & ~(Qt::ItemIsEditable|Qt::ItemIsEnabled);
	}
};

///////////////////////////////////////////////////////////////////////////////

template <typename TWidget, typename THints, typename T>
struct SpinBoxPropertyTraits : public QVariantPropertyTraits<T> {
	typedef T Type;
	typedef QVariantPropertyTraits<T> Super;
	typedef SpinBoxPropertyTraits<TWidget, THints, T> Self;
	typedef TProperty<Self> PropertyType;

	static QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS AllFilesPathFieldWidgetPropertyTraits : public QVariantPropertyTraits<QString> {
	typedef QString Type;
	typedef QVariantPropertyTraits<QString> Super;
	typedef AllFilesPathFieldWidgetPropertyTraits Self;
	typedef TProperty<Self> PropertyType;
	
	static QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS BoolComboBoxPropertyTraits : public QVariantPropertyTraits<bool> {
	typedef bool Type;
	typedef QVariantPropertyTraits<bool> Super;
	typedef BoolComboBoxPropertyTraits Self;
	typedef TProperty<Self> PropertyType;

	static QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

template <typename TWidget, typename TExtractor, typename T>
struct WidgetPropertyTraits : public QVariantPropertyTraits<T> {
	typedef T Type;
	typedef QVariantPropertyTraits<T> Super;
	typedef WidgetPropertyTraits<TWidget, TExtractor, T> Self;
	typedef TProperty<Self> PropertyType;

	static QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS LineEditPropertyTraits : public QVariantPropertyTraits<QString> {
	typedef QString Type;
	typedef QVariantPropertyTraits<QString> Super;
	typedef LineEditPropertyTraits Self;
	typedef TProperty<Self> PropertyType;

	static QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS ColorFieldWidgetTraits : public QVariantPropertyTraits<QString> {
	typedef QString Type;
	typedef QVariantPropertyTraits<QString> Super;
	typedef ColorFieldWidgetTraits Self;
	typedef TProperty<Self> PropertyType;

	static QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		const QVariant &v,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static QVariant ToVariant(
		const QString &t, 
		int role, 
		const Property &context
	);
};

} // editor
} // tools

#include <Runtime/PopPack.h>
#include "EditorWidgetPropertyEditors.inl"
