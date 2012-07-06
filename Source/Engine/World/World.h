// World.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
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
#include "../Engine.h"
#include "../Renderer/Mesh.h"
#include "../Renderer/Material.h"
#include "../UI/UIWidgetDef.h"
#include "../Sound/SoundDef.h"
#include "../Game/GameNetworkDef.h"
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

class Game;
class Engine;

namespace world {

class RADENG_CLASS World
{
public:

	enum
	{
		MaxEnts = 1024,
		MaxTempEnts = 64,
		MaxStaticEnts = MaxEnts-MaxTempEnts,
		FirstTempEntId = MaxStaticEnts
	};

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

	void SetGameSpeed(float speed, float time);

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

	Entity::Ref FindEntityId(int id) const;
	Entity::Vec FindEntityClass(const char *classname) const;
	Entity::Vec FindEntityTargets(const char *targetname) const;
	Entity::Vec BBoxTouching(const BBox &bbox, int stypes) const;
	ZoneTagRef ZoneTag(int id) const;
	
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
	RAD_DECLARE_READONLY_PROPERTY(World, pkgZone, pkg::Zone);
	RAD_DECLARE_READONLY_PROPERTY(World, uiRoot, const ui::RootRef&);
	RAD_DECLARE_READONLY_PROPERTY(World, destroy, bool);
	RAD_DECLARE_READONLY_PROPERTY(World, sound, SoundContext*);
	RAD_DECLARE_READONLY_PROPERTY(World, slot, int);
	RAD_DECLARE_READONLY_PROPERTY(World, drawCounters, const WorldDraw::Counters*);
	RAD_DECLARE_READONLY_PROPERTY(World, listenerPos, const Vec3&);

private:

	World(Game &game, int slot, const SoundContextRef &sound, pkg::Zone zone);

	enum
	{
		SS_None,
		SS_SoundEmitter,
		SS_Draw,
		SS_Materials,
		SS_Models,
		SS_Cinematics,
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

	void SetupEntity(const Entity::Ref &entity, int id);
	void MapEntity(const Entity::Ref &entity);
	int CreateEntity(const Keys &keys);
	int CreateEntity(const bsp_file::BSPFile &bsp, U32 entityNum);
	Keys LoadEntityKeys(const bsp_file::BSPFile &bsp, U32 entityNum);
	void TickState(float dt, float unmod_dt);
	void DispatchEvents();
	void FlushEvents();
	int PostSpawn(const xtime::TimeSlice &time, int flags);

	RAD_DECLARE_GET(viewController, Entity::Ref) { return m_viewController; }
	RAD_DECLARE_GET(playerPawn, Entity::Ref) { return m_playerPawn; }
	RAD_DECLARE_GET(worldspawn, Entity::Ref) { return m_worldspawn; }
	RAD_DECLARE_SET(viewController, Entity::Ref) { m_viewController = value; }
	RAD_DECLARE_SET(playerPawn, Entity::Ref) { m_playerPawn = value; }
	RAD_DECLARE_SET(worldspawn, Entity::Ref) { m_worldspawn = value; }
	RAD_DECLARE_GET(time, float) { return m_time; }
	RAD_DECLARE_GET(gameTime, float) { return m_gameTime; }
	RAD_DECLARE_GET(dt, float) { return m_dt; }
	RAD_DECLARE_GET(camera, Camera*) { return &const_cast<World*>(this)->m_cam; }
	RAD_DECLARE_GET(game, Game*) { return m_game; }
	RAD_DECLARE_GET(lua, WorldLua*) { return m_lua.get(); }
	RAD_DECLARE_GET(draw, WorldDraw*) { return m_draw.get(); }
	RAD_DECLARE_GET(cinematics, WorldCinematics*) { return m_cinematics.get(); }
	RAD_DECLARE_GET(pkgZone, pkg::Zone) { return m_pkgZone; }
	RAD_DECLARE_GET(uiRoot, const ui::RootRef&) { return m_uiRoot; }
	RAD_DECLARE_GET(destroy, bool) { return m_destroy; }
	RAD_DECLARE_GET(pauseState, int) { return m_pauseState; }
	RAD_DECLARE_SET(pauseState, int) { m_pauseState = value; }
	RAD_DECLARE_GET(sound, SoundContext*) { return m_sound.get(); }
	RAD_DECLARE_GET(slot, int) { return m_slot; }
	RAD_DECLARE_GET(enabledGestures, int) { return m_gestures; }
	RAD_DECLARE_SET(enabledGestures, int);
	RAD_DECLARE_GET(drawCounters, const WorldDraw::Counters*) { return &m_drawCounters; }
	RAD_DECLARE_GET(listenerPos, const Vec3&);
	
	typedef zone_list<Event::Ref, ZWorldT>::type EventList;
	typedef zone_map<int, ZoneTagRef, ZWorldT>::type ZoneIdMap;

	EventList m_events;
	ZoneRef m_zone;
	EntityRef m_spawnEnt;
	Keys m_spawnKeys;
	pkg::Asset::Ref m_spawnAsset;
	pkg::Asset::Vec m_bspMaterials;
	r::Mesh::Vec m_staticMeshes;
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
	Game *m_game;
	WorldLua::Ref m_lua;
	pkg::Zone m_pkgZone;
	ui::RootRef m_uiRoot;
	SoundContextRef m_sound;
	WorldDraw::Counters m_drawCounters;
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
	bool m_destroy;
	String m_loadMap;
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

class RADENG_CLASS Zone
{
public:

	typedef ZoneRef Ref;
	typedef ZoneWRef WRef;

	RAD_DECLARE_READONLY_PROPERTY(Zone, world, World*);

private:

	explicit Zone(const World::Ref &world) : m_world(world) {}
	RAD_DECLARE_GET(world, World*) { return m_world.lock().get(); }
	
	friend class World;

	World::WRef m_world;
};

class RADENG_CLASS ZoneTag
{
public:

	typedef ZoneTagRef Ref;
	typedef ZoneTagWRef WRef;

	RAD_DECLARE_READONLY_PROPERTY(ZoneTag, zone, Zone::Ref);
	RAD_DECLARE_READONLY_PROPERTY(ZoneTag, game, Game*);

private:

	friend class World;
	ZoneTag(const Zone::Ref &zone, Game &game) : m_zone(zone), m_game(&game) {}
	RAD_DECLARE_GET(zone, Zone::Ref) { return m_zone.lock(); }
	RAD_DECLARE_GET(game, Game*) { return m_game; }

	Zone::WRef m_zone;
	Game *m_game;
};

} // world

#include <Runtime/PopPack.h>
