// EditorSoundThumb.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorSoundThumb.h"
#include "../EditorMainWindow.h"
#include "../../../Packages/Packages.h"
#include <QtGui/QMessageBox>

namespace tools {
namespace editor {

SoundThumb::SoundThumb(ContentBrowserView &view) : ContentAssetThumb(view)
{
}

void SoundThumb::OpenEditor(const pkg::Package::Entry::Ref &entry, bool editable, bool modal)
{
	if (m_sound && m_sound->asset->id == entry->id)
	{ // restart sound.
		if (m_sound->playing)
		{
			m_sound->stop();
		}
		else
		{
			m_sound->play(SC_FX, 0);
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
			if (r == pkg::SR_InvalidFormat)
			{
				QMessageBox::critical(&View(), "Error", "Invalid wave file format!");
			}
			else
			{
				QMessageBox::critical(&View(), "Error", "Error reading file!");
			}

			return;
		}

		asset->Process(
			xtime::TimeSlice::Infinite,
			pkg::P_Trim
		);

		m_sound = MainWindow::Get()->soundContext->newSound(asset);
		if (m_sound)
			m_sound->play(SC_FX, 0);
	}
}

void SoundThumb::NotifyAddRemovePackages()
{
	m_sound.reset();
}

void SoundThumb::NotifyAddRemoveContent(const pkg::IdVec &added, const pkg::IdVec &removed)
{
	m_sound.reset();
}

void SoundThumb::NotifyContentChanged(const ContentChange::Vec &changed)
{
	m_sound.reset();
}

void SoundThumb::New(ContentBrowserView &view)
{
	SoundThumb *t = new (ZEditor) SoundThumb(view);
	ContentAssetThumb::Ref self(t);
	t->Register(self, asset::AT_Sound);
}

void CreateSoundThumb(ContentBrowserView &view)
{
	SoundThumb::New(view);
}

} // editor
} // tools

