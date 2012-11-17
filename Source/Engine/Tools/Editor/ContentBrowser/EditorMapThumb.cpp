// EditorMapThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMapThumb.h"
#include "../EditorMainWindow.h"
#include "../EditorPopupMenu.h"
#include "../MapEditor/EditorMapEditorWindow.h"
#include "../EditorPIEWidget.h"
#include "../EditorBSPDebugWidget.h"
#include "../EditorPathfindingDebugWidget.h"
#include "../EditorUtils.h"

namespace tools {
namespace editor {

MapThumb::MapThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
	PopupMenu *m = CreateMenu(false);
	QAction *play = m->AddAction("Play...", this, SLOT(Play()));
	QAction *debugBSP = m->AddAction("Debug BSP...", this, SLOT(DebugBSP()));
	QAction *debugPathfinding = m->AddAction("Debug Pathfinding...", this, SLOT(DebugPathfinding()));

	m->qmenu->setDefaultAction(play);
}

void MapThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal) {
	if (modal)
		return;
	Play(entry);
}

void MapThumb::New(ContentBrowserView &view) {
	MapThumb *t = new (ZEditor) MapThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Map);
}

void MapThumb::Play() {
	Play(clickedItem);
}

void MapThumb::DebugBSP() {
	DebugBSP(clickedItem);
}

void MapThumb::DebugPathfinding() {
	DebugPathfinding(clickedItem);
}

void MapThumb::Play(const pkg::Package::Entry::Ref &entry) {
	RAD_ASSERT(entry);

	std::pair<int, int> res = View().selectedResolution();

	PIEWidget *w = new (ZEditor) PIEWidget(0, 
		Qt::Window|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint|
		Qt::WindowSystemMenuHint|
		Qt::WindowCloseButtonHint|
		Qt::WindowMinimizeButtonHint|
		Qt::MSWindowsFixedSizeDialogHint
	);
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->setWindowTitle(entry->name.get());
	w->resize(res.first, res.second);
	CenterWidget(*w, *MainWindow::Get());
	w->show();
	w->RunMap(entry->id);
}

void MapThumb::DebugBSP(const pkg::Package::Entry::Ref &entry) {
	RAD_ASSERT(entry);

	std::pair<int, int> res = View().selectedResolution();

	BSPDebugWidget *w = new (ZEditor) BSPDebugWidget(0, 
		Qt::Window|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint|
		Qt::WindowSystemMenuHint|
		Qt::WindowCloseButtonHint|
		Qt::WindowMinimizeButtonHint|
		Qt::MSWindowsFixedSizeDialogHint
	);
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->setWindowTitle(entry->name.get());
	w->resize(res.first, res.second);
	CenterWidget(*w, *MainWindow::Get());
	w->show();
	w->DebugMap(entry->id);
}


void MapThumb::DebugPathfinding(const pkg::Package::Entry::Ref &entry) {
	RAD_ASSERT(entry);

	std::pair<int, int> res = View().selectedResolution();

	PathfindingDebugWidget *w = new (ZEditor) PathfindingDebugWidget(0, 
		Qt::Window|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint|
		Qt::WindowSystemMenuHint|
		Qt::WindowCloseButtonHint|
		Qt::WindowMinimizeButtonHint|
		Qt::MSWindowsFixedSizeDialogHint
	);
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->setWindowTitle(entry->name.get());
	w->resize(res.first, res.second);
	CenterWidget(*w, *MainWindow::Get());
	w->show();
	w->DebugMap(entry->id);
}

void CreateMapThumb(ContentBrowserView &view) {
	MapThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorMapThumb.cc"
