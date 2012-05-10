// Entity.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../App.h"
#include "World.h"
#include "Entity.h"
#include "../COut.h"
#include "Lua/D_SkModel.h"
#include "Lua/D_Sound.h"
#include "Lua/D_Mesh.h"
#include "Lua/T_Tick.h"
#include "../Sound.h"
#include "../MathUtils.h"
#include <Runtime/Reflect.h>

namespace world {

PState::PState() :
pos(Vec3::Zero),
origin(Vec3::Zero),
worldPos(Vec3::Zero),
cameraPos(Vec3::Zero),
originAngles(Vec3::Zero),
worldAngles(Vec3::Zero),
velocity(Vec3::Zero),
targetAngles(Vec3::Zero),
cameraAngles(Vec3::Zero),
accel(Vec3::Zero),
bbox(Vec3::Zero, Vec3::Zero),
groundFriction(0.f),
airFriction(0.f),
maxGroundSpeed(0.f),
maxAirSpeed(0.f),
splineDistance(0.f),
maxSplineBank(0.f),
splineBankScale(0.f),
splineBankLerp(-1.f),
mtype(MT_None),
stype(ST_None),
flags(0)
{
}

Entity::PSVars::PSVars() :
splineFwd(Vec3::Zero),
lerpFlags(0),
splineId(-1),
motionType(ska::Ska::MT_None)
{
	motion.s = Vec3(1, 1, 1);
	motion.r = Quat::Identity;
	motion.t = Vec3::Zero;
}

Entity::Ref Entity::Create(const char *classname)
{
	RAD_ASSERT(classname);

	String factory("spawn::");
	factory += classname;

	const reflect::Class *type = reflect::Class::Find(factory.c_str());
	if (!type)
		return Entity::Ref();

	void *entity = 0;

	try
	{
		reflect::Reflected r(reflect::Reflect(entity));
		type->FindAndCallStaticMethod(
			r,
			"Create",
			reflect::NullArgs(),
			false
		);
	}
	catch (reflect::MethodNotFoundException&)
	{
		COut(C_Warn) << "Entity::Create() caught reflect::MethodNotFoundException trying to call CLCreate() on '" << factory << "' for class '" << classname << "'" << std::endl;
	}

	if (!entity)
		COut(C_Warn) << "Entity::Create() failed for '" << classname << "', factory '" << factory << "'" << std::endl;

	Entity::Ref r(reinterpret_cast<Entity*>(entity));
	r->m_classname = classname;
	return r;
}

Entity::Ref Entity::LuaCreate(const char *classname)
{
	Entity::Ref r = Create(classname);
	if (!r)
	{
		r.reset(new (ZWorld) Entity());
		r->m_classname = classname;
	}

	r->m_scripted = true;
	return r;
}

Entity::Entity() :
m_spawnState(S_LuaCreate), 
m_id(-1),
m_nextLuaThink(1000), 
m_lastLuaThink(0),
m_scripted(false),
m_frame(-1),
m_luaCallbackIdx(0),
m_gc(false)
{
	for (int i = 0; i < NumLuaCallbackBuckets; ++i)
		m_luaCallbacks[i] = 0;
}

Entity::~Entity()
{
}

void Entity::CleanLuaState()
{
	if (m_scripted)
		world->lua->DeleteEntId(*this);
}

void Entity::PreTick(
	int frame,
	float dt, 
	const xtime::TimeSlice &time
)
{
}

void Entity::Tick(
	int frame,
	float dt, 
	const xtime::TimeSlice &time
)
{
}

int Entity::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
)
{
	m_targetname = keys.StringForKey("targetname", "");
	m_ps.cameraPos = m_ps.worldPos = m_ps.pos = keys.Vec3ForKey("origin");
	return pkg::SR_Success;
}

void Entity::PostSpawn()
{
}

