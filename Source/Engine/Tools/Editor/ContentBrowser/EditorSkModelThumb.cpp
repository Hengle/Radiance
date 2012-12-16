// EditorSkModelThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorSkModelThumb.h"
#include "../EditorModelEditorWindow.h"
#include "../EditorUtils.h"
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace editor {

SkModelThumb::SkModelThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
}

void SkModelThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal) {
	
	pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);
	ModelEditorWindow *w;

	if (modal) {
		w = EditorWindow::CreateDialog<ModelEditorWindow>(asset, editable, View().parentWidget());
	} else {
		w = EditorWindow::Open<ModelEditorWindow>(asset, View().parentWidget());
	}

	if (w) {
		if (!w->Load())
			w->close();
	}
}

void SkModelThumb::New(ContentBrowserView &view) {
	SkModelThumb *t = new (ZEditor) SkModelThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_SkModel);
}

void CreateSkModelThumb(ContentBrowserView &view) {
	SkModelThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorSkModelThumb.cc"
