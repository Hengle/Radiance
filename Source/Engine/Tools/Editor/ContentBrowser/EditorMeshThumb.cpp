// EditorMeshThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMeshThumb.h"
#include "../EditorModelEditorWindow.h"
#include "../EditorUtils.h"
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace editor {

MeshThumb::MeshThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
}

void MeshThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal) {
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

void MeshThumb::New(ContentBrowserView &view) {
	MeshThumb *t = new (ZEditor) MeshThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Mesh);
}

void CreateMeshThumb(ContentBrowserView &view) {
	MeshThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorMeshThumb.cc"
