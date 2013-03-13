/*! \file EditorConversationTreeEditorView.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup editor
*/

#include RADPCH
#include "EditorConversationTreeEditorView.h"
#include <QtGui/QGraphicsView>
#include <QtGui/QGraphicsScene>
#include <QtGui/QGraphicsRectItem>
#include <QtGui/QGraphicsSceneDragDropEvent>
#include <QtGui/QGraphicsSceneMouseEvent>
#include <QtGui/QBoxLayout>
#include <QtGui/QKeyEvent>
#include <QtGui/QMessageBox>
#include <QtCore/QMimeData.h>
#include <QtCore/QByteArray>
#include <QtCore/QBuffer>

namespace tools {
namespace editor {

ConversationTreeEditorView::ConversationTreeEditorView(
	int langId,
	asset::ConversationTreeParser *parser,
	QWidget *parent
) : QWidget(parent), m_langId(langId), m_parser(parser) {

	m_scene = new (ZEditor) ConversationTreeEditorViewGraphicsScene(this);
	
	RAD_VERIFY(connect(m_scene, SIGNAL(selectionChanged()), SLOT(SelectionChanged())));
	RAD_VERIFY(connect(m_scene, SIGNAL(OnDialogDropped(ConversationTreeEditorViewItem *, int)), SLOT(OnDialogDropped(ConversationTreeEditorViewItem *, int))));
		
	m_view = new (ZEditor) ConversationTreeEditorViewGraphicsView(m_scene);
	m_view->setAlignment(Qt::AlignLeft | Qt::AlignTop);
	m_view->setAcceptDrops(true);
	m_view->setViewportUpdateMode(QGraphicsView::FullViewportUpdate);

	RAD_VERIFY(connect(m_view, SIGNAL(OnDeleteKey()), SLOT(OnDeleteKey())));
			
	QHBoxLayout *layout = new (ZEditor) QHBoxLayout(this);
	layout->addWidget(m_view);

	Bind(
		m_parser->stringTableAsset->entry->OnAssetModified,
		&ConversationTreeEditorView::OnStringTableDataChanged
	);

	setEnabled(false);
}

void ConversationTreeEditorView::SetLangId(int langId) {
	m_langId = langId;
	ReloadItems();
}

void ConversationTreeEditorView::SelectRoot(asset::ConversationTree::Root *root) {
	m_scene->blockSignals(true);
	m_root.reset();
	m_scene->clear();

	if (root) {
		setEnabled(true);
		m_root = ItemForRoot(*root);
		SelectItem(m_root);
		m_root->viewItem->drawSelected = false;
		LayoutItems();
	} else {
		setEnabled(false);
	}

	m_scene->blockSignals(false);
}

void ConversationTreeEditorView::SelectionChanged() {
	TreeItem::Ref item = GetSelectedItemFromScene();
	if (item && !item->viewItem->drawSelected)
		SelectItem(item);
}

void ConversationTreeEditorView::OnDeleteKey() {
	TreeItem::Ref item = GetSelectedItemFromScene();
	if (item) {
		RemoveItem(item);
		LayoutItems();
	}
}

void ConversationTreeEditorView::OnDialogDropped(ConversationTreeEditorViewItem *target, int uid) {
	TreeItem::Ref targetItem;
	
	if (target) {
		targetItem = ItemForViewItem(*target);
		RAD_ASSERT(targetItem);
		if (targetItem->dropTarget == target) {
			target = 0; // append.
		}
	} else {
		targetItem = GetSelectedItemFromTree();
	}

	RAD_ASSERT(targetItem);

	asset::ConversationTree::Dialog *dialog = m_parser->conversationTree->DialogForUID(uid);
	if (dialog) {
		if (target) {
			ReplaceDialog(targetItem, *dialog);
		} else {
			AddDialogToItem(targetItem, *dialog);
		}
	}

	UpdateView();
}

void ConversationTreeEditorView::DialogDeleted(asset::ConversationTree::Dialog &dialog) {
	if (m_root) {
		DialogDeleted(m_root, dialog);
		LayoutItems();
	}
}

void ConversationTreeEditorView::AddDialogToItem(const TreeItem::Ref &item, asset::ConversationTree::Dialog &dialog) {

	if (IsDialogParent(*item, dialog)) {
		QMessageBox::critical(this, "Error", "This dialog item is already placed above, this would create a circular chat.");
		return;
	}

	if (item->root) {
		bool found = false;
		for (asset::ConversationTree::Dialog::PtrVec::const_iterator it = item->root->dialog->begin(); it != item->root->dialog->end(); ++it) {
			if (*it == &dialog) {
				found = true;
				break;
			}
		}

		if (found) {
			QMessageBox::critical(this, "Error", "This dialog item already exists here!");
			return;
		}

		item->root->dialog->push_back(&dialog);
	} else {
		RAD_ASSERT(item->dialog);

		bool found = false;
		for (asset::ConversationTree::Dialog::PtrVec::const_iterator it = item->dialog->choices->begin(); it != item->dialog->choices->end(); ++it) {
			if (*it == &dialog) {
				found = true;
				break;
			}
		}

		if (found) {
			QMessageBox::critical(this, "Error", "This dialog item already exists here!");
			return;
		}

		item->dialog->choices->push_back(&dialog);
	}

	m_parser->conversationTree->RefDialog(dialog);
	ItemForDialog(dialog, item.get());
	LayoutItems();
	emit OnDataChanged();
}

void ConversationTreeEditorView::ReplaceDialog(const TreeItem::Ref &item, asset::ConversationTree::Dialog &dialog) {
	RAD_ASSERT(item->parent);
	RAD_ASSERT(item->dialog);

	if (IsDialogParent(*item, dialog)) {
		QMessageBox::critical(this, "Error", "This dialog item is already placed above, this would create a circular chat.");
		return;
	}

	m_scene->blockSignals(true);

	TreeItem *parent = item->parent;
	if (parent->root) {
		bool found = false;
		for (asset::ConversationTree::Dialog::PtrVec::const_iterator it = parent->root->dialog->begin(); it != parent->root->dialog->end(); ++it) {
			if (*it == &dialog) {
				found = true;
				break;
			}
		}

		if (found) {
			QMessageBox::critical(this, "Error", "This dialog item already exists here!");
			return;
		}

		for (size_t i = 0; i < parent->children.size(); ++i) {
			const TreeItem::Ref &x = parent->children[i];
			if (x.get() == item.get()) {
				// replace-in-place
				m_parser->conversationTree->UnrefDialog(*item->dialog);
				item->dialog = &dialog;
				parent->root->dialog[i] = &dialog;
				m_parser->conversationTree->RefDialog(dialog);
				break;
			}
		}
	} else {
		bool found = false;
		for (asset::ConversationTree::Dialog::PtrVec::const_iterator it = parent->dialog->choices->begin(); it != parent->dialog->choices->end(); ++it) {
			if (*it == &dialog) {
				found = true;
				break;
			}
		}

		if (found) {
			QMessageBox::critical(this, "Error", "This dialog item already exists here!");
			return;
		}

		for (size_t i = 0; i < parent->children.size(); ++i) {
			const TreeItem::Ref &x = parent->children[i];
			if (x.get() == item.get()) {
				// replace-in-place
				m_parser->conversationTree->UnrefDialog(*item->dialog);
				item->dialog = &dialog;
				parent->dialog->choices[i] = &dialog;
				m_parser->conversationTree->RefDialog(dialog);
				break;
			}
		}
	}

	item->children.clear();
	ReloadStrings(m_root);
	LayoutItems();

	m_scene->blockSignals(false);
	emit OnDataChanged();
}

void ConversationTreeEditorView::DialogDeleted(const TreeItem::Ref &item, asset::ConversationTree::Dialog &dialog) {
	m_scene->blockSignals(true);
	for (TreeItem::Vec::iterator it = item->children.begin(); it != item->children.end();) {
		TreeItem::Vec::iterator next(it); ++it;
		const TreeItem::Ref &child = *it;
		if (child->dialog == &dialog) {
			item->children.erase(it);
		} else {
			DialogDeleted(child, dialog);
		}
		it = next;
	}
	m_scene->blockSignals(false);
}

void ConversationTreeEditorView::ReloadItems() {
	if (m_root) {
		ReloadItems(m_root);
		LayoutItems();
	}
}

void ConversationTreeEditorView::ReloadItems(const TreeItem::Ref &item) {
	m_scene->blockSignals(true);

	item->viewItem->setText(TextForItem(*item));

	if (item->dialog && item->promptItem) {
		item->promptItem->setText(TextForDialogPrompt(*item));
	}

	if (!item->children.empty()) {
		if (item->root) {
			if (item->root->dialog->size() < item->children.size()) {
				item->children.resize(item->root->dialog->size());
			}

			for (size_t i = 0; i < item->children.size(); ++i) {
				if (item->children[i]->dialog != item->root->dialog[i]) {
					item->children.clear();
					break;
				}
			}

			if (item->root->dialog->size() > item->children.size()) {
				for (asset::ConversationTree::Dialog::PtrVec::const_iterator it = item->root->dialog->begin(); it != item->root->dialog->end(); ++it) {
					ItemForDialog(**it, item.get());
				}
			}
		} else {
			RAD_ASSERT(item->dialog);

			if (item->dialog->choices->size() < item->children.size()) {
				item->children.resize(item->dialog->choices->size());
			}

			for (size_t i = 0; i < item->children.size(); ++i) {
				if (item->children[i]->dialog != item->dialog->choices[i]) {
					item->children.clear();
					break;
				}
			}

			if (item->dialog->choices->size() > item->children.size()) {
				for (asset::ConversationTree::Dialog::PtrVec::const_iterator it = item->dialog->choices->begin(); it != item->dialog->choices->end(); ++it) {
					ItemForDialog(**it, item.get());
				}
			}
		}

		for (TreeItem::Vec::const_iterator it = item->children.begin(); it != item->children.end(); ++it) {
			const TreeItem::Ref &child = *it;
			ReloadItems(child);
		}
	}

	m_scene->blockSignals(false);
}

void ConversationTreeEditorView::ReloadStrings(const TreeItem::Ref &item) {
	RAD_ASSERT(item->viewItem);
	item->viewItem->setText(TextForItem(*item));
	item->viewItem->update();

	if (item->dialog && item->promptItem) {
		item->promptItem->setText(TextForDialogPrompt(*item));
		item->promptItem->update();
	}

	for (TreeItem::Vec::const_iterator it = item->children.begin(); it != item->children.end(); ++it) {
		ReloadStrings(*it);
	}
}

void ConversationTreeEditorView::LayoutItems() {
	if (m_root)
		LayoutItems(m_root, 8.f, 8.f);
}

bool ConversationTreeEditorView::LayoutItems(const TreeItem::Ref &item, qreal x, qreal y) {
	item->viewItem->setPos(x, y);

	QRectF bounds = item->viewItem->sceneBoundingRect();
	y = bounds.bottom() + 12.f;
	
	if (item->promptItem) {
		item->promptItem->setPos(8.f, y);
		bounds = item->promptItem->sceneBoundingRect();
		y = bounds.bottom() + 12.f;
	}

	x = 20.f;
	
	for (TreeItem::Vec::const_iterator it = item->children.begin(); it != item->children.end(); ++it) {
		const TreeItem::Ref &child = *it;
		LayoutItems(child, x, y);
		x = child->viewItem->sceneBoundingRect().right() + 16.f;
	}

	if (item->dropTarget)
		item->dropTarget->setPos(x, y);

	return item->children.empty();
}

void ConversationTreeEditorView::OnStringTableDataChanged(const pkg::Package::Entry::AssetModifiedEventData &data) {
	if (m_root) {
		ReloadStrings(m_root);
		LayoutItems();
		UpdateView();
	}
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::ItemForRoot(asset::ConversationTree::Root &root) {

	QGraphicsRectItem *origin = new (ZEditor) QGraphicsRectItem(0.f, 0.f, 1.f, 1.f);
	origin->setBrush(Qt::NoBrush);
	origin->setPen(Qt::NoPen);
	m_scene->addItem(origin);

	TreeItem::Ref item(new (ZEditor) TreeItem());
	item->root = &root;
	item->viewItem = new (ZEditor) ConversationTreeEditorViewItem();
	item->viewItem->setText(TextForItem(*item));
	m_scene->addItem(item->viewItem);

	return item;
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::ItemForDialog(asset::ConversationTree::Dialog &dialog, TreeItem *parent) {
	TreeItem::Ref item(new (ZEditor) TreeItem());
	item->dialog = &dialog;
	item->parent = parent;
	item->viewItem = new (ZEditor) ConversationTreeEditorViewItem();
	item->viewItem->setText(TextForItem(*item));
	item->viewItem->setFlags(QGraphicsItem::ItemIsSelectable);
	item->viewItem->setAcceptDrops(true);
	m_scene->addItem(item->viewItem);

	if (parent)
		parent->children.push_back(item);

	return item;
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::ItemForViewItem(ConversationTreeEditorViewItem &item) {
	if (m_root) {
		if (m_root->dropTarget == &item)
			return m_root;
		return ItemForViewItem(m_root, item);
	}

	return TreeItem::Ref();
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::ItemForViewItem(const TreeItem::Ref &tree, ConversationTreeEditorViewItem &item) {
	for (TreeItem::Vec::const_iterator it = tree->children.begin(); it != tree->children.end(); ++it) {
		const TreeItem::Ref &child = *it;
		if (child->viewItem == &item)
			return child;
		if (child->dropTarget == &item)
			return child;
		TreeItem::Ref sub = ItemForViewItem(child, item);
		if (sub)
			return sub;
	}

	return TreeItem::Ref();
}

void ConversationTreeEditorView::RemoveItem(const TreeItem::Ref &item) {
	m_scene->blockSignals(true);

	// remove from parent?
	if (item->parent) {
		if (item->parent->root) {
			for (asset::ConversationTree::Dialog::PtrVec::iterator it = item->parent->root->dialog->begin(); it != item->parent->root->dialog->end(); ++it) {
				if (*it == item->dialog) {
					item->parent->root->dialog->erase(it);
					m_parser->conversationTree->UnrefDialog(*item->dialog);
					break;
				}
			}
		}

		if (item->parent->dialog) {
			for (asset::ConversationTree::Dialog::PtrVec::iterator it = item->parent->dialog->choices->begin(); it != item->parent->dialog->choices->end(); ++it) {
				if (*it == item->dialog) {
					item->parent->dialog->choices->erase(it);
					m_parser->conversationTree->UnrefDialog(*item->dialog);
					break;
				}
			}
		}

		for (TreeItem::Vec::iterator it = item->parent->children.begin(); it != item->parent->children.end(); ++it) {
			if ((*it).get() == item.get()) {
				it = item->parent->children.erase(it);
				break;
			}
		}
	}

	m_scene->blockSignals(false);
}

void ConversationTreeEditorView::SelectItem(const TreeItem::Ref &item) {
	if (item->parent) {
		PruneChildren(item->parent, item.get());
	}

	item->viewItem->drawSelected = true;

	if (item->root) {
		for (asset::ConversationTree::Dialog::PtrVec::iterator it = item->root->dialog->begin(); it != item->root->dialog->end(); ++it) {
			ItemForDialog(**it, item.get());
		}
	} else {
		RAD_ASSERT(item->dialog);
		for (asset::ConversationTree::Dialog::PtrVec::iterator it = item->dialog->choices->begin(); it != item->dialog->choices->end(); ++it) {
			ItemForDialog(**it, item.get());
		}

		item->promptItem = new ConversationTreeEditorViewItem();
		m_scene->addItem(item->promptItem);
		item->promptItem->setText(TextForDialogPrompt(*item));
	}

	item->dropTarget = new (ZEditor) ConversationTreeEditorDropTarget();
	item->dropTarget->setAcceptDrops(true);
	m_scene->addItem(item->dropTarget);

	LayoutItems();
	UpdateView();
}

void ConversationTreeEditorView::PruneChildren(TreeItem *parent, TreeItem *except) {
	for (TreeItem::Vec::iterator it = parent->children.begin(); it != parent->children.end(); ++it) {
		const TreeItem::Ref &child = *it;
		if (child.get() != except) {
			child->children.clear();
			child->viewItem->drawSelected = false;
			if (child->dropTarget) {
				m_scene->removeItem(child->dropTarget);
				delete child->dropTarget;
				child->dropTarget = 0;
			}
			if (child->promptItem) {
				m_scene->removeItem(child->promptItem);
				delete child->promptItem;
				child->promptItem = 0;
			}
		}
	}

	if (parent->parent)
		PruneChildren(parent->parent, parent);
}

bool ConversationTreeEditorView::IsDialogParent(const TreeItem &item, asset::ConversationTree::Dialog &dialog) {
	if (item.dialog == &dialog)
		return true;
	if (item.parent)
		return IsDialogParent(*item.parent, dialog);
	return false;
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::GetSelectedItemFromScene() {

	QList<QGraphicsItem *> list = m_scene->selectedItems();
	
	if (list.empty())
		return TreeItem::Ref();

	return ItemForViewItem(*static_cast<ConversationTreeEditorViewItem*>(list[0]));
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::GetSelectedItemFromTree() {
	if (m_root)
		return GetSelectedItemFromTree(m_root);
	return TreeItem::Ref();
}

ConversationTreeEditorView::TreeItem::Ref ConversationTreeEditorView::GetSelectedItemFromTree(const TreeItem::Ref &item) {
	for (TreeItem::Vec::iterator it = item->children.begin(); it != item->children.end(); ++it) {
		TreeItem::Ref sub = GetSelectedItemFromTree(*it);
		if (sub)
			return sub;
	}

	for (TreeItem::Vec::iterator it = item->children.begin(); it != item->children.end(); ++it) {
		TreeItem::Ref sub = GetSelectedItemFromTree(*it);
		if ((*it)->viewItem->drawSelected)
			return *it;
	}

	return TreeItem::Ref();
}

QString ConversationTreeEditorView::TextForItem(TreeItem &item) {
	const String *s = 0;
	if (item.root) {
		if (!item.root->prompts->empty())
			s = m_parser->stringTable->Find(item.root->prompts->at(0).text.c_str, (StringTable::LangId)m_langId);
	} else {
		RAD_ASSERT(item.dialog);
		if (!item.dialog->prompts->empty())
			s = m_parser->stringTable->Find(item.dialog->prompts->at(0).text.c_str, (StringTable::LangId)m_langId);
	}

	if (s)
		return QString(s->c_str.get());

	return QString("<NOT TRANSLATED>");
}

QString ConversationTreeEditorView::TextForDialogPrompt(TreeItem &item) {
	RAD_ASSERT(item.dialog);
	const String *s = 0;
	
	if (!item.dialog->replies->empty()) {
		s = m_parser->stringTable->Find(item.dialog->replies->at(0).text.c_str, (StringTable::LangId)m_langId);
	}

	if (s)
		return QString(s->c_str.get());

	return QString("<NOT TRANSLATED>");
}

void ConversationTreeEditorView::UpdateView() {
	m_scene->update();
}

ConversationTreeEditorView::TreeItem::TreeItem() : parent(0), root(0), dialog(0), viewItem(0), promptItem(0), dropTarget(0) {
}

ConversationTreeEditorView::TreeItem::~TreeItem() {
	QGraphicsScene *scene = viewItem ? viewItem->scene() : 0;
	if (scene) {
		scene->removeItem(viewItem);
		delete viewItem;

		if (promptItem) {
			scene->removeItem(promptItem);
			delete promptItem;
		}

		if (dropTarget) {
			scene->removeItem(dropTarget);
			delete dropTarget;
		}
	}
}

///////////////////////////////////////////////////////////////////////////////

ConversationTreeEditorViewGraphicsView::ConversationTreeEditorViewGraphicsView(QWidget *parent) 
	: QGraphicsView(parent) {
}

ConversationTreeEditorViewGraphicsView::ConversationTreeEditorViewGraphicsView(QGraphicsScene *scene, QWidget *parent)
	: QGraphicsView(scene, parent) {
}

void ConversationTreeEditorViewGraphicsView::keyPressEvent(QKeyEvent *event) {
	if (event->key() == Qt::Key_Delete) {
		emit OnDeleteKey();
	}
}

///////////////////////////////////////////////////////////////////////////////

ConversationTreeEditorViewGraphicsScene::ConversationTreeEditorViewGraphicsScene(QWidget *parent) 
	: QGraphicsScene(parent) {
}

void ConversationTreeEditorViewGraphicsScene::EmitDialogDropped(ConversationTreeEditorViewItem *item, int uid) {
	emit OnDialogDropped(item, uid);
}

void ConversationTreeEditorViewGraphicsScene::mousePressEvent(QGraphicsSceneMouseEvent *mouseEvent) {
	mouseEvent->setModifiers(Qt::NoModifier); // no ctrl + multi select
	QGraphicsScene::mousePressEvent(mouseEvent);
}

void ConversationTreeEditorViewGraphicsScene::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
	QGraphicsScene::dragEnterEvent(event);
//	event->setAccepted(true);
}

void ConversationTreeEditorViewGraphicsScene::dragMoveEvent(QGraphicsSceneDragDropEvent *event) {
	QGraphicsScene::dragMoveEvent(event);
//	event->setAccepted(true);
}

void ConversationTreeEditorViewGraphicsScene::dropEvent(QGraphicsSceneDragDropEvent *event) {
	QGraphicsScene::dropEvent(event);
	/*if (!event->isAccepted()) {
		event->setAccepted(true);
	}*/
}

int ConversationTreeEditorViewGraphicsScene::DialogIDFromDrop(QGraphicsSceneDragDropEvent *event) {
	const QMimeData *mime = event->mimeData();
	if (mime->hasFormat("application/conversationDialogId")) {
		QByteArray bytes = mime->data("application/conversationDialogId");
		QBuffer buffer(&bytes);
		buffer.open(QIODevice::ReadOnly);
		int uid;
		buffer.read((char*)&uid, sizeof(int));
		return uid;
	}

	return -1;
}

///////////////////////////////////////////////////////////////////////////////

ConversationTreeEditorViewItem::ConversationTreeEditorViewItem(QGraphicsItem *parent) 
	: QGraphicsSimpleTextItem(parent), m_dropFocus(false), drawSelected(false) {
}

ConversationTreeEditorViewItem::ConversationTreeEditorViewItem(const QString& text, QGraphicsItem* parent)
	: QGraphicsSimpleTextItem(text, parent), m_dropFocus(false), drawSelected(false) {
}

void ConversationTreeEditorViewItem::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QRectF rect = boundingRect();
	
	if (m_dropFocus) {
		painter->setBrush(QColor(qRgb(240, 240, 240)));
	} else {
		painter->setBrush(QColor(qRgb(215, 215, 215)));
	}
	
	if (drawSelected) {
		painter->setPen(QColor(qRgb(255, 0, 0)));
	} else {
		painter->setPen(Qt::NoPen);
	}

	painter->drawRect(rect);

	painter->setPen(QColor(qRgb(0,0,0)));
	painter->drawText(QGraphicsSimpleTextItem::boundingRect(), text());
}

void ConversationTreeEditorViewItem::dragEnterEvent(QGraphicsSceneDragDropEvent *event) {
	m_dropFocus = true;
	QRectF rect = boundingRect();
	rect.adjust(-4.f, -4.f, 4.f, 4.f);
	update(rect);
	event->setAccepted(true);
}

void ConversationTreeEditorViewItem::dragLeaveEvent(QGraphicsSceneDragDropEvent *event) {
	m_dropFocus = false;
	QRectF rect = boundingRect();
	rect.adjust(-4.f, -4.f, 4.f, 4.f);
	update(rect);
	event->setAccepted(true);
}

void ConversationTreeEditorViewItem::dragMoveEvent(QGraphicsSceneDragDropEvent *event) {
	event->setAccepted(true);
}

void ConversationTreeEditorViewItem::dropEvent(QGraphicsSceneDragDropEvent *event) {
	m_dropFocus = false;
	update();

	int uid = ConversationTreeEditorViewGraphicsScene::DialogIDFromDrop(event);
	if (uid != -1)
		static_cast<ConversationTreeEditorViewGraphicsScene*>(scene())->EmitDialogDropped(this, uid);
}

QRectF ConversationTreeEditorViewItem::boundingRect() {
	QRectF rect = QGraphicsSimpleTextItem::boundingRect();
	rect.adjust(-4.f, -4.f, 4.f, 4.f);
	return rect;
}

///////////////////////////////////////////////////////////////////////////////

ConversationTreeEditorDropTarget::ConversationTreeEditorDropTarget(QGraphicsItem *parent) 
	: ConversationTreeEditorViewItem("(Drop New Item Here)", parent) {
}

void ConversationTreeEditorDropTarget::paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) {
	QRectF rect = boundingRect();
	
	painter->setPen(QColor(qRgb(0, 0, 0)));
	if (m_dropFocus) {
		painter->setBrush(QColor(qRgb(240, 240, 240)));
	} else {
		painter->setBrush(QColor(qRgb(215, 215, 215)));
	}
	painter->drawRect(rect);
	QGraphicsSimpleTextItem::paint(painter, option, widget);
}

} // editor
} // tools

#include "moc_EditorConversationTreeEditorView.cc"
