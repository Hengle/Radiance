// EditorWidgetPropertyEditors.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorWidgetPropertyEditors.h"
#include "EditorFilePathFieldWidget.h"
#include "EditorColorFieldWidget.h"
#include <QtGui/QComboBox>
#include <QtGui/QLineEdit>

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

PropertyComboBox::PropertyComboBox(QWidget *parent) : QComboBox(parent) {
	RAD_VERIFY(connect(this, SIGNAL(activated(int)), SLOT(OnSelectionChanged())));
}

void PropertyComboBox::OnSelectionChanged() {
	emit CloseEditor();
}

///////////////////////////////////////////////////////////////////////////////

QWidget *AllFilesPathFieldWidgetPropertyTraits::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
) {
	return new (ZEditor) FilePathFieldWidget(parent);
}

bool AllFilesPathFieldWidgetPropertyTraits::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
) {
	FilePathFieldWidget &fw = static_cast<FilePathFieldWidget&>(editor);
	String prefix;
	Files()->GetAbsolutePath("", prefix, file::kFileMask_Base);
	fw.SetPrefix(prefix.c_str.get());
	fw.SetFilter("All Files (*.*)");
	fw.SetPath(v.toString());
	return true;
}

bool AllFilesPathFieldWidgetPropertyTraits::SetModelData(
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

///////////////////////////////////////////////////////////////////////////////

QWidget *ColorFieldWidgetTraits::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	ColorFieldWidget *w = new (ZEditor) ColorFieldWidget(parent);
	return w;
}

bool ColorFieldWidgetTraits::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	static_cast<ColorFieldWidget&>(editor).SetColor(v.toString());
	return true;
}

bool ColorFieldWidgetTraits::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	QVariant v = static_cast<ColorFieldWidget&>(editor).Color();
	if (v.isValid())
		model.setData(index, v, Qt::EditRole);
	return true;
}

QVariant ColorFieldWidgetTraits::ToVariant(
	const QString &t, 
	int role, 
	const Property &context
)
{
	if (role == Qt::DisplayRole)
		return QVariant();
	if (role != Qt::BackgroundRole)
		return QVariantPropertyTraits<QString>::ToVariant(t, role, context);
	int r, g, b, a;
	sscanf(t.toAscii().constData(), "%d %d %d %d",
		&r, &g, &b, &a
	);
	return QColor(r, g, b, a);
}

} // editor
} // tools

#include "moc_EditorWidgetPropertyEditors.cc"

