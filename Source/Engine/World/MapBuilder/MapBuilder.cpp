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
namespace map_builder {

MapBuilder::MapBuilder(Engine &engine)
: m_e(engine), m_ui(0), m_debugUI(0), m_result(SR_Success), m_compiling(false) {
}

MapBuilder::~MapBuilder() {
}

void MapBuilder::SetProgressIndicator(UIProgress *ui) {
	m_ui = ui;
}

void MapBuilder::SetDebugUI(DebugUI *ui) {
	m_debugUI = ui;
}

void MapBuilder::SetCinematicActorCompression(const CinematicActorCompressionMap &map) {
	m_caMap = map;
}

void MapBuilder::DebugDraw(float time, float dt, const QRect &viewport) {
	m_bspBuilder.DebugDraw(time, dt, viewport);
}

void MapBuilder::OnDebugMenu(const QVariant &data) {
	m_bspBuilder.OnDebugMenu(data);
}

bool MapBuilder::LoadEntSpawn(const EntSpawn &spawn) {
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
	} else if (classname == "waypoint") {
		m_result = SR_Success;
		return ParseWaypoint(spawn);
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
	ConnectWaypoints();
	m_compiling = true;
	return m_bspBuilder.SpawnCompile(m_map, m_caMap, m_ui, m_debugUI, &COut(C_Debug));
}

void MapBuilder::WaitForCompletion() {
	if (m_compiling) {
		m_bspBuilder.WaitForCompletion();
	} else {
		Join();
	}
}

bool MapBuilder::ParseWorldSpawn(const EntSpawn &spawn) {
	if (!m_map.worldspawn) // may be spawned by static_mesh_scene
		m_map.worldspawn.reset(new (ZTools) SceneFile::Entity());
	m_map.worldspawn->keys = spawn.keys;
	// NOTE: worldspawn doesn't get brushes
	return true;
}

bool MapBuilder::ParseEntity(const EntSpawn &spawn) {
	SceneFile::Entity::Ref entity(new (ZTools) SceneFile::Entity());
	entity->keys = spawn.keys;
	entity->brushes = spawn.brushes;
	Vec3 org(spawn.keys.Vec3ForKey("origin"));
	entity->origin = SceneFile::Vec3(org.X(), org.Y(), org.Z());
	m_map.ents.push_back(entity);
	return true;
}

bool MapBuilder::ParseWaypoint(const EntSpawn &spawn) {
	SceneFile::Waypoint::Ref waypoint(new (ZTools) SceneFile::Waypoint());

	Vec3 org(spawn.keys.Vec3ForKey("origin"));
	waypoint->pos = SceneFile::Vec3(org.X(), org.Y(), org.Z());
	waypoint->uid = spawn.keys.IntForKey("uid");

	const char *sz = spawn.keys.StringForKey("floor");
	if (sz)
		waypoint->floorName = sz;

	sz = spawn.keys.StringForKey("targetname");
	if (sz)
		waypoint->targetName = sz;

	sz = spawn.keys.StringForKey("userid");
	if (sz)
		waypoint->userId = sz;

	for (int i = 0; ; ++i) {
		String key;
		key.PrintfASCII("connection %i", i);
		sz = spawn.keys.StringForKey(key.c_str);
		if (!sz)
			break;

		SceneFile::WaypointConnection::Ref connection(new (ZTools) SceneFile::WaypointConnection());
		connection->waypoints.head = waypoint.get();
		connection->waypoints.headId = waypoint->uid;
		connection->waypoints.tail = 0;

		float ctrls[2][3];
		sscanf(
			sz,
			"%d %d ( %f %f %f ) ( %f %f %f )", 
			&connection->waypoints.tailId, 
			&connection->flags,
			&ctrls[0][0], &ctrls[0][1], &ctrls[0][2],
			&ctrls[1][0], &ctrls[1][1], &ctrls[1][2]
		);

		key.PrintfASCII("connection_fwd_start %i", i);
		sz = spawn.keys.StringForKey(key.c_str);
		if (sz)
			connection->cmds[0] = sz;

		key.PrintfASCII("connection_fwd_end %i", i);
		sz = spawn.keys.StringForKey(key.c_str);
		if (sz)
			connection->cmds[1] = sz;

		key.PrintfASCII("connection_back_start %i", i);
		sz = spawn.keys.StringForKey(key.c_str);
		if (sz)
			connection->cmds[2] = sz;

		key.PrintfASCII("connection_back_end %i", i);
		sz = spawn.keys.StringForKey(key.c_str);
		if (sz)
			connection->cmds[3] = sz;

		waypoint->connections.insert(SceneFile::WaypointConnection::Map::value_type(connection->waypoints, connection));
		m_map.waypointConnections.insert(SceneFile::WaypointConnection::Map::value_type(connection->waypoints, connection));
	}

	m_map.waypoints.insert(SceneFile::Waypoint::Map::value_type(waypoint->uid, waypoint));
	return true;
}

bool MapBuilder::LoadScene(const EntSpawn &spawn) {
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

void MapBuilder::ConnectWaypoints() {
	for (SceneFile::WaypointConnection::Map::iterator it = m_map.waypointConnections.begin(); it != m_map.waypointConnections.end(); ++it) {
		const SceneFile::WaypointConnection::Ref &connection = it->second;
		SceneFile::Waypoint::Map::iterator wpIt = m_map.waypoints.find(connection->waypoints.tailId);
		if (wpIt != m_map.waypoints.end()) {
			connection->waypoints.tail = wpIt->second.get();
			connection->waypoints.tail->connections.insert(SceneFile::WaypointConnection::Map::value_type(connection->waypoints, connection));
		}
	}
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

} // map_builder
} // tools

#endif // RAD_OPT_TOOLS

