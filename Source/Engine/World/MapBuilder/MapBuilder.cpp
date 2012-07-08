// MapBuilder.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(RAD_OPT_TOOLS)

#include "MapBuilder.h"
#include "../World.h"
#include "../../Tools/MaxScene.h"
#include "../../COut.h"
#include "../../Engine.h"
#include <Runtime/Stream.h>

namespace tools {

MapBuilder::MapBuilder(Engine &engine)
: m_e(engine)
{
}

MapBuilder::~MapBuilder()
{
}

bool MapBuilder::LoadEntSpawn(const world::EntSpawn &spawn)
{
	const char *sz = spawn.keys.StringForKey("classname");
	if (!sz)
	{
		COut(C_Error) << "ERROR: Entity missing classname." << std::endl;
		return false;
	}

	String classname(sz);

	if (classname == "worldspawn")
	{
		return ParseWorldSpawn(spawn);
	}
	else if (classname == "static_mesh_scene")
	{
		return LoadScene(spawn);
	}

	return ParseEntity(spawn);
}

bool MapBuilder::Compile()
{
	return m_bspBuilder.Build(m_map);
}

bool MapBuilder::ParseWorldSpawn(const world::EntSpawn &spawn)
{
	if (!m_map.worldspawn) // may be spawned by static_mesh_scene
		m_map.worldspawn.reset(new (ZTools) Map::Entity());
	m_map.worldspawn->keys = spawn.keys;
	return true;
}

bool MapBuilder::ParseEntity(const world::EntSpawn &spawn)
{
	Map::Entity::Ref entity(new (ZTools) Map::Entity());
	entity->keys = spawn.keys;
	Vec3 org(spawn.keys.Vec3ForKey("origin"));
	entity->origin = Map::Vec3(org.X(), org.Y(), org.Z());
	m_map.ents.push_back(entity);
	return true;
}

bool MapBuilder::LoadScene(const world::EntSpawn &spawn)
{
	const char *sz = spawn.keys.StringForKey("file");
	if (!sz || !sz[0])
	{
		COut(C_Error) << "ERROR: static_mesh_scene missing file key!" << std::endl;
		return false;
	}

	String path(CStr(sz));
	path += ".3dx";

	file::HStreamInputBuffer fs;
	int media = file::AllMedia;

	if (m_e.sys->files->OpenFileStream(
		path.c_str,
		media,
		fs,
		file::HIONotify()
	) < file::Success)
	{
		COut(C_Error) << "ERROR: unable to open '" << sz << "'" << std::endl;
		return false;
	}

	stream::InputStream is(fs->buffer);

	return LoadMaxScene(is, m_map, false);
}

} // tools

#endif // RAD_OPT_TOOLS

