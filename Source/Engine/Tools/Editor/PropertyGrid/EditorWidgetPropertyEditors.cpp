// EditorWidgetPropertyEditors.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorWidgetPropertyEditors.h"
#include "EditorFilePathFieldWidget.h"
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

PropertyComboBox::PropertyComboBox(QWidget *parent) : QComboBox(parent)
{
	RAD_VERIFY(connect(this, SIGNAL(activated(int)), SLOT(OnSelectionChanged())));
}

void PropertyComboBox::OnSelectionChanged()
{
	emit CloseEditor();
}

///////////////////////////////////////////////////////////////////////////////

QWidget *FilePathFieldWidgetPropertyTraits::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	return new (ZEditor) FilePathFieldWidget(parent);
}

bool FilePathFieldWidgetPropertyTraits::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	static_cast<FilePathFieldWidget&>(editor).SetPath(v.toString());
	return true;
}

bool FilePathFieldWidgetPropertyTraits::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	FilePathFieldWidget &fw = static_cast<FilePathFieldWidget&>(editor);
	return model.setData(index, fw.Path(), Qt::EditRole);
}

///////////////////////////////////////////////////////////////////////////////

QWidget *BoolComboBoxPropertyTraits::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	return new (ZEditor) PropertyComboBox(parent);
}

bool BoolComboBoxPropertyTraits::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	QComboBox &cb = static_cast<QComboBox&>(editor);
	cb.setEditable(false);
	cb.addItem("true");
	cb.addItem("false");
	cb.setCurrentIndex(v.toBool() ? 0 : 1);
	cb.showPopup();
	return true;
}

bool BoolComboBoxPropertyTraits::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	QComboBox &cb = static_cast<QComboBox&>(editor);
	return model.setData(index, cb.currentIndex() == 0 ? true : false, Qt::EditRole);
}

///////////////////////////////////////////////////////////////////////////////

QWidget *LineEditPropertyTraits::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	return new (ZEditor) QLineEdit(parent);
}

bool LineEditPropertyTraits::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	QLineEdit &edit = static_cast<QLineEdit&>(editor);
	edit.setText(v.toString());
	edit.selectAll();
	return true;
}

bool LineEditPropertyTraits::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	QLineEdit &edit = static_cast<QLineEdit&>(editor);
	return model.setData(index, edit.text(), Qt::EditRole);
}

} // editor
} // tools

#include "moc_EditorWidgetPropertyEditors.cc"