bool Entity::HandleEvent(const Event::Ref &event)
{
	if (scripted)
	{
		lua_State *L = world->lua->L;
		PushEntityFrame(L);
		lua_getfield(L, -1, "OnEvent");
		if (lua_type(L, -1) == LUA_TFUNCTION)
		{
			lua_pushvalue(L, -2);
			lua_pushstring(L, event->cmd.get());

			if (event->args.get() && event->args.get()[0] != 0)
				lua_pushstring(L, event->args.get());
			else
				lua_pushnil(L);

			bool r = world->lua->Call(L, "Entity::HandleEvent", 3, 1, 0);
			if (r)
			{
				r = lua_toboolean(L, -1) ? true : false;
				lua_pop(L, 1);
			}
			lua_pop(L, 1);
			return r;
		}
		else
		{
			lua_pop(L, 2);
		}
	}
	return false;
}

void Entity::QueueScriptTask(const Tickable::Ref &task)
{
	m_scriptTasks.Push(task);
}

void Entity::QueueTask(const Tickable::Ref &task)
{
	m_tasks.Push(task);
}

void Entity::TickOther(
	Entity &entity,
	int frame,
	float dt,
	const xtime::TimeSlice &time
)
{
	entity.PrivateTick(frame, dt, time);
}

void Entity::DoScriptDamage(
	lua_State *L,
	const Ref &src, // inflictor
	float damage
)
{
	if (L == 0)
		L = world->lua->L;

	if (PushEntityCall(L, "OnDamage"))
	{
		src->PushEntityFrame(L);
		lua_pushnumber(L, (lua_Number)damage);
		WorldLua::Call(L, "DoScriptDamage", 3, 0, 0);
	}
}

void Entity::PrivateTick(
	int frame,
	float dt,
	const xtime::TimeSlice &time
)
{
	if (m_frame == frame)
		return;
	m_frame = frame;

	PreTick(frame, dt, time);

	bool think = !m_scriptTasks.Tick(*this, dt, time, 0);
	
	int worldTime = (int)(world->gameTime.get()*1000.f);

	if (m_scripted && think && (worldTime-m_lastLuaThink) >= m_nextLuaThink)
	{
		bool unused;
		world->lua->RunCo(*this, unused);
		m_lastLuaThink = worldTime;
	}

	m_tasks.Tick(*this, dt, xtime::TimeSlice::Infinite, TF_PreTick);
	Tick(frame, dt, time);
	m_tasks.Tick(*this, dt, xtime::TimeSlice::Infinite, TF_PostTick);
	TickViewModels(dt); // before physics, for motion
	TickPhysics(frame, dt, time);
	m_tasks.Tick(*this, dt, xtime::TimeSlice::Infinite, TF_PostPhysics);
	TickSounds(dt);
}

void Entity::TickViewModels(float dt)
{
	for (ViewModel::Map::const_iterator it = m_models.begin(); it != m_models.end(); ++it)
	{
		it->second->Tick(App::Get()->time, dt);

		SkMeshViewModel::Ref skMesh(boost::dynamic_pointer_cast<SkMeshViewModel>(it->second));
		if (skMesh)
		{
			m_psv.motion = skMesh->motion;
			m_psv.motionType = skMesh->motionType;
		}
	}
}

void Entity::TickSounds(float dt)
{
	Quat q = QuatFromAngles(m_ps.worldAngles);

	for (SoundMap::const_iterator it = m_sounds.begin(); it != m_sounds.end(); ++it)
	{
		const Sound::Ref &sound = it->second;
		sound->pos = m_ps.worldPos;
		sound->rot = q;
	}
}

int Entity::PrivateSpawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
)
{
	while (m_spawnState != S_Done && time.remaining)
	{
		switch (m_spawnState)
		{
		case S_LuaCreate:
			{
				if (!world->lua->CreateEntity(*this, m_id, m_scripted ? m_classname.c_str() : 0))
					return pkg::SR_ScriptError;
				++m_spawnState;
				if (!time.remaining)
					return pkg::SR_Pending;
			} break;
		case S_Native:
			{
				int r = Spawn(keys, time, flags);
				if (r == pkg::SR_Success)
				{
					if (m_scripted)
					{	
						++m_spawnState;
					}
					else
					{
						m_spawnState = S_Done;
						return pkg::SR_Success;
					}
				}
				else if (r < pkg::SR_Success)
				{
					return r; // error state
				}
			} break;
		case S_LuaCoSpawn:
			{
				world->lua->CoSpawn(*this, keys);
				++m_spawnState;
			} break;
		case S_LuaSpawn:
			{
				bool complete;
				m_scriptTasks.Tick(*this, 0.f, time, flags); // tick lua spawn tasks.
				if (!world->lua->RunCo(*this, complete))
					return pkg::SR_ScriptError;
				if (complete)
				{
					++m_spawnState;
					m_ps.worldPos = m_ps.pos + m_ps.origin;
					m_ps.worldAngles = WrapAngles(m_ps.angles.pos + m_ps.originAngles);
				}
			} break;
		}
	}

	return (m_spawnState == S_Done) ? pkg::SR_Success : pkg::SR_Pending;
}

