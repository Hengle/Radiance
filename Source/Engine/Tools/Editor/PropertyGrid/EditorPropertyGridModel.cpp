// EditorPropertyGridModel.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorPropertyGridModel.h"
#include "EditorProperty.h"
#include <QtCore/QList>

namespace tools {
namespace editor {

namespace details {

class MultiProperty : public Property
{
public:
	MultiProperty(Property *property, QObject *parent = 0)
		: Property(property->Name(), property->Context(), property->Widget(), parent), m_equal(true)
	{
		Add(property);
	}

	virtual ~MultiProperty() 
	{
	}

	virtual int RowCount() const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->RowCount();
	}

	virtual QVariant Data(int role, bool nil) const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->Data(role, !m_equal);
	}

	virtual bool SetData(const QVariant &value, int role)
	{
		RAD_ASSERT(!m_props.isEmpty());

		bool accepted = true;
		bool first = true;

		foreach(Property *p, m_props)
		{
			if (!p->SetData(value, role))
			{
				accepted = false;
				break;
			}

			first = false;
		}

		m_equal = accepted || first;
		return accepted;
	}

	virtual Qt::ItemFlags Flags() const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->Flags();
	}

	virtual bool Equals(const Property &other) const
	{
		return false;
	}

	virtual QWidget *CreateEditor(
		QWidget *parent, 
		const QStyleOptionViewItem &option, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->CreateEditor(
			parent,
			option,
			index,
			source
		);
	}

	virtual bool SetEditorData(
		QWidget &editor, 
		const QModelIndex &index,
		bool nil,
		PropertyGridItemDelegate &source
	) const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->SetEditorData(
			editor,
			index,
			nil || !m_equal,
			source
		);
	}

	virtual bool SetModelData(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->SetModelData(
			editor,
			model,
			index,
			source
		);
	}

	virtual bool UpdateEditorGeometry(
		QWidget &editor,
		const QStyleOptionViewItem &option,
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->UpdateEditorGeometry(
			editor,
			option,
			index,
			source
		);
	}

	virtual bool Validate(
		QWidget &editor, 
		QAbstractItemModel &model, 
		const QModelIndex &index,
		PropertyGridItemDelegate &source
	) const
	{
		RAD_ASSERT(!m_props.isEmpty());
		return m_props.first()->Validate(
			editor,
			model,
			index,
			source
		);
	}

	void Add(Property *property)
	{
		m_props.append(property);
		property->setParent(this);
		m_equal = m_props.count() == 1 || (m_equal && m_props.first()->Equals(*property));
	}

	bool Remove(Property *property)
	{
		if (m_props.count() > 1)
		{ // keep the last prop in there so beginRemoveRows works.
			m_props.removeOne(property);
			delete property;
			RefreshEqual();
			return false;
		}
		return m_props.count() == 1;
	}

private:

	void RefreshEqual()
	{
		m_equal = true;
		foreach(Property *p, m_props)
		{
			if (p != m_props.first())
			{
				m_equal = p->Equals(*m_props.first());
				if (!m_equal)
					break;
			}
		}
	}

	typedef QList<Property*> PropPtrList;
	bool m_equal;
	PropPtrList m_props;
};

} // details

PropertyGridModel::PropertyGridModel(QObject *parent)
: QAbstractItemModel(parent), m_root(new QObject())
{
}

PropertyGridModel::~PropertyGridModel()
{
	delete m_root;
}

int PropertyGridModel::columnCount(const QModelIndex &parent) const
{
	return 2;
}

QVariant PropertyGridModel::data(const QModelIndex &index, int role) const
{
	if (!index.isValid())
		return QVariant();
	const Property *p = reinterpret_cast<Property*>(index.internalPointer());
	if (!p)
		return QVariant();

	if (index.column() == 0)
	{ // we handle this
		if (role == Qt::DisplayRole)
			return QVariant(p->Name());
		return QVariant();
	}

	return p->Data(role);
}

bool PropertyGridModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
	Property *p = reinterpret_cast<Property*>(index.internalPointer());
	if (!p || index.column() != 1)
		return false;

	return p->SetData(value, role);
}

QModelIndex PropertyGridModel::index(int row, int column, const QModelIndex &parent) const
{
	if (!parent.isValid())
	{
		if (column < 0 || column > 1 ||
			row < 0 || row >= m_root->children().count())
		{
			return QModelIndex();
		}

		return createIndex(
			row,
			column,
			static_cast<Property*>(m_root->children().at(row))
		);
	}

	Property *p = reinterpret_cast<Property*>(parent.internalPointer());

	if (column < 0 || column > 1 || row >= p->RowCount())
		return QModelIndex();

	return createIndex(
		row,
		column,
		static_cast<Property*>(p->children().at(row))
	);
}

