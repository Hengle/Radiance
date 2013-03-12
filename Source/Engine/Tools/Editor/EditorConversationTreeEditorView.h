/*! \file EditorConversationTreeEditorView.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#pragma once

#include "EditorTypes.h"
#include "../../Assets/ConversationTreeParser.h"
#include <QtGui/QWidget>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsSimpleTextItem>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class ConversationTreeEditorViewItem;
class ConversationTreeEditorViewGraphicsScene;
class ConversationTreeEditorViewGraphicsView;
class ConversationTreeEditorDropTarget;

class RADENG_CLASS ConversationTreeEditorView : public QWidget {
	Q_OBJECT
	RAD_EVENT_CLASS(EventNoAccess)
public:
	ConversationTreeEditorView(
		int langId,
		asset::ConversationTreeParser *parser,
		QWidget *parent = 0
	);

	void SetLangId(int langId);
	void SelectRoot(asset::ConversationTree::Root *root);
	void DialogDeleted(asset::ConversationTree::Dialog &dialog);
	void ReloadItems();

private slots:

	void SelectionChanged();
	void OnDeleteKey();
	void OnDialogDropped(ConversationTreeEditorViewItem *target, int uid);

private:

	struct TreeItem {
		typedef boost::shared_ptr<TreeItem> Ref;
		typedef zone_vector<Ref, ZEditorT>::type Vec;

		TreeItem();
		~TreeItem();

		Vec children;
		TreeItem *parent;
		asset::ConversationTree::Root *root;
		asset::ConversationTree::Dialog *dialog;
		ConversationTreeEditorViewItem *viewItem;
		ConversationTreeEditorViewItem *promptItem;
	};

	void ReloadStrings(const TreeItem::Ref &item);
	void LayoutItems();
	bool LayoutItems(const TreeItem::Ref &item, qreal x, qreal y);
	void DialogDeleted(const TreeItem::Ref &item, asset::ConversationTree::Dialog &dialog);
	void AddDialogToItem(const TreeItem::Ref &item, asset::ConversationTree::Dialog &dialog);
	void ReplaceDialog(const TreeItem::Ref &item, asset::ConversationTree::Dialog &dialog);
	void ReloadItems(const TreeItem::Ref &item);

	void OnStringTableDataChanged(const pkg::Package::Entry::AssetModifiedEventData &data);
	TreeItem::Ref ItemForRoot(asset::ConversationTree::Root &root);
	TreeItem::Ref ItemForDialog(asset::ConversationTree::Dialog &dialog, TreeItem *parent);
	TreeItem::Ref ItemForViewItem(ConversationTreeEditorViewItem &item);
	TreeItem::Ref ItemForViewItem(const TreeItem::Ref &tree, ConversationTreeEditorViewItem &item);
	void RemoveItem(const TreeItem::Ref &item);
	void SelectItem(const TreeItem::Ref &item);
	void PruneChildren(TreeItem *parent, TreeItem *except);
	TreeItem::Ref GetSelectedItemFromScene();
	TreeItem::Ref GetSelectedItemFromTree();
	TreeItem::Ref GetSelectedItemFromTree(const TreeItem::Ref &item);

	QString TextForItem(TreeItem &item);
	QString TextForDialogPrompt(TreeItem &item);

	TreeItem::Ref m_root;
	ConversationTreeEditorDropTarget *m_dropTarget;
	ConversationTreeEditorViewGraphicsScene *m_scene;
	ConversationTreeEditorViewGraphicsView *m_view;
	asset::ConversationTreeParser *m_parser;
	int m_langId;
};

class RADENG_CLASS ConversationTreeEditorViewGraphicsView : public QGraphicsView {
	Q_OBJECT
public:
	ConversationTreeEditorViewGraphicsView(QWidget *parent = 0);
	ConversationTreeEditorViewGraphicsView(QGraphicsScene *scene, QWidget *parent = 0);

signals:

	void OnDeleteKey();

protected:

	virtual void keyPressEvent(QKeyEvent *event);
};

class RADENG_CLASS ConversationTreeEditorViewGraphicsScene : public QGraphicsScene {
	Q_OBJECT
public:
	ConversationTreeEditorViewGraphicsScene(QWidget *parent = 0);

	void EmitDialogDropped(ConversationTreeEditorViewItem *target, int uid);

signals:

	void OnDialogDropped(ConversationTreeEditorViewItem *target, int uid);

protected:

	virtual void mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent);
	virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
	virtual void dropEvent(QGraphicsSceneDragDropEvent *event);
};

class RADENG_CLASS ConversationTreeEditorViewItem : public QGraphicsSimpleTextItem {
public:

	ConversationTreeEditorViewItem(QGraphicsItem* parent = 0);
	ConversationTreeEditorViewItem(const QString& text, QGraphicsItem* parent = 0);

	bool drawSelected;

protected:

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
	virtual void dragEnterEvent(QGraphicsSceneDragDropEvent *event);
	virtual void dragLeaveEvent(QGraphicsSceneDragDropEvent *event);
	virtual void dragMoveEvent(QGraphicsSceneDragDropEvent *event);
	virtual void dropEvent(QGraphicsSceneDragDropEvent *event);

	bool m_dropFocus;
};

class RADENG_CLASS ConversationTreeEditorDropTarget : public ConversationTreeEditorViewItem {
public:

	ConversationTreeEditorDropTarget(QGraphicsItem* parent = 0);
	
protected:

	virtual void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget);
};

} // editor
} // tools

#include <Runtime/PopPack.h>