int Entity::PrivatePostSpawn(
	const xtime::TimeSlice &time,
	int flags
)
{
	if (m_spawnState == S_Done)
		++m_spawnState;

	if (m_spawnState == S_NativePostSpawn)
	{
		PostSpawn();
		++m_spawnState;
		m_ps.worldPos = m_ps.pos + m_ps.origin;
		m_ps.worldAngles = WrapAngles(m_ps.angles.pos + m_ps.originAngles);
		if (m_scripted)
		{
			if (!world->lua->CoPostSpawn(*this))
			{
				world->lua->CoThink(*this); // setup CoThink
				++m_spawnState;
			}
		}
	}

	int r = pkg::SR_Success;

	if (m_scripted && m_spawnState == S_PostSpawn)
	{
		bool complete;
		m_scriptTasks.Tick(*this, 0.f, time, flags); // tick lua spawn tasks.
		if (!world->lua->RunCo(*this, complete))
			return pkg::SR_ScriptError;
		if (complete)
		{
			++m_spawnState;
			world->lua->CoThink(*this); // setup CoThink
		}
		else
		{
			r = pkg::SR_Pending;
		}
	}

	return r;
}

void Entity::PrivateLevelStart()
{
	if (PushEntityCall("OnLevelStart"))
	{
		world->lua->Call("Entity::PrivateLevelStart()", 1, 0, 0);
	}

	LevelStart();
}

bool Entity::PrivateHandleEvent(const Event::Ref &event)
{
	return HandleEvent(event);
}

World *Entity::RAD_IMPLEMENT_GET(world)
{
	ZoneTagRef tag = m_zoneTag.lock();
	RAD_ASSERT(tag);
	ZoneRef zone = tag->zone;
	RAD_ASSERT(zone);
	return zone->world;
}

// ------------- Lua Calls --------------

void Entity::PushCallTable(lua_State *L)
{
	lua_pushcfunction(L, lua_AttachViewModel);
	lua_setfield(L, -2, "AttachViewModel");
	lua_pushcfunction(L, lua_RemoveViewModel);
	lua_setfield(L, -2, "RemoveViewModel");
	lua_pushcfunction(L, lua_SetViewModelAngles);
	lua_setfield(L, -2, "SetViewModelAngles");
	lua_pushcfunction(L, lua_SetViewModelScale);
	lua_setfield(L, -2, "SetViewModelScale");
	lua_pushcfunction(L, lua_SetViewModelVisible);
	lua_setfield(L, -2, "SetViewModelVisible");
	lua_pushcfunction(L, lua_ViewModelVisible);
	lua_setfield(L, -2, "ViewModelVisible");
	lua_pushcfunction(L, lua_SetViewModelPos);
	lua_setfield(L, -2, "SetViewModelPos");
	lua_pushcfunction(L, lua_ViewModelPos);
	lua_setfield(L, -2, "ViewModelPos");
	lua_pushcfunction(L, lua_FadeViewModel);
	lua_setfield(L, -2, "FadeViewModel");
	lua_pushcfunction(L, lua_ViewModelBonePos);
	lua_setfield(L, -2, "ViewModelBonePos");
	lua_pushcfunction(L, lua_ViewModelFindBone);
	lua_setfield(L, -2, "ViewModelFindBone");
	lua_pushcfunction(L, lua_TickViewModel);
	lua_setfield(L, -2, "TickViewModel");
	lua_pushcfunction(L, lua_AttachSound);
	lua_setfield(L, -2, "AttachSound");
	lua_pushcfunction(L, lua_RemoveSound);
	lua_setfield(L, -2, "RemoveSound");
	lua_pushcfunction(L, lua_AddTickable);
	lua_setfield(L, -2, "AddTickable");
	lua_pushcfunction(L, lua_Delete);
	lua_setfield(L, -2, "Delete");
	LUART_REGISTER_GETSET(L, Parent);
	LUART_REGISTER_GETSET(L, Pos);
	LUART_REGISTER_GETSET(L, Origin);
	LUART_REGISTER_GETSET(L, WorldPos);
	LUART_REGISTER_GETSET(L, CameraPos);
	LUART_REGISTER_GETSET(L, Angles);
	LUART_REGISTER_GETSET(L, OriginAngles);
	LUART_REGISTER_GETSET(L, WorldAngles);
	LUART_REGISTER_GETSET(L, AngleSpring);
	LUART_REGISTER_GETSET(L, TargetAngles);
	LUART_REGISTER_GETSET(L, Velocity);
	LUART_REGISTER_GETSET(L, Mins);
	LUART_REGISTER_GETSET(L, Maxs);
	LUART_REGISTER_GETSET(L, Accel);
	LUART_REGISTER_GETSET(L, GroundFriction);
	LUART_REGISTER_GETSET(L, AirFriction);
	LUART_REGISTER_GETSET(L, MaxGroundSpeed);
	LUART_REGISTER_GETSET(L, MaxAirSpeed);
	LUART_REGISTER_GETSET(L, SplineDistance);
	LUART_REGISTER_GETSET(L, MaxSplineBank);
	LUART_REGISTER_GETSET(L, SplineBankScale);
	LUART_REGISTER_GETSET(L, SplineBankLerp);
	LUART_REGISTER_GETSET(L, MoveType);
	LUART_REGISTER_GETSET(L, SolidType);
	LUART_REGISTER_GETSET(L, Flags);
	LUART_REGISTER_GETSET(L, NextThink);
}

