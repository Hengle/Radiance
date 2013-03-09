// EditorProperty.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../EditorTypes.h"
#include "EditorPropertyGridItemDelegate.h"
#include <QtCore/QObject>
#include <QtCore/QVariant>
#include <QtCore/QString>
#include <QtGui/QItemDelegate>
#include <QtCore/QList>
#include <QtCore/QAbstractItemModel>
#include <Runtime/Event.h>
#include <Runtime/PushPack.h>

class QSignalMapper;

namespace tools {
namespace editor {

class Property;
typedef QList<Property*> PropertyList;

class PropertyGridItemDelegate;
namespace details { class MultiProperty; }

class RADENG_CLASS Property : public QObject {
	Q_OBJECT

public:

	class UserContext {
	public:
		typedef boost::shared_ptr<UserContext> Ref;
	};

	typedef QList<Property*> List;

	Property(const QString &name, const UserContext::Ref &context, QWidget &view, QObject *parent = 0);
	virtual ~Property();

	virtual int Row() const { 
		RAD_ASSERT(parent());
		return parent()->children().indexOf(const_cast<Property*>(this)); 
	}

	virtual int RowCount() const {
		return children().size();
	}

	UserContext::Ref Context() const { return m_context; }
	QWidget &Widget() const { return *m_widget; }

	QString Name() const { return m_name; }
	void SetName(const QString &name) { m_name = name; }
	virtual QVariant Data(int role, bool nil = false) const = 0;
	virtual bool SetData(const QVariant &value, int role) = 0;
	virtual Qt::ItemFlags Flags() const = 0;
	virtual bool Equals(const Property &other) const = 0;

	virtual QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const {
		// NULL calls QItemDelegate (see EditorPropertyGridItemDelegate.cpp)
		return 0;
	}

	virtual bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		bool nil,
		PropertyGridItemDelegate &source
	) const {
		// false calls QItemDelegate (see EditorPropertyGridItemDelegate.cpp)
		return false;
	}

	virtual bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const {
		// false calls QItemDelegate (see EditorPropertyGridItemDelegate.cpp)
		return false;
	}

	virtual bool UpdateEditorGeometry(
		QWidget &editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const {
		return false;
	}

	virtual bool Validate(
		QWidget &editor,
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const {
		return true;
	}

signals:

	void Changed();

protected:

	virtual void SignalChanged();

private:

	QWidget *MapEditFinished(QWidget *editor);
	QWidget *m_widget;

	UserContext::Ref m_context;
	QString m_name;
};

///////////////////////////////////////////////////////////////////////////////

// For types that are directly supported by QVariant

template <typename T>
struct QVariantPropertyTraits {
	typedef T Type;

	static QVariant ToVariant(
		const T &t, 
		int role, 
		const Property &context
	);

	static bool FromVariant(
		const QVariant &v, 
		T &out, 
		int role, 
		const Property &context
	);

	static QVariant NilVariant(int role, const Property &context);
	static bool Equals(const Property &a, const Property &b);
	static bool SupportsEquals();

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

	static bool UpdateEditorGeometry(
		QWidget &editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static bool Validate(
		QWidget &editor,
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source,
		const Property &context
	);

	static void Changed(Property &context);
	static bool SetValue(const T &t, Property &context);
	static Qt::ItemFlags Flags(const Property &context);
};

///////////////////////////////////////////////////////////////////////////////

template <typename Traits>
class TProperty : public Property
{
public:

	typedef Traits TraitsType;
	typedef typename Traits::Type T;
	typedef TProperty<Traits> SelfType;

	TProperty(
		const T &value,
		const UserContext::Ref &context,
		const QString &name,
		QWidget &widget,
		QObject *parent = 0
	);

	virtual ~TProperty();

	virtual QVariant Data(int role, bool nil = false) const;
	virtual bool SetData(const QVariant &value, int role);
	virtual Qt::ItemFlags Flags() const;
	virtual bool Equals(const Property &other) const;

	virtual QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const;

	virtual bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		bool nil,
		PropertyGridItemDelegate &source
	) const;

	virtual bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const;

	virtual bool UpdateEditorGeometry(
		QWidget &editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const;

	virtual bool Validate(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const;

	T Value() const;

protected:

	virtual void SignalChanged();

private:

	T m_value;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
#include "EditorProperty.inl"

