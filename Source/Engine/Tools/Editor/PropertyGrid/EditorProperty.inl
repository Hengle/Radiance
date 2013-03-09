// EditorProperty.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace tools {
namespace editor {

template <typename T>
inline QVariant QVariantPropertyTraits<T>::ToVariant(const T &t, int role, const Property &) {
	if (role == Qt::EditRole || role == Qt::DisplayRole)
		return QVariant(t);
	return QVariant();
}

template <typename T>
inline bool QVariantPropertyTraits<T>::FromVariant(const QVariant &t, T &out, int role, const Property &) {
	if (role != Qt::EditRole)
		return false;
	out = t.value<T>();
	return true;
}

template <typename T>
bool QVariantPropertyTraits<T>::SupportsEquals() {
	return false;
}

template <typename T>
bool QVariantPropertyTraits<T>::Equals(const Property &a, const Property &b) {
	return false;
}

template <typename T>
inline QVariant QVariantPropertyTraits<T>::NilVariant(int role, const Property &p) {
	return QVariant();
}

template <typename T>
inline QWidget *QVariantPropertyTraits<T>::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
) { 
	// NULL calls QItemDelegate (see EditorPropertyGridItemDelegate.cpp)
	return 0;
}

template <typename T>
inline bool QVariantPropertyTraits<T>::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	const QVariant &value,
	PropertyGridItemDelegate &source,
	const Property &context
) { 
	// false calls QItemDelegate (see EditorPropertyGridItemDelegate.cpp)
	return false;
}

template <typename T>
inline bool QVariantPropertyTraits<T>::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
)
{ // false calls QItemDelegate (see EditorPropertyGridItemDelegate.cpp)
	return false;
}

template <typename T>
inline bool QVariantPropertyTraits<T>::UpdateEditorGeometry(
	QWidget &editor,
	const QStyleOptionViewItem &option,
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
) {
	return false;
}

template <typename T>
inline bool QVariantPropertyTraits<T>::Validate(
	QWidget &editor,
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source,
	const Property &context
) {
	return true;
}

template <typename T>
inline void QVariantPropertyTraits<T>::Changed(Property &context) {
}

template <typename T>
inline bool QVariantPropertyTraits<T>::SetValue(const T &t, Property &context) {
	return true;
}

template <typename T>
Qt::ItemFlags QVariantPropertyTraits<T>::Flags(const Property &context) {
	return Qt::ItemIsSelectable|Qt::ItemIsEditable|Qt::ItemIsEnabled;
}

///////////////////////////////////////////////////////////////////////////////

template <typename Traits>
inline TProperty<Traits>::TProperty(
	const T &value,
	const UserContext::Ref &context,
	const QString &name,
	QWidget &widget,
	QObject *parent
) : Property(name, context, widget, parent), m_value(value) {
}

template <typename Traits>
inline TProperty<Traits>::~TProperty() {
}

template <typename Traits>
inline QVariant TProperty<Traits>::Data(int role, bool nil) const {
	if (role == Qt::EditRole || 
		role == Qt::DisplayRole || 
		role == Qt::BackgroundRole ||
		role == Qt::CheckStateRole) {
		return nil ? Traits::NilVariant(role, *this) : Traits::ToVariant(m_value, role, *this);
	}
	return QVariant();
}

template <typename Traits>
inline bool TProperty<Traits>::SetData(const QVariant &value, int role) {
	T x;
	if (!Traits::FromVariant(value, x, role, *this))
		return false;
	if (x != m_value) {
		// returns true if handled, returning false sends this on to 
		// to QItemDelegate (we don't want that)
		if (!Traits::SetValue(x, *this))
			return true;
		m_value = x;
		SignalChanged();
	}
	return true;
}

template <typename Traits>
inline Qt::ItemFlags TProperty<Traits>::Flags() const {
	return Traits::Flags(*this);
}

template <typename Traits>
inline bool TProperty<Traits>::Equals(const Property &other) const {
	if(Traits::SupportsEquals())
		return Traits::Equals(*this, other);
	return static_cast<const SelfType&>(other).Value() == Value();
}

template <typename Traits>
inline QWidget *TProperty<Traits>::CreateEditor(
	QWidget *parent, 
	const QStyleOptionViewItem &option, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source
) const {
	return Traits::CreateEditor(
		parent,
		option,
		index,
		source,
		*this
	);
}

template <typename Traits>
inline bool TProperty<Traits>::SetEditorData(
	QWidget &editor, 
	const QModelIndex &index,
	bool nil,
	PropertyGridItemDelegate &source
) const {
	return Traits::SetEditorData(
		editor,
		index,
		nil ? Traits::NilVariant(Qt::EditRole, *this) : Traits::ToVariant(m_value, Qt::EditRole, *this),
		source,
		*this
	);
}

template <typename Traits>
inline bool TProperty<Traits>::SetModelData(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source
) const {
	return Traits::SetModelData(
		editor,
		model,
		index,
		source,
		*this
	);
}

template <typename Traits>
inline bool TProperty<Traits>::Validate(
	QWidget &editor, 
	QAbstractItemModel &model, 
	const QModelIndex &index,
	PropertyGridItemDelegate &source
) const {
	return Traits::Validate(
		editor,
		model,
		index,
		source,
		*this
	);
}

template <typename Traits>
inline bool TProperty<Traits>::UpdateEditorGeometry(
	QWidget &editor,
	const QStyleOptionViewItem &option,
	const QModelIndex &index,
	PropertyGridItemDelegate &source
) const {
	return Traits::UpdateEditorGeometry(
		editor,
		option,
		index,
		source,
		*this
	);
}

template <typename Traits>
inline typename Traits::Type TProperty<Traits>::Value() const {
	return m_value;
}

template <typename Traits>
inline void TProperty<Traits>::SignalChanged() {
	Traits::Changed(*this);
	Property::SignalChanged();
}

} // editor
} // tools


