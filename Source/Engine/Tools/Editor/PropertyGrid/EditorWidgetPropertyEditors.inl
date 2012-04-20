// EditorWidgetPropertyEditors.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <algorithm>
#undef min
#undef max

namespace tools {
namespace editor {

template <typename TWidget, typename THints, typename T>
inline QWidget *SpinBoxPropertyTraits<TWidget, THints, T>::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	TWidget *editor = new (ZEditor) TWidget(parent);
	THints().ApplyHints(*editor, context);
	return editor;
}

template <typename TWidget, typename THints, typename T>
inline bool SpinBoxPropertyTraits<TWidget, THints, T>::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	static_cast<TWidget&>(editor).setRange(
		std::numeric_limits<int>::min(), 
		std::numeric_limits<int>::max()
	);
	static_cast<TWidget&>(editor).setValue(v.value<T>());
	return true;
}

template <typename TWidget, typename THints, typename T>
inline bool SpinBoxPropertyTraits<TWidget, THints, T>::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	TWidget &spin = static_cast<TWidget&>(editor);
	spin.interpretText();
	model.setData(index, spin.value(), Qt::EditRole);
	return true;
}

///////////////////////////////////////////////////////////////////////////////

template <typename TWidget, typename TExtractor, typename T>
inline QWidget *WidgetPropertyTraits<TWidget, TExtractor, T>::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	TWidget *w = new (ZEditor) TWidget(parent);
	TExtractor().ApplyHints(*w);
	return w;
}

template <typename TWidget, typename TExtractor, typename T>
inline bool WidgetPropertyTraits<TWidget, TExtractor, T>::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &v,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	TExtractor().SetEditorData(static_cast<TWidget&>(editor), v, context);
	return true;
}

template <typename TWidget, typename TExtractor, typename T>
inline bool WidgetPropertyTraits<TWidget, TExtractor, T>::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{
	QVariant v = TExtractor().ToVariant(static_cast<TWidget&>(editor), context);
	if (v.isValid())
		model.setData(index, v, Qt::EditRole);
	return true;
}

} // editor
} // tools
