/*! \file MapBuilder.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#pragma once

#include "../../Types.h"
#include "../Entity.h"
#include "../WorldDef.h"
#include "SolidBSP/SolidBSP.h"
#include "../../Tools/SceneFile.h"
#include "../../Tools/Progress.h"
#include "../../SkAnim/SkAnimDef.h"
#include "MapBuilderDebugUI.h"
#include <QtCore/QVariant>
#include <QtCore/QRect>
#include <Runtime/Thread.h>
#include <Runtime/PushPack.h>

class Engine;

namespace tools {
namespace map_builder {

struct EntSpawn {
	world::Keys keys;
	SceneFile::Brush::Vec brushes;
};

class RADENG_CLASS MapBuilder : private thread::Thread {
public:
	typedef boost::shared_ptr<MapBuilder> Ref;

	MapBuilder(Engine &engine);
	~MapBuilder();

	void SetProgressIndicator(UIProgress *ui);
	void SetDebugUI(DebugUI *ui);
	void SetCinematicActorCompression(const CinematicActorCompressionMap &map);
	bool LoadEntSpawn(const EntSpawn &spawn);
	bool SpawnCompile();
	void WaitForCompletion();
	void DebugDraw(float time, float dt, const QRect &viewport);
	void OnDebugMenu(const QVariant &data);
	
	RAD_DECLARE_READONLY_PROPERTY(MapBuilder, bspFile, world::bsp_file::BSPFile::Ref);
	RAD_DECLARE_READONLY_PROPERTY(MapBuilder, bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref);
	RAD_DECLARE_READONLY_PROPERTY(MapBuilder, result, int);

protected:

	virtual int ThreadProc();

private:

	RAD_DECLARE_GET(bspFile, world::bsp_file::BSPFile::Ref) { 
		return m_bspBuilder.bspFile; 
	}
	
	RAD_DECLARE_GET(bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref) { 
		return m_bspBuilder.bspFileBuilder; 
	}

	RAD_DECLARE_GET(result, int);

	bool ParseWorldSpawn(const EntSpawn &spawn);
	bool ParseEntity(const EntSpawn &spawn);
	bool ParseWaypoint(const EntSpawn &spawn);
	bool LoadScene(const EntSpawn &spawn);
	bool MergeCommonScene();
	void ConnectWaypoints();

	SceneFile m_map;
	Engine &m_e;
	UIProgress *m_ui;
	DebugUI *m_debugUI;
	int m_result;
	bool m_compiling;
	EntSpawn m_spawn;
	CinematicActorCompressionMap m_caMap;
	solid_bsp::BSPBuilder m_bspBuilder;
};

} // map_builder
} // tools

#include <Runtime/PopPack.h>
