/*! \file EditorConversationTreeEditorItemEditDialog.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#include RADPCH
#include "EditorConversationTreeEditorItemEditDialog.h"
#include "EditorLineEditDialog.h"
#include "EditorUtils.h"
#include <QtGui/QGridLayout>
#include <QtGui/QBoxLayout>
#include <QtGui/QFormLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QComboBox>
#include <QtGui/QLabel>
#include <QtGui/QDoubleValidator>
#include <QtGui/QGroupBox>
#include <QtGui/QCheckBox>
#include <QtGui/QListWidget>
#include <QtGui/QPushButton>
#include <QtGui/QTextEdit>

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

ConversationTreeStringOptionEditWidget::ConversationTreeStringOptionEditWidget(
	int langId,
	const StringTable &stringTable, 
	QWidget *parent
) : QDialog(parent), m_langId(langId), m_opt(0) {

	m_stringTable = &stringTable;

	QFormLayout *form = new (ZEditor) QFormLayout(this);

	m_string = new (ZEditor) QComboBox();
	form->addRow("String:", m_string);
	RAD_VERIFY(connect(m_string, SIGNAL(currentIndexChanged(int)), SLOT(DoDataChanged())));

	m_text = new (ZEditor) QTextEdit();
	m_text->setReadOnly(true);
	m_text->setLineWrapMode(QTextEdit::WidgetWidth);
	m_text->setFixedHeight(75);
	form->addRow("Text:", m_text);

	QGroupBox *group = new (ZEditor) QGroupBox("Probability");
	QFormLayout *gform = new (ZEditor) QFormLayout(group);

	m_probabilityLow = new (ZEditor) QLineEdit();
	m_probabilityLow->setValidator(new (ZEditor) QDoubleValidator(this));
	gform->addRow("Min:", m_probabilityLow);

	RAD_VERIFY(connect(m_probabilityLow, SIGNAL(textEdited(const QString&)), SLOT(DoDataChanged())));

	m_probabilityHigh = new (ZEditor) QLineEdit();
	m_probabilityHigh->setValidator(new (ZEditor) QDoubleValidator(this));
	gform->addRow("Max:", m_probabilityHigh);

	RAD_VERIFY(connect(m_probabilityHigh, SIGNAL(textEdited(const QString&)), SLOT(DoDataChanged())));

	form->addRow(group);

	ReloadStrings();
	setEnabled(false);
}

void ConversationTreeStringOptionEditWidget::Load(asset::ConversationTree::StringOption *opt) {
	m_opt = opt;

	if (opt) {
		int index = m_string->findText(opt->text.c_str.get());
		m_string->blockSignals(true);
		m_probabilityLow->blockSignals(true);
		m_probabilityHigh->blockSignals(true);
		m_string->setCurrentIndex(index);
		if (index >= 0) {
			m_opt->text = m_string->itemText(index).toAscii().constData();
		} else {
			m_opt->text.Clear();
		}
		m_probabilityLow->setText(QString("%1").arg(opt->probability[0]));
		m_probabilityHigh->setText(QString("%1").arg(opt->probability[1]));
		m_string->blockSignals(false);
		m_probabilityLow->blockSignals(false);
		m_probabilityHigh->blockSignals(false);
		setEnabled(true);
	} else {
		m_string->setCurrentIndex(-1);
		m_probabilityLow->setText("");
		m_probabilityHigh->setText("");
		setEnabled(false);
	}

	LoadText();
}

void ConversationTreeStringOptionEditWidget::DoDataChanged() {
	if (m_opt) {
		int index = m_string->currentIndex();
		if (index >= 0) {
			m_opt->text = m_string->itemText(index).toAscii().constData();
		} else {
			m_opt->text.Clear();
		}
		m_opt->probability[0] = m_probabilityLow->text().toFloat();
		m_opt->probability[1] = m_probabilityHigh->text().toFloat();
	}
	LoadText();
	emit OnDataChanged();
}

void ConversationTreeStringOptionEditWidget::ReloadStrings() {
	m_string->blockSignals(true);
	m_string->clear();
	for (StringTable::Entry::Map::const_iterator it = m_stringTable->entries->begin(); it != m_stringTable->entries->end(); ++it) {
		m_string->addItem(it->first.c_str.get());
	}
	
	if (m_opt) {
		int index = m_string->findText(m_opt->text.c_str.get());
		m_string->setCurrentIndex(index);
	}

	LoadText();

	m_string->blockSignals(false);
}

void ConversationTreeStringOptionEditWidget::SetLangId(int langId) {
	m_langId = langId;
	LoadText();
}

void ConversationTreeStringOptionEditWidget::LoadText() {
	if (m_opt) {
		const String *string = m_stringTable->Find(m_opt->text.c_str, (StringTable::LangId)m_langId);
		if (string) {
			m_text->setText(string->c_str.get());
		} else {
			m_text->setText("<NOT TRANSLATED>");
		}
	} else {
		m_text->setText("");
	}
}

///////////////////////////////////////////////////////////////////////////////

ConversationTreeEditorItemEditDialog::ConversationTreeEditorItemEditDialog(
	int langId,
	asset::ConversationTree::Root &root,
	asset::ConversationTreeParser &parser,
	QWidget *parent
) : QDialog(parent), m_rootCopy(root), m_dialog(0), m_parser(&parser),  m_langId(langId) {
	setWindowModality(Qt::WindowModal);
	m_root = &m_rootCopy;
	InitRoot();

	Bind(
		m_parser->stringTableAsset->entry->OnAssetModified,
		&ConversationTreeEditorItemEditDialog::OnStringTableDataChanged
	);
}

ConversationTreeEditorItemEditDialog::ConversationTreeEditorItemEditDialog(
	int langId,
	asset::ConversationTree::Dialog &dialog,
	asset::ConversationTreeParser &parser,
	QWidget *parent
) : QDialog(parent), m_dialogCopy(dialog), m_root(0), m_parser(&parser),  m_langId(langId) {
	setWindowModality(Qt::WindowModal);
	m_dialog = &m_dialogCopy;
	InitDialog();

	Bind(
		m_parser->stringTableAsset->entry->OnAssetModified,
		&ConversationTreeEditorItemEditDialog::OnStringTableDataChanged
	);
}

void ConversationTreeEditorItemEditDialog::InitRoot() {

	m_action = 0;
	m_condition = 0;
	m_deleteReply = 0;
	m_replies = 0;
	m_replyEdit = 0;

	setWindowTitle("Edit Topic");

	QGridLayout *outer = new (ZEditor) QGridLayout(this);

	QGroupBox *group = new (ZEditor) QGroupBox("Properties");
	QFormLayout *form = new (ZEditor) QFormLayout(group);
		
	m_name = new (ZEditor) QLineEdit();
	form->addRow("Name:", m_name);
	
	m_group = new (ZEditor) QLineEdit();
	form->addRow("Group:", m_group);

	QGroupBox *pgroup = new (ZEditor) QGroupBox("Probability");
	QFormLayout *pform = new (ZEditor) QFormLayout(pgroup);

	m_probabilityLow = new (ZEditor) QLineEdit();
	m_probabilityLow->setValidator(new (ZEditor) QDoubleValidator(this));
	pform->addRow("Min:", m_probabilityLow);

	m_probabilityHigh = new (ZEditor) QLineEdit();
	m_probabilityHigh->setValidator(new (ZEditor) QDoubleValidator(this));
	pform->addRow("Max:", m_probabilityHigh);

	form->addRow(pgroup);

	pgroup = new (ZEditor) QGroupBox("Flags");
	QGridLayout *pgrid = new (ZEditor) QGridLayout(pgroup);
	m_hidden = new (ZEditor) QCheckBox("Hidden");
	pgrid->addWidget(m_hidden, 0, 0);
	m_locked = new (ZEditor) QCheckBox("Locked");
	pgrid->addWidget(m_locked, 0, 1);

	form->addRow(pgroup);

	outer->addWidget(group, 0, 0);

	QIcon addIcon = LoadIcon("Editor/add2_small.png");
	QIcon deleteIcon = LoadIcon("Editor/delete2_small.png");

	group = new QGroupBox("Prompts");
	
	QVBoxLayout *vbox = new (ZEditor) QVBoxLayout(group);
	m_prompts = new (ZEditor) QListWidget();
	m_prompts->setSelectionMode(QAbstractItemView::SingleSelection);
	RAD_VERIFY(connect(m_prompts, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnSelectedPromptChanged(QListWidgetItem*))));
	vbox->addWidget(m_prompts);

	QHBoxLayout *hbox = new (ZEditor) QHBoxLayout();
	hbox->addStretch(1);

	QPushButton *b = new (ZEditor) QPushButton(addIcon, "Add Prompt");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(OnAddPrompt())));
	hbox->addWidget(b);

	m_deletePrompt = new (ZEditor) QPushButton(deleteIcon, "Delete Prompt");
	m_deletePrompt->setEnabled(false);
	RAD_VERIFY(connect(m_deletePrompt, SIGNAL(clicked()), SLOT(OnDeletePrompt())));
	hbox->addWidget(m_deletePrompt);
	vbox->addLayout(hbox);

	m_promptEdit = new (ZEditor) ConversationTreeStringOptionEditWidget(m_langId, *m_parser->stringTable);
	vbox->addWidget(m_promptEdit);

	outer->addWidget(group, 0, 1);

	hbox = new (ZEditor) QHBoxLayout();
	hbox->addStretch(1);

	m_ok = new (ZEditor) QPushButton("OK");
	RAD_VERIFY(connect(m_ok, SIGNAL(clicked()), SLOT(accept())));
	hbox->addWidget(m_ok);

	b = new (ZEditor) QPushButton("Cancel");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(reject())));
	hbox->addWidget(b);

	outer->addLayout(hbox, 1, 0, 1, 2);

	Load();
}

void ConversationTreeEditorItemEditDialog::InitDialog() {
	setWindowTitle("Edit Dialog");

	m_hidden = 0;
	m_locked = 0;
	m_group = 0;
	m_probabilityHigh = 0;

	QGridLayout *outer = new (ZEditor) QGridLayout(this);

	QGroupBox *group = new (ZEditor) QGroupBox("Properties");
	QFormLayout *form = new (ZEditor) QFormLayout(group);
		
	m_name = new (ZEditor) QLineEdit();
	form->addRow("Name:", m_name);
	
	m_action = new (ZEditor) QLineEdit();
	form->addRow("Action:", m_action);

	m_condition = new (ZEditor) QLineEdit();
	form->addRow("Condition:", m_condition);

	m_probabilityLow = new (ZEditor) QLineEdit();
	m_probabilityLow->setValidator(new (ZEditor) QDoubleValidator(this));
	form->addRow("Probability (0-1)", m_probabilityLow);

	outer->addWidget(group, 0, 0);

	QIcon addIcon = LoadIcon("Editor/add2_small.png");
	QIcon deleteIcon = LoadIcon("Editor/delete2_small.png");

	group = new QGroupBox("Prompts");
	
	QVBoxLayout *vbox = new (ZEditor) QVBoxLayout(group);
	m_prompts = new (ZEditor) QListWidget();
	m_prompts->setSelectionMode(QAbstractItemView::SingleSelection);
	RAD_VERIFY(connect(m_prompts, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnSelectedPromptChanged(QListWidgetItem*))));
	vbox->addWidget(m_prompts);

	QHBoxLayout *hbox = new (ZEditor) QHBoxLayout();
	hbox->addStretch(1);

	QPushButton *b = new (ZEditor) QPushButton(addIcon, "Add Prompt");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(OnAddPrompt())));
	hbox->addWidget(b);

	m_deletePrompt = new (ZEditor) QPushButton(deleteIcon, "Delete Prompt");
	m_deletePrompt->setEnabled(false);
	RAD_VERIFY(connect(m_deletePrompt, SIGNAL(clicked()), SLOT(OnDeletePrompt())));
	hbox->addWidget(m_deletePrompt);
	vbox->addLayout(hbox);

	m_promptEdit = new (ZEditor) ConversationTreeStringOptionEditWidget(m_langId, *m_parser->stringTable);
	vbox->addWidget(m_promptEdit);

	outer->addWidget(group, 0, 1);

	group = new QGroupBox("Replies");
	
	vbox = new (ZEditor) QVBoxLayout(group);
	m_replies = new (ZEditor) QListWidget();
	m_replies->setSelectionMode(QAbstractItemView::SingleSelection);
	RAD_VERIFY(connect(m_replies, SIGNAL(currentItemChanged(QListWidgetItem*, QListWidgetItem*)), SLOT(OnSelectedReplyChanged(QListWidgetItem*))));
	vbox->addWidget(m_replies);

	hbox = new (ZEditor) QHBoxLayout();
	hbox->addStretch(1);

	b = new (ZEditor) QPushButton(addIcon, "Add Reply");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(OnAddReply())));
	hbox->addWidget(b);

	m_deleteReply = new (ZEditor) QPushButton(deleteIcon, "Delete Reply");
	m_deleteReply->setEnabled(false);
	RAD_VERIFY(connect(m_deleteReply, SIGNAL(clicked()), SLOT(OnDeleteReply())));
	hbox->addWidget(m_deleteReply);
	vbox->addLayout(hbox);

	m_replyEdit = new (ZEditor) ConversationTreeStringOptionEditWidget(m_langId, *m_parser->stringTable);
	vbox->addWidget(m_replyEdit);

	outer->addWidget(group, 0, 2);

	hbox = new (ZEditor) QHBoxLayout();
	hbox->addStretch(1);

	m_ok = new (ZEditor) QPushButton("OK");
	RAD_VERIFY(connect(m_ok, SIGNAL(clicked()), SLOT(accept())));
	hbox->addWidget(m_ok);

	b = new (ZEditor) QPushButton("Cancel");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(reject())));
	hbox->addWidget(b);

	outer->addLayout(hbox, 1, 0, 1, 3);

	Load();
}

void ConversationTreeEditorItemEditDialog::OnAddPrompt() {
	if (m_root) {
		AddRootPrompt();
	} else {
		AddDialogPrompt();
	}
}

void ConversationTreeEditorItemEditDialog::AddRootPrompt() {

	int num = m_prompts->count();

	asset::ConversationTree::StringOption opt;
	m_root->prompts->push_back(opt);
	
	QListWidgetItem *item = new (ZEditor) QListWidgetItem(QString("Prompt %1").arg(num+1), 0);
	item->setData(Qt::UserRole, num);
	m_prompts->addItem(item);
	m_prompts->setCurrentItem(item);
}

void ConversationTreeEditorItemEditDialog::AddDialogPrompt() {

	int num = m_prompts->count();

	asset::ConversationTree::StringOption opt;
	m_dialog->prompts->push_back(opt);
	
	QListWidgetItem *item = new (ZEditor) QListWidgetItem(QString("Prompt %1").arg(num+1), 0);
	item->setData(Qt::UserRole, num);
	m_prompts->addItem(item);
	m_prompts->setCurrentItem(item);
}

void ConversationTreeEditorItemEditDialog::OnDeletePrompt() {
	if (m_root) {
		DeleteRootPrompt();
	} else {
		DeleteDialogPrompt();
	}
}

void ConversationTreeEditorItemEditDialog::DeleteRootPrompt() {
	QListWidgetItem *item = m_prompts->currentItem();
	m_root->prompts->erase(m_root->prompts->begin() + item->data(Qt::UserRole).toInt());
	LoadRootPrompts();
}

void ConversationTreeEditorItemEditDialog::DeleteDialogPrompt() {
	QListWidgetItem *item = m_prompts->currentItem();
	m_dialog->prompts->erase(m_dialog->prompts->begin() + item->data(Qt::UserRole).toInt());
	LoadDialogPrompts();
}

void ConversationTreeEditorItemEditDialog::OnAddReply() {
	int num = m_replies->count();

	asset::ConversationTree::StringOption opt;
	m_dialog->replies->push_back(opt);
	
	QListWidgetItem *item = new (ZEditor) QListWidgetItem(QString("Reply %1").arg(num+1), 0);
	item->setData(Qt::UserRole, num);
	m_replies->addItem(item);
	m_replies->setCurrentItem(item);
}

void ConversationTreeEditorItemEditDialog::OnDeleteReply() {
	QListWidgetItem *item = m_replies->currentItem();
	m_dialog->replies->erase(m_dialog->replies->begin() + item->data(Qt::UserRole).toInt());
	LoadReplies();
}

void ConversationTreeEditorItemEditDialog::SelectName() {
	m_name->selectAll();
	m_name->setFocus();
}

void ConversationTreeEditorItemEditDialog::OnSelectedPromptChanged(QListWidgetItem *current) {
	if (current) {
		m_deletePrompt->setEnabled(true);
		if (m_root) {
			m_promptEdit->Load(&m_root->prompts->at(current->data(Qt::UserRole).toInt()));
		} else {
			m_promptEdit->Load(&m_dialog->prompts->at(current->data(Qt::UserRole).toInt()));
		}
	} else {
		m_deletePrompt->setEnabled(false);
		m_promptEdit->Load(0);
	}
}

void ConversationTreeEditorItemEditDialog::OnSelectedReplyChanged(QListWidgetItem *current) {
	if (current) {
		m_deleteReply->setEnabled(true);
		m_replyEdit->Load(&m_dialog->replies->at(current->data(Qt::UserRole).toInt()));
	} else {
		m_deleteReply->setEnabled(false);
		m_replyEdit->Load(0);
	}
}

void ConversationTreeEditorItemEditDialog::OnNameChanged(const QString &text) {
	m_ok->setEnabled(!text.isEmpty());
}

void ConversationTreeEditorItemEditDialog::Load() {
	if (m_root) {
		LoadRoot();
	} else {
		LoadDialog();
	}
}

void ConversationTreeEditorItemEditDialog::Save() {
	if (m_root) {
		SaveRoot();
	} else {
		SaveDialog();
	}
}

void ConversationTreeEditorItemEditDialog::LoadPrompts() {
	if (m_root) {
		LoadRootPrompts();
	} else {
		LoadDialogPrompts();
	}
}

void ConversationTreeEditorItemEditDialog::LoadRoot() {
	m_name->setText(m_root->name.c_str.get());
	m_ok->setEnabled(!m_name->text().isEmpty());
	m_group->setText(m_root->group.c_str.get());
	m_probabilityLow->setText(QString("%1").arg(m_root->probability[0]));
	m_probabilityHigh->setText(QString("%1").arg(m_root->probability[0]));

	m_hidden->setChecked((m_root->flags & asset::ConversationTree::kRootFlag_Hidden) ? true : false);
	m_locked->setChecked((m_root->flags & asset::ConversationTree::kRootFlag_Locked) ? true : false);
	
	LoadRootPrompts();
}

void ConversationTreeEditorItemEditDialog::LoadDialog() {
	m_name->setText(m_dialog->name.c_str.get());
	m_ok->setEnabled(!m_name->text().isEmpty());
	m_action->setText(m_dialog->action.c_str.get());
	m_condition->setText(m_dialog->condition.c_str.get());
	m_probabilityLow->setText(QString("%1").arg(m_dialog->probability));
	LoadDialogPrompts();
	LoadReplies();
}

void ConversationTreeEditorItemEditDialog::LoadRootPrompts() {
	m_prompts->blockSignals(true);
	m_prompts->clear();

	int num = 0;
	for (asset::ConversationTree::StringOption::Vec::const_iterator it = m_root->prompts->begin(); it != m_root->prompts->end(); ++it) {
		QListWidgetItem *item = new (ZEditor) QListWidgetItem(QString("Prompt %1").arg(num+1));
		item->setData(Qt::UserRole, num++);
		m_prompts->addItem(item);
	}

	m_deletePrompt->setEnabled(false);
	m_promptEdit->Load(0);
	m_prompts->blockSignals(false);

	if (m_prompts->count() > 0) {
		m_prompts->setCurrentItem(m_prompts->item(0));
		OnSelectedPromptChanged(m_prompts->item(0));
	}
}

void ConversationTreeEditorItemEditDialog::LoadDialogPrompts() {
	m_prompts->blockSignals(true);
	m_prompts->clear();

	int num = 0;
	for (asset::ConversationTree::StringOption::Vec::const_iterator it = m_dialog->prompts->begin(); it != m_dialog->prompts->end(); ++it) {
		QListWidgetItem *item = new (ZEditor) QListWidgetItem(QString("Prompt %1").arg(num+1));
		item->setData(Qt::UserRole, num++);
		m_prompts->addItem(item);
	}

	m_deletePrompt->setEnabled(false);
	m_promptEdit->Load(0);
	m_prompts->blockSignals(false);

	if (m_prompts->count() > 0) {
		m_prompts->setCurrentItem(m_prompts->item(0));
		OnSelectedPromptChanged(m_prompts->item(0));
	}
}

void ConversationTreeEditorItemEditDialog::LoadReplies() {
	m_replies->blockSignals(true);
	m_replies->clear();

	int num = 0;
	for (asset::ConversationTree::StringOption::Vec::const_iterator it = m_dialog->replies->begin(); it != m_dialog->replies->end(); ++it) {
		QListWidgetItem *item = new (ZEditor) QListWidgetItem(QString("Reply %1").arg(num+1));
		item->setData(Qt::UserRole, num++);
		m_replies->addItem(item);
	}

	m_deleteReply->setEnabled(false);
	m_replyEdit->Load(0);
	m_replies->blockSignals(false);

	if (m_replies->count() > 0) {
		m_replies->setCurrentItem(m_replies->item(0));
		OnSelectedReplyChanged(m_replies->item(0));
	}
}

void ConversationTreeEditorItemEditDialog::SaveRoot() {
	m_root->name = m_name->text().toAscii().constData();
	m_root->group = m_group->text().toAscii().constData();
	m_root->probability[0] = m_probabilityLow->text().toFloat();
	m_root->probability[1] = m_probabilityHigh->text().toFloat();

	m_root->flags = 0;
	if (m_hidden->isChecked())
		m_root->flags |= asset::ConversationTree::kRootFlag_Hidden;
	if (m_locked->isChecked())
		m_root->flags |= asset::ConversationTree::kRootFlag_Locked;
}

void ConversationTreeEditorItemEditDialog::SaveDialog() {
	m_dialog->name = m_name->text().toAscii().constData();
	m_dialog->action = m_action->text().toAscii().constData();
	m_dialog->condition = m_condition->text().toAscii().constData();
	m_dialog->probability = m_probabilityLow->text().toFloat();
}

void ConversationTreeEditorItemEditDialog::done(int r) {
	if (r == QDialog::Accepted)
		Save();
	QDialog::done(r);
}

void ConversationTreeEditorItemEditDialog::OnStringTableDataChanged(const pkg::Package::Entry::AssetModifiedEventData &data) {
	m_promptEdit->ReloadStrings();
	if (m_replyEdit)
		m_replyEdit->ReloadStrings();
}

} // editor
} // tools

#include "moc_EditorConversationTreeEditorItemEditDialog.cc"
