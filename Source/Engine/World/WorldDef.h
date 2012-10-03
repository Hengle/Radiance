// WorldDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once
#include "../Types.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Math/ConvexPolygon.h>
#include <bitset>

namespace world {

class Entity;

enum UnloadDisposition {
	kUD_None,
	kUD_Slot,
	kUD_All
};

enum PauseFlags {
	RAD_FLAG(kPauseGame),
	RAD_FLAG(kPauseUI),
	RAD_FLAG(kPauseCinematics),
	kPauseAll = kPauseGame|kPauseUI|kPauseCinematics
};

enum {
	kMaxEnts = 1024,
	kMaxTempEnts = 64,
	kMaxStaticEnts = kMaxEnts-kMaxTempEnts,
	kFirstTempEntId = kMaxStaticEnts,
	kMaxAreas = 256
};

typedef zone_vector<Plane, ZWorldT>::type PlaneVec;
typedef zone_set<Entity*, ZWorldT>::type EntityPtrSet;
typedef zone_set<int, ZWorldT>::type IntSet;
typedef std::bitset<kMaxEnts> EntityBits;
typedef std::bitset<kMaxAreas> AreaBits;
typedef math::ConvexPolygon<Vec3, Plane, zone_allocator<Vec3, ZWorldT> > Winding;
typedef zone_vector<Winding, ZWorldT>::type WindingVec;

struct dBSPNode {
	typedef zone_vector<dBSPNode, ZWorldT>::type Vec;
	BBox bounds;
	int parent;
	int planenum;
	int children[2];
};

struct dBSPLeaf {
	typedef zone_vector<dBSPLeaf, ZWorldT>::type Vec;
	typedef zone_vector<dBSPLeaf*, ZWorldT>::type PtrVec;
	BBox bounds;
	int parent;
	int area;
	int contents;
	int firstClipSurface;
	int numClipSurfaces;

	EntityPtrSet occupants;
};

struct dBSPAreaNode {
	typedef zone_vector<dBSPAreaNode, ZWorldT>::type Vec;
	BBox bounds;
	int parent;
	int planenum;
	int children[2];
};

struct dBSPAreaLeaf {
	typedef zone_vector<dBSPAreaLeaf, ZWorldT>::type Vec;
	typedef zone_vector<dBSPAreaLeaf*, ZWorldT>::type PtrVec;
	BBox bounds;
	int firstModel;
	int numModels;

	EntityPtrSet occupants;
};

class World;
class WorldLua;
class WorldDraw;
class Zone;
class ZoneTag;

typedef boost::shared_ptr<World> WorldRef;
typedef boost::weak_ptr<World> WorldWRef;
typedef boost::shared_ptr<Zone> ZoneRef;
typedef boost::weak_ptr<Zone> ZoneWRef;
typedef boost::shared_ptr<ZoneTag> ZoneTagRef;
typedef boost::weak_ptr<ZoneTag> ZoneTagWRef;

} // world
