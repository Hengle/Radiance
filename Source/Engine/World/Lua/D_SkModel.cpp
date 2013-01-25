// D_SkModel.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "D_SkModel.h"
#include "../World.h"

namespace world {

D_SkModel::D_SkModel(const r::SkMesh::Ref &mesh) : D_Asset(mesh->asset),
m_mesh(mesh) {
	RAD_ASSERT(m_mesh);
}

void D_SkModel::PushElements(lua_State *L) {
	D_Asset::PushElements(L);
	lua_pushcfunction(L, lua_BlendToState);
	lua_setfield(L, -2, "BlendToState");
	lua_pushcfunction(L, lua_SetRootController);
	lua_setfield(L, -2, "SetRootController");
	lua_pushcfunction(L, lua_HasState);
	lua_setfield(L, -2, "HasState");
}

bool D_SkModel::SetRootController(lua_State *Lerr, Entity *entity, const char *type) {
	if (!string::cmp(type, "BlendToController")) {
		if (!m_blendTo) {
			m_blendTo = ska::BlendToController::New(*m_mesh->ska.get(), ska::Notify::Ref());
			m_mesh->ska->root = m_blendTo;
			m_blendTo->Activate();
		}

		return true;
	}

	return false;
}

bool D_SkModel::BlendToState(const char *state, const char *blendTarget, bool restart, const ska::Notify::Ref &notify) {
	if (!m_blendTo || !m_mesh->ska->root.get())
		return false;
	
	String tname(CStr(state));

	if (m_curState == tname && !restart)
		return false; // no action taken

	ska::Controller::Ref target;

	// check animstate first
	{
		ska::AnimState::Map::const_iterator it = m_mesh->states->find(String(tname));

		if (it != m_mesh->states->end()) {
			ska::AnimationVariantsSource::Ref animSource = ska::AnimationVariantsSource::New(
				it->second,
				*m_mesh->ska.get(),
				notify,
				blendTarget
			);

			target = boost::static_pointer_cast<ska::Controller>(animSource);
		}
	}

	if (!target) {
		ska::Animation::Map::const_iterator it = m_mesh->ska->anims->find(tname);
		
		if (it != m_mesh->ska->anims->end()) {
			ska::AnimationSource::Ref animSource = ska::AnimationSource::New(
				*it->second,
				1.f,
				1.f,
				1.f,
				0,
				*m_mesh->ska.get(),
				notify
			);

			target = boost::static_pointer_cast<ska::Controller>(animSource);
		}
	}

	if (target) {
		m_curState = state; // make copy of string.
		m_blendTo->BlendTo(target);
	}

	return target ? true : false;
}

bool D_SkModel::HasState(const char *state) {
	if (!m_blendTo || !m_mesh->ska->root.get())
		return false;
	
	String tname(CStr(state));

	{
		ska::AnimState::Map::const_iterator it = m_mesh->states->find(String(tname));
		if (it != m_mesh->states->end())
			return true;
	}

	{
		ska::Animation::Map::const_iterator it = m_mesh->ska->anims->find(tname);
		
		if (it != m_mesh->ska->anims->end())
			return true;
	}

	return false;
}

int D_SkModel::lua_BlendToState(lua_State *L) {
	Ref self = Get<D_SkModel>(L, "D_SkModel", 1, true);
	const char *string = luaL_checkstring(L, 2);
	const char *blendTarget = lua_tostring(L, 3);
	bool restart = lua_toboolean(L, 4) ? true : false;

	ska::Notify::Ref notify;

	Entity *entity = WorldLua::EntFramePtr(L, 5, false);
	if (entity && lua_gettop(L) > 5)  {
		// passed in a callback table
		int callbackId = entity->StoreLuaCallback(L, 6, 5);
		RAD_ASSERT(callbackId != -1);
		notify.reset(new Notify(*entity, callbackId));
	}

	bool r = self->BlendToState(string, blendTarget, restart, notify);

	if (notify && r) {
		static_cast<Notify&>(*notify).Push(L);
		return 1;
	}

	lua_pushnil(L);
	return 1;
}

int D_SkModel::lua_SetRootController(lua_State *L) {
	Ref self = Get<D_SkModel>(L, "D_SkModel", 1, true);
	const char *type;
	
	Entity *entity = 0;

	if (lua_type(L, 2) == LUA_TSTRING) {
		type = lua_tostring(L, 2);
	} else {
		entity = WorldLua::EntFramePtr(L, 2, true);
		type = luaL_checkstring(L, 3);
	}

	self->SetRootController(L, entity, type);	
	return 0;
}

int D_SkModel::lua_HasState(lua_State *L) {
	Ref self = Get<D_SkModel>(L, "D_SkModel", 1, true);
	lua_pushboolean(L, self->HasState(luaL_checkstring(L, 2)) ? 1 : 0);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////

D_SkModel::Notify::Notify(Entity &entity, int callbackId) :
m_entity(entity.shared_from_this()),
m_callbackId(callbackId) {
}

D_SkModel::Notify::~Notify() {
	Entity::Ref entity = m_entity.lock();
	if (entity)
		entity->ReleaseLuaCallback(m_callbackId, lua::InvalidIndex);
}

void D_SkModel::Notify::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_SetMasked);
	lua_setfield(L, -2, "SetMasked");
}

void D_SkModel::Notify::OnTag(const ska::AnimTagEventData &data) {
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
	lua_pushstring(L, data.tag.c_str);
	entity->world->lua->Call(L, "D_SkModel::Notify::OnTag", 2, 0, 0);
	lua_pop(L, 1); // pop callback table
}

void D_SkModel::Notify::OnEndFrame(const ska::AnimStateEventData &data) {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnEndFrame");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	entity->world->lua->Call(L, "D_SkModel::Notify::OnEndFrame", 1, 0, 0);
	lua_pop(L, 1); // pop callback table
}

void D_SkModel::Notify::OnFinish(const ska::AnimStateEventData &data, bool masked) {
	Entity::Ref entity = m_entity.lock();
	if (!entity)
		return;
	if (!entity->LoadLuaCallback(m_callbackId))
		return;

	lua_State *L = entity->world->lua->L;

	lua_getfield(L, -1, "OnFinish");
	if (lua_type(L, -1) != LUA_TFUNCTION) {
		lua_pop(L, 2);
		return;
	}

	entity->PushEntityFrame(L);
	entity->world->lua->Call(L, "D_SkModel::Notify::OnFinish", 1, 0, 0);
	lua_pop(L, 1); // pop callback table
}

int D_SkModel::Notify::lua_SetMasked(lua_State *L) {
	Ref self = lua::SharedPtr::Get<Notify>(L, "D_SkModel::Notify", 1, true);
	int oldMask = self->masked;
	self->masked = (int)luaL_checkinteger(L, 2);
	lua_pushinteger(L, oldMask);
	return 1;
}

} // world
