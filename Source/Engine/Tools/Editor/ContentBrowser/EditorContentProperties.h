// EditorContentProperties.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "../PropertyGrid/EditorWidgetPropertyEditors.h"
#include "EditorContentPropertyGrid.h"
#include "EditorContentImportFieldWidget.h"
#include "../EditorUtils.h"
#include "../../../Packages/Packages.h"
#include <QtGui/QSpinBox>
#include <QtGui/QComboBox>
#include <Runtime/PushPack.h>

class QWidget;

namespace tools {
namespace editor {

class FilePathFieldWidget;
class ComboCheckBox;

namespace content_property_details {

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS KeyContext : public Property::UserContext {
	typedef boost::shared_ptr<KeyContext> Ref;

	KeyContext(
		int _id, 
		int _flags, 
		const pkg::KeyVal::Ref &_key, 
		const pkg::KeyDef::Ref &_def
	) :	id(_id), flags(_flags), key(_key), def(_def) {}

	void MakeKey(const pkg::Package::Entry::Ref &entry) {
		if (!key) {
			RAD_ASSERT(def);
			key = def->CreateKey(flags);
			entry->AddKey(key, true);
		}
	}

	const lua::Variant &Variant() const {
		if (key)
			return key->val;
		RAD_ASSERT(def);
		return def->val;
	}

	int id;
	int flags;
	pkg::KeyVal::Ref key;
	pkg::KeyDef::Ref def;
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS EntryIdContext : public Property::UserContext {
	typedef boost::shared_ptr<EntryIdContext> Ref;
	EntryIdContext(int _id) : id(_id) {}
	int id;
};

///////////////////////////////////////////////////////////////////////////////

inline String TrimKeyName(const String &path, int flags) {
	if (flags&pkg::P_AllTargets) {
		return pkg::Package::Entry::TrimKeyName(path);
	}
	return path;
}

///////////////////////////////////////////////////////////////////////////////

inline void SendChangedEvent(
	const pkg::Package::Entry::Ref &entry,
	const KeyContext::Ref &context,
	Property &p
) {
	pkg::Package::Entry::KeyChangedEventData d;
	d.origin = entry;
	d.key = context->key;
	d.path = TrimKeyName(context->key->path, context->flags);
	d.flags = context->flags;
	entry->OnKeyChange.Trigger(d);
}

///////////////////////////////////////////////////////////////////////////////

template <typename T>
struct ComboBoxExtractor {
	static void ApplyHints(QComboBox &cb);
	static void SetEditorData(QComboBox &cb, const QVariant &v, const Property &context);
	static QVariant ToVariant(QComboBox &cb, const Property &context);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS ComboCheckBoxExtractor { 
	// only supports strings (no other way to make value lists).
	static void ApplyHints(ComboCheckBox &cb) {}
	static void SetEditorData(ComboCheckBox &cb, const QVariant &v, const Property &context);
	static QVariant ToVariant(ComboCheckBox &cb, const Property &context);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS FilePathExtractor {
	static void ApplyHints(FilePathFieldWidget &fw);
	static void SetEditorData(FilePathFieldWidget &fw, const QVariant &v, const Property &context);
	static QVariant ToVariant(FilePathFieldWidget &fw, const Property &context);
};

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS ContentImportPathExtractor {
	static void ApplyHints(ContentImportFieldWidget &fw);
	static void SetEditorData(ContentImportFieldWidget &fw, const QVariant &v, const Property &context);
	static QVariant ToVariant(ContentImportFieldWidget &fw, const Property &context);
};

///////////////////////////////////////////////////////////////////////////////

template <typename T, typename X = typename T::Type>
struct KeyTraits : public T
{
	typedef typename T::Type Type;
	typedef T Super;
	typedef KeyTraits<T> Self;
	typedef TProperty<Self> PropertyType;

	static bool SetValue(const Type &t, Property &p);
	static Qt::ItemFlags Flags(const Property &context);
};

typedef KeyTraits<SpinBoxPropertyTraits<QSpinBox, NullPropertyHints, int> > IntTraits;
typedef KeyTraits<WidgetPropertyTraits<PropertyComboBox, ComboBoxExtractor<int>, int> > IntListTraits;
typedef KeyTraits<BoolComboBoxPropertyTraits> BoolTraits;
typedef KeyTraits<WidgetPropertyTraits<FilePathFieldWidget, FilePathExtractor, QString> > FilePathTraits;
typedef KeyTraits<ColorFieldWidgetTraits> ColorTraits;
typedef KeyTraits<WidgetPropertyTraits<ContentImportFieldWidget, ContentImportPathExtractor, QString> > ContentImportTraits;
typedef KeyTraits<LineEditPropertyTraits> StringTraits;
typedef KeyTraits<WidgetPropertyTraits<PropertyComboBox, ComboBoxExtractor<int>, int> > IntComboBoxTraits;
typedef KeyTraits<WidgetPropertyTraits<PropertyComboBox, ComboBoxExtractor<QString>, QString> > StringComboBoxTraits;
typedef KeyTraits<WidgetPropertyTraits<ComboCheckBox, ComboCheckBoxExtractor, QString> > StringComboCheckBoxTraits;

///////////////////////////////////////////////////////////////////////////////

struct RADENG_CLASS AssetRenameTraits : public LineEditPropertyTraits {
	typedef QString Type;
	typedef LineEditPropertyTraits Super;
	typedef AssetRenameTraits Self;
	typedef TProperty<Self> PropertyType;
	static bool SetValue(const QString &value, Property &p);
};

///////////////////////////////////////////////////////////////////////////////

RADENG_API Property * RADENG_CALL PropertyForKey(const char *name, const KeyContext::Ref &key, QWidget &widget);
RADENG_API Property * RADENG_CALL PropertyForName(const pkg::Package::Entry::Ref &e, QWidget &widget);
RADENG_API PropertyList RADENG_CALL CreatePropertiesForAsset(const pkg::Package::Entry::Ref &e, int flags, QWidget &widget);
RADENG_API PropertyList RADENG_CALL CreateDefaultPropertiesForAsset(const pkg::Package::Entry::Ref &e, int flags, QWidget &widget);

} // content_property_details
} // editor
} // tools

#include <Runtime/PopPack.h>
#include "EditorContentProperties.inl"
