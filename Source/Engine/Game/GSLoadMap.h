// GSLoadMap.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Game.h"
#include "GSPriority.h"
#include "../Assets/MapAsset.h"
#include <Runtime/PushPack.h>

#if defined(RAD_OPT_PC_TOOLS)
class QWidget;
namespace tools {
namespace editor {
class ProgressDialog;
} // editor
} // tools
#endif

class RADENG_CLASS GSLoadMap : public Game::Tickable {
public:
	static Game::Tickable::Ref New(int mapId, int slot, bool play, bool loadScreen);
	enum { Priority = GSP_Default };
	GSLoadMap(int mapId, int slot, bool play, bool loadScreen);

	virtual int Tick(Game &game, float dt, const xtime::TimeSlice &time, int flags);

#if defined(RAD_OPT_PC_TOOLS)
	void EnableProgressIndicator(QWidget *parent);
#endif

protected:

	RAD_DECLARE_READONLY_PROPERTY(GSLoadMap, loadScreen, bool);
	RAD_DECLARE_READONLY_PROPERTY(GSLoadMap, mapAsset, asset::MapAsset*);

	virtual void Draw(Game &game, float dt) {}

private:

	RAD_DECLARE_GET(loadScreen, bool) { 
		return m_loadScreen; 
	}

	RAD_DECLARE_GET(mapAsset, asset::MapAsset*) { 
		return m_mapAsset; 
	}

	int m_mapId;
	int m_slot;
	bool m_play;
	bool m_loadScreen;
	pkg::Asset::Ref m_map;
	asset::MapAsset *m_mapAsset;

#if defined(RAD_OPT_PC_TOOLS)
	bool m_nested;
	QWidget *m_progressIndicatorParent;
	tools::editor::ProgressDialog *m_progress;
#endif
};

#include <Runtime/PopPack.h>