void Entity::AttachViewModel(const ViewModel::Ref &ref)
{
	m_models[ref.get()] = ref;
}

void Entity::RemoveViewModel(const ViewModel::Ref &ref)
{
	m_models.erase(ref.get());
}

void Entity::PushEntityFrame()
{
	world->lua->PushEntityFrame(*this);
}

void Entity::PushEntityFrame(lua_State *L)
{
	WorldLua::PushEntityFrame(L, *this);
}

bool Entity::PushEntityCall(const char *name)
{
	return world->lua->PushEntityCall(*this, name);
}

bool Entity::PushEntityCall(lua_State *L, const char *name)
{
	return WorldLua::PushEntityCall(L, *this, name);
}

int Entity::StoreLuaCallback(lua_State *L, int index, int frame)
{
	int idx = FindLuaCallbackSlot();
	if (idx == -1)
		luaL_error(L, "Entity::PushCallback: MaxLuaCallbacks");

	if (frame == lua::InvalidIndex)
		PushEntityFrame(L);
	if (!lua::GetFieldExt(L, (frame==lua::InvalidIndex) ? -1 : frame, "sys.callbacks"))
		luaL_typerror(L, (frame==lua::InvalidIndex) ? -1 : frame, "sys.callbacks");
		
	luaL_checktype(L, -1, LUA_TTABLE);
	lua_pushinteger(L, idx);
	lua_pushvalue(L, index);
	lua_settable(L, -3);

	if (frame == lua::InvalidIndex)
		lua_pop(L, 2); // sys.callbacks + frame
	else
		lua_pop(L, 1);

	return idx;
}

// If release is true, callback slot is released.
bool Entity::LoadLuaCallback(int id, int frame, bool release)
{
	return LoadLuaCallback(world->lua->L, id, frame, release, false);
}

