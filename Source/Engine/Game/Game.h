// Game.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Engine.h"
#include "../Tickable.h"
#include "../Input.h"
#include "../World/Event.h"
#include "../World/World.h"
#include "../Packages/PackagesDef.h"
#include "GameNetwork.h"
#include <Runtime/TimeDef.h>
#include <Runtime/Container/ZoneDeque.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/PushPack.h>

class App;
class GSLoadMap;

#if defined(RAD_OPT_PC_TOOLS)
class IToolsCallbacks
{
public:
	virtual void SwapBuffers() = 0;
};
#endif

class RADENG_CLASS Game
{
public:

	typedef boost::shared_ptr<Game> Ref;
	typedef Tickable<Game> Tickable;

	struct Map
	{
		typedef boost::shared_ptr<Map> Ref;
		typedef zone_vector<Ref, ZEngineT>::type Vec;
		::world::World::Ref world;
		pkg::AssetRef asset;
		int id;
	};

	static Ref New();

	Game();
	virtual ~Game();

	virtual bool LoadEntry() { return true; }

#if defined(RAD_OPT_PC_TOOLS)
	void Tick(float dt, IToolsCallbacks *cb);
#else
	void Tick(float dt);
#endif

	// Tickable
	void Push(const Tickable::Ref &state);
	void Pop();

	void LoadMap(int id, int slot, world::UnloadDisposition ud, bool play, bool loadScreen);
	bool LoadMap(const char *name, int slot, world::UnloadDisposition ud, bool play, bool loadScreen);
	bool LoadMapSeq(int id, int slot, world::UnloadDisposition ud, bool play);
	bool LoadMapSeq(const char *name, int slot, world::UnloadDisposition ud, bool play);

	void Play();
	void Return();
	void Switch(int slot);
	void Unload(int slot);

	void CreateSaveGame(const char *name);
	void LoadSavedGame(const char *name);
	void SaveGame();
	void LoadSavedGameConflict(int num);
	void ResolveSavedGameConflict(int chosen);
	void NotifySaveState();
	void NotifyRestoreState();

	void PostInputEvent(const InputEvent &e);
	void FlushInput(bool reset=false);
	void ProcessInput();
	void SetViewport(int x, int y, int w, int h);
	void Viewport(int &x, int &y, int &w, int &h);
	bool CreateGameNetwork();

	RAD_DECLARE_READONLY_PROPERTY(Game, state, Tickable::Ref);
	RAD_DECLARE_READONLY_PROPERTY(Game, world, ::world::World::Ref);
	RAD_DECLARE_READONLY_PROPERTY(Game, inputState, const InputState*);
	RAD_DECLARE_READONLY_PROPERTY(Game, session, const Persistence::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(Game, saveGame, const Persistence::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(Game, numSavedGameConflicts, int);
	RAD_DECLARE_READONLY_PROPERTY(Game, gameNetwork, gn::GameNetwork *);
	RAD_DECLARE_PROPERTY(Game, cloudStorage, bool, bool);

#if defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_READONLY_PROPERTY(Game, toolsCallback, IToolsCallbacks*);
#endif

protected:

	virtual int OnWorldInit(world::World &world);

	virtual void OnTick(float dt);
	virtual void DoTickable(float dt);
	// Returns true if event was absorbed (not used for gesture processing)
	virtual bool OnInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is);
	virtual bool OnGesture(const InputGesture &g, const TouchState &touch, const InputState &is);
	
	virtual bool GestureInput(
		const InputEvent &e, 
		InputState &is, 
		InputGesture &g, 
		TouchState &touch,
		int enabledGestures
	);

	// Gestures
	InputEvent *DelayedEvent(const InputEvent &e);
	InputEvent *CreateDelayedEvent(const InputEvent &e);
	void RemoveDelayedEvent(void *touch);

	bool G_LClick(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g);
	bool G_RClick(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g);
	bool G_DoubleTap(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g);
	bool G_Tap(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g);
	bool G_Circle(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, bool lineGestureEnabled);
	bool G_Line(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g);
	bool G_Pinch(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g);
	
	typedef zone_set<void*, ZEngineT>::type TouchSet;
	
	InputEvent m_doubleTap;
	xtime::TimeVal m_pinch;
	xtime::TimeVal m_pinchDelay;
	TouchSet m_pinchTouches;

private:

	friend class world::World;
	friend class GSLoadMap;

	struct MapSlot
	{
		typedef zone_map<int, MapSlot, ZEngineT>::type Map;
		Game::Map::Vec queue;
		Game::Map::Ref active;
	};

	typedef zone_deque<InputEvent, ZEngineT>::type InputEventList;

	TouchState *UpdateState(const InputEvent &e, InputState &is);

	RAD_DECLARE_GET(state, Tickable::Ref) { return m_tickable.state; }
	RAD_DECLARE_GET(world, ::world::World::Ref) { return (m_slot && m_slot->active) ? m_slot->active->world : ::world::World::Ref(); }
	RAD_DECLARE_GET(inputState, const InputState*) { return &m_inputState; }
	RAD_DECLARE_GET(session, const Persistence::Ref&) { return m_session; }
	RAD_DECLARE_GET(saveGame, const Persistence::Ref&) { return m_saveGame; }
	RAD_DECLARE_GET(numSavedGameConflicts, int) { return (int)m_cloudVersions.size(); }
	RAD_DECLARE_GET(cloudStorage, bool) { return m_cloudStorage; }
	RAD_DECLARE_SET(cloudStorage, bool) { m_cloudStorage = value; }
	RAD_DECLARE_GET(gameNetwork, gn::GameNetwork*) { return m_gameNetwork.get(); }

#if defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_GET(toolsCallback, IToolsCallbacks*) { return m_toolsCallback; }
	IToolsCallbacks *m_toolsCallback;
#endif

	TickQueue<Game> m_tickable;
	InputEventList m_inputEvents;
	InputEventList m_delayedEvents;
	InputState m_inputState;
	MapSlot *m_slot;
	MapSlot::Map m_maps;
	String m_saveGameName;
	Persistence::Ref m_session;
	Persistence::Ref m_saveGame;
	CloudFile::Ref m_cloudFile;
	CloudFile::Vec m_cloudVersions;
	gn::GameNetworkRef m_gameNetwork;
	gn::GameNetworkEventQueue m_gameNetworkEventQueue;
	bool m_cloudStorage;
	int m_vp[4];
};


#include <Runtime/PopPack.h>

