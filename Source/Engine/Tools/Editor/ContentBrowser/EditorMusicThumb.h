// EditorMusicThumb.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../../Sound.h"
#include "EditorContentBrowserView.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class MusicThumb : public ContentAssetThumb
{
public:
	MusicThumb(ContentBrowserView &view);
	virtual void OpenEditor(const pkg::Package::Entry::Ref &entry);
	virtual void NotifyAddRemovePackages();
	virtual void NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed);
	virtual void NotifyContentChanged(const ContentChange::Vec &changed);
	static void New(ContentBrowserView &view);

private:

	Sound::Ref m_sound;
};

} // editor
} // tools

#include <Runtime/PopPack.h>

