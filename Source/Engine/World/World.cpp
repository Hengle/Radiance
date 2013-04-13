// World.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "World.h"
#include "../Game/Game.h"
#include "../Game/GameCVars.h"
#include "../UI/UIWidget.h"
#include <Runtime/Tokenizer.h>
#include <Runtime/Stream/MemoryStream.h>
#include "../Packages/PackagesDef.h"
#include "../App.h"
#include "../Engine.h"
#include "../Sound/Sound.h"
#include "../MathUtils.h"
#include <algorithm>

namespace world {

World::Ref World::New(Game &game, int slot, const SoundContext::Ref &sound, pkg::Zone zone) {
	World::Ref w(new (ZEngine) World(game, slot, sound, zone));
	w->m_zone.reset(new (ZEngine) Zone(w));
	return w;
}

World::World(Game &game, int slot, const SoundContext::Ref &sound, pkg::Zone zone) : 
m_game(&game), 
m_spawnState(SS_None), 
m_spawnOfs(0),
m_nextEntId(0), 
m_nextTempEntId(0),
m_time(0.f),
m_gameTime(0.f),
m_dt(0.f),
m_pkgZone(zone),
m_frame(0),
m_destroy(false),
m_pauseState(0),
m_sound(sound),
m_loadReq(false),
m_loadScreen(false),
m_unloadDisp(kUD_None),
m_returnReq(false),
m_slot(slot),
m_switchReq(false),
m_switchSlot(-1),
m_unloadSlot(-1),
m_unloadSlotReq(false),
m_switchLoad(false),
m_switchLoadScreen(false),
m_generateSave(false),
m_gestures(0) {
	m_draw.reset(new (ZWorld) WorldDraw(this));
	m_cinematics.reset(new (ZWorld) WorldCinematics(this));
	m_gameSpeed[0] = m_gameSpeed[1] = m_gameSpeed[2] = 1.f;
	m_gameSpeedTime[0] = m_gameSpeedTime[1] = 0.f;
}

World::~World() {
	m_destroy = true;
	m_classnames.clear();
	m_targetnames.clear();
	m_playerPawn.reset();
	m_viewController.reset();
	m_worldspawn.reset();
	m_cinematics.reset();
	m_uids.clear();
	m_ents.clear();
	m_lua.reset();
	m_uiRoot.reset();
	m_draw.reset();
}

int World::Init() {
	int r = m_draw->LoadMaterials();
	if (r != pkg::SR_Success)
		return r;

	m_lua.reset(new (ZWorld) WorldLua(this));
	if (!m_lua->Init())
		return pkg::SR_ScriptError;
	m_uiRoot = ui::Root::New(m_lua->L);

	return m_game->OnWorldInit(*this);
}

void World::SetGameSpeed(float speed, float duration) {
	if (duration > 0.f) {
		m_gameSpeed[1] = m_gameSpeed[0];
		m_gameSpeed[2] = speed;
		m_gameSpeedTime[0] = 0.f;
		m_gameSpeedTime[1] = duration;
	} else {
		m_gameSpeed[0] = speed;
		m_gameSpeedTime[1] = 0.f;
	}
}

void World::Tick(float dt) {
	if (m_gameSpeedTime[1] > 0.f) {
		m_gameSpeedTime[0] += dt;
		if (m_gameSpeedTime[0] >= m_gameSpeedTime[1]) {
			m_gameSpeed[0] = m_gameSpeed[2];
			m_gameSpeedTime[1] = 0.f;
		} else {
			m_gameSpeed[0] = math::Lerp(m_gameSpeed[1], m_gameSpeed[2], m_gameSpeedTime[0]/m_gameSpeedTime[1]);
		}
	}
	
	float unmod_dt = dt;
	dt *= m_gameSpeed[0];
	m_time += dt;
	m_dt = dt;
	TickState(dt, unmod_dt);

	// tick game materials?
	if ((m_pauseState&(kPauseGame|kPauseCinematics)) != (kPauseGame|kPauseCinematics)) {
		m_draw->Tick(
			unmod_dt
		);
	}
}

void World::Draw() {
	m_draw->Draw(&m_drawCounters);

	if (dt != 0.f) {
		m_drawCounters.fps = 1.f / dt;
	} else {
		m_drawCounters.fps = 0.f;
	}
}

void World::NotifyBackground() {
	m_lua->NotifyBackground();
	DispatchEvents();
}

void World::NotifyResume() {
	m_lua->NotifyResume();
}

void World::SaveApplicationState() {
	m_lua->SaveApplicationState();
}

void World::RestoreApplicationState() {
	m_lua->RestoreApplicationState();
}

bool World::PendingLoadRequest(String &mapName, UnloadDisposition &ud, bool &loadScreen) {
	mapName = m_loadMap;
	ud = m_unloadDisp;
	loadScreen = m_loadScreen;
	bool r = m_loadReq;
	m_loadReq = false;
	return r;
}

bool World::PendingReturnRequest() {
	bool r = m_returnReq;
	m_returnReq = false;
	return r;
}

bool World::PendingUnloadSlot(int &slot) {
	slot = m_unloadSlot;
	bool r = m_unloadSlotReq;
	m_unloadSlotReq = false;
	return r;
}

bool World::PendingSwitch(int &slot) {
	slot = m_switchSlot;
	bool r = m_switchReq;
	m_switchReq = false;
	return r;
}

bool World::PendingSwitchLoadRequest(int &slot, String &mapName, UnloadDisposition &ud, bool &loadScreen) {
	slot = m_switchSlot;
	mapName = m_loadMap;
	ud = m_unloadDisp;
	loadScreen = m_switchLoadScreen;
	bool r = m_switchLoad;
	m_switchLoad = false;
	return r;
}

void World::RequestLoad(const char *map, UnloadDisposition ud, bool loadScreen) {
	RAD_ASSERT(map);
	m_loadMap = map;
	m_unloadDisp = ud;
	m_loadScreen = loadScreen;
	m_loadReq = true;
}

void World::RequestReturn() {
	m_returnReq = true;
}

void World::RequestSwitch(int slot) {
	m_switchSlot = slot;
	m_switchReq = true;
}

void World::RequestUnloadSlot(int slot) {
	m_unloadSlotReq = true;
	m_unloadSlot = slot;
}

void World::RequestSwitchLoad(int slot, const char *map, UnloadDisposition ud, bool loadScreen) {
	RAD_ASSERT(map);
	m_switchSlot = slot;
	m_loadMap = map;
	m_unloadDisp = ud;
	m_switchLoadScreen = loadScreen;
	m_switchLoad = true;
}

void World::TickState(float dt, float unmod_dt) {
	int frame = m_frame++;
	bool gc = false;

	int pauseState = m_pauseState;

	if (!(pauseState&kPauseGame)) {
		m_gameTime += dt;

		m_lua->Tick(dt);

		DispatchEvents();

		for (Entity::IdMap::const_iterator it = m_ents.begin(); it != m_ents.end(); ++it) {
			if (it->second.get() != m_viewController.get())
				it->second->PrivateTick(frame, dt, xtime::TimeSlice::Infinite);
		}

		if (m_viewController)
			m_viewController->PrivateTick(frame, dt, xtime::TimeSlice::Infinite);

		gc = true;
	} else { 
		// game is paused, must tick world!
		m_lua->Tick(0.f);
		if (m_gameCode)
			m_gameCode->PrivateTick(frame, 0.f, xtime::TimeSlice::Infinite);
	}

	if (!(pauseState&kPauseCinematics))
		m_cinematics->Tick(frame, dt);

	// update listener position.
	bool cameraSoundPos = m_cinematics->cameraActive || !m_playerPawn;

	if (cameraSoundPos) {
		m_sound->pos = m_cam.pos;

		if (m_cam.quatMode) {
			m_sound->rot = m_cam.rot;
		} else {
			m_sound->rot = QuatFromAngles(m_cam.angles);
		}
	} else {
		m_sound->pos = m_playerPawn->ps->worldPos;
		m_sound->rot = QuatFromAngles(m_playerPawn->ps->worldAngles);
	}

	m_sound->Tick(dt, m_time > 0.5f);
	
	if (gc) {
		// garbage collect
		for (Entity::IdMap::const_iterator it = m_ents.begin(); it != m_ents.end();) {
			Entity::IdMap::const_iterator next = it; ++next;

			if (it->second->gc) {
				Entity::Ref r(it->second);
				UnlinkEntity(r.get());
				UnmapEntity(r);
			}
			
			it = next;
		}
	}

	if (!(pauseState&kPauseUI)) {
		m_uiRoot->Tick(
			m_time, 
			unmod_dt,
			(pauseState&(kPauseGame|kPauseCinematics))==(kPauseGame|kPauseCinematics)
		);
	}

	GenerateSaveGame();
}

void World::GenerateSaveGame() {
	if (!m_generateSave)
		return;

	m_lua->SaveState();
	m_generateSave = false;
}

Event::Vec World::ParseMultiEvent(const char *string) {
	Event::Vec events;
		
	stream::MemInputBuffer ib(string, string::len(string));
	stream::InputStream is(ib);
	Tokenizer script(is);

	for (;;) {
		String target;
		if (!script.GetToken(target, Tokenizer::kTokenMode_CrossLine)) {
			if (events.empty())
				COut(C_Warn) << "Malformed script command: '" << string << "'" << std::endl;
			return events;
		}

		Event::Target evTarget = Event::T_Name;

		if (target == "@world") {
			evTarget = Event::T_World;
		} else if (target == "@view") {
			evTarget = Event::T_ViewController;
		} else if (target == "@player") {
			evTarget = Event::T_PlayerPawn;
		}
		
		String cmd;
		if (!script.GetToken(cmd, Tokenizer::kTokenMode_SameLine)) {
			COut(C_Warn) << "Malformed script command: '" << string << "'" << std::endl;
			return events;
		}

		String args;
		script.GetRemaining(args, Tokenizer::kFetchMode_RestOfLine);

		Event::Ref event;

		switch (evTarget) {
		case Event::T_Name:
			event.reset(new (ZWorld) Event(target.c_str.get(), cmd.c_str.get(), args.c_str.get()));
			break;
		default:
			event.reset(new (ZWorld) Event(evTarget, cmd.c_str.get(), args.c_str.get()));
			break;
		}

		RAD_ASSERT(event);
		events.push_back(event);
	}
	
	return events;
}

void World::PostEvent(const char *string) {
	Event::Vec events = ParseMultiEvent(string);
	if (!events.empty())
		PostEvents(events);
}

void World::DispatchEvent(const char *string) {
	Event::Vec events = ParseMultiEvent(string);
	if (!events.empty())
		DispatchEvents(events);
}

void World::PostEvent(const Event::Ref &event) {
	m_events.push_back(event);
}

void World::PostEvents(const Event::Vec &events) {
	for (Event::Vec::const_iterator it = events.begin(); it != events.end(); ++it)
		PostEvent(*it);
}

void World::DispatchEvent(const Event::Ref &event) {
	Entity::Vec ents;
	ents.reserve(8);

	switch (event->target.get())
	{
	case Event::T_Id: {
			Entity::IdMap::const_iterator it = m_ents.find(event->targetId);
			if (it != m_ents.end())
				ents.push_back(it->second);
		} break;
	case Event::T_Name: {
			ents = FindEntityTargets(event->name);
		} break;
	case Event::T_ViewController:
			ents.push_back(m_viewController);
			break;
	case Event::T_PlayerPawn:
			ents.push_back(m_playerPawn);
			break;
	case Event::T_World:
			ents.push_back(m_worldspawn);
			break;
	}

	if (ents.empty()) {
		COut(C_Warn) << "Undeliverable event (no targets found): Id: (" << event->targetId.get() <<
			"), Name: (" << event->name.get() << "), Command: (" << event->cmd.get() << "), Args: (" <<
			(event->args.get()?event->args.get():"") << ")" << std::endl;
		return;
	}

	for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it) {
		(*it)->ProcessEvent(*event);
	}
}

