/*! \file WorldDef.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

#pragma once
#include "../Types.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/StackVector.h>
#include <Runtime/Math/Winding.h>
#include <bitset>

#define WORLD_DEBUG_DRAW

namespace world {

class Entity;
class MBatchOccupant;
class World;
class WorldLua;
class WorldDraw;
class Zone;
class ZoneTag;
class WorldDrawLight;
class RB_WorldDraw;
class MBatchDraw;
class Light;

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
	kMaxEnts = 2048,
	kMaxTempEnts = 1024,
	kMaxStaticEnts = kMaxEnts-kMaxTempEnts,
	kFirstTempEntId = kMaxStaticEnts,
	kMaxAreas = 64,
	kSkyArea = 0
};

typedef zone_vector<Plane, ZWorldT>::type PlaneVec;
typedef zone_vector<int, ZWorldT>::type IntVec;
typedef zone_pool_set<Entity*, ZWorldT>::type EntityPtrSet;
typedef zone_pool_set<Light*, ZWorldT>::type LightPtrSet;
typedef zone_pool_set<MBatchOccupant*, ZWorldT>::type MBatchOccupantPtrSet;
typedef zone_pool_set<int, ZWorldT>::type IntSet;
typedef std::bitset<kMaxEnts> EntityBits;
typedef std::bitset<kMaxAreas> AreaBits;
typedef math::Winding<Vec3, Plane, zone_allocator<Vec3, ZWorldT> > Winding;
typedef zone_vector<Winding, ZWorldT>::type WindingVec;
typedef math::Winding<Vec3, Plane, math::stack_tag<24> > StackWinding;
typedef zone_vector<StackWinding, ZWorldT>::type StackWindingVec;
typedef stackify<StackWindingVec, 12> StackWindingStackVec;
typedef zone_vector<Light*, ZWorldT>::type LightVec;

typedef boost::shared_ptr<World> WorldRef;
typedef boost::weak_ptr<World> WorldWRef;
typedef boost::shared_ptr<Zone> ZoneRef;
typedef boost::weak_ptr<Zone> ZoneWRef;
typedef boost::shared_ptr<ZoneTag> ZoneTagRef;
typedef boost::weak_ptr<ZoneTag> ZoneTagWRef;

struct ClippedAreaVolume {

	ClippedAreaVolume(int _area, const StackWindingStackVec &_volume, const BBox &_bounds) :
		area(_area), volume(_volume), bounds(_bounds) {
	}

	int area;
	StackWindingStackVec volume;
	BBox bounds;
};

typedef stackify< std::vector<ClippedAreaVolume>, 8 > ClippedAreaVolumeStackVec;

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
	int firstClipModel;
	int numClipModels;

	EntityPtrSet entities;
	MBatchOccupantPtrSet occupants;
	LightPtrSet lights;
};

struct dBSPArea {
	typedef zone_vector<dBSPArea, ZWorldT>::type Vec;
	typedef zone_vector<dBSPArea*, ZWorldT>::type PtrVec;

	BBox bounds;
	int firstPortal;
	int numPortals;
	int firstModel;
	int numModels;
	bool sky;

	EntityPtrSet entities;
	MBatchOccupantPtrSet occupants;
	LightPtrSet lights;
};

struct dAreaportal {
	typedef zone_vector<dAreaportal, ZWorldT>::type Vec;

	bool open;
	int areas[2];
	int planenum;
	Winding winding;
	BBox bounds;
};

struct Trace {
	Vec3 start;
	Vec3 end;
	Vec3 traceEnd;
	Vec3 normal;
	int contents;
	float frac;
	bool startSolid;
};

RAD_BEGIN_FLAGS
	RAD_FLAG(kLightingFlag_CastShadows),
	kLightingFlag_None = 0
RAD_END_FLAGS(LightingFlags)

namespace details {

struct MBatchDrawLink;

struct MatRef;
typedef zone_map<int, MatRef, ZWorldT>::type MatRefMap;

struct MBatch;
typedef boost::shared_ptr<MBatch> MBatchRef;
typedef zone_map<int, MBatchRef, ZWorldT>::type MBatchIdMap;

struct LightInteraction {
	LightInteraction *prevOnLight;
	LightInteraction *nextOnLight;
	LightInteraction *prevOnBatch;
	LightInteraction *nextOnBatch;
	MBatchDraw *draw;
	Light *light;
	Entity *entity;
	MBatchOccupant *occupant;
	bool dirty;
};

typedef zone_pool_map<int, LightInteraction*, ZWorldT>::type MatInteractionChain;

} // details
} // world

RAD_IMPLEMENT_FLAGS(world::LightingFlags)