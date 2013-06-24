// WorldLua.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../App.h"
#include "../Engine.h"
#include "World.h"
#include "WorldDraw.h"
#include "ScreenOverlay.h"
#include "PostProcess.h"
#include "Lua/D_ScreenOverlay.h"
#include "../Game/Game.h"
#include "../Renderer/Renderer.h"
#include "../UI/UIWidget.h"
#include "../UI/UIMatWidget.h"
#include "../UI/UITextLabel.h"
#include "../UI/UIVListWidget.h"
#include "../Assets/MaterialParser.h"	
#include "../Renderer/Material.h"
#include "Lua/T_Precache.h"
#include "Lua/T_Spawn.h"
#include "Lua/T_TempSpawn.h"
#include "Lua/D_Asset.h"
#include "Lua/D_SkModel.h"
#include "Lua/D_Sound.h"
#include "Lua/D_Mesh.h"
#include "Lua/D_SpriteBatch.h"
#include "Lua/D_Light.h"
#include "../Sound/Sound.h"
#include "WorldLuaCommon.h"

namespace world {

bool WorldLua::InputEventFilter(const InputEvent &e, const TouchState *touch, const InputState &is) {
	if (!PushGlobalCall("World.InputEventFilter"))
		return false;
	lua_State *L = m_L->L;

	lua::Marshal<InputEvent>::Push(L, e, touch);	

	bool r = false;
	if (Call("WorldLua::InputEventFilter", 1, 1, 0)) {
		r = lua_toboolean(L, -1) ? true : false;
		lua_pop(L, 1);
	}

	return r;
}

bool WorldLua::InputGestureFilter(const InputGesture &g, const TouchState &touch, const InputState &is) {
	if (!PushGlobalCall("World.InputGestureFilter"))
		return false;
	lua_State *L = m_L->L;

	lua::Marshal<InputGesture>::Push(L, g, touch);

	bool r = false;
	if (Call("World::InputGestureFilter", 1, 1, 0)) {
		r = lua_toboolean(L, -1) ? true : false;
		lua_pop(L, 1);
	}

	return r;
}

bool WorldLua::HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is) {
	if (!PushGlobalCall("World.OnInputEvent"))
		return false;
	lua_State *L = m_L->L;

	lua::Marshal<InputEvent>::Push(L, e, touch);	

	bool r = false;
	if (Call("WorldLua::HandleInputEvent", 1, 1, 0)) {
		r = lua_toboolean(L, -1) ? true : false;
		lua_pop(L, 1);
	}

	return r;
}

bool WorldLua::HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is) {
	if (!PushGlobalCall("World.OnInputGesture"))
		return false;
	lua_State *L = m_L->L;

	lua::Marshal<InputGesture>::Push(L, g, touch);

	bool r = false;
	if (Call("World::HandleInputGesture", 1, 1, 0)) {
		r = lua_toboolean(L, -1) ? true : false;
		lua_pop(L, 1);
	}

	return r;
}

StringVec WorldLua::GetBuiltIns() {
	if (!PushGlobalCall("World.BuiltIns"))
		return StringVec();
	lua_State *L = m_L->L;

	StringVec v;

	if (Call("World.BuiltIns", 0, 1, 0)) {
		if (lua_type(L, -1) == LUA_TTABLE) {
			lua_checkstack(L, 3);
			lua_pushnil(L); // iterate table from start
			while (lua_next(L, -2) != 0) {
				if (lua_type(L, -1) == LUA_TSTRING) { // value is not string!
					v.push_back(String(lua_tolstring(L, -1, 0)));
				} else {
					COut(C_Error) << "WorldLua::GetBuiltIns: value in table is not a string!" << std::endl;
				}
				lua_pop(L, 1); // remove value, key tells lua_next where to start.
			}
		}
		lua_pop(L, 1);
	}

	return v;
}

void WorldLua::NotifyBackground() {
	if (!PushGlobalCall("World.NotifyBackground"))
		return;
	Call("World::NotifyBackground", 0, 0, 0);
}

void WorldLua::NotifyResume() {
	if (!PushGlobalCall("World.NotifyResume"))
		return;
	Call("World::NotifyResume", 0, 0, 0);
}

void WorldLua::SaveApplicationState() {
	if (!PushGlobalCall("World.SaveApplicationState"))
		return;
	Call("World::SaveApplicationState", 0, 0, 0);
}

void WorldLua::RestoreApplicationState() {
	if (!PushGlobalCall("World.RestoreApplicationState"))
		return;
	Call("World::RestoreApplicationState", 0, 0, 0);
}

int WorldLua::lua_World_RequestGenerateSaveGame(lua_State *L) {
	LOAD_SELF
	self->m_world->generateSaveGame = true;
	return 0;
}

