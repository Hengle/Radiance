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
#include <Runtime/PushPack.h>

class QGraphicsScene;
class QGraphicsView;
class QListWidget;
class QListWidgetItem;
class QPushButton;
class QComboBox;

namespace tools {
namespace editor {

class RADENG_CLASS ConversationTreeEditorWidget : public QWidget {
	Q_OBJECT
	RAD_EVENT_CLASS(EventNoAccess);
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

private:

	void LoadRoots();
	void LoadDialog();
	void ReloadStrings();
	void OnStringTableDataChanged(const pkg::Package::Entry::AssetModifiedEventData &data);

	pkg::Asset::Ref m_conversationTree;
	asset::ConversationTreeParser *m_parser;
	QGraphicsScene *m_scene;
	QGraphicsView  *m_view;
	QListWidget *m_roots;
	QListWidget *m_dialog;
	QPushButton *m_deleteRoot;
	QPushButton *m_editRoot;
	QPushButton *m_deleteDialog;
	QPushButton *m_editDialog;
	QComboBox *m_languages;
	int m_langId;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
