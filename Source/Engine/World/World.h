// World.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "WorldDef.h"
#include "BSPFile.h"
#include "Entity.h"
#include "Event.h"
#include "../Camera.h"
#include "WorldDraw.h"
#include "WorldLua.h"
#include "WorldCinematics.h"
#include "Floors.h"
#include "../Engine.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../UI/UIWidgetDef.h"
#include "../Sound/SoundDef.h"
#include "../Game/GameNetworkDef.h"
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

class Game;
class Engine;

namespace world {

class WorldDraw;
class RADENG_CLASS World {
public:

	typedef WorldRef Ref;
	typedef WorldWRef WRef;

	static Ref New(Game &game, int slot, const SoundContextRef &sound, pkg::Zone zone);
	virtual ~World();

	int Init();

	int Spawn(
		const char *mapName,
		const bsp_file::BSPFile::Ref &bsp,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnEntity(
		Entity::Ref &entity,
		const Keys &keys,
		const xtime::TimeSlice &time
	);

	int SpawnTempEntity(
		Entity::Ref &entity,
		const Keys &keys,
		const xtime::TimeSlice &time
	);

	int PostSpawnEntity(
		const Entity::Ref &entity,
		const xtime::TimeSlice &time
	);

	void SetGameSpeed(float speed, float duration);

	void Tick(float dt);
	void Draw();

	void NotifyBackground();
	void NotifyResume();
	void SaveApplicationState();
	void RestoreApplicationState();

	bool PendingLoadRequest(String &mapName, UnloadDisposition &ud, bool &loadScreen);
	bool PendingReturnRequest();
	bool PendingSwitch(int &slot);
	bool PendingUnloadSlot(int &slot);
	bool PendingSwitchLoadRequest(int &slot, String &mapName, UnloadDisposition &ud, bool &loadScreen);

	void RequestLoad(const char *map, UnloadDisposition ud, bool loadScreen);
	void RequestReturn();
	void RequestSwitch(int slot);
	void RequestUnloadSlot(int slot);
	void RequestSwitchLoad(int slot, const char *map, UnloadDisposition ud, bool loadScreen); 

	Event::Vec ParseMultiEvent(const char *string);
	void PostEvent(const char *string);
	void DispatchEvent(const char *string);
	void PostEvent(const Event::Ref &event);
	void PostEvents(const Event::Vec &events);
	void DispatchEvent(const Event::Ref &event);
	void DispatchEvents(const Event::Vec &events);
	void UnmapEntity(const Entity::Ref &entity);
	bool HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is);
	bool HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is);

	void OnLocalPlayerAuthenticated(gn::NetResult r);
	void OnShowLeaderboard(bool show);
	void OnShowAchievements(bool show);

	void SetAreaportalState(int areaportalNum, bool open, bool relinkOccupants);

	Entity::Ref FindEntityId(int id) const;
	Entity::Vec FindEntityClass(const char *classname) const;
	Entity::Vec FindEntityTargets(const char *targetname) const;
	Entity::Vec BBoxTouching(const BBox &bbox, int stypes) const;
	ZoneTagRef ZoneTag(int id) const;

	//! Clips a volume into the world, and returns a mask indicating visible areas.
	/*! Set toArea to -1 to find all areas seen within the volume, otherwise the mask 
	    may only indicate areas through which toArea was seen.

		\remarks NOTE: clipped may contain multiple volumes for each area (if there were
		multiple portals into the area in question.
	*/
	bool ClipOccupantVolume(
		const Vec3 *pos, // if null just bounding volume test
		const StackWindingStackVec *volume,  // if null just bounding volume test
		const BBox &volumeBounds,
		ClippedAreaVolumeStackVec *clipped,
		int fromArea,
		int toArea,
		AreaBits &visible,
		WorldDraw::Counters *counters = 0
	);

	//! Trace a line through the world BSP.
	bool LineTrace(Trace &trace);
	
