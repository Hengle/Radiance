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
#include "EditorConversationTreeEditorView.h"
#include "../../StringTable.h"
#include <QtGui/QBoxLayout>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>
#include <QtGui/QListWidget>
#include <QtGui/QSplitter>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QMessageBox>
#include <QtCore/QMimeData>
#include <QtCore/QByteArray>
#include <QtCore/QBuffer>

namespace tools {
namespace editor {

// See EditorStringTableEditorWidget.cpp
extern const char *s_stringTableIconNames[StringTable::LangId_MAX];

ConversationTreeEditorWidget::ConversationTreeEditorWidget(
	const pkg::Asset::Ref &conversationTree,
	bool editable,
	QWidget *parent
) : QWidget(parent), m_langId(StringTable::LangId_EN) {

	if (conversationTree->type == asset::AT_ConversationTree) {
		m_conversationTree = conversationTree;
		m_parser = asset::ConversationTreeParser::Cast(conversationTree);
	}

	QHBoxLayout *hbox = new (ZEditor) QHBoxLayout(this);
	
	m_view = new (ZEditor) ConversationTreeEditorView(m_langId, m_parser);

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

	label = new QLabel("Topics:");
	vbox->addWidget(label);
	m_roots = new (ZEditor) QListWidget();
	m_roots->setSortingEnabled(true);
	RAD_VERIFY(connect(m_roots, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnRootListSelected(QListWidgetItem*))));
	RAD_VERIFY(connect(m_roots, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(EditRootPressed())));

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
	m_dialog = new (ZEditor) ConversationTreeEditorWidgetDialogDragList();
	m_dialog->setSortingEnabled(true);
	m_dialog->setDragDropMode(QAbstractItemView::DragOnly);
	m_dialog->setDragEnabled(true);
	m_dialog->setDropIndicatorShown(true);
	RAD_VERIFY(connect(m_dialog, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnDialogListSelected(QListWidgetItem*))));
	RAD_VERIFY(connect(m_dialog, SIGNAL(itemDoubleClicked(QListWidgetItem*)), SLOT(EditDialogPressed())));
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
	QListWidgetItem *item = m_roots->currentItem();
	asset::ConversationTree::Root *root = (asset::ConversationTree::Root*)item->data(Qt::UserRole).value<void*>();

	ConversationTreeEditorItemEditDialog dlg(m_langId, *root, *m_parser, this);
	
	if (dlg.exec() == QDialog::Accepted) {
		*root = *dlg.root.get();
		item->setText(root->name.c_str.get());
		m_view->ReloadItems();
	}
}

void ConversationTreeEditorWidget::AddRootPressed() {
	asset::ConversationTree::Root root;
	root.name = "[Enter Name]";
	root.group = "Default";
	
	ConversationTreeEditorItemEditDialog dlg(m_langId, root, *m_parser, this);
	dlg.SelectName();

	if (dlg.exec() == QDialog::Accepted) {
		asset::ConversationTree::Root *x = m_parser->conversationTree->NewRoot();
		*x = *dlg.root.get();
		(*m_parser->conversationTree->roots.get())->push_back(x);
		QListWidgetItem *item = new QListWidgetItem(x->name.c_str.get());
		item->setData(Qt::UserRole, qVariantFromValue((void*)x));
		m_roots->addItem(item);
		m_roots->setCurrentItem(item);
	}
}

void ConversationTreeEditorWidget::DeleteRootPressed() {

	if (QMessageBox::warning(
		this,
		"Are you sure?", 
		"This will delete the selected topic. This action cannot be undone. Proceed?", 
		QMessageBox::Yes|QMessageBox::Cancel
	) != QMessageBox::Yes) {
		return;
	}

	QListWidgetItem *item = m_roots->currentItem();

	asset::ConversationTree::Root *root = (asset::ConversationTree::Root*)item->data(Qt::UserRole).value<void*>();
	for (asset::ConversationTree::Root::PtrVec::iterator it = (*m_parser->conversationTree->roots)->begin(); it != (*m_parser->conversationTree->roots)->end(); ++it) {
		if (*it == root) {
			(*m_parser->conversationTree->roots)->erase(it);
			m_parser->conversationTree->DeleteRoot(*root);
			LoadRoots();
			break;
		}
	}
}

void ConversationTreeEditorWidget::EditDialogPressed() {
	QListWidgetItem *item = m_dialog->currentItem();
	asset::ConversationTree::Dialog *dialog = (asset::ConversationTree::Dialog*)item->data(Qt::UserRole).value<void*>();

	ConversationTreeEditorItemEditDialog dlg(m_langId, *dialog, *m_parser, this);
	
	if (dlg.exec() == QDialog::Accepted) {
		*dialog = *dlg.dialog.get();
		item->setText(dialog->name.c_str.get());
		m_view->ReloadItems();
	}
}

void ConversationTreeEditorWidget::AddDialogPressed() {
	asset::ConversationTree::Dialog dialog;
	dialog.name = "[Enter Name]";
		
	ConversationTreeEditorItemEditDialog dlg(m_langId, dialog, *m_parser, this);
	dlg.SelectName();

	if (dlg.exec() == QDialog::Accepted) {
		asset::ConversationTree::Dialog *x = m_parser->conversationTree->NewDialog();
		*x = *dlg.dialog.get();
		QListWidgetItem *item = new QListWidgetItem(x->name.c_str.get());
		item->setData(Qt::UserRole, qVariantFromValue((void*)x));
		m_dialog->addItem(item);
		m_dialog->setCurrentItem(item);
	}
}

