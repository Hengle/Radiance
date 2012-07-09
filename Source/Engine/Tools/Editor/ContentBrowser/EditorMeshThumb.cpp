// EditorMeshThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMeshThumb.h"
#include "../EditorMeshEditorWindow.h"
#include "../EditorUtils.h"

namespace tools {
namespace editor {

MeshThumb::MeshThumb(ContentBrowserView &view) : ContentAssetThumb(view)
{
}

void MeshThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal)
{
	// TODO: Change legacy viewers over to EditorWindow subclasses
	// and use EditorWindow::CreateDialog for modal cases

	if (!modal)
		MeshEditorWindow::LaunchEditor(entry->id);
}

void MeshThumb::New(ContentBrowserView &view)
{
	MeshThumb *t = new (ZEditor) MeshThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Mesh);
}

void CreateMeshThumb(ContentBrowserView &view)
{
	MeshThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorMeshThumb.cc"
