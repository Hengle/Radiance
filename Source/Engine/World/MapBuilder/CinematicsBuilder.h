/*! \file CinematicsBuilder.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_builder
*/

#pragma once

#include "../BSPFile.h"
#include "../../SkAnim/SkAnimDef.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/PushPack.h>

namespace tools {

class CinematicsBuilder {
public:

	CinematicsBuilder() : m_skaSize(0) {}

	bool Compile(
		const SceneFile &map,
		const CinematicActorCompressionMap &caMap,
		const world::bsp_file::BSPFileBuilder::Ref &bspFile
	);

	RAD_DECLARE_READONLY_PROPERTY(CinematicsBuilder, skaSize, int);

private:

	typedef zone_vector<int, world::bsp_file::ZBSPBuilderT>::type IntVec;
	typedef zone_set<String, world::bsp_file::ZBSPBuilderT>::type StringSet;

	struct Actor {
		typedef zone_vector<Actor, world::bsp_file::ZBSPBuilderT>::type Vec;
		Actor() : emitId(-1), index(-1) {}

		int flags;
		int emitId;
		int index;
	};

	struct Trigger {
		typedef zone_map<int, Trigger, world::bsp_file::ZBSPBuilderT>::type Map;
		int frame;
		int camera;
		IntVec actors;
	};

	RAD_DECLARE_GET(skaSize, int) {
		return m_skaSize;
	}

	bool EmitActor(const SceneFile &map, const SkaCompressionMap *compression, Actor &actor);
	bool EmitCinematic(const SceneFile &map, const CinematicActorCompressionMap &caMap, const String &name);

	StringSet m_cinematics;
	Actor::Vec m_actors;
	world::bsp_file::BSPFileBuilder::Ref m_bspFile;
	int m_skaSize;
};

} // tools

#include <Runtime/PopPack.h>