int WorldLua::lua_World_FindEntityId(lua_State *L) {
	LOAD_SELF

	int id = (int)luaL_checkinteger(L, 1);
	Entity::Ref ref = self->m_world->FindEntityId(id);

	if (ref) {
		ref->PushEntityFrame(L);
		return 1;
	}

	return 0;
}

int WorldLua::lua_World_FindEntityUID(lua_State *L) {
	LOAD_SELF

	int uid = (int)luaL_checkinteger(L, 1);
	Entity::Ref ref = self->m_world->FindEntityUID(uid);

	if (ref) {
		ref->PushEntityFrame(L);
		return 1;
	}

	return 0;
}

int WorldLua::lua_World_FindEntityClass(lua_State *L) {
	LOAD_SELF

	const char *classname = luaL_checkstring(L, 1);
	Entity::Vec ents = self->m_world->FindEntityClass(classname);
	
	if (ents.empty()) {
		lua_pushnil(L);
	} else {
		int c = 1;
		lua_createtable(L, (int)ents.size(), 0);
		for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it, ++c) {
			lua_pushinteger(L, c);
			(*it)->PushEntityFrame(L);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int WorldLua::lua_World_FindEntityTargets(lua_State *L) {
	LOAD_SELF

	const char *targetname = luaL_checkstring(L, 1);
	Entity::Vec ents = self->m_world->FindEntityTargets(targetname);
	
	if (ents.empty()) {
		lua_pushnil(L);
	} else {
		int c = 1;
		lua_createtable(L, (int)ents.size(), 0);
		for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it, ++c) {
			lua_pushinteger(L, c);
			(*it)->PushEntityFrame(L);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int WorldLua::lua_World_BBoxTouching(lua_State *L) {
	LOAD_SELF

	BBox bbox(
		lua::Marshal<Vec3>::Get(L, 1, true),
		lua::Marshal<Vec3>::Get(L, 2, true)
	);

	int classtypes = (int)luaL_checknumber(L, 3);
	Entity::Vec ents = self->m_world->BBoxTouching(bbox, classtypes);

	if (ents.empty()) {
		lua_pushnil(L);
	} else {
		int c = 1;
		lua_createtable(L, (int)ents.size(), 0);
		for (Entity::Vec::const_iterator it = ents.begin(); it != ents.end(); ++it, ++c) {
			lua_pushinteger(L, c);
			(*it)->PushEntityFrame(L);
			lua_settable(L, -3);
		}
	}

	return 1;
}

int WorldLua::lua_World_LineTrace(lua_State *L) {
	LOAD_SELF

	Trace t = lua::Marshal<Trace>::Get(L, 1, true);
	if (self->m_world->LineTrace(t)) {
		lua::Marshal<Trace>::Push(L, t);
		return 1;
	}

	return 0;
}

int WorldLua::lua_World_ClipToFloor(lua_State *L) {

	LOAD_SELF

	if (lua_type(L, 1) == LUA_TNUMBER) {
		Vec3 start = lua::Marshal<Vec3>::Get(L, 2, true);
		Vec3 end = lua::Marshal<Vec3>::Get(L, 3, true);
		
		FloorPosition p;
		bool r = self->m_world->floors->ClipToFloor((int)luaL_checkinteger(L, 1), start, end, p);
		if (r)
			lua::Marshal<FloorPosition>::Push(L, p);
		return r ? 1 : 0;
	}

	Vec3 start = lua::Marshal<Vec3>::Get(L, 1, true);
	Vec3 end = lua::Marshal<Vec3>::Get(L, 2, true);

	FloorPosition p;
	bool r = self->m_world->floors->ClipToFloor(start, end, p);
	if (r)
		lua::Marshal<FloorPosition>::Push(L, p);
	return r ? 1 : 0;
}

int WorldLua::lua_World_CreateFloorMove(lua_State *L) {

	LOAD_SELF

	FloorPosition start = lua::Marshal<FloorPosition>::Get(L, 1, true);
	FloorPosition end   = lua::Marshal<FloorPosition>::Get(L, 2, true);

	FloorMove::Ref move = self->m_world->floors->CreateMove(start, end);
	if (move)
		move->Push(L);
	return move ? 1 : 0;
}

int WorldLua::lua_World_CreateFloorMoveSeq(lua_State *L) {

	LOAD_SELF

	luaL_checktype(L, 1, LUA_TTABLE);

	FloorPosition::Vec vec;
	vec.reserve(4);

	for (int i = 0; ; ++i) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, 1);
		if (lua_isnil(L, -1)) {
			lua_pop(L, 1);
			break;
		}
		vec.push_back(lua::Marshal<FloorPosition>::Get(L, -1, true));
		lua_pop(L, 1);
	}

	if (vec.size() < 2)
		return 0;

	FloorMove::Ref move;

	if (vec.size() == 2) {
		move = self->m_world->floors->CreateMove(vec[0], vec[1]);
	} else {
		move = self->m_world->floors->CreateMoveSeq(vec);
	}

	if (move)
		move->Push(L);

	return move ? 1 : 0;
}

int WorldLua::lua_World_CreateFloorPosition(lua_State *L) {
	LOAD_SELF

	Vec3 pos = lua::Marshal<Vec3>::Get(L, 1, true);
	int floorNum = (int)luaL_checkinteger(L, 2);
	int triNum = (int)luaL_checkinteger(L, 3);

	FloorPosition fp(pos, floorNum, triNum);
	lua::Marshal<FloorPosition>::Push(L, fp);

	return 1;
}

int WorldLua::lua_World_CreateScreenOverlay(lua_State *L) {
	LOAD_SELF

	D_Asset::Ref dAsset = lua::SharedPtr::Get<D_Asset>(L, "Asset", 1, true);
	luaL_argcheck(L, (dAsset->asset->type==asset::AT_Material), 1, "Expected Material!");

	ScreenOverlay::Ref overlay = self->m_world->draw->CreateScreenOverlay(dAsset->asset->id);
	if (!overlay)
		return 0;

	D_ScreenOverlay::Ref dOverlay = D_ScreenOverlay::New(overlay);
	dOverlay->Push(L);
	return 1;
}

int WorldLua::lua_World_PostEvent(lua_State *L) {
	LOAD_SELF
	const char *event = luaL_checkstring(L, 1);
	self->m_world->PostEvent(event);
	return 0;
}

int WorldLua::lua_World_DispatchEvent(lua_State *L) {
	LOAD_SELF
	const char *event = luaL_checkstring(L, 1);
	self->m_world->DispatchEvent(event);
	return 0;
}

int WorldLua::lua_World_FlushEvents(lua_State *L) {
	LOAD_SELF
	self->m_world->FlushEvents();
	return 0;
}

int WorldLua::lua_World_GetEvents(lua_State *L) {
	LOAD_SELF

	const World::EventList &events = self->m_world->m_events;

	if (events.empty())
		return 0;

	lua_createtable(L, (int)events.size(), 0);

	String x;
	int ofs = 0;
	for (World::EventList::const_iterator it = events.begin(); it != events.end(); ++it) {
		const Event::Ref &event = *it;
		switch (event->target) {
		case Event::T_Id:
			continue;
			break;
		case Event::T_Name:
			x = "\"";
			x += event->name.get();
			x += "\" ";
			break;
		case Event::T_ViewController:
			x = "@view ";
			break;
		case Event::T_PlayerPawn:
			x = "@player ";
			break;
		case Event::T_World:
			x = "@world ";
			break;
		}

		x += "\"";
		x += event->cmd.get();
		x += "\"";

		if (event->args.get()) {
			x += " ";
			x += event->args.get();
		}

		lua_pushinteger(L, ++ofs);
		lua_pushstring(L, x.c_str.get());
		lua_settable(L, -3);
	}

	return 1;
}

int WorldLua::lua_World_Project(lua_State *L) {
	LOAD_SELF

	Vec3 out;
	bool r = self->m_world->draw->Project(
		lua::Marshal<Vec3>::Get(L, 1, true),
		out
	);
	lua::Marshal<Vec3>::Push(L, out);
	lua_pushboolean(L, r ? 1 : 0);
	return 2;
}

int WorldLua::lua_World_Unproject(lua_State *L) {
	LOAD_SELF

	Vec3 p = self->m_world->draw->Unproject(lua::Marshal<Vec3>::Get(L, 1, true));
	lua::Marshal<Vec3>::Push(L, p);
	return 1;
}

int WorldLua::lua_World_SetUIViewport(lua_State *L) {
	LOAD_SELF

	self->m_world->uiRoot->SetSourceViewport(
		(int)luaL_checkinteger(L, 1),
		(int)luaL_checkinteger(L, 2),
		(int)luaL_checkinteger(L, 3),
		(int)luaL_checkinteger(L, 4)
	);

	return 0;
}

int WorldLua::lua_World_SetRootWidget(lua_State *L) {
	LOAD_SELF

	int layer = (int)luaL_checkinteger(L, 1);

	if (lua_isnil(L, 2)) {
		self->m_world->uiRoot->SetRootWidget(layer, ui::Widget::Ref());
	} else {
		self->m_world->uiRoot->SetRootWidget(
			layer, 
			ui::Widget::GetRef<ui::Widget>(L, "Widget", 2, true)
		);
	}
	
	return 0;
}

int WorldLua::lua_World_CreateWidget(lua_State *L) {
	LOAD_SELF

	String type(luaL_checkstring(L, 1));
	luaL_checktype(L, 2, LUA_TTABLE);

	ui::Widget::Ref w;

	if (type == "Widget") {
		w.reset(new (ui::ZUI) ui::Widget());
	} else if(type == "MatWidget") {
		w.reset(new (ui::ZUI) ui::MatWidget());
	} else if(type == "TextLabel") {
		w.reset(new (ui::ZUI) ui::TextLabel());
	} else if (type == "VListWidget") {
		w.reset(new (ui::ZUI) ui::VListWidget());
	} else {
		luaL_argerror(L, 1, "Invalid widget type!");
	}

	lua_pushvalue(L, 2);
	w->Spawn(self->L, L);
	lua_pop(L, 1);
	w->PushFrame(L);
	return 1;
}

int WorldLua::lua_World_AddWidgetTickMaterial(lua_State *L) {
	LOAD_SELF

	D_Asset::Ref dAsset = lua::SharedPtr::Get<D_Asset>(L, "D_Asset", 1, true);
	luaL_argcheck(L, (dAsset->asset->type==asset::AT_Material), 1, "Expected Material!");
	self->m_world->uiRoot->AddTickMaterial(dAsset->asset);
	self->m_world->draw->AddMaterial(dAsset->asset->id);
	return 0;
}

int WorldLua::lua_World_CreatePostProcessEffect(lua_State *L) {
	LOAD_SELF

	int id = (int)luaL_checkinteger(L, 1);
	D_Asset::Ref dAsset = lua::SharedPtr::Get<D_Asset>(L, "D_Asset", 2, true);
	luaL_argcheck(L, (dAsset->asset->type==asset::AT_Material), 2, "Expected Material!");

	PostProcessEffect::Ref effect(new (r::ZRender) PostProcessEffect());
	if (effect->BindMaterial(dAsset->asset))
		self->m_world->draw->AddEffect(id, effect);
	return 0;
}

int WorldLua::lua_World_FadePostProcessEffect(lua_State *L) {
	LOAD_SELF

	int id = (int)luaL_checkinteger(L, 1);
	Vec4 color = lua::Marshal<Vec4>::Get(L, 2, true);
	float time = (float)luaL_checknumber(L, 3);

	const PostProcessEffect::Ref &r = self->m_world->draw->PostFX(id);
	if (r)
		r->FadeTo(color, time);
	return 0;
}

int WorldLua::lua_World_EnablePostProcessEffect(lua_State *L) {
	LOAD_SELF

	int id = (int)luaL_checkinteger(L, 1);
	bool enabled = lua_toboolean(L, 2) ? true : false;

	const PostProcessEffect::Ref &r = self->m_world->draw->PostFX(id);
	if (r)
		r->enabled = enabled;
	return 0;
}

WorldLua::CinematicsNotify::CinematicsNotify(World *world, Entity &entity, int callbackId) :
m_world(world),
m_entity(entity.shared_from_this()),
m_callbackId(callbackId),
m_masked(false) {
}

WorldLua::CinematicsNotify::~CinematicsNotify() {
	if (m_world->destroy) // don't clean ents up (done by world).
		return;

	Entity::Ref entity = m_entity.lock();
	if (entity)
		entity->ReleaseLuaCallback(m_callbackId, lua::InvalidIndex);
}

void WorldLua::CinematicsNotify::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_SetMasked);
	lua_setfield(L, -2, "SetMasked");
}