bool Entity::LoadLuaCallback(lua_State *L, int id, int frame, bool release, bool luaError)
{
	RAD_ASSERT(id >= 0 && id < MaxLuaCallbacks);

	int bucket = id / MaxLuaCallbacks;
	int idx = id & (MaxLuaCallbacks-1);
	if ((m_luaCallbacks[bucket]&(1<<idx)) == 0)
		return false;

	if (frame == lua::InvalidIndex)
		PushEntityFrame(L);
	if (!lua::GetFieldExt(L, (frame==lua::InvalidIndex) ? -1 : frame, "sys.callbacks"))
	{
		if (luaError)
			luaL_typerror(L, (frame==lua::InvalidIndex) ? -1 : frame, "sys.callbacks");
		else if (frame==lua::InvalidIndex)
			lua_pop(L, 1); // frame
		return false;
	}

	if (luaError)
	{
		luaL_checktype(L, -1, LUA_TTABLE);
	}
	else if (lua_type(L, -1) != LUA_TTABLE)
	{
		if (frame==lua::InvalidIndex)
			lua_pop(L, 1); // frame
		lua_pop(L, 1);
		return false;
	}

	lua_pushinteger(L, id);
	lua_gettable(L, -2);
	lua_remove(L, -2); // pop entity frame and sys.callbacks

	if (frame==lua::InvalidIndex)
		lua_remove(L, -2);

	bool r = !lua_isnil(L, -1);

	if (!r)
	{ // invalid (someone cleared this)
		m_luaCallbacks[bucket] &= ~(1<<idx);
		lua_pop(L, 1); // remove nil value
	}
	else if (release)
	{
		ReleaseLuaCallback(id, frame);
	}

	return r;
}

void Entity::ReleaseLuaCallback(int id, int frame)
{
	ZoneTag::Ref t = zoneTag;
	if (t)
	{
		Zone::Ref z = t->zone;
		if (z)
		{
			World* w = z->world;
			if (w && !w->destroy)
				ReleaseLuaCallback(world->lua->L, id, frame, false);
		}
	}
}

void Entity::ReleaseLuaCallback(lua_State *L, int id, int frame, bool luaError)
{
	int bucket = id / MaxLuaCallbacks;
	int idx = id & (MaxLuaCallbacks-1);
	RAD_ASSERT(m_luaCallbacks[bucket]&(1<<idx));
	m_luaCallbacks[bucket] &= ~(1<<idx);
	m_luaCallbackIdx = id;

	if (frame == lua::InvalidIndex)
		PushEntityFrame(L);
	if (!lua::GetFieldExt(L, (frame==lua::InvalidIndex) ? -1 : frame, "sys.callbacks"))
	{
		if (luaError)
			luaL_typerror(L, (frame==lua::InvalidIndex) ? -1 : frame, "sys.callbacks");
		return;
	}

	lua_pushinteger(L, idx);
	lua_pushnil(L);
	lua_settable(L, -3);

	if (frame == lua::InvalidIndex)
		lua_pop(L, 2); // sys.callbacks + frame
	else
		lua_pop(L, 1);
}

int Entity::FindLuaCallbackSlot()
{
	for (int i = 0; i < MaxLuaCallbacks; ++i)
	{
		int z = (m_luaCallbackIdx+i) & (MaxLuaCallbacks-1);
		int bucket = z / MaxLuaCallbacks;
		int idx = z & (MaxLuaCallbacks-1);

		if ((m_luaCallbacks[bucket] & (1<<idx)) == 0)
		{
			m_luaCallbackIdx = (z+1)&(MaxLuaCallbacks-1);
			m_luaCallbacks[bucket] |= (1<<idx);
			return z;
		}
	}

	return -1;
}

int Entity::lua_AttachViewModel(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for(;it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
					break;
			}
			if (it == self->m_models.end())
			{ // nope, create it
				SkMeshViewModel::Ref r = SkMeshViewModel::New(
					*self->world->draw.get(),
					self, 
					skModel->mesh
				);
				if (r)
					self->AttachViewModel(boost::static_pointer_cast<ViewModel>(r));
			}

			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
					break;
			}
			if (it == self->m_models.end())
			{ // create bundle
				MeshBundleViewModel::Ref r = MeshBundleViewModel::New(
					*self->world->draw.get(), 
					self, 
					dmesh->bundle
				);
				if (r)
					self->AttachViewModel(boost::static_pointer_cast<ViewModel>(r));
			}

			return 0;
		}
	}

	return 0;
}

int Entity::lua_RemoveViewModel(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view model with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					self->RemoveViewModel(boost::static_pointer_cast<ViewModel>(r));
					break;
				}
			}

			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					self->RemoveViewModel(boost::static_pointer_cast<ViewModel>(r));
					break;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_SetViewModelAngles(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	Vec3 angles = lua::Marshal<Vec3>::Get(L, 3, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->angles = angles;
					break;
				}
			}

			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					r->angles = angles;
					break;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_SetViewModelMotionType(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	int motionType = (int)luaL_checkinteger(L, 3);

	if (motionType < ska::Ska::MT_None || motionType > ska::Ska::MT_Absolute)
		luaL_argerror(L, 3, "motion type out of range");

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->motionType = (ska::Ska::MotionType)motionType;
					break;
				}
			}

			return 0;
		}
	}

	return 0;
}

