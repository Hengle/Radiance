/*! \file EditorVtModelThumb.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "EditorVtModelThumb.h"
#include "../EditorModelEditorWindow.h"
#include "../EditorUtils.h"
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace editor {

VtModelThumb::VtModelThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
}

void VtModelThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal) {
	
	pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);
	ModelEditorWindow *w;

	if (modal) {
		w = EditorWindow::CreateDialog<ModelEditorWindow>(asset, editable);
	} else {
		w = EditorWindow::Open<ModelEditorWindow>(asset);
	}

	if (w) {
		if (!w->Load())
			w->close();
	}
}

void VtModelThumb::New(ContentBrowserView &view) {
	VtModelThumb *t = new (ZEditor) VtModelThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_VtModel);
}

void CreateVtModelThumb(ContentBrowserView &view) {
	VtModelThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorVtModelThumb.cc"
