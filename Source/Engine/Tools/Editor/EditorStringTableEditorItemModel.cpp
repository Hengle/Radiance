// EditorStringTableEditorItemModel.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorStringTableEditorItemModel.h"
#include <QtGui/QIcon>
#include <QtGui/QMessageBox>

namespace tools {
namespace editor {

StringTableEditorItemModel::StringTableEditorItemModel(
	StringTable &stringTable,
	StringTable::LangId langId,
	bool editable,
	const QIcon *icons,
	QObject *parent
) : 
QAbstractItemModel(parent), 
m_stringTable(stringTable), 
m_langId(langId), 
m_editable(editable),
m_icons(icons) {
}

QVariant StringTableEditorItemModel::data(const QModelIndex &index, int role) const {
	if ((role != Qt::DisplayRole) && (role != Qt::EditRole))
		return QVariant();
	const StringTable::Entry::Map::const_iterator *it = IteratorForIndex(index);
	if (!it)
		return QVariant();

	switch (index.column()) {
		case 0: {
			return QVariant((*it)->first.c_str.get());
		} break;
		case 1: {
			StringTable::Entry::Strings::const_iterator string = (*it)->second.strings.find(StringTable::LangId_EN);
			if (string == (*it)->second.strings.end())
				return QVariant();
			return QVariant(QString::fromUtf8(string->second.c_str.get()));
		} break;
		case 2: {
			StringTable::Entry::Strings::const_iterator string = (*it)->second.strings.find(m_langId);
			if (string == (*it)->second.strings.end())
				return QVariant();
			return QVariant(QString::fromUtf8(string->second.c_str.get()));
		} break;
		default:
			return QVariant();
	}
}

bool StringTableEditorItemModel::setData(const QModelIndex &index, const QVariant &value, int role) {
	if ((role != Qt::EditRole) || (value.type() != QVariant::String))
		return false;
	const StringTable::Entry::Map::const_iterator *it = IteratorForIndex(index);
	if (!it)
		return false;

	QString s = value.toString();
	switch (index.column()) {
		case 0: {
			if (QString((*it)->first.c_str.get()) != s) {
				QByteArray ascii = s.toAscii();
				// renaming the field
				if (!m_stringTable.ChangeId((*it)->first.c_str, ascii.constData())) {
					QMessageBox::critical(0, "Error", "A string with that name already exists");
					return false;
				}
				Load();
				int row = RowForId(ascii.constData());
				if (row != -1) {
					QModelIndex idx = IndexForRow(row);
					emit dataChanged(idx, idx);
					emit ItemRenamed(idx);
				}
			}
		} break;
		case 1: {
			m_stringTable.SetString((*it)->first.c_str, StringTable::LangId_EN, s.toUtf8().constData());
			emit dataChanged(
				createIndex(index.row(), 1),
				createIndex(index.row(), 1)
			);
			return true;
		} break;
		case 2: {
			m_stringTable.SetString((*it)->first.c_str, m_langId, s.toUtf8().constData());
			emit dataChanged(
				createIndex(index.row(), 2),
				createIndex(index.row(), 2)
			);
		} break;
		default:
			return false;
	}
	return true;
}

Qt::ItemFlags StringTableEditorItemModel::flags(const QModelIndex &index) const {
	if (m_editable && (index.column() == 0)) // only name is editable in this way.
		return Qt::ItemIsEditable|Qt::ItemIsEnabled|Qt::ItemIsSelectable;
	return Qt::ItemIsEnabled|Qt::ItemIsSelectable;
}

QVariant StringTableEditorItemModel::headerData(int section, Qt::Orientation orientation, int role) const {
	if (section < 0 || section > 2)
		return QVariant();
	if (role == Qt::DisplayRole) {
		if (section == 0)
			return QVariant("Name");
		if (section == 1)
			return QVariant("English");
		return QVariant(StringTable::LangTitles[m_langId]);
	} else if (role == Qt::DecorationRole) {
		if (section == 1)
			return QVariant(m_icons[StringTable::LangId_EN]);
		if (section == 2)
			return QVariant(m_icons[m_langId]);
	}
	return QVariant();
}

void StringTableEditorItemModel::Load() {
	m_stringMap.clear();
	m_stringVec.clear();
	m_stringVec.reserve(m_stringTable.entries->size());
	for (StringTable::Entry::Map::const_iterator it = m_stringTable.entries->begin(); it != m_stringTable.entries->end(); ++it) {
		m_stringMap[it->first] = (int)m_stringVec.size();
		m_stringVec.push_back(it);
	}
	reset();
}

const StringTable::Entry::Map::const_iterator *StringTableEditorItemModel::IteratorForIndex(const QModelIndex &index) const {
	if (index.isValid() && (index.row() < (int)m_stringVec.size()))
		return &m_stringVec[index.row()];
	return 0;
}

int StringTableEditorItemModel::RowForId(const char *id) const {
	RAD_ASSERT(id);
	Map::const_iterator it = m_stringMap.find(String(id));
	if (it != m_stringMap.end())
		return it->second;
	return -1;
}

QModelIndex StringTableEditorItemModel::IndexForRow(int row) const {
	if (row >= 0 && row < (int)m_stringVec.size())
		return createIndex(row, 0);
	return QModelIndex();
}

void StringTableEditorItemModel::DeleteItem(const QModelIndex &index) {
	if (index.row() < (int)m_stringVec.size()) {
		beginRemoveRows(QModelIndex(), index.row(), index.row());
		m_stringTable.DeleteId(m_stringVec[index.row()]->first.c_str);
		m_stringVec.erase(m_stringVec.begin()+index.row());
		endRemoveRows();
	}
}

void StringTableEditorItemModel::DeleteItems(const QModelIndexList &indices) {
	foreach(QModelIndex index, indices) {
		m_stringTable.DeleteId(m_stringVec[index.row()]->first.c_str);
	}
	Load();
}

void StringTableEditorItemModel::RAD_IMPLEMENT_SET(langId) (StringTable::LangId value) {
	if (m_langId != value) {
		m_langId = value;
		emit dataChanged(
			createIndex(0, 2),
			createIndex((int)m_stringVec.size(), 2)
		);
		emit headerDataChanged(Qt::Horizontal, 2, 2);
	}
}

} // editor
} // tools

#include "moc_EditorStringTableEditorItemModel.cc"