int Entity::lua_SetViewModelScale(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	Vec3 scale = lua::Marshal<Vec3>::Get(L, 3, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->scale = scale;
					break;
				}
			}
			
			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					r->scale = scale;
					break;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_SetViewModelVisible(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	bool visible = lua_toboolean(L, 3) ? true : false;

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->visible = visible;
					break;
				}
			}
			
			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					r->visible = visible;
					break;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_ViewModelVisible(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					lua_pushboolean(L, r->visible ? 1 : 0);
					return 1;
				}
			}
			
			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					lua_pushboolean(L, r->visible ? 1 : 0);
					return 1;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_SetViewModelPos(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	Vec3 pos = lua::Marshal<Vec3>::Get(L, 3, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->pos = pos;
					break;
				}
			}
			
			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					r->pos = pos;
					break;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_ViewModelPos(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					lua::Marshal<Vec3>::Push(L, r->pos);
					return 1;
				}
			}
			
			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					lua::Marshal<Vec3>::Push(L, r->pos);
					return 1;
				}
			}
			
			return 0;
		}
	}

	return 0;
}

int Entity::lua_FadeViewModel(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	Vec4 rgba = lua::Marshal<Vec4>::Get(L, 3, true);
	float time = (float)luaL_checknumber(L, 4);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->Fade(rgba, time);
					break;
				}
			}
			
			return 0;
		}
	}
	{
		D_Mesh::Ref dmesh = lua::SharedPtr::Get<D_Mesh>(L, "D_Mesh", 2, false);
		if (dmesh)
		{ // already a view model with this attached?
			ViewModel::Map::const_iterator it = self->m_models.begin();
			for (;it != self->m_models.end(); ++it)
			{
				MeshBundleViewModel::Ref r = boost::dynamic_pointer_cast<MeshBundleViewModel>(it->second);
				if (r && r->bundle.get().get() == dmesh->bundle.get().get())
				{
					r->Fade(rgba, time);
					break;
				}
			}
			
			return 0;
		}
	}

	return 0;
}
	
int Entity::lua_ViewModelBonePos(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	int idx = (int)luaL_checkinteger(L, 3);
	
	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					Vec3 p = r->BonePos(idx);
					lua::Marshal<Vec3>::Push(L, p);
					return 1;
				}
			}
		}
	}
	
	return 0;
}
	
int Entity::lua_ViewModelFindBone(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	const char *name = luaL_checkstring(L, 3);
	
	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					lua_pushinteger(L, r->mesh->ska->FindBone(name));
					return 1;
				}
			}
		}
	}
	
	return -1;
}

int Entity::lua_TickViewModel(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);

	// figure out what kind of model is attached:
	{
		D_SkModel::Ref skModel = lua::SharedPtr::Get<D_SkModel>(L, "D_SkModel", 2, false);
		if (skModel)
		{ // is there a view controller with this attached?
			for (ViewModel::Map::const_iterator it = self->m_models.begin(); it != self->m_models.end(); ++it)
			{
				SkMeshViewModel::Ref r = boost::dynamic_pointer_cast<SkMeshViewModel>(it->second);
				if (r && r->mesh.get().get() == skModel->mesh.get().get())
				{
					r->Tick(App::Get()->time+0.001f, 0.001f);
					return 1;
				}
			}
		}
	}
	
	return -1;
}

int Entity::lua_AttachSound(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	D_Sound::Ref sound = lua::SharedPtr::Get<D_Sound>(L, "D_Sound", 2, true);
	self->m_sounds[sound->sound.get().get()] = sound->sound;
	sound->sound->sourceRelative = false;
	return 0;
}

int Entity::lua_RemoveSound(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	D_Sound::Ref sound = lua::SharedPtr::Get<D_Sound>(L, "D_Sound", 2, true);
	self->m_sounds.erase(sound->sound.get().get());
	sound->sound->sourceRelative = true;
	return 0;
}