void World::DispatchEvents(const Event::Vec &events) {
	for (Event::Vec::const_iterator it = events.begin(); it != events.end(); ++it)
		DispatchEvent(*it);
}

void World::DispatchEvents() {
	EventList events;
	events.swap(m_events);

	while (!events.empty()) {
		DispatchEvent(events.front());
		events.pop_front();
	}
}

void World::FlushEvents() {
	m_events.clear();
}

bool World::HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is) {

	if (m_lua->InputEventFilter(e, touch, is))
		return true;

	bool handled = false;

	if (m_uiRoot->HandleInputEvent(e, touch, is)) {
		handled = true;
		// always post cancellations to script.
		if ((e.type != InputEvent::T_MouseUp) && 
			(e.type != InputEvent::T_TouchEnd) && 
			(e.type != InputEvent::T_TouchCancelled))
			return true;
	}

	return m_lua->HandleInputEvent(e, touch, is) || handled;
}

bool World::HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is) {
	if (m_lua->InputGestureFilter(g, touch, is))
		return true;
	if (m_uiRoot->HandleInputGesture(g, touch, is))
		return true;
	return m_lua->HandleInputGesture(g, touch, is);
}

void World::OnLocalPlayerAuthenticated(gn::NetResult r) {
	m_lua->OnLocalPlayerAuthenticated(r);
}

