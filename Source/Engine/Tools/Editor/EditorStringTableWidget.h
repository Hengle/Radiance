// EditorStringTableWidget.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include "../../Assets/StringTableParser.h"
#include <QtGui/QWidget>
#include <QtGui/QIcon>
#include <Runtime/PushPack.h>

class QTableView;
class QComboBox;
class QItemSelection;
class QPushButton;
class QSortFilterProxyModel;
class QModelIndex;
class QShowEvent;
class QResizeEvent;

namespace tools {
namespace editor {

class StringTableItemModel;
class ComboCheckBox;
class SearchLineWidget;

class RADENG_CLASS StringTableWidget : public QWidget {
	Q_OBJECT
public:

	StringTableWidget(
		const pkg::Asset::Ref &stringTable,
		bool editable,
		QWidget *parent = 0
	);

protected:

	virtual void showEvent(QShowEvent*);
	virtual void resizeEvent(QResizeEvent*);

	StringTableItemModel *Model() const { return m_model; }

	QTableView *m_table;
	SearchLineWidget *m_search;
	QComboBox *m_languages;
	ComboCheckBox *m_searchTypes;

private slots:

	void OnSearchTextChanged(const QString &text);
	void OnSearchItemChecked(int index, bool checked);
	void OnSearchItemAllChecked(bool checked);
	void OnLanguageChanged(int index);
	void OnAddClicked();
	void OnDeleteClicked();
	void OnSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
	void OnSortChanged(int index, Qt::SortOrder sort);
	void OnItemRenamed(const QModelIndex &index);
	void OnItemDoubleClicked(const QModelIndex &index);
	void OnModelReset();

private:

	void SaveChanges();

	QIcon m_icons[StringTable::LangId_MAX];
	pkg::Asset::Ref m_stringTable;
	asset::StringTableParser *m_parser;
	StringTableItemModel *m_model;
	QSortFilterProxyModel *m_sortModel;
	QPushButton *m_delButton;
	int m_sortColumn;
	Qt::SortOrder m_sort;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
