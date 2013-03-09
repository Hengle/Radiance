// EditorContentProperties.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace tools {
namespace editor {
namespace content_property_details {

///////////////////////////////////////////////////////////////////////////////

template <>
struct ComboBoxExtractor<QString> {
	static void ApplyHints(QComboBox &cb) {}
	static void SetEditorData(QComboBox &cb, const QVariant &v, const Property &context);
	static QVariant ToVariant(QComboBox &cb, const Property &context);
};

template <typename T>
inline void ComboBoxExtractor<T>::ApplyHints(QComboBox &cb) {
}

template <typename T>
void ComboBoxExtractor<T>::SetEditorData(QComboBox &cb, const QVariant &v, const Property &p) {
	KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
	RAD_ASSERT(context);

	if (!context->def)
		return;

	pkg::KeyDef::Pair::Map::const_iterator it = context->def->pairs.find(String("values"));
	if (it == context->def->pairs.end())
		return;

	const String *s = static_cast<const String*>(it->second.val);
	if (!s)
		return;

	QString qs(s->c_str.get());
	QStringList values = qs.split(';', QString::SkipEmptyParts);
	cb.addItems(values);

	cb.setCurrentIndex(cb.findText(v.toString()));
	cb.showPopup();
}

template <typename T>
inline QVariant ComboBoxExtractor<T>::ToVariant(QComboBox &cb, const Property &p) {
	if (cb.currentIndex() != -1)
		return QVariant(cb.currentText()).value<T>();
	return QVariant();
}

///////////////////////////////////////////////////////////////////////////////

template<typename T, typename X>
bool KeyTraits<T, X>::SetValue(const Type &value, Property &p) {
	// store and notify.
	KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
	RAD_ASSERT(context);
	
	pkg::Package::Entry::Ref entry = Packages()->FindEntry(context->id);
	if (!entry)
		return false;

	context->MakeKey(entry);

	Type *i = static_cast<Type*>(context->key->val);
	if (i) {
		*i = value;
		entry->UpdateModifiedTime();
		ContentPropertyGrid::PropertyChanged(
			entry,
			context->key
		);
		SendChangedEvent(entry, context, p);
	}

	return true;
}

template<typename T, typename X>
Qt::ItemFlags KeyTraits<T, X>::Flags(const Property &p) {
	KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
	RAD_ASSERT(context);

	Qt::ItemFlags flags = T::Flags(p);
	if (context->def && context->def->style&pkg::K_ReadOnly)
		return flags & ~(Qt::ItemIsEnabled|Qt::ItemIsEditable);
	return flags;
}

template <typename T>
struct KeyTraits<T, QString> : public T {
	typedef typename T::Type Type;
	typedef KeyTraits<T> Self;
	typedef TProperty<Self> PropertyType;

	static bool SetValue(const Type &value, Property &p) {
		// store and notify.
		KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
		RAD_ASSERT(context);
		
		pkg::Package::Entry::Ref entry = Packages()->FindEntry(context->id);
		if (!entry)
			return false;

		context->MakeKey(entry);

		String *s = static_cast<String*>(context->key->val);
		if (s)
		{
			bool isImport = context->def->style&pkg::K_Import;
			if (isImport)
				entry->UnmapImport(context->key);
			*s = value.toAscii().constData();
			entry->UpdateModifiedTime();
			if (isImport)
				entry->MapImport(context->key);
			ContentPropertyGrid::PropertyChanged(
				entry,
				context->key
			);
			SendChangedEvent(entry, context, p);
		}

		return true;
	}

	static Qt::ItemFlags Flags(const Property &p) {
		KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
		RAD_ASSERT(context);

		Qt::ItemFlags flags = T::Flags(p);
		if (context->def && context->def->style&pkg::K_ReadOnly)
			return flags & ~(Qt::ItemIsEnabled|Qt::ItemIsEditable);
		return flags;
	}
};

} // content_property_details
} // editor
} // tools

