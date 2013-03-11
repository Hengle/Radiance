// EditorStringTableThumb.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorStringTableThumb.h"
#include "../EditorStringTableEditorWindow.h"
#include <QtGui/QMessageBox>
#include <Runtime/PushSystemMacros.h>

using namespace pkg;

namespace tools {
namespace editor {

StringTableThumb::StringTableThumb(ContentBrowserView &view) : ContentAssetThumb(view)
{
}

void StringTableThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal)
{
	pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		P_Load|P_Create
	);

	if (r != pkg::SR_Success) {
		QMessageBox::critical(0, "Error", "Failed to load string table!");
		return;
	}

	if (modal) {
		EditorWindow::CreateDialog<StringTableEditorWindow>(asset, editable);
	} else {
		EditorWindow::Open<StringTableEditorWindow>(asset);
	}
}

void StringTableThumb::New(ContentBrowserView &view)
{
	StringTableThumb *t = new (ZEditor) StringTableThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_StringTable);
}

void CreateStringTableThumb(ContentBrowserView &view)
{
	StringTableThumb::New(view);
}

} // editor
} // tools

#include <Runtime/PopSystemMacros.h>
#include "moc_EditorStringTableThumb.cc"
