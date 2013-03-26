// EditorStringTableWidget.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorStringTableEditorWidget.h"
#include "EditorSearchLineWidget.h"
#include "EditorComboCheckBox.h"
#include "EditorUtils.h"
#include <QtGui/QGridLayout>
#include <QtGui/QHBoxLayout>
#include <QtGui/QPushButton>
#include <QtGui/QLabel>
#include <QtGui/QGroupBox>
#include <QtGui/QTableView>
#include <QtGui/QItemDelegate>
#include <QtGui/QHeaderView>
#include <QtGui/QSortFilterProxyModel>
#include <QtGui/QTextEdit>
#include <QtCore/QAbstractItemModel>
#include "EditorStringTableEditorItemModel.h"
#include "EditorTextEditorDialog.h"

namespace tools {
namespace editor {

namespace {
inline QString ExpandNewlines(const QString &str) {
	QString x(str);
	x.replace('\n', "\\n");
	return x;
}

inline QString CollapseNewlines(const QString & str) {
	QString x(str);
	x.replace("\\n", "\n");
	return x;
}
}

const char *s_stringTableIconNames[StringTable::LangId_MAX] = {
	"Editor/Flags/flag_usa.png",
	"Editor/Flags/flag_france.png",
	"Editor/Flags/flag_italy.png",
	"Editor/Flags/flag_germany.png",
	"Editor/Flags/flag_spain.png",
	"Editor/Flags/flag_russia.png",
	"Editor/Flags/flag_japan.png",
	"Editor/Flags/flag_china.png"
};

StringTableEditorWidget::StringTableEditorWidget(
	const pkg::Asset::Ref &stringTable,
	bool editable,
	QWidget *parent
) : 
QWidget(parent),
m_sortColumn(0),
m_parser(0),
m_sort(Qt::AscendingOrder) 
{

	RAD_ASSERT(stringTable);
	if (stringTable->type == asset::AT_StringTable) {
		m_stringTable = stringTable;
		m_parser = asset::StringTableParser::Cast(stringTable);
	}

	for (int i = 0; i < StringTable::LangId_MAX; ++i) {
		if (s_stringTableIconNames[i])
			m_icons[i] = LoadIcon(s_stringTableIconNames[i]);
	}

	QGridLayout *mainLayout = new (ZEditor) QGridLayout(this);
	QHBoxLayout *controlStrip = new (ZEditor) QHBoxLayout();

	QPushButton *addButton = new (ZEditor) QPushButton(
		LoadIcon("Editor/add2.png"),
		QString(),
		this
	);

	addButton->setIconSize(QSize(32, 32));
	addButton->setToolTip("Add String");

	RAD_VERIFY(connect(addButton, SIGNAL(clicked()), SLOT(OnAddClicked())));

	controlStrip->addWidget(addButton);

	m_cloneButton = new (ZEditor) QPushButton(
		LoadIcon("Editor/documents_new.png"),
		QString(),
		this
	);

	m_cloneButton->setIconSize(QSize(32, 32));
	m_cloneButton->setEnabled(false);
	m_cloneButton->setToolTip("Clone Selected String(s)");

	RAD_VERIFY(connect(m_cloneButton, SIGNAL(clicked()), SLOT(OnCloneClicked())));

	controlStrip->addWidget(m_cloneButton);

	m_delButton = new (ZEditor) QPushButton(
		LoadIcon("Editor/delete2.png"),
		QString(),
		this
	);

	m_delButton->setIconSize(QSize(32, 32));
	m_delButton->setEnabled(false);
	m_delButton->setToolTip("Delete Selected String(s)");

	RAD_VERIFY(connect(m_delButton, SIGNAL(clicked()), SLOT(OnDeleteClicked())));

	controlStrip->addWidget(m_delButton);
	controlStrip->addSpacing(48);

	QLabel *label = new (ZEditor) QLabel(
		"Language:",
		this
	);

	controlStrip->addWidget(label);

	m_languages = new (ZEditor) QComboBox(this);

	for (int i = StringTable::LangId_FR; i < StringTable::LangId_MAX; ++i) {
		m_languages->addItem(
			m_icons[i],
			StringTable::LangTitles[i],
			qVariantFromValue(i)
		);
	}

	RAD_VERIFY(connect(m_languages, SIGNAL(currentIndexChanged(int)), SLOT(OnLanguageChanged(int))));

	controlStrip->addWidget(m_languages);
	controlStrip->addStretch(1);

	QGroupBox *filterGroup = new (ZEditor) QGroupBox("Filter", this);
	QHBoxLayout *groupLayout = new (ZEditor) QHBoxLayout(filterGroup);

	label = new (ZEditor) QLabel("Fields:", this);
	groupLayout->addWidget(label);

	m_searchTypes = new ComboCheckBox(this);
	m_searchTypes->setStrings("All Fields", "");
	m_searchTypes->addItem("Name", true);
	m_searchTypes->addItem("English Text", true);

	RAD_VERIFY(connect(m_searchTypes, SIGNAL(OnItemChecked(int, bool)), SLOT(OnSearchItemChecked(int, bool))));
	RAD_VERIFY(connect(m_searchTypes, SIGNAL(OnAllChecked(bool)), SLOT(OnSearchItemAllChecked(bool))));

	groupLayout->addWidget(m_searchTypes);
	groupLayout->addSpacing(24);

	m_search = new (ZEditor) SearchLineWidget(this);
	RAD_VERIFY(connect(m_search, SIGNAL(textChanged(const QString&)), SLOT(OnSearchTextChanged(const QString&))));

	groupLayout->addWidget(m_search);

	controlStrip->addWidget(filterGroup);

	mainLayout->addLayout(controlStrip, 0, 0);

	m_table = new (ZEditor) QTableView(this);
	m_table->setSortingEnabled(true);
	m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
	m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
	m_table->sortByColumn(m_sortColumn, m_sort);
	m_table->setShowGrid(true);
	m_table->verticalHeader()->hide();
	m_table->horizontalHeader()->setStretchLastSection(true);

	RAD_VERIFY(connect(
		m_table->horizontalHeader(), 
		SIGNAL(sortIndicatorChanged(int, Qt::SortOrder)), 
		SLOT(OnSortChanged(int, Qt::SortOrder))
	));

	RAD_VERIFY(connect(m_table, SIGNAL(doubleClicked(const QModelIndex&)), SLOT(OnItemDoubleClicked(const QModelIndex&))));

	m_model = new (ZEditor) StringTableEditorItemModel(
		*m_parser->stringTable.get(),
		StringTable::LangId_FR,
		editable,
		m_icons,
		this
	);

	m_sortModel = new (ZEditor) QSortFilterProxyModel(this);
	m_sortModel->setSourceModel(m_model);
	m_sortModel->setDynamicSortFilter(true);
	
	RAD_VERIFY(connect(m_model, SIGNAL(ItemRenamed(const QModelIndex&)), SLOT(OnItemRenamed(const QModelIndex&))));
	RAD_VERIFY(connect(m_sortModel, SIGNAL(modelReset()), SLOT(OnModelReset())));

	m_table->setModel(m_sortModel);
	RAD_VERIFY(connect(
		m_table->selectionModel(), 
		SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)),
		SLOT(OnSelectionChanged(const QItemSelection&, const QItemSelection&))
	));

