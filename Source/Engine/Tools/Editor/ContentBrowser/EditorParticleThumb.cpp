/*! \file EditorParticleThumb.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorParticleThumb.h"
#include "../EditorParticleEditorWindow.h"
#include "../EditorUtils.h"
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace editor {

ParticleModelThumb::ParticleModelThumb(ContentBrowserView &view) : ContentAssetThumb(view) {
}

void ParticleModelThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal) {
	
	pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);
	ParticleEditorWindow *w;

	if (modal) {
		w = EditorWindow::CreateDialog<ParticleEditorWindow>(asset, editable);
	} else {
		w = EditorWindow::Open<ParticleEditorWindow>(asset);
	}

	if (w) {
		if (!w->Load())
			w->close();
	}
}

void ParticleModelThumb::New(ContentBrowserView &view) {
	ParticleModelThumb *t = new (ZEditor) ParticleModelThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Particle);
}

void CreateParticleThumb(ContentBrowserView &view) {
	ParticleModelThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorParticleThumb.cc"
