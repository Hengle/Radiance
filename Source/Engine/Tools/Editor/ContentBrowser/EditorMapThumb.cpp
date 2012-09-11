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
#include "../EditorUtils.h"

namespace tools {
namespace editor {

MapThumb::MapThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
	PopupMenu *m = CreateMenu(false);
	QAction *play = m->AddAction("Play...", this, SLOT(Play()));
	m->AddAction("Edit...", this, SLOT(Edit()));
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

void MapThumb::Edit() {
	Edit(clickedItem);
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

void MapThumb::Edit(const pkg::Package::Entry::Ref &entry) {
	pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);
	if (asset)
		EditorWindow::Open<map_editor::MapEditorWindow>(asset, View().parentWidget());
}

void CreateMapThumb(ContentBrowserView &view) {
	MapThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorMapThumb.cc"