QModelIndex PropertyGridModel::parent(const QModelIndex &index) const
{
	if (!index.isValid() || !index.internalPointer())
		return QModelIndex();
	const Property *p = reinterpret_cast<Property*>(index.internalPointer());
	if (p->parent() == static_cast<const QObject*>(m_root))
		return QModelIndex();
	return createIndex(
		p->Row(),
		index.column(),
		static_cast<Property*>(p->parent())
	);
}

Qt::ItemFlags PropertyGridModel::flags(const QModelIndex &index) const
{
	if (!index.isValid() || !index.internalPointer())
		return 0;
	if (index.column() == 0)
		return Qt::ItemIsSelectable|Qt::ItemIsEnabled;

	const Property *p = reinterpret_cast<Property*>(index.internalPointer());
	return p->Flags() & ~(Qt::ItemIsDragEnabled|Qt::ItemIsDropEnabled);
}

int PropertyGridModel::rowCount(const QModelIndex &parent) const
{
	if (!parent.isValid() || !parent.internalPointer())
	{
		int size = m_root->children().count();
		return size;
	}
	const Property *p = reinterpret_cast<Property*>(parent.internalPointer());
	return p->RowCount();
}

QVariant PropertyGridModel::headerData(int section, Qt::Orientation orientation, int role) const
{
	if (role != Qt::DisplayRole || section < 0 || section > 1)
		return QAbstractItemModel::headerData(section, orientation, role);
	return section == 0 ? QVariant("Property") : QVariant("Value");
}

void PropertyGridModel::Add(Property *p)
{
	bool insert = true;
	details::MultiProperty *mp = 0;

	PHash::const_iterator it = m_props.find(p->Name());
	insert = it == m_props.end();

	if (insert)
	{
		mp = new (ZEditor) details::MultiProperty(p, m_root);
		RAD_ASSERT(mp->parent() == static_cast<QObject*>(m_root));
		m_props.insert(p->Name(), mp);
	}
	else
	{
		mp = static_cast<details::MultiProperty*>(it.value());
		mp->Add(p);
	}

	QModelIndex c0 = createIndex(
		mp->Row(),
		0,
		static_cast<Property*>(mp)
	);

	if (insert)
	{
		emit beginInsertRows(c0.parent(), mp->Row(), mp->Row());
		emit endInsertRows();
	}
	else
	{
		QModelIndex c1 = createIndex(
			mp->Row(),
			1,
			static_cast<Property*>(mp)
		);

		emit dataChanged(c0, c1);
	}
}

void PropertyGridModel::Remove(Property *p)
{
	PHash::iterator it = m_props.find(p->Name());
	if (it != m_props.end())
	{
		details::MultiProperty *mp = static_cast<details::MultiProperty*>(it.value());

		QModelIndex start = createIndex(
			mp->Row(),
			0,
			it.value()
		);

		if (mp->Remove(p))
		{
			// last one.
			m_props.erase(it);
			
			emit beginRemoveRows(start.parent(), mp->Row(), mp->Row());
			delete mp;
			emit endRemoveRows();
		}
		else
		{
			QModelIndex end = createIndex(
				mp->Row(),
				1,
				it.value()
			);
			emit dataChanged(start, end);
		}
	}
}

void PropertyGridModel::Clear()
{
	QObjectList c = m_root->children();
	if (c.isEmpty())
		return;

	emit beginRemoveRows(
		QModelIndex(),
		0,
		rowCount()-1
	);

	foreach(QObject *q, c)
	{
		delete q;
	}

	emit endRemoveRows();

	m_props.clear();
}

Property *PropertyGridModel::PropertyForIndex(const QModelIndex &index) const
{
	if (!index.isValid())
		return 0;
	return reinterpret_cast<Property*>(index.internalPointer());
}

QModelIndex PropertyGridModel::FindIndex(const QString &name) const
{
	PHash::const_iterator it = m_props.find(name);
	if (it == m_props.end())
		return QModelIndex();
	details::MultiProperty *mp = static_cast<details::MultiProperty*>(it.value());
	return createIndex(mp->Row(), 0, static_cast<Property*>(mp));
}

} // editor
} // tools

#include "moc_EditorPropertyGridModel.cc"
