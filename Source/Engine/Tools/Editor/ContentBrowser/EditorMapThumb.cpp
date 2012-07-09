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

MapThumb::MapThumb(ContentBrowserView &view) : ContentAssetThumb(view)
{
	PopupMenu *m = CreateMenu(false);
	m->AddItem("Play", this, SLOT(Play()));
}

void MapThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal)
{
	if (editable && !modal)
		Play(entry);
}

void MapThumb::New(ContentBrowserView &view)
{
	MapThumb *t = new (ZEditor) MapThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Map);
}

void MapThumb::Play()
{
	Play(clickedItem);
}

void MapThumb::Play(const pkg::Package::Entry::Ref &entry)
{
	RAD_ASSERT(entry);

	std::pair<int, int> res = View().selectedResolution();

	PIEWidget *w = new (ZEditor) PIEWidget(0, 
		Qt::Window|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint|
		Qt::WindowSystemMenuHint|
		Qt::WindowCloseButtonHint|
		Qt::WindowMinMaxButtonsHint
	);
	w->setAttribute(Qt::WA_DeleteOnClose);
	w->setWindowTitle(entry->name.get());
	w->resize(res.first, res.second);
	//PercentSize(*w, *MainWindow::Get(), 0.85f, 0.85f);
	CenterWidget(*w, *MainWindow::Get());
	w->show();
	w->RunMap(entry->id);
}

void CreateMapThumb(ContentBrowserView &view)
{
	MapThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorMapThumb.cc"
