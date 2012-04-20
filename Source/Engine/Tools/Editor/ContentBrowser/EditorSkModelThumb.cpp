// EditorSkModelThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorSkModelThumb.h"
#include "../EditorSkModelEditorWindow.h"
#include "../EditorUtils.h"

namespace tools {
namespace editor {

SkModelThumb::SkModelThumb(ContentBrowserView &view) : ContentAssetThumb(view)
{
}

void SkModelThumb::OpenEditor(const pkg::Package::Entry::Ref &entry)
{
	SkModelEditorWindow::LaunchEditor(entry->id);
}

void SkModelThumb::New(ContentBrowserView &view)
{
	SkModelThumb *t = new (ZEditor) SkModelThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_SkModel);
}

void CreateSkModelThumb(ContentBrowserView &view)
{
	SkModelThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorSkModelThumb.cc"
