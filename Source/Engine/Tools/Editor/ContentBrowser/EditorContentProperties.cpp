// EditorContentProperties.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentProperties.h"
#include "EditorContentTexture.h"
#include "../PropertyGrid/EditorFilePathFieldWidget.h"
#include "../PropertyGrid/EditorColorFieldWidget.h"
#include "../EditorComboCheckBox.h"
#include <Runtime/StringBase.h>
#include <QtGui/QMessageBox>

namespace tools {
namespace editor {
namespace content_property_details {

RADENG_API PropertyList RADENG_CALL CreatePropertiesForAsset(const pkg::Package::Entry::Ref &e, int flags, QWidget &widget)
{
	PropertyList l = CreateDefaultPropertiesForAsset(e, flags, widget);

	switch (e->type.get())
	{
	case asset::AT_Texture:
		AddTextureProperties(l, e, flags, widget);
		break;
	default:
		break;
	}

	return l;
}

///////////////////////////////////////////////////////////////////////////////

void ComboCheckBoxExtractor::SetEditorData(ComboCheckBox &cb, const QVariant &v, const Property &p)
{
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
	
	qs = v.toString();
	QStringList sel = qs.split(';', QString::SkipEmptyParts);

	foreach(QString s, values)
	{
		bool checked = sel.indexOf(s) > -1;
		cb.addItem(s, checked);
	}

	cb.showPopup();
}

QVariant ComboCheckBoxExtractor::ToVariant(ComboCheckBox &cb, const Property &context)
{
	return cb.getSelString();
}

///////////////////////////////////////////////////////////////////////////////

void ComboBoxExtractor<QString>::SetEditorData(QComboBox &cb, const QVariant &v, const Property &p)
{
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

	// find selected item.

	cb.setCurrentIndex(cb.findText(v.toString()));
	cb.showPopup();
}

QVariant ComboBoxExtractor<QString>::ToVariant(QComboBox &cb, const Property &context)
{
	if (cb.currentIndex() != -1)
		return QVariant(cb.currentText());
	return QVariant();
}

///////////////////////////////////////////////////////////////////////////////

void FilePathExtractor::ApplyHints(FilePathFieldWidget &fw)
{
}

void FilePathExtractor::SetEditorData(FilePathFieldWidget &fw, const QVariant &v, const Property &p)
{
	KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
	RAD_ASSERT(context);

	if (!context->def)
		return;

	QString prefix("9:/");
	prefix += Files()->hddRoot.get();
	fw.SetPrefix(prefix);
	fw.SetFilter("All Files (*.*)");

	pkg::KeyDef::Pair::Map::const_iterator it = context->def->pairs.find(CStr("filter"));
	if (it != context->def->pairs.end())
	{
		const String *s = static_cast<const String*>(it->second.val);
		if (s)
			fw.SetFilter(s->c_str.get());
	}

	fw.SetPath(v.toString());
}

QVariant FilePathExtractor::ToVariant(FilePathFieldWidget &fw, const Property &context)
{
	return fw.Path();
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

///////////////////////////////////////////////////////////////////////////////

void ContentImportPathExtractor::ApplyHints(ContentImportFieldWidget &fw)
{
}

void ContentImportPathExtractor::SetEditorData(ContentImportFieldWidget &fw, const QVariant &v, const Property &p)
{
	KeyContext::Ref context = boost::static_pointer_cast<KeyContext, Property::UserContext>(p.Context());
	RAD_ASSERT(context);

	if (!context->def)
		return;

	pkg::KeyDef::Pair::Map::const_iterator it = context->def->pairs.find(String("types"));
	if (it != context->def->pairs.end())
	{
		const String *s = static_cast<const String*>(it->second.val);
		if (s)
			fw.SetFilter(s->c_str.get());
	}

	fw.SetPath(v.toString());
}

QVariant ContentImportPathExtractor::ToVariant(ContentImportFieldWidget &fw, const Property &context)
{
	return fw.Path();
}

///////////////////////////////////////////////////////////////////////////////

bool AssetRenameTraits::SetValue(const QString &value, Property &p)
{
	EntryIdContext::Ref context = boost::static_pointer_cast<EntryIdContext, Property::UserContext>(p.Context());
	RAD_ASSERT(context);

	pkg::Package::Entry::Ref entry = Packages()->FindEntry(context->id);
	if (!entry)
		return false;

	if (entry->Rename(value.toAscii().constData()))
	{
		ContentPropertyGrid::PropertyChanged(entry);
		return true;
	}

	QMessageBox::critical(
		&p.Widget(),
		"Error",
		"An asset with that name already exists."
	);

	return false;
}

///////////////////////////////////////////////////////////////////////////////

RADENG_API Property *RADENG_CALL PropertyForKey(const char *name, const KeyContext::Ref &key, QWidget &widget)
{
	int type = key->def->Type();

	// int
	{
		const int *i = static_cast<const int*>(key->Variant());
		if (i)
		{
			switch (type)
			{
			case pkg::K_List:
				return new (ZEditor) IntComboBoxTraits::PropertyType(
					*i,
					key,
					name,
					widget
				);
			}

			return new (ZEditor) IntTraits::PropertyType(
				*i,
				key,
				name,
				widget
			);
		}
	}
	// bool
	{
		const bool *b = static_cast<const bool*>(key->Variant());
		if (b)
		{
			return new (ZEditor) BoolTraits::PropertyType(
				*b,
				key,
				name,
				widget
			);
		}
	}
	// string
	{
		const String *s = static_cast<const String*>(key->Variant());
		if (s)
		{
			switch (type)
			{
			case pkg::K_Import:
				return new (ZEditor) ContentImportTraits::PropertyType(
					s->c_str.get(),
					key,
					name,
					widget
				);
			case pkg::K_File:
				return new (ZEditor) FilePathTraits::PropertyType(
					s->c_str.get(),
					key,
					name,
					widget
				);
			case pkg::K_List:
				return new (ZEditor) StringComboBoxTraits::PropertyType(
					s->c_str.get(),
					key,
					name,
					widget
				);
			case pkg::K_CheckBoxes:
				return new (ZEditor) StringComboCheckBoxTraits::PropertyType(
					s->c_str.get(),
					key,
					name,
					widget
				);
			case pkg::K_Color:
				return new (ZEditor) ColorTraits::PropertyType(
					s->c_str.get(),
					key,
					name,
					widget
				);
			}

			return new (ZEditor) StringTraits::PropertyType(
				s->c_str.get(),
				key,
				name,
				widget
			);
		}
	}

	return 0;
}

RADENG_API Property *RADENG_CALL PropertyForName(const pkg::Package::Entry::Ref &e, QWidget &widget)
{ // Get property for editing the asset name.
	return new (ZEditor) AssetRenameTraits::PropertyType(
		e->name.get(),
		Property::UserContext::Ref(new (ZEditor) EntryIdContext(e->id)),
		"Name",
		widget
	);
}

RADENG_API PropertyList RADENG_CALL CreateDefaultPropertiesForAsset(const pkg::Package::Entry::Ref &e, int flags, QWidget &widget)
{
	PropertyList l;

	flags &= pkg::P_AllTargets;

	if (!flags)
	{
		Property *p = PropertyForName(e, widget);
		if (p)
			l.append(p);
	}

	pkg::KeyDef::MapRef defs = Packages()->KeyDefsForType(e->type);
	const pkg::KeyVal::Map &keys = e->Keys();

	for (int t = pkg::P_FirstTarget; t <= pkg::P_LastTarget; t <<= 1)
	{
		if (flags && !(flags&t))
			continue;

		for (pkg::KeyDef::Map::const_iterator it = defs->begin(); it != defs->end(); ++it)
		{
			const pkg::KeyDef::Ref &def = it->second;

			if (def->style&pkg::K_Hidden)
				continue;

			if (def->flags == 0 && def->val.Valid())
			{ // only select defs that aren't tagged for any platform
			  // then look for a platform specific version. if none found, default to def
			  // SetValue()'s will add key if necessary.

				if ((flags && (def->style&pkg::K_Global)) ||
					(!flags && !(def->style&pkg::K_Global)))
					continue;

				pkg::KeyVal::Ref key;
				String path = def->path;
				if (flags)
				{
					path += ".";
					path += pkg::PlatformNameForFlags(t);
				}
				pkg::KeyVal::Map::const_iterator k = keys.find(path);

				if (k != keys.end())
				{
					key = k->second;
				}

				KeyContext::Ref ctx(new (ZEditor) KeyContext(e->id, flags ? t : 0, key, def));
				Property *p = PropertyForKey(def->path.c_str, ctx, widget);

				if (p)
					l.append(p);
			}
		}

		if (!flags)
			break;
	}

	return l;
}

} // content_property_details
} // editor
} // tools