	m_model->Load();

	mainLayout->addWidget(m_table, 1, 0);
	mainLayout->setRowStretch(1, 1);
}

void StringTableEditorWidget::showEvent(QShowEvent *e) {
	QWidget::showEvent(e);
	m_table->horizontalHeader()->resizeSections(QHeaderView::Stretch);
}

void StringTableEditorWidget::resizeEvent(QResizeEvent *e) {
	QWidget::resizeEvent(e);
	m_table->horizontalHeader()->resizeSections(QHeaderView::Stretch);
}

void StringTableEditorWidget::OnSearchTextChanged(const QString &text) {
}

void StringTableEditorWidget::OnSearchItemChecked(int index, bool checked) {
}

void StringTableEditorWidget::OnSearchItemAllChecked(bool checked) {
}

void StringTableEditorWidget::OnLanguageChanged(int index) {
	QVariant data = m_languages->itemData(index);
	if (data.isValid() && data.type() == QVariant::Int)
		m_model->langId = (StringTable::LangId)data.toInt();
}

void StringTableEditorWidget::OnAddClicked() {
	String id(CStr("New String"));

	for (int i = 2; ; ++i) {
		if (m_parser->stringTable->CreateId(id.c_str))
			break;
		id.Printf("New String %d", i);
	}

	m_model->Load();
	// select the new string
	int row = m_model->RowForId(id.c_str);
	QModelIndex index = m_model->IndexForRow(row);
	index = m_sortModel->mapFromSource(index);
	m_table->selectRow(index.row());
	m_table->scrollTo(index);

	m_table->setFocus(Qt::MouseFocusReason);
	SaveChanges();
}

void StringTableEditorWidget::OnCloneClicked() {
	QModelIndexList sel = m_table->selectionModel()->selectedIndexes();
	if (sel.empty())
		return;

	QModelIndexList mapped;

	foreach (QModelIndex index, sel) {
		if (index.column() == 0) {
			mapped.push_back(m_sortModel->mapToSource(index));
		}
	}

	CloneItems(mapped);
}

