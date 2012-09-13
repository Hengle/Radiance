// EditorMapThumb.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorContentBrowserView.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class MapThumb : public ContentAssetThumb {
	Q_OBJECT
public:
	MapThumb(ContentBrowserView &view);
	virtual void OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal);
	static void New(ContentBrowserView &view);

private slots:

	void Play();
	void Debug();

private:

	void Play(const pkg::Package::Entry::Ref &entry);
	void Debug(const pkg::Package::Entry::Ref &entry);

};

} // editor
} // tools

#include <Runtime/PopPack.h>

