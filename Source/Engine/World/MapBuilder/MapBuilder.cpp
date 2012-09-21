/*! \file MapBuilder.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "MapBuilder.h"
#include "../World.h"
#include "../../Tools/SceneFile.h"
#include "../../COut.h"
#include "../../Engine.h"
#include "../../Packages/PackagesDef.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace tools {

MapBuilder::MapBuilder(Engine &engine)
: m_e(engine), m_ui(0), m_debugUI(0), m_result(SR_Success), m_compiling(false) {
}

MapBuilder::~MapBuilder() {
}

void MapBuilder::SetProgressIndicator(UIProgress &ui) {
	m_ui = &ui;
}

void MapBuilder::SetDebugUI(MapBuilderDebugUI &ui) {
	m_debugUI = &ui;
}

void MapBuilder::DebugDraw(float time, float dt, const QRect &viewport) {
	m_bspBuilder.DebugDraw(time, dt, viewport);
}

void MapBuilder::OnDebugMenu(const QVariant &data) {
	m_bspBuilder.OnDebugMenu(data);
}

bool MapBuilder::LoadEntSpawn(const world::EntSpawn &spawn) {
	const char *sz = spawn.keys.StringForKey("classname");
	if (!sz) {
		m_result = SR_ParseError;
		COut(C_Error) << "ERROR: Entity missing classname." << std::endl;
		return false;
	}

	String classname(sz);

	if (classname == "worldspawn") {
		m_result = SR_Success;
		return ParseWorldSpawn(spawn);
	} else if (classname == "static_mesh_scene") {
		m_spawn = spawn;
		m_result = SR_Pending;
		Run();
		return true;
	}

	m_result = SR_Success;
	return ParseEntity(spawn);
}

bool MapBuilder::SpawnCompile() {
	RAD_ASSERT(Thread::exited);
	m_compiling = true;
	return m_bspBuilder.SpawnCompile(m_map, m_ui, m_debugUI, &COut(C_Debug));
}

void MapBuilder::WaitForCompletion() {
	if (m_compiling) {
		m_bspBuilder.WaitForCompletion();
	} else {
		Join();
	}
}

bool MapBuilder::ParseWorldSpawn(const world::EntSpawn &spawn) {
	if (!m_map.worldspawn) // may be spawned by static_mesh_scene
		m_map.worldspawn.reset(new (ZTools) SceneFile::Entity());
	m_map.worldspawn->keys = spawn.keys;
	return true;
}

bool MapBuilder::ParseEntity(const world::EntSpawn &spawn) {
	SceneFile::Entity::Ref entity(new (ZTools) SceneFile::Entity());
	entity->keys = spawn.keys;
	Vec3 org(spawn.keys.Vec3ForKey("origin"));
	entity->origin = SceneFile::Vec3(org.X(), org.Y(), org.Z());
	m_map.ents.push_back(entity);
	return true;
}

bool MapBuilder::LoadScene(const world::EntSpawn &spawn) {
	const char *sz = spawn.keys.StringForKey("file");
	if (!sz || !sz[0]) {
		COut(C_Error) << "ERROR: static_mesh_scene missing file key!" << std::endl;
		return false;
	}

	String path(CStr(sz));
	path += ".3dx";

	file::MMFileInputBuffer::Ref ib = m_e.sys->files->OpenInputBuffer(path.c_str, ZTools);
	if (!ib) {
		COut(C_Error) << "ERROR: unable to open '" << path << "'" << std::endl;
		return false;
	}

	if (m_ui) {
		String title(CStr("Loading Scene '") + path + CStr("'"));
		m_ui->title = title.c_str;
	}

	stream::InputStream is(*ib);
	return LoadSceneFile(is, m_map, true, m_ui);
}

int MapBuilder::ThreadProc() {
	if (LoadScene(m_spawn)) {
		m_result = SR_Success;
	} else {
		m_result = SR_ParseError;
	}

	return 0;
}

int MapBuilder::RAD_IMPLEMENT_GET(result) {
	if (m_compiling)
		return m_bspBuilder.result;
	if (Thread::exited) {
		Join();
		return m_result;
	}
	return SR_Pending;
}

} // tools

#endif // RAD_OPT_TOOLS