int Entity::lua_AddTickable(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	int flags = (int)luaL_checkinteger(L, 2);
	luaL_checktype(L, 3, LUA_TFUNCTION);

	T_Tick::Ref tick(new (ZWorld) T_Tick(self->shared_from_this(), flags));
	tick->m_id = self->StoreLuaCallback(L, 3, 1); // store LUA_TFUNCTION in callbacks set
	
	self->QueueTask(boost::static_pointer_cast<Entity::Tickable>(tick));
	tick->Push(L);
	return 1;
}

int Entity::lua_Delete(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);

	if (self->m_id < World::FirstTempEntId)
		luaL_error(L, "Illegal call to Delete() on non temporary entity!");

	self->gc = true;

	return 0;
}

ENT_GETSET_WEAK_ENT(Entity, Parent, m_ps.parent);
ENT_GETSET(Entity, Pos, Vec3, m_ps.pos);
ENT_GETSET(Entity, Origin, Vec3, m_ps.origin);
ENT_GETSET(Entity, WorldPos, Vec3, m_ps.worldPos);
ENT_GETSET(Entity, CameraPos, Vec3, m_ps.cameraPos);
ENT_GETSET(Entity, Angles, physics::SpringVertex, m_ps.angles);
ENT_GETSET(Entity, OriginAngles, Vec3, m_ps.originAngles);
ENT_GETSET(Entity, WorldAngles, Vec3, m_ps.worldAngles);
ENT_GETSET(Entity, AngleSpring, physics::Spring, m_ps.angleSpring);
ENT_GETSET(Entity, TargetAngles, Vec3, m_ps.targetAngles);
ENT_GETSET(Entity, Velocity, Vec3, m_ps.velocity);
ENT_GET(Entity, Mins, Vec3, m_ps.bbox.Mins());
ENT_SET_CUSTOM(Entity, Mins, self->m_ps.bbox.SetMins(lua::Marshal<Vec3>::Get(L, 2, true)));
ENT_GET(Entity, Maxs, Vec3, m_ps.bbox.Maxs());
ENT_SET_CUSTOM(Entity, Maxs, self->m_ps.bbox.SetMaxs(lua::Marshal<Vec3>::Get(L, 2, true)));
ENT_GETSET(Entity, Accel, Vec3, m_ps.accel);
ENT_GETSET(Entity, GroundFriction, float, m_ps.groundFriction);
ENT_GETSET(Entity, AirFriction, float, m_ps.airFriction);
ENT_GETSET(Entity, MaxGroundSpeed, float, m_ps.maxGroundSpeed);
ENT_GETSET(Entity, MaxAirSpeed, float, m_ps.maxAirSpeed);

ENT_GET(Entity, SplineDistance, float, m_ps.splineDistance);
int Entity::LUART_SETFN(SplineDistance)(lua_State *L)
{
	Entity *self = WorldLua::EntFramePtr(L, 1, true);
	self->m_ps.splineDistance = (float)luaL_checknumber(L, 2);
	self->m_psv.splineIdx = 0;
	return 0;
}

ENT_GETSET(Entity, MaxSplineBank, float, m_ps.maxSplineBank);
ENT_GETSET(Entity, SplineBankScale, float, m_ps.splineBankScale);
ENT_GETSET(Entity, SplineBankLerp, float, m_ps.splineBankLerp);
ENT_GET(Entity, MoveType, int, m_ps.mtype);
ENT_SET_CUSTOM(Entity, MoveType, self->m_ps.mtype = (MoveType)luaL_checkinteger(L, 2));
ENT_GET(Entity, SolidType, int, m_ps.stype);
ENT_SET_CUSTOM(Entity, SolidType, self->m_ps.stype = (SolidType)luaL_checkinteger(L, 2));
ENT_GETSET(Entity, Flags, int, m_ps.flags);
ENT_GETSET(Entity, NextThink, int, m_nextLuaThink);

} // world

#include <Runtime/ReflectMap.h>

RADREFLECT_BEGIN_CLASS_NAMESPACE("world::EntityFactoryAttribute", world, EntityFactoryAttribute)
RADREFLECT_END_NAMESPACE(RADENG_API, world, EntityFactoryAttribute)