	RAD_DECLARE_PROPERTY(World, viewController, Entity::Ref, Entity::Ref);
	RAD_DECLARE_PROPERTY(World, playerPawn, Entity::Ref, Entity::Ref);
	RAD_DECLARE_PROPERTY(World, worldspawn, Entity::Ref, Entity::Ref);
	RAD_DECLARE_PROPERTY(World, pauseState, int, int);
	RAD_DECLARE_PROPERTY(World, enabledGestures, int, int);
	RAD_DECLARE_READONLY_PROPERTY(World, time, float);
	RAD_DECLARE_READONLY_PROPERTY(World, gameTime, float);
	RAD_DECLARE_READONLY_PROPERTY(World, dt, float);
	RAD_DECLARE_READONLY_PROPERTY(World, camera, Camera*);
	RAD_DECLARE_READONLY_PROPERTY(World, game, Game*);
	RAD_DECLARE_READONLY_PROPERTY(World, lua, WorldLua*);
	RAD_DECLARE_READONLY_PROPERTY(World, draw, WorldDraw*);
	RAD_DECLARE_READONLY_PROPERTY(World, cinematics, WorldCinematics*);
	RAD_DECLARE_READONLY_PROPERTY(World, floors, Floors*);
	RAD_DECLARE_READONLY_PROPERTY(World, pkgZone, pkg::Zone);
	RAD_DECLARE_READONLY_PROPERTY(World, uiRoot, const ui::RootRef&);
	RAD_DECLARE_READONLY_PROPERTY(World, destroy, bool);
	RAD_DECLARE_READONLY_PROPERTY(World, sound, SoundContext*);
	RAD_DECLARE_READONLY_PROPERTY(World, slot, int);
	RAD_DECLARE_READONLY_PROPERTY(World, drawCounters, const WorldDraw::Counters*);
	RAD_DECLARE_READONLY_PROPERTY(World, listenerPos, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(World, bspFile, const bsp_file::BSPFile*);

private:

	friend class WorldDraw;
	friend class Entity;
	typedef zone_list<Event::Ref, ZWorldT>::type EventList;
	typedef zone_map<int, ZoneTagRef, ZWorldT>::type ZoneIdMap;

	World(Game &game, int slot, const SoundContextRef &sound, pkg::Zone zone);

	enum {
		SS_None,
		SS_BSP,
		SS_SoundEmitter,
		SS_Draw,
		SS_Materials,
		SS_Models,
		SS_Cinematics,
		SS_BuiltIns,
		SS_Ents,
		SS_Specials,
		SS_PostSpawn,
		SS_Done
	};

	int SpawnMaterials(
		const bsp_file::BSPFile &bsp,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnMaterial(
		const bsp_file::BSPFile &bsp,
		U32 matNum,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnModels(
		const bsp_file::BSPFile &bsp,
		const xtime::TimeSlice &time,
		int flags
	);

	void SpawnModel(
		const bsp_file::BSPFile &bsp,
		U32 modelNum
	);

	int SpawnSpecials(
		const bsp_file::BSPFile &bsp,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnSoundEntities(
		const bsp_file::BSPFile &bsp,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnNonSoundEntities(
		const bsp_file::BSPFile &bsp,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnEntity(
		const bsp_file::BSPFile &bsp,
		U32 entityNum,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnSoundEmitter(
		const bsp_file::BSPFile &bsp,
		U32 entityNum,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnNonSoundEmitter(
		const bsp_file::BSPFile &bsp,
		U32 entityNum,
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnBuiltIns(
		const xtime::TimeSlice &time,
		int flags
	);

	int SpawnBuiltIn(
		U32 entityNum,
		const xtime::TimeSlice &time,
		int flags
	);

	int LevelStart();

	void SetupEntity(const Entity::Ref &entity, int id);
	void MapEntity(const Entity::Ref &entity);
	int CreateEntity(const Keys &keys);
	int CreateEntity(const bsp_file::BSPFile &bsp, U32 entityNum);
	Keys LoadEntityKeys(const bsp_file::BSPFile &bsp, U32 entityNum);
	void TickState(float dt, float unmod_dt);
	void DispatchEvents();
	void FlushEvents();
	int PostSpawn(const xtime::TimeSlice &time, int flags);
	void LoadBSP(const bsp_file::BSPFile &bsp);
	void LinkEntity(Entity *entity, const BBox &bounds);
	void UnlinkEntity(Entity *entity);

	struct LinkEntityParms {

		LinkEntityParms(
			Entity *_entity,
			const BBox &_bounds,
			AreaBits &_visible
		) : entity(_entity),
		    bounds(_bounds),
			visible(_visible) {
		}

		Entity *entity;
		const BBox &bounds;
		AreaBits &visible;
	};

	void LinkEntity(const LinkEntityParms &constArgs, int nodeNum);

	struct ClipOccupantVolumeParms {

		ClipOccupantVolumeParms(
			const Vec3 *_pos,
			ClippedAreaVolumeStackVec *_clipped,
			int _toArea,
			AreaBits &_visible,
			AreaBits &_stack,
			WorldDraw::Counters *_counters
		) : pos(_pos), 
		    clipped(_clipped), 
			toArea(_toArea), 
			visible(_visible),
			stack(_stack),
			counters(_counters) {
		}
		
		const Vec3 *pos;
		ClippedAreaVolumeStackVec *clipped;
		int toArea;
		AreaBits &visible;
		AreaBits &stack;
		WorldDraw::Counters *counters;
	};

	bool ClipOccupantVolume(
		const ClipOccupantVolumeParms &constArgs,
		const StackWindingStackVec *volume,
		const BBox &volumeBounds,
		int fromArea
	);

	bool LineTrace(Trace &trace, int node);

	dBSPLeaf *LeafForPoint(const Vec3 &pos);
	dBSPLeaf *LeafForPoint(const Vec3 &pos, int nodeNum);

	static void BoundWindings(const BBox &bounds, StackWindingStackVec &windings);
	static bool ChopWindingToVolume(const StackWindingStackVec &volume, StackWinding &out);
	static bool ChopVolume(StackWindingStackVec &volume, BBox &bounds, const Plane &p);
	static bool ChopVolume(StackWindingStackVec &volume, BBox &bounds, const PlaneVec &planes);
	static bool IntersectVolumes(const StackWindingStackVec &a, StackWindingStackVec &out, BBox &bounds);
	static void MakeVolume(const Plane *planes, int num, StackWindingStackVec &volume, BBox &bounds);
	static void MakeBoundingPlanes(const Vec3 &pos, const StackWinding &portal, PlaneVec &planes);

	void BBoxTouching(
		const BBox &bbox,
		int stypes,
		int nodeNum,
		Entity::Vec &out,
		EntityBits &bits
	) const;

	RAD_DECLARE_GET(viewController, Entity::Ref) { 
		return m_viewController; 
	}

	RAD_DECLARE_GET(playerPawn, Entity::Ref) { 
		return m_playerPawn; 
	}

	RAD_DECLARE_GET(worldspawn, Entity::Ref) { 
		return m_worldspawn; 
	}

	RAD_DECLARE_SET(viewController, Entity::Ref) { 
		m_viewController = value; 
	}

	RAD_DECLARE_SET(playerPawn, Entity::Ref) { 
		m_playerPawn = value; 
	}

	RAD_DECLARE_SET(worldspawn, Entity::Ref) { 
		m_worldspawn = value; 
	}

	RAD_DECLARE_GET(time, float) { 
		return m_time; 
	}

	RAD_DECLARE_GET(gameTime, float) { 
		return m_gameTime; 
	}

	RAD_DECLARE_GET(dt, float) { 
		return m_dt; 
	}

	RAD_DECLARE_GET(camera, Camera*) { 
		return const_cast<Camera*>(&m_cam); 
	}

	RAD_DECLARE_GET(game, Game*) { 
		return m_game; 
	}

	RAD_DECLARE_GET(lua, WorldLua*) { 
		return m_lua.get(); 
	}

	RAD_DECLARE_GET(draw, WorldDraw*) { 
		return m_draw.get(); 
	}

	RAD_DECLARE_GET(cinematics, WorldCinematics*) { 
		return m_cinematics.get(); 
	}

	RAD_DECLARE_GET(floors, Floors*) {
		return const_cast<Floors*>(&m_floors);
	}

	RAD_DECLARE_GET(pkgZone, pkg::Zone) { 
		return m_pkgZone; 
	}

	RAD_DECLARE_GET(uiRoot, const ui::RootRef&) { 
		return m_uiRoot; 
	}

	RAD_DECLARE_GET(destroy, bool) { 
		return m_destroy; 
	}

	RAD_DECLARE_GET(pauseState, int) { 
		return m_pauseState; 
	}

	RAD_DECLARE_SET(pauseState, int) { 
		m_pauseState = value; 
	}

	RAD_DECLARE_GET(sound, SoundContext*) { 
		return m_sound.get();
	}

	RAD_DECLARE_GET(slot, int) { 
		return m_slot; 
	}

	RAD_DECLARE_GET(enabledGestures, int) { 
		return m_gestures; 
	}

	RAD_DECLARE_SET(enabledGestures, int);

	RAD_DECLARE_GET(drawCounters, const WorldDraw::Counters*) { 
		return &m_drawCounters; 
	}

	RAD_DECLARE_GET(bspFile, const bsp_file::BSPFile*) {
		return m_bsp.get();
	}
	
	RAD_DECLARE_GET(listenerPos, const Vec3&);

	EventList m_events;
	ZoneRef m_zone;
	EntityRef m_spawnEnt;
	Keys m_spawnKeys;
	pkg::Asset::Ref m_spawnAsset;
	pkg::Asset::Vec m_bspMaterials;
	bsp_file::BSPFile::Ref m_bsp;
	Entity::IdMap m_ents;
	Entity::StringMMap m_classnames;
	Entity::StringMMap m_targetnames;
	Entity::Ref m_playerPawn;
	Entity::Ref m_viewController;
	Entity::Ref m_worldspawn;
	ZoneIdMap m_zoneTags;
	Camera m_cam;
	WorldDraw::Ref m_draw;
	WorldCinematics::Ref m_cinematics;
	Floors m_floors;
	Game *m_game;
	WorldLua::Ref m_lua;
	pkg::Zone m_pkgZone;
	ui::RootRef m_uiRoot;
	SoundContextRef m_sound;
	WorldDraw::Counters m_drawCounters;
	dBSPNode::Vec m_nodes;
	dBSPLeaf::Vec m_leafs;
	dBSPArea::Vec m_areas;
	StringVec m_builtIns;
	dAreaportal::Vec m_areaportals;
	PlaneVec m_planes;
	U32 m_spawnOfs;
	int m_frame;
	int m_spawnState;
	int m_nextEntId;
	int m_nextTempEntId;
	int m_pauseState;
	float m_time;
	float m_gameTime;
	float m_dt;
	String m_mapPath;
	String m_loadMap;
	bool m_destroy;
	bool m_loadReq;
	bool m_loadScreen;
	UnloadDisposition m_unloadDisp;
	bool m_returnReq;
	bool m_levelStart;
	bool m_switchReq;
	int m_switchSlot;
	bool m_unloadSlotReq;
	int m_unloadSlot;
	int m_slot;
	bool m_switchLoad;
	bool m_switchLoadScreen;
	float m_gameSpeed[3];
	float m_gameSpeedTime[2];
	int m_gestures;
};

class RADENG_CLASS Zone {
public:

	typedef ZoneRef Ref;
	typedef ZoneWRef WRef;

	RAD_DECLARE_READONLY_PROPERTY(Zone, world, World*);

private:

	explicit Zone(const World::Ref &world) : m_world(world) {}
	
	RAD_DECLARE_GET(world, World*) { 
		return m_world.lock().get(); 
	}
	
	friend class World;

	World::WRef m_world;
};

class RADENG_CLASS ZoneTag {
public:

	typedef ZoneTagRef Ref;
	typedef ZoneTagWRef WRef;

	RAD_DECLARE_READONLY_PROPERTY(ZoneTag, zone, Zone::Ref);
	RAD_DECLARE_READONLY_PROPERTY(ZoneTag, game, Game*);

private:

	friend class World;
	ZoneTag(const Zone::Ref &zone, Game &game) : m_zone(zone), m_game(&game) {}

	RAD_DECLARE_GET(zone, Zone::Ref) { 
		return m_zone.lock(); 
	}

	RAD_DECLARE_GET(game, Game*) { 
		return m_game; 
	}

	Zone::WRef m_zone;
	Game *m_game;
};

} // world

#include <Runtime/PopPack.h>