void ConversationTreeEditorWidget::DeleteDialogPressed() {

	if (QMessageBox::warning(
		this,
		"Are you sure?", 
		"This will delete the selected dialog. This dialog will be removed from any parent dialogs and topics. This action cannot be undone. Proceed?", 
		QMessageBox::Yes|QMessageBox::Cancel
	) != QMessageBox::Yes) {
		return;
	}

	QListWidgetItem *item = m_dialog->currentItem();

	asset::ConversationTree::Dialog *dialog = (asset::ConversationTree::Dialog*)item->data(Qt::UserRole).value<void*>();
	RemoveDialogFromTree(*dialog);
	RAD_ASSERT(dialog->refs == 0);
	m_view->DialogDeleted(*dialog);
	m_parser->conversationTree->DeleteDialog(*dialog);
	LoadDialog();
}

void ConversationTreeEditorWidget::OnLanguageChanged(int index) {
	m_langId = m_languages->itemData(index).toInt();
	m_view->SetLangId(m_langId);
}

void ConversationTreeEditorWidget::OnRootListSelected(QListWidgetItem *item) {
	m_deleteRoot->setEnabled(item != 0);
	m_editRoot->setEnabled(item != 0);

	if (item) {
		asset::ConversationTree::Root *root = (asset::ConversationTree::Root*)item->data(Qt::UserRole).value<void*>();
		m_view->SelectRoot(root);
	} else {
		m_view->SelectRoot(0);
	}
}

void ConversationTreeEditorWidget::OnDialogListSelected(QListWidgetItem *item) {
	m_deleteDialog->setEnabled(item != 0);
	m_editDialog->setEnabled(item != 0);
}

void ConversationTreeEditorWidget::LoadRoots() {
	m_roots->blockSignals(true);
	m_roots->clear();

	for (asset::ConversationTree::Root::PtrVec::const_iterator it = (*m_parser->conversationTree->roots)->begin(); it != (*m_parser->conversationTree->roots)->end(); ++it) {
		const asset::ConversationTree::Root &root = **it;
		QListWidgetItem *item = new (ZEditor) QListWidgetItem(root.name.c_str.get());
		item->setData(Qt::UserRole, qVariantFromValue((void*)&root));
		m_roots->addItem(item);
	}

	m_deleteRoot->setEnabled(false);
	m_editRoot->setEnabled(false);
	m_view->SelectRoot(0);
	m_roots->blockSignals(false);
}

void ConversationTreeEditorWidget::LoadDialog() {
	m_dialog->blockSignals(true);
	m_dialog->clear();

	for (asset::ConversationTree::Dialog::UIDMap::const_iterator it = m_parser->conversationTree->dialogs->begin(); it != m_parser->conversationTree->dialogs->end(); ++it) {
		const asset::ConversationTree::Dialog &dialog = *it->second;
		QListWidgetItem *item = new (ZEditor) QListWidgetItem(dialog.name.c_str.get());
		item->setData(Qt::UserRole, qVariantFromValue((void*)&dialog));
		m_roots->addItem(item);
	}

	m_deleteDialog->setEnabled(false);
	m_editDialog->setEnabled(false);
	m_dialog->blockSignals(false);
}

void ConversationTreeEditorWidget::RemoveDialogFromTree(asset::ConversationTree::Dialog &dialog) {
	asset::ConversationTree::Root::PtrVec &roots = *m_parser->conversationTree->roots;

	for (asset::ConversationTree::Root::PtrVec::iterator it = roots->begin(); it != roots->end(); ++it) {
		RemoveDialogFromTree(**it, dialog);
	}
}

void ConversationTreeEditorWidget::RemoveDialogFromTree(
	asset::ConversationTree::Root &root,
	asset::ConversationTree::Dialog &dialog
) {
	for (asset::ConversationTree::Dialog::PtrVec::iterator it = root.dialog->begin(); it != root.dialog->end();) {
		asset::ConversationTree::Dialog::PtrVec::iterator next(it); ++next;

		if (*it == &dialog) {
			root.dialog->erase(it);
			m_parser->conversationTree->UnrefDialog(dialog);
		} else {
			RemoveDialogFromTree(**it, dialog);
		}

		it = next;
	}
}

void ConversationTreeEditorWidget::RemoveDialogFromTree(
	asset::ConversationTree::Dialog &parent,
	asset::ConversationTree::Dialog &dialog
) {
	for (asset::ConversationTree::Dialog::PtrVec::iterator it = parent.choices->begin(); it != parent.choices->end();) {
		asset::ConversationTree::Dialog::PtrVec::iterator next(it); ++next;

		if (*it == &dialog) {
			parent.choices->erase(it);
			m_parser->conversationTree->UnrefDialog(dialog);
		} else {
			RemoveDialogFromTree(**it, dialog);
		}

		it = next;
	}
}

///////////////////////////////////////////////////////////////////////////////

ConversationTreeEditorWidgetDialogDragList::ConversationTreeEditorWidgetDialogDragList(QWidget *parent)
	: QListWidget(parent) {
}

QMimeData *ConversationTreeEditorWidgetDialogDragList::mimeData(const QList<QListWidgetItem*> items) const {
	asset::ConversationTree::Dialog *dialog = (asset::ConversationTree::Dialog*)items[0]->data(Qt::UserRole).value<void*>();
	RAD_ASSERT(dialog);

	QByteArray output;
	QBuffer buffer(&output);
	buffer.open(QIODevice::WriteOnly);
	buffer.write((const char*)&dialog->uid, sizeof(dialog->uid));

	QMimeData *mimeData = new QMimeData();
	mimeData->setData("application/conversationDialogId", output);
	return mimeData;
}

} // editor
} // tools

#include "moc_EditorConversationTreeEditorWidget.cc"