void World::OnShowLeaderboard(bool show) {
	m_lua->OnShowLeaderboard(show);
}

void World::OnShowAchievements(bool show) {
	m_lua->OnShowAchievements(show);
}

Entity::Vec World::FindEntityClass(const char *classname) const {
	RAD_ASSERT(classname);
	String s(CStr(classname));

	std::pair<Entity::StringMMap::const_iterator, 
	          Entity::StringMMap::const_iterator> pair = m_classnames.equal_range(s);

	Entity::Vec vec;

	while (pair.first != pair.second) {
		vec.push_back(pair.first->second);
		++pair.first;
	}

	return vec;
}

Entity::Vec World::FindEntityTargets(const char *targetname) const {
	RAD_ASSERT(targetname);
	String s(CStr(targetname));
	
	std::pair<Entity::StringMMap::const_iterator, 
	          Entity::StringMMap::const_iterator> pair = m_targetnames.equal_range(s);

	Entity::Vec vec;

	while (pair.first != pair.second) {
		vec.push_back(pair.first->second);
		++pair.first;
	}

	return vec;
}

Entity::Ref World::FindEntityId(int id) const {
	Entity::IdMap::const_iterator it = m_ents.find(id);
	if (it != m_ents.end())
		return it->second;
	return Entity::Ref();
}

Entity::Ref World::FindEntityUID(int uid) const {
	Entity::IdMap::const_iterator it = m_uids.find(uid);
	if (it != m_uids.end())
		return it->second;
	return Entity::Ref();
}

ZoneTagRef World::ZoneTag(int id) const {
	ZoneIdMap::const_iterator it = m_zoneTags.find(id);
	if (it != m_zoneTags.end())
		return it->second;
	return ZoneTagRef();
}

const Vec3 &World::RAD_IMPLEMENT_GET(listenerPos) {
	bool cameraSoundPos = m_cinematics->cameraActive || !m_playerPawn;
	if (cameraSoundPos)
		return m_cam.pos;
	return m_playerPawn->ps->worldPos;
}

void World::RAD_IMPLEMENT_SET(enabledGestures) (int value) {
	m_gestures = value;
	World::Ref active = m_game->world;
	if (active && active.get() == this)
		m_game->FlushInput(true); // flush input if gesture types change
}

GameCVars* World::RAD_IMPLEMENT_GET(cvars) {
	return m_game->cvars;
}

} // world
