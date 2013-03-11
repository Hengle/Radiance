/*! \file EditorConversationTreeEditorItemEditDialog.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#pragma once

#include "EditorTypes.h"
#include <QtGui/QDialog>
#include "../../Assets/ConversationTreeParser.h"
#include "../../StringTable.h"
#include <Runtime/PushPack.h>

class QPushButton;
class QLineEdit;
class QListWidget;
class QListWidgetItem;
class QCheckBox;
class QComboBox;

namespace tools {
namespace editor {

///////////////////////////////////////////////////////////////////////////////

class ConversationTreeStringOptionEditWidget : public QDialog {
	Q_OBJECT
public:
	ConversationTreeStringOptionEditWidget(const StringTable &stringTable, QWidget *parent = 0);

	void Load(asset::ConversationTree::StringOption *opt);
	void ReloadStrings();

signals:

	void OnDataChanged();

private slots:

	void DoDataChanged();

private:

	QComboBox *m_text;
	QLineEdit *m_probabilityLow;
	QLineEdit *m_probabilityHigh;
	asset::ConversationTree::StringOption *m_opt;
	const StringTable *m_stringTable;
};

///////////////////////////////////////////////////////////////////////////////

class ConversationTreeEditorItemEditDialog : public QDialog {
	Q_OBJECT
	RAD_EVENT_CLASS(EventNoAccess)
public:
	ConversationTreeEditorItemEditDialog(
		asset::ConversationTree::Root &root,
		asset::ConversationTreeParser &parser,
		QWidget *parent = 0
	);

	ConversationTreeEditorItemEditDialog(
		asset::ConversationTree::Dialog &dialog,
		asset::ConversationTreeParser &parser,
		QWidget *parent = 0
	);

	RAD_DECLARE_READONLY_PROPERTY(
		ConversationTreeEditorItemEditDialog,
		root,
		asset::ConversationTree::Root*
	);

	RAD_DECLARE_READONLY_PROPERTY(
		ConversationTreeEditorItemEditDialog,
		dialog,
		asset::ConversationTree::Dialog*
	);

public slots:

	virtual void done(int r);
	void SelectName();

private slots:

	void OnAddPrompt();
	void OnDeletePrompt();
	void OnAddReply();
	void OnDeleteReply();
	void OnNameChanged(const QString &text);
	void OnSelectedPromptChanged(QListWidgetItem *current);

private:

	void InitRoot();
	void InitDialog();
	void Load();
	void Save();

	void LoadRoot();
	void SaveRoot();

	void LoadPrompts();
	void LoadRootPrompts();

	void AddRootPrompt();
	void DeleteRootPrompt();

	void OnStringTableDataChanged(const pkg::Package::Entry::AssetModifiedEventData &data);

	RAD_DECLARE_GET(root, asset::ConversationTree::Root*) {
		return m_root;
	}

	RAD_DECLARE_GET(dialog, asset::ConversationTree::Dialog*) {
		return m_dialog;
	}

	asset::ConversationTree::Dialog m_dialogCopy;
	asset::ConversationTree::Root m_rootCopy;

	asset::ConversationTree::Dialog *m_dialog;
	asset::ConversationTree::Root *m_root;

	QLineEdit *m_name;
	QLineEdit *m_group;
	QLineEdit *m_action;
	QLineEdit *m_condition;
	QLineEdit *m_probabilityLow;
	QLineEdit *m_probabilityHigh;
	QListWidget *m_prompts;
	QListWidget *m_replies;
	QCheckBox *m_hidden;
	QCheckBox *m_locked;
	QPushButton *m_deletePrompt;
	QPushButton *m_deleteReply;
	QPushButton *m_ok;
	ConversationTreeStringOptionEditWidget *m_promptEdit;
	ConversationTreeStringOptionEditWidget *m_replyEdit;
	asset::ConversationTreeParser *m_parser;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