void WorldLua::CinematicsNotify::OnTag(const char *str) {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnTag");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	lua_pushstring(L, str);
	entity->world->lua->Call(L, "WorldLua::CinematicsNotify::OnTag", 2, 0, 0);
	lua_pop(L, 1); // pop callback table
}

void WorldLua::CinematicsNotify::OnComplete() {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnComplete");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	entity->world->lua->Call(L, "WorldLua::CinematicsNotify::OnComplete", 1, 0, 0);
	lua_pop(L, 1); // pop callback table
}

void WorldLua::CinematicsNotify::OnSkip() {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnSkip");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	entity->world->lua->Call(L, "WorldLua::CinematicsNotify::OnSkip", 1, 0, 0);
	lua_pop(L, 1); // pop callback table
}

int WorldLua::CinematicsNotify::lua_SetMasked(lua_State *L) {
	Ref self = lua::SharedPtr::Get<CinematicsNotify>(L, "WorldLua::CinematicsNotify", 1, true);
	bool wasMasked = self->m_masked;
	self->m_masked = lua_toboolean(L, 2) ? true : false;
	lua_pushboolean(L, wasMasked ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_PlayCinematic(lua_State *L) {
	LOAD_SELF

	WorldCinematics::Notify::Ref notify;

	Entity *entity = EntFramePtr(L, 5, false);
	if (entity && lua_gettop(L) > 5) { // passed in callbacks?
		int callbackId = entity->StoreLuaCallback(L, 6, 5);
		RAD_ASSERT(callbackId != -1);
		notify.reset(new CinematicsNotify(self->m_world, *entity, callbackId));
	}

	Entity *cameraOrigin = EntFramePtr(L, 4, false);

	bool r = self->m_world->cinematics->PlayCinematic(
		luaL_checkstring(L, 1),
		(int)luaL_checkinteger(L, 2),
		(float)luaL_checknumber(L, 3),
		cameraOrigin ? cameraOrigin->shared_from_this() : Entity::Ref(),
		notify
	);

	lua_pushboolean(L, r ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_StopCinematic(lua_State *L) {
	LOAD_SELF

	self->m_world->cinematics->StopCinematic(
		luaL_checkstring(L, 1),
		lua_toboolean(L, 2) ? true : false
	);

	return 0;
}

int WorldLua::lua_World_SkipCinematics(lua_State *L) {
	LOAD_SELF

	self->m_world->cinematics->Skip();
	return 0;
}

int WorldLua::lua_World_CinematicTime(lua_State *L) {
	LOAD_SELF

	float r = self->m_world->cinematics->CinematicTime(luaL_checkstring(L, 1));
	lua_pushnumber(L, r);
	return 1;
}

int WorldLua::lua_World_SetCinematicTime(lua_State *L) {
	LOAD_SELF

	bool r = self->m_world->cinematics->SetCinematicTime(
		luaL_checkstring(L, 1),
		(float)luaL_checknumber(L, 2)
	);

	lua_pushboolean(L, r ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_SoundFadeMasterVolume(lua_State *L) {
	LOAD_SELF

	self->m_world->sound->FadeMasterVolume(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2)
	);
	return 0;
}

int WorldLua::lua_World_SoundFadeChannelVolume(lua_State *L) {
	LOAD_SELF

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	self->m_world->sound->FadeChannelVolume(
		(SoundChannel)c,
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int WorldLua::lua_World_SoundChannelVolume(lua_State *L) {
	LOAD_SELF

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	lua_pushnumber(L, (lua_Number)self->m_world->sound->ChannelVolume((SoundChannel)c));
	return 1;
}

int WorldLua::lua_World_SoundPauseChannel(lua_State *L) {
	LOAD_SELF

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	self->m_world->sound->PauseChannel((SoundChannel)c, lua_toboolean(L, 2) ? true : false);
	return 0;
}

int WorldLua::lua_World_SoundChannelIsPaused(lua_State *L) {
	LOAD_SELF

	int c = (int)luaL_checkinteger(L, 1);
	if (c < SC_First || c >= SC_Max)
		luaL_error(L, "Invalid SoundChannel %d", c);
	lua_pushboolean(L, self->m_world->sound->ChannelIsPaused((SoundChannel)c) ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_SoundPauseAll(lua_State *L) {
	LOAD_SELF

	self->m_world->sound->PauseAll(lua_toboolean(L, 1) ? true : false);
	return 0;
}

int WorldLua::lua_World_SoundStopAll(lua_State *L) {
	LOAD_SELF

	self->m_world->sound->StopAll();
	return 0;
}

int WorldLua::lua_World_SoundSetDoppler(lua_State *L) {
	LOAD_SELF

	self->m_world->sound->SetDoppler(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2)
	);
	return 0;
}

int WorldLua::lua_World_RequestLoad(lua_State *L) {
	LOAD_SELF
	self->m_world->RequestLoad(
		luaL_checkstring(L, 1),
		(UnloadDisposition)luaL_checkinteger(L, 2),
		(lua_isnone(L, 3) || lua_toboolean(L, 3)) ? true : false
	);
	return 0;
}

int WorldLua::lua_World_RequestReturn(lua_State *L) {
	LOAD_SELF
	self->m_world->RequestReturn();
	return 0;
}

int WorldLua::lua_World_RequestSwitch(lua_State *L) {
	LOAD_SELF
	self->m_world->RequestSwitch((int)luaL_checkinteger(L, 1));
	return 0;
}

int WorldLua::lua_World_RequestUnloadSlot(lua_State *L) {
	LOAD_SELF
	self->m_world->RequestUnloadSlot((int)luaL_checkinteger(L, 1));
	return 0;
}

int WorldLua::lua_World_RequestSwitchLoad(lua_State *L) {
	LOAD_SELF
	self->m_world->RequestSwitchLoad(
		(int)luaL_checkinteger(L, 1),
		luaL_checkstring(L, 2),
		(UnloadDisposition)luaL_checkinteger(L, 3),
		(lua_isnone(L, 4) || lua_toboolean(L, 4)) ? true : false
	);
	return 0;
}

int WorldLua::lua_World_SetGameSpeed(lua_State *L) {
	LOAD_SELF
	self->m_world->SetGameSpeed(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2)
	);
	return 0;
}

int WorldLua::lua_World_SetPauseState(lua_State *L) {
	LOAD_SELF
	self->m_world->pauseState = (int)luaL_checkinteger(L, 1);
	return 0;
}

int WorldLua::lua_World_PlayerPawn(lua_State *L) {
	LOAD_SELF
	Entity::Ref r = self->m_world->playerPawn;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetPlayerPawn(lua_State *L) {
	LOAD_SELF

	if (lua_isnil(L, 1)) {
		self->m_world->playerPawn = Entity::Ref();
	} else {
		self->m_world->playerPawn = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_Worldspawn(lua_State *L) {
	LOAD_SELF
	Entity::Ref r = self->m_world->worldspawn;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetWorldspawn(lua_State *L) {
	LOAD_SELF

	if (lua_isnil(L, 1)) {
		self->m_world->worldspawn = Entity::Ref();
	} else {
		self->m_world->worldspawn = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_ViewController(lua_State *L) {
	LOAD_SELF
	Entity::Ref r = self->m_world->viewController;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetViewController(lua_State *L) {
	LOAD_SELF

	if (lua_isnil(L, 1)) {
		self->m_world->viewController = Entity::Ref();
	} else {
		self->m_world->viewController = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_GameCode(lua_State *L) {
	LOAD_SELF
	Entity::Ref r = self->m_world->gameCode;
	if (r) {
		r->PushEntityFrame(L);
	} else {
		lua_pushnil(L);
	}

	return 1;
}

int WorldLua::lua_World_SetGameCode(lua_State *L) {
	LOAD_SELF

	if (lua_isnil(L, 1)) {
		self->m_world->gameCode = Entity::Ref();
	} else {
		self->m_world->gameCode = EntFramePtr(L, 1, true)->shared_from_this();
	}

	return 0;
}

int WorldLua::lua_World_GameTime(lua_State *L) {
	LOAD_SELF
	lua_pushnumber(L, self->m_world->gameTime.get());
	return 1;
}

int WorldLua::lua_World_SysTime(lua_State *L) {
	LOAD_SELF
	lua_pushnumber(L, self->m_world->time.get());
	return 1;
}

int WorldLua::lua_World_DeltaTime(lua_State *L) {
	LOAD_SELF
	lua_pushnumber(L, self->m_world->dt.get());
	return 1;
}

int WorldLua::lua_World_Viewport(lua_State *L) {
	LOAD_SELF

	int vpx, vpy, vpw, vph;
	self->m_world->game->Viewport(vpx, vpy, vpw, vph);
	lua::Marshal<Vec2>::Push(L, Vec2((float)vpw, (float)vph));
	return 1;
}

int WorldLua::lua_World_CameraPos(lua_State *L) {
	LOAD_SELF
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->pos);
	return 1;
}

int WorldLua::lua_World_CameraFarClip(lua_State *L) {
	LOAD_SELF
	lua_pushnumber(L, (lua_Number)self->m_world->camera->farClip.get());
	return 1;
}

int WorldLua::lua_World_SetCameraFarClip(lua_State *L) {
	LOAD_SELF
	self->m_world->camera->farClip = (float)luaL_checknumber(L, 1);
	return 0;
}

int WorldLua::lua_World_CameraAngles(lua_State *L) {
	LOAD_SELF
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->angles);
	return 1;
}

int WorldLua::lua_World_CameraFOV(lua_State *L) {
	LOAD_SELF
	lua_pushnumber(L, (lua_Number)self->m_world->camera->fov.get());
	return 1;
}

int WorldLua::lua_World_CameraFwd(lua_State *L) {
	LOAD_SELF
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->fwd);
	return 1;
}

int WorldLua::lua_World_CameraLeft(lua_State *L) {
	LOAD_SELF
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->left);
	return 1;
}

int WorldLua::lua_World_CameraUp(lua_State *L) {
	LOAD_SELF
	lua::Marshal<Vec3>::Push(L, self->m_world->camera->up);
	return 1;
}

int WorldLua::lua_World_SetCamera(lua_State *L) {
	LOAD_SELF
	self->m_world->camera->pos = lua::Marshal<Vec3>::Get(L, 1, true);
	self->m_world->camera->angles = lua::Marshal<Vec3>::Get(L, 2, true);
	self->m_world->camera->fov = (float)luaL_checknumber(L, 3);
	self->m_world->camera->quatMode = false;
	return 0;
}
	
int WorldLua::lua_World_SetEnabledGestures(lua_State *L) {
	LOAD_SELF
	self->m_world->enabledGestures = (int)luaL_checkinteger(L, 1);
	return 0;
}

int WorldLua::lua_World_FlushInput(lua_State *L) {
	LOAD_SELF
	self->m_world->game->FlushInput(lua_toboolean(L, 1) ? true : false);
	return 0;
}

int WorldLua::lua_World_FindFloor(lua_State *L) {
	LOAD_SELF
	lua_pushinteger(L, self->m_world->floors->FindFloor(luaL_checkstring(L, 1)));
	return 1;
}

int WorldLua::lua_World_FloorState(lua_State *L) {
	LOAD_SELF
	lua_pushinteger(L, self->m_world->floors->FloorState(luaL_checkinteger(L, 1)));
	return 1;
}

int WorldLua::lua_World_FloorName(lua_State *L) {
	LOAD_SELF
	const char *sz = self->m_world->floors->FloorName(luaL_checkinteger(L, 1));
	if (sz) {
		lua_pushstring(L, sz);
		return 1;
	}

	return 0;
}

int WorldLua::lua_World_SetFloorState(lua_State *L) {
	LOAD_SELF
	self->m_world->floors->SetFloorState(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
	return 0;
}

int WorldLua::lua_World_WaypointPosition(lua_State *L) {
	LOAD_SELF
	FloorPosition pos;
	if (self->m_world->floors->WaypointPosition(luaL_checkinteger(L, 1), pos)) {
		lua::Marshal<Vec3>::Push(L, pos.pos);
		return 1;
	}

	return 0;
}

int WorldLua::lua_World_WaypointFloorPosition(lua_State *L) {
	LOAD_SELF
	FloorPosition pos;
	if (self->m_world->floors->WaypointPosition(luaL_checkinteger(L, 1), pos)) {
		lua::Marshal<FloorPosition>::Push(L, pos);
		return 1;
	}

	return 0;
}

int WorldLua::lua_World_WaypointState(lua_State *L) {
	LOAD_SELF
	lua_pushinteger(L, self->m_world->floors->WaypointState(luaL_checkinteger(L, 1)));
	return 1;
}

int WorldLua::lua_World_SetWaypointState(lua_State *L) {
	LOAD_SELF
	self->m_world->floors->SetWaypointState(luaL_checkinteger(L, 1), luaL_checkinteger(L, 2));
	return 0;
}

int WorldLua::lua_World_WaypointsForTargetname(lua_State *L) {
	LOAD_SELF
	IntVec vec = self->m_world->floors->WaypointsForTargetname(luaL_checkstring(L, 1));
	if (!vec.empty()) {
		lua_createtable(L, (int)vec.size(), 0);
		int ofs = 1; // lua is 1 based
		for (IntVec::const_iterator it = vec.begin(); it != vec.end(); ++it, ++ofs) {
			lua_pushinteger(L, ofs);
			lua_pushinteger(L, *it);
			lua_settable(L, -3);
		}

		return 1;
	}

	return 0;
}

int WorldLua::lua_World_WaypointsForUserId(lua_State *L) {
	LOAD_SELF
	IntVec vec = self->m_world->floors->WaypointsForUserId(luaL_checkstring(L, 1));
	if (!vec.empty()) {
		lua_createtable(L, (int)vec.size(), 0);
		int ofs = 1; // lua is 1 based
		for (IntVec::const_iterator it = vec.begin(); it != vec.end(); ++it, ++ofs) {
			lua_pushinteger(L, ofs);
			lua_pushinteger(L, *it);
			lua_settable(L, -3);
		}

		return 1;
	}

	return 0;

}

int WorldLua::lua_World_NumFloors(lua_State *L) {
	LOAD_SELF

	lua_pushinteger(L, self->m_world->floors->numFloors);
	return 1;
}

int WorldLua::lua_World_NumWaypoints(lua_State *L) {
	LOAD_SELF

	lua_pushinteger(L, self->m_world->floors->numWaypoints);
	return 1;
}

int WorldLua::lua_World_PickWaypoint(lua_State *L) {
	LOAD_SELF
	int idx = self->m_world->floors->PickWaypoint(
		(float)luaL_checknumber(L, 1),
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3),
		(float)luaL_checknumber(L, 4)
	);
	
	if (idx > -1) {
		lua_pushinteger(L, idx);
		return 1;
	}
	
	return 0;
}

int WorldLua::lua_World_DrawCounters(lua_State *L) {
	LOAD_SELF
	const WorldDraw::Counters *counters = self->m_world->drawCounters;

	lua_createtable(L, 0, 12);
	lua_pushnumber(L, counters->fps);
	lua_setfield(L, -2, "fps");
	lua_pushinteger(L, counters->area);
	lua_setfield(L, -2, "area");
	lua_pushinteger(L, counters->drawnAreas);
	lua_setfield(L, -2, "drawnAreas");
	lua_pushinteger(L, counters->testedPortals);
	lua_setfield(L, -2, "testedPortals");
	lua_pushinteger(L, counters->drawnPortals);
	lua_setfield(L, -2, "drawnPortals");
	lua_pushinteger(L, counters->testedWorldModels);
	lua_setfield(L, -2, "testedWorldModels");
	lua_pushinteger(L, counters->drawnWorldModels);
	lua_setfield(L, -2, "drawnWorldModels");
	lua_pushinteger(L, counters->drawnEntities);
	lua_setfield(L, -2, "drawnEntities");
	lua_pushinteger(L, counters->testedEntityModels);
	lua_setfield(L, -2, "testedEntityModels");
	lua_pushinteger(L, counters->drawnEntityModels);
	lua_setfield(L, -2, "drawnEntityModels");
	lua_pushinteger(L, counters->drawnActors);
	lua_setfield(L, -2, "drawnActors");
	lua_pushinteger(L, counters->testedActorModels);
	lua_setfield(L, -2, "testedActorModels");
	lua_pushinteger(L, counters->drawnActorModels);
	lua_setfield(L, -2, "drawnActorModels");
	lua_pushinteger(L, counters->testedLights);
	lua_setfield(L, -2, "testedLights");
	lua_pushinteger(L, counters->visLights);
	lua_setfield(L, -2, "visLights");
	lua_pushinteger(L, counters->drawnLights);
	lua_setfield(L, -2, "drawnLights");
	lua_pushinteger(L, counters->numBatches);
	lua_setfield(L, -2, "numBatches");
	lua_pushinteger(L, counters->numTris);
	lua_setfield(L, -2, "numTris");
	lua_pushinteger(L, counters->numMaterials);
	lua_setfield(L, -2, "numMaterials");

	return 1;
}

int WorldLua::lua_World_QuitGame(lua_State *L) {
	LOAD_SELF
	self->m_world->game->quit = true;
	return 0;
}

int WorldLua::lua_World_SetDrawUIOnly(lua_State *L) {
	LOAD_SELF
	self->m_world->draw->uiOnly = lua_toboolean(L, 1) ? true : false;
	return 0;
}

int WorldLua::lua_World_DrawUIOnly(lua_State *L) {
	LOAD_SELF
	lua_pushboolean(L, self->m_world->draw->uiOnly ? 1 : 0);
	return 1;
}

int WorldLua::lua_World_CreateSpriteBatch(lua_State *L) {
	r::SpriteBatch::Ref sprites(new (r::ZRender) r::SpriteBatch(
		(int)luaL_checkinteger(L, 1),
		(int)luaL_checkinteger(L, 2)
	));
	D_SpriteBatch::Ref dsprites = D_SpriteBatch::New(sprites);
	dsprites->Push(L);
	return 1;
}

int WorldLua::lua_World_CreateDynamicLight(lua_State *L) {
	LOAD_SELF
	D_Light::Ref light = D_Light::New(self->m_world->draw->CreateLight());
	light->Push(L);
	return 1;
}

} // world


namespace lua {

void Marshal<world::Trace>::Push(lua_State *L, const world::Trace &val) {
	lua_createtable(L, 0, 6);
	Marshal<Vec3>::Push(L, val.start);
	lua_setfield(L, -2, "start");
	Marshal<Vec3>::Push(L, val.end);
	lua_setfield(L, -2, "_end");
	Marshal<Vec3>::Push(L, val.normal);
	lua_setfield(L, -2, "normal");
	Marshal<Vec3>::Push(L, val.traceEnd);
	lua_setfield(L, -2, "traceEnd");
	lua_pushinteger(L, val.contents);
	lua_setfield(L, -2, "contents");
	lua_pushnumber(L, val.frac);
	lua_setfield(L, -2, "frac");
	lua_pushboolean(L, val.startSolid ? 1 : 0);
	lua_setfield(L, -2, "startSolid");
}

world::Trace Marshal<world::Trace>::Get(lua_State *L, int index, bool luaError) {
	world::Trace t;
	t.startSolid = false;
	t.frac = 1.f;
	t.contents = world::bsp_file::kContentsFlag_Solid;

	lua_getfield(L, index, "start");
	t.start = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "_end");
	t.end = Marshal<Vec3>::Get(L, -1, luaError);
	lua_pop(L, 1);

	lua_getfield(L, index, "contents");
	if (lua_type(L, -1) == LUA_TNUMBER)
		t.contents = (int)lua_tointeger(L, -1);
	lua_pop(L, 1);

	// we do not marshal frac, startSolid, normal, or traceend (output params only).
	return t;
}

bool Marshal<world::Trace>::IsA(lua_State *L, int index) {
	return lua_type(L, index) == LUA_TTABLE;
}

} // lua
