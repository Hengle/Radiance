// MapBuilder.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../Entity.h"
#include "../WorldDef.h"
#include "BoxBSP/BoxBSP.h"
#include "../../Tools/Map.h"
#include <Runtime/PushPack.h>

class Engine;

namespace tools {

class RADENG_CLASS MapBuilder
{
public:
	typedef boost::shared_ptr<MapBuilder> Ref;

	MapBuilder(Engine &engine);
	~MapBuilder();

	bool LoadEntSpawn(const world::EntSpawn &spawn);
	bool Compile();
	
	RAD_DECLARE_READONLY_PROPERTY(MapBuilder, bspFile, world::bsp_file::BSPFile::Ref);
	RAD_DECLARE_READONLY_PROPERTY(MapBuilder, bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref);

private:

	RAD_DECLARE_GET(bspFile, world::bsp_file::BSPFile::Ref) { return m_bspBuilder.bspFile; }
	RAD_DECLARE_GET(bspFileBuilder, world::bsp_file::BSPFileBuilder::Ref) { return m_bspBuilder.bspFileBuilder; }

	bool ParseWorldSpawn(const world::EntSpawn &spawn);
	bool ParseEntity(const world::EntSpawn &spawn);
	bool LoadScene(const world::EntSpawn &spawn);

	Map m_map;
	Engine &m_e;
	box_bsp::BSPBuilder m_bspBuilder;
};

} // tools

#include <Runtime/PopPack.h>
