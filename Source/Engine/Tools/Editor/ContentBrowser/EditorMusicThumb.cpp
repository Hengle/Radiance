// EditorMusicThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorMusicThumb.h"
#include "../EditorMainWindow.h"
#include "../../../Packages/Packages.h"
#include <QtGui/QMessageBox>

namespace tools {
namespace editor {

MusicThumb::MusicThumb(ContentBrowserView &view) : ContentAssetThumb(view)
{
}

void MusicThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal)
{
	if (m_sound && m_sound->asset->id == entry->id)
	{
		if (m_sound->playing)
		{
			m_sound->Pause(!m_sound->paused);
		}
		else
		{
			m_sound->Play(SC_Music, 0);
		}
	}
	else
	{
		pkg::Asset::Ref asset = entry->Asset(pkg::Z_ContentBrowser);

		int r = asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Load
		);

		if (r != pkg::SR_Success)
		{
			QMessageBox::critical(&View(), "Error", "File not found!");
			return;
		}

		asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Trim
		);

		m_sound = MainWindow::Get()->soundContext->NewSound(asset);
		if (m_sound)
		{
			m_sound->loop = true;
			m_sound->Play(SC_Music, 0);
		}
	}
}

void MusicThumb::NotifyAddRemovePackages()
{
	m_sound.reset();
}

void MusicThumb::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
	m_sound.reset();
}

void MusicThumb::NotifyContentChanged(const ContentChange::Vec &changed)
{
	m_sound.reset();
}

void MusicThumb::New(ContentBrowserView &view)
{
	MusicThumb *t = new (ZEditor) MusicThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Music);
}

void CreateMusicThumb(ContentBrowserView &view)
{
	MusicThumb::New(view);
}

} // editor
} // tools

#include "moc_EditorMusicThumb.cc"
