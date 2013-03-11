/*! \file EditorConversationTreeEditorWidget.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#pragma once

#include RADPCH
#include "EditorUtils.h"
#include "EditorConversationTreeEditorWidget.h"
#include "EditorConversationTreeEditorItemEditDialog.h"
#include "../../StringTable.h"
#include <QtGui/QBoxLayout>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>
#include <QtGui/QListWidget>
#include <QtGui/QSplitter>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>

namespace tools {
namespace editor {

// See EditorStringTableEditorWidget.cpp
extern const char *s_stringTableIconNames[StringTable::LangId_MAX];

ConversationTreeEditorWidget::ConversationTreeEditorWidget(
	const pkg::Asset::Ref &conversationTree,
	bool editable,
	QWidget *parent
) : QWidget(parent) {

	if (conversationTree->type == asset::AT_ConversationTree) {
		m_conversationTree = conversationTree;
		m_parser = asset::ConversationTreeParser::Cast(conversationTree);

		Bind(
			m_parser->stringTableAsset->entry->OnAssetModified,
			&ConversationTreeEditorWidget::OnStringTableDataChanged
		);
	}

	QHBoxLayout *hbox = new (ZEditor) QHBoxLayout(this);
	
	m_scene = new (ZEditor) QGraphicsScene(this);
	m_view = new (ZEditor) QGraphicsView(m_scene);

	hbox->addWidget(m_view);

	QSplitter *splitter = new (ZEditor) QSplitter();
	splitter->setOpaqueResize();
	splitter->setOrientation(Qt::Vertical);

	QWidget *container = new (ZEditor) QWidget();
	QVBoxLayout *vbox = new (ZEditor) QVBoxLayout(container);

	QLabel *label = new QLabel("Preview Language:");
	
	QHBoxLayout *hbox2 = new QHBoxLayout();
	hbox2->addStretch(1);
	hbox2->addWidget(label);

	m_languages = new QComboBox();
	hbox2->addWidget(m_languages);

	vbox->addLayout(hbox2);
	vbox->addSpacing(16);
	
	QIcon icons[StringTable::LangId_MAX];
	for (int i = 0; i < StringTable::LangId_MAX; ++i) {
		if (s_stringTableIconNames[i])
			icons[i] = LoadIcon(s_stringTableIconNames[i]);
	}

	for (int i = StringTable::LangId_EN; i < StringTable::LangId_MAX; ++i) {
		m_languages->addItem(
			icons[i],
			StringTable::LangTitles[i],
			QVariant(i)
		);
	}

	RAD_VERIFY(connect(m_languages, SIGNAL(currentIndexChanged(int)), SLOT(OnLanguageChanged(int))));

	m_langId = StringTable::LangId_EN;

	label = new QLabel("Topics:");
	vbox->addWidget(label);
	m_roots = new (ZEditor) QListWidget();
	m_roots->setSortingEnabled(true);
	RAD_VERIFY(connect(m_roots, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnRootListSelected(QListWidgetItem*))));

	vbox->addWidget(m_roots);

	QIcon editIcon = LoadIcon("Editor/edit_small.png");
	QIcon addIcon = LoadIcon("Editor/add2_small.png");
	QIcon deleteIcon = LoadIcon("Editor/delete2_small.png");

	QHBoxLayout *buttonLayout = new (ZEditor) QHBoxLayout();
	buttonLayout->addStretch(1);
	QPushButton *button = new (ZEditor) QPushButton(addIcon, "Add Topic...");
	buttonLayout->addWidget(button);
	RAD_VERIFY(connect(button, SIGNAL(clicked()), SLOT(AddRootPressed())));
	m_editRoot = new (ZEditor) QPushButton(editIcon, "Edit Topic...");
	m_editRoot->setEnabled(false);
	buttonLayout->addWidget(m_editRoot);
	RAD_VERIFY(connect(m_editRoot, SIGNAL(clicked()), SLOT(EditRootPressed())));
	m_deleteRoot = new (ZEditor) QPushButton(deleteIcon, "Delete Topic");
	m_deleteRoot->setEnabled(false);
	buttonLayout->addWidget(m_deleteRoot);
	RAD_VERIFY(connect(m_deleteRoot, SIGNAL(clicked()), SLOT(DeleteRootPressed())));
	vbox->addLayout(buttonLayout);

	splitter->addWidget(container);

	container = new (ZEditor) QWidget();
	vbox = new (ZEditor) QVBoxLayout(container);

	label = new QLabel("Dialog:");
	vbox->addWidget(label);
	m_dialog = new (ZEditor) QListWidget();
	m_dialog->setSortingEnabled(true);
	RAD_VERIFY(connect(m_dialog, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnDialogListSelected(QListWidgetItem*))));
	vbox->addWidget(m_dialog);

	buttonLayout = new (ZEditor) QHBoxLayout();
	buttonLayout->addStretch(1);
	button = new (ZEditor) QPushButton(editIcon, "Add Dialog...");
	buttonLayout->addWidget(button);
	RAD_VERIFY(connect(button, SIGNAL(clicked()), SLOT(AddDialogPressed())));
	m_editDialog = new (ZEditor) QPushButton(editIcon, "Edit Dialog...");
	m_editDialog->setEnabled(false);
	buttonLayout->addWidget(m_editDialog);
	RAD_VERIFY(connect(m_editDialog, SIGNAL(clicked()), SLOT(EditDialogPressed())));
	m_deleteDialog = new (ZEditor) QPushButton(addIcon, "Delete Dialog");
	m_deleteDialog->setEnabled(false);
	buttonLayout->addWidget(m_deleteDialog);
	RAD_VERIFY(connect(m_deleteDialog, SIGNAL(clicked()), SLOT(DeleteDialogPressed())));
	vbox->addLayout(buttonLayout);

	splitter->addWidget(container);
	hbox->addWidget(splitter);

	LoadRoots();
	LoadDialog();
}

void ConversationTreeEditorWidget::EditRootPressed() {
}

void ConversationTreeEditorWidget::AddRootPressed() {
	asset::ConversationTree::Root root;
	root.name = "Enter Name";
	root.group = "Default";
	
	ConversationTreeEditorItemEditDialog dlg(root, *m_parser, this);
	dlg.SelectName();

	if (dlg.exec() == QDialog::Accepted) {
		asset::ConversationTree::Root *x = m_parser->conversationTree->NewRoot();
		*x = *dlg.root.get();
		(*m_parser->conversationTree->roots.get())->push_back(x);
		int pos = (int)(*m_parser->conversationTree->roots.get())->size();
		QListWidgetItem *item = new QListWidgetItem(x->name.c_str.get());
		item->setData(Qt::UserRole, qVariantFromValue((void*)x));
		m_roots->addItem(item);
		m_roots->setCurrentItem(item);
	}
}

void ConversationTreeEditorWidget::DeleteRootPressed() {
}

void ConversationTreeEditorWidget::EditDialogPressed() {
}

void ConversationTreeEditorWidget::AddDialogPressed() {
}

void ConversationTreeEditorWidget::DeleteDialogPressed() {
}

void ConversationTreeEditorWidget::OnLanguageChanged(int index) {
	m_langId = m_languages->itemData(index).toInt();
	ReloadStrings();
}

void ConversationTreeEditorWidget::OnRootListSelected(QListWidgetItem *item) {
	m_deleteRoot->setEnabled(item != 0);
}

void ConversationTreeEditorWidget::OnDialogListSelected(QListWidgetItem *item) {
	m_deleteDialog->setEnabled(item != 0);
}

void ConversationTreeEditorWidget::LoadRoots() {
}

void ConversationTreeEditorWidget::LoadDialog() {
}

void ConversationTreeEditorWidget::OnStringTableDataChanged(const pkg::Package::Entry::AssetModifiedEventData &data) {
	ReloadStrings();
}

void ConversationTreeEditorWidget::ReloadStrings() {
}

} // editor
} // tools

#include "moc_EditorConversationTreeEditorWidget.cc"
