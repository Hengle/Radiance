/*! \file EditorConversationTreeEditorWidget.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#pragma once

#include "EditorTypes.h"
#include "../../Assets/ConversationTreeParser.h"
#include <QtGui/QWidget>
#include <QtGui/QListWidget>
#include <Runtime/PushPack.h>

class QGraphicsScene;
class QGraphicsView;
class QPushButton;
class QComboBox;

namespace tools {
namespace editor {

class ConversationTreeEditorView;
class ConversationTreeEditorWidgetDialogDragList;

class RADENG_CLASS ConversationTreeEditorWidget : public QWidget {
	Q_OBJECT
public:

	ConversationTreeEditorWidget(
		const pkg::Asset::Ref &conversationTree,
		bool editable,
		QWidget *parent = 0
	);

private slots:

	void EditRootPressed();
	void AddRootPressed();
	void DeleteRootPressed();
	void EditDialogPressed();
	void AddDialogPressed();
	void DeleteDialogPressed();
	void OnLanguageChanged(int index);
	void OnRootListSelected(QListWidgetItem *item);
	void OnDialogListSelected(QListWidgetItem *item);
	void SaveChanges();
	void OnEditDialog(asset::ConversationTree::Dialog &dialog);
	void OnEditRoot(asset::ConversationTree::Root &root);

private:

	void LoadRoots();
	void LoadDialog();

	void EditRoot(QListWidgetItem &item);
	void EditDialog(QListWidgetItem &item);
	
	void RemoveDialogFromTree(asset::ConversationTree::Dialog &dialog);

	void RemoveDialogFromTree(
		asset::ConversationTree::Root &root,
		asset::ConversationTree::Dialog &dialog
	);

	void RemoveDialogFromTree(
		asset::ConversationTree::Dialog &parent,
		asset::ConversationTree::Dialog &dialog
	);

	asset::ConversationTree::Root *FindRoot(const char *name);
	asset::ConversationTree::Dialog *FindDialog(const char *name);

	pkg::Asset::Ref m_conversationTree;
	asset::ConversationTreeParser *m_parser;
	ConversationTreeEditorView *m_view;
	QListWidget *m_roots;
	ConversationTreeEditorWidgetDialogDragList *m_dialog;
	QPushButton *m_deleteRoot;
	QPushButton *m_editRoot;
	QPushButton *m_deleteDialog;
	QPushButton *m_editDialog;
	QComboBox *m_languages;
	int m_langId;
};

class ConversationTreeEditorWidgetDialogDragList : public QListWidget {
	Q_OBJECT
public:
	ConversationTreeEditorWidgetDialogDragList(QWidget *parent = 0);

protected:

	virtual QMimeData *mimeData(const QList<QListWidgetItem*> items) const;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