void StringTableEditorWidget::CloneItems(const QModelIndexList &items) {

	StringVec added;

	foreach (QModelIndex index, items) {
		const StringTable::Entry::Map::const_iterator *it = m_model->IteratorForIndex(index);
		if (it) {
			const String &baseName = (*it)->first;
			String name;

			for (int i = 2; ; ++i) {
				name.Printf("%s_%d", baseName.c_str.get(), i);

				if (m_parser->stringTable->CreateId(name.c_str))
					break;
			}

			const StringTable::Entry &entry = (*it)->second;

			for (StringTable::Entry::Strings::const_iterator it = entry.strings.begin(); it != entry.strings.end(); ++it) {
				m_parser->stringTable->SetString(name.c_str, (StringTable::LangId)it->first, it->second.c_str);
			}

			added.push_back(name);
		}
	}

	if (!added.empty()) {
		m_model->Load();
		// select the new strings
		m_table->setSelectionMode(QAbstractItemView::MultiSelection);
		for (StringVec::const_iterator it = added.begin(); it != added.end(); ++it) {
			int row = m_model->RowForId((*it).c_str);
			QModelIndex index = m_model->IndexForRow(row);
			index = m_sortModel->mapFromSource(index);
			m_table->selectRow(index.row());
			if (it == added.begin())
				m_table->scrollTo(index);
		}
		m_table->setSelectionMode(QAbstractItemView::ExtendedSelection);
		m_table->setFocus(Qt::MouseFocusReason);
	}

	SaveChanges();
}

void StringTableEditorWidget::OnDeleteClicked() {
	QModelIndexList sel = m_table->selectionModel()->selectedIndexes();
	if (sel.empty())
		return;

	QModelIndexList mapped;

	foreach (QModelIndex index, sel) {
		if (index.column() == 0) {
			mapped.push_back(m_sortModel->mapToSource(index));
		}
	}

	if (mapped.size() > 1) {
		m_model->DeleteItems(mapped);
	} else {
		m_model->DeleteItem(mapped[0]);
	}

	m_delButton->setEnabled(false);
	SaveChanges();
}

void StringTableEditorWidget::OnSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
	m_delButton->setEnabled(m_table->selectionModel()->hasSelection());
	m_cloneButton->setEnabled(m_table->selectionModel()->hasSelection());
}

void StringTableEditorWidget::OnSortChanged(int index, Qt::SortOrder sort) {
	if (index != m_sortColumn || sort != m_sort) {
		m_sortColumn = index;
		m_sort = sort;
		m_table->sortByColumn(index, sort);
	}
}

void StringTableEditorWidget::OnItemRenamed(const QModelIndex &index) {
	QModelIndex dst = m_sortModel->mapFromSource(index);
	m_table->selectRow(dst.row());
	m_table->scrollTo(dst);
	SaveChanges();
}

void StringTableEditorWidget::OnItemDoubleClicked(const QModelIndex &index) {
	if (!index.isValid() || index.column() == 0)
		return;
	QModelIndex mappedIndex = m_sortModel->mapToSource(index);

	QVariant data = m_sortModel->data(index, Qt::EditRole);
	
	const StringTable::Entry::Map::const_iterator *it = m_model->IteratorForIndex(mappedIndex);
	if (it) {

		QString editString;
		if (data.isValid() && data.type() == QVariant::String)
			editString = CollapseNewlines(data.toString());

		TextEditorDialog dlg(QString(), parentWidget());
		dlg.textEdit->setPlainText(editString);

		QString lang;

		if (index.column() == 1) {
			dlg.setWindowIcon(m_icons[StringTable::LangId_EN]);
			lang = "English";
		} else {
			dlg.setWindowIcon(m_icons[m_model->langId]);
			lang = StringTable::LangTitles[m_model->langId];
		}

		dlg.setWindowTitle(QString("Editing: %1 (%2)").arg((*it)->first.c_str.get(), lang));

		if (dlg.exec() == QDialog::Accepted) {
			if (m_sortModel->setData(index, ExpandNewlines(dlg.textEdit->toPlainText()), Qt::EditRole)) {
				SaveChanges();
			}
		}
	}
}

void StringTableEditorWidget::OnModelReset() {
	m_delButton->setEnabled(false);
	m_cloneButton->setEnabled(false);
}

void StringTableEditorWidget::SaveChanges() {
	asset::StringTableParser::Save(
		*App::Get()->engine.get(),
		m_stringTable,
		0
	);

	pkg::Package::Entry::AssetModifiedEventData data;
	data.origin = m_stringTable->entry;
	data.zone = m_stringTable->zone;
	m_stringTable->entry->OnAssetModified.Trigger(data);
}

} // editor
} // tools

#include "moc_EditorStringTableEditorWidget.cc"
