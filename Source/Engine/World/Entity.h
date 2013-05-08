/*! \file Entity.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "WorldDef.h"
#include "EntityDef.h"
#include "Floors.h"
#include "Event.h"
#include "Keys.h"
#include "../Packages/Packages.h"
#include "../Lua/LuaRuntime.h"
#include "../Tickable.h"
#include "../MathUtils.h"
#include "DrawModel.h"
#include "../SkAnim/SkAnim.h"
#include "../Physics/Spring.h"
#include "../Sound/SoundDef.h"
#include <Runtime/ReflectDef.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>

#include <Runtime/PushPack.h>

namespace world {

///////////////////////////////////////////////////////////////////////////////

struct EntSpawn {
	Keys keys;
};

///////////////////////////////////////////////////////////////////////////////

class T_Tick;
class EntityFactoryAttribute {};

///////////////////////////////////////////////////////////////////////////////

enum TickFlags {
	RAD_FLAG(kTickFlag_PreTick),
	RAD_FLAG(kTickFlag_PostTick),
	RAD_FLAG(kTickFlag_PostPhysics)
};

enum MoveType {
	kMoveType_None,
	kMoveType_Fly,
	kMoveType_Spline, // ps.ground points to spline track.
	kMoveType_Floor, // Floor move system
	kMoveType_Ska // Animation controlled move
};

enum SolidType {
	RAD_FLAG(kSolidType_BBox),
	kSolidType_All = 0xff,
	kSolidType_None = 0
};

enum OccupantType {
	kOccupantType_None,
	kOccupantType_BBox,
	kOccupantType_Volume
};

enum PhysicsFlags {
	RAD_FLAG(kPhysicsFlag_OnGround),
	RAD_FLAG(kPhysicsFlag_AutoFace), // for MT_Fly, will adjust velocity for angles
	RAD_FLAG(kPhysicsFlag_Friction),
	RAD_FLAG(kPhysicsFlag_SeekAngles), // Physics will seek to targetAngles.
	RAD_FLAG(kPhysicsFlag_ResetSpline), // Reset spline to beginning
	RAD_FLAG(kPhysicsFlag_LoopSpline), // Loop on spline
	RAD_FLAG(kPhysicsFlag_SplineEvents),
	RAD_FLAG(kPhysicsFlag_SplineBank),
	RAD_FLAG(kPhysicsFlag_FlipSplineBank),
};

enum {
	kEntityClassBits_Any = 0
};

struct PState {
// I haven't been able to figure out how I want to represent physics.
// Will improve when I have time to think more. Right now we need to ship Crow.
	PState();

	EntityWRef parent;

	Vec3 pos;
	Vec3 origin;
	Vec3 worldPos; // origin + pos
	Vec3 cameraPos; // worldPos + cameraShift
	Vec3 cameraShift;
	Vec3 originAngles;
	Vec3 worldAngles;
	Vec3 targetAngles;
	Vec3 cameraAngles;
	Vec3 velocity;
	Vec3 accel;
	Vec3 snapTurnAngles;
	BBox bbox;
	FloorMove::Ref activeMove;
	FloorMove::Ref desiredMove;
	FloorMove::State moveState;
	r::SkMesh::Ref motionSka;
	physics::Spring angleSpring;
	physics::SpringVertex angles;
	float groundFriction; // units per second on ground
	float airFriction; // units per second in air
	float maxGroundSpeed;
	float maxAirSpeed;
	float splineDistance;
	float maxSplineBank;
	float splineBankScale;
	float splineBankLerp;
	float autoDecelDistance;
	float motionScale;
	float distanceMoved; // set by physics engine
	MoveType mtype;
	SolidType stype;
	OccupantType otype;
	int flags; // physics flags
};

///////////////////////////////////////////////////////////////////////////////
// Lua Marshling helpers

#define ENT_DECL_GET(_name) \
	LUART_DECL_GET(_name)

#define ENT_DECL_SET(_name) \
	LUART_DECL_SET(_name)

#define ENT_DECL_GETSET(_name) \
	LUART_DECL_GETSET(_name)

///////////////////////////////////////////////////////////////////////////////

#define ENT_GET(_class, _name, _type, _member) \
	LUART_GET(_class, _name, _type, _member, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)))

#define ENT_GET_CUSTOM(_class, _name, _push) \
	LUART_GET_CUSTOM(_class, _name, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)), _push)

#define ENT_SET(_class, _name, _type, _member) \
	LUART_SET(_class, _name, _type, _member, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)))

#define ENT_SET_CUSTOM(_class, _name, _get) \
	LUART_SET_CUSTOM(_class, _name, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)), _get)

#define ENT_GETSET(_class, _name, _type, _member) \
	LUART_GETSET(_class, _name, _type, _member, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)))

#define ENT_GET_WEAK_ENT(_class, _name, _member) \
	LUART_GET_CUSTOM(_class, _name, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)), {Entity::Ref z = self->_member.lock(); if(z){z->PushEntityFrame(L);}else{lua_pushnil(L);}})

#define ENT_GET_ENT(_class, _name, _member) \
	LUART_GET_CUSTOM(_class, _name, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)), {const Entity::Ref &z = self->_member; if(z){z->PushEntityFrame(L);}else{lua_pushnil(L);}})

#define ENT_SET_ENT(_class, _name, _member) \
	LUART_SET_CUSTOM(_class, _name, _class *self = static_cast<_class*>(WorldLua::EntFramePtr(L, 1, true)), {Entity* z = WorldLua::EntFramePtr(L, 2, false); if (z){self->_member=z->shared_from_this();}else{self->_member.reset();}})

#define ENT_GETSET_WEAK_ENT(_class, _name, _member) \
	ENT_GET_WEAK_ENT(_class, _name, _member) \
	ENT_SET_ENT(_class, _name, _member)

#define ENT_GETSET_ENT(_class, _name, _member) \
	ENT_GET_ENT(_class, _name, _member) \
	ENT_SET_ENT(_class, _name, _member)

///////////////////////////////////////////////////////////////////////////////

class Entity : public boost::enable_shared_from_this<Entity> {
public:
	typedef EntityRef Ref;
	typedef EntityWRef WRef;
	typedef EntityWRefList WRefList;
	typedef zone_vector<Ref, ZWorldT>::type Vec;
	typedef zone_map<int, Ref, ZWorldT>::type IdMap;
	typedef zone_map<String, Ref, ZWorldT>::type StringMap;
	typedef zone_multimap<String, Ref, ZWorldT>::type StringMMap;
	typedef Tickable<Entity> Tickable;

	Entity();
	virtual ~Entity();

	RAD_DECLARE_READONLY_PROPERTY(Entity, id, int);
	RAD_DECLARE_READONLY_PROPERTY(Entity, uid, int);
	RAD_DECLARE_READONLY_PROPERTY(Entity, zoneTag, ZoneTagRef);
	RAD_DECLARE_READONLY_PROPERTY(Entity, world, World*);
	RAD_DECLARE_READONLY_PROPERTY(Entity, classname, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Entity, targetname, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Entity, scripted, bool);
	RAD_DECLARE_READONLY_PROPERTY(Entity, models, const DrawModel::Map*);
	RAD_DECLARE_READONLY_PROPERTY(Entity, bspLeafs, const dBSPLeaf::PtrVec*);
	RAD_DECLARE_READONLY_PROPERTY(Entity, classbits, int);
	RAD_DECLARE_READONLY_PROPERTY(Entity, ps, PState*);
	RAD_DECLARE_PROPERTY(Entity, lightInteractionFlags, int, int);
	RAD_DECLARE_PROPERTY(Entity, lightingFlags, LightingFlags, LightingFlags);
	RAD_DECLARE_PROPERTY(Entity, gc, bool, bool);
	
	virtual bool HandleEvent(const Event &event);

	// Will call HandleEvent and pass it to script code
	bool ProcessEvent(const Event &event);

	void QueueTask(const Tickable::Ref &task);
	void QueueScriptTask(const Tickable::Ref &task);

	void CleanLuaState();
	void AttachDrawModel(const DrawModel::Ref &ref);
	void RemoveDrawModel(const DrawModel::Ref &ref);

	void PushEntityFrame();
	void PushEntityFrame(lua_State *L);
	bool PushEntityCall(const char *name);
	bool PushEntityCall(lua_State *L, const char *name);

	// Store should only be called from lua code as it may call lua_error
	// index == pos of callback object
	// frame == index of entity frame if already on the stack, otherwise lua::InvalidIndex
	int StoreLuaCallback(lua_State *L, int index, int frame=lua::InvalidIndex);

	// If release is true, callback slot is released.
	// Returns true if valid callback object was returned
	bool LoadLuaCallback(int id, int frame=lua::InvalidIndex, bool release=false);
	bool LoadLuaCallback(lua_State *L, int id, int frame=lua::InvalidIndex, bool release=false, bool luaError=true);

	void ReleaseLuaCallback(int id, int frame=lua::InvalidIndex);
	void ReleaseLuaCallback(lua_State *L, int id, int frame=lua::InvalidIndex, bool luaError=true);

	void DoScriptDamage(
		lua_State *L,
		const Ref &src, // inflictor
		float damage
	);

	// Link and Unlink add/remove the entity from collision and visibility sets
	// in the world.
	void Link();
	void Unlink();
	void CancelFloorMove();

protected:

	struct PSVars {
		PSVars();
		Vec3 splineFwd;
		int splineId;
		int splineIdx;
	};

	virtual void PreTick(
		int frame,
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual void Tick(
		int frame,
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

	virtual int PostSpawn();
	virtual void LevelStart();

	virtual void PushCallTable(lua_State *L);
	virtual void TickDrawModels(float dt);
	virtual void TickSounds(float dt);

	virtual void TickPhysics(
		int frame, 
		float dt, 
		const xtime::TimeSlice &time
	);
	
	virtual void Tick_MT_Fly(
		int frame, 
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual void Tick_MT_Spline(
		int frame, 
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual void Tick_MT_Floor(
		int frame, 
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual void CustomMoveComplete();

	void TickOther(
		Entity &entity,
		int frame,
		float dt,
		const xtime::TimeSlice &time
	);

	void SeekAngles(float dt);
	void UpdateVelocity(float dt);
	void AutoFace(float dt);
	Vec3 ApplyVelocity(float dt);
	void Move();
	void SkaMove();
	void TransitionFloorMove();
	void SetNextTick(float dt);

	PState m_ps;
	PSVars m_psv;

private:

	enum {
		kMaxLuaCallbacks = 32,
		kNumLuaCallbackBuckets = (kMaxLuaCallbacks+31)/32
	};

	friend class World;
	friend class WorldLua;
	friend class WorldDraw;

	static Ref Create(const char *classname);

	// lua backed object.
	static Ref LuaCreate(const char *classname);

	void PrivateTick(
		int frame,
		float dt,
		const xtime::TimeSlice &time
	);

	int PrivateSpawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

	int PrivatePostSpawn(const xtime::TimeSlice &time, int flags);
	void PrivateLevelStart();

	int FindLuaCallbackSlot();

	// ------------- Lua Calls --------------
	static int lua_AttachDrawModel(lua_State *L);
	static int lua_RemoveDrawModel(lua_State *L);
	static int lua_AttachSound(lua_State *L);
	static int lua_RemoveSound(lua_State *L);
	static int lua_AddTickable(lua_State *L);
	static int lua_Link(lua_State *L);
	static int lua_Unlink(lua_State *L);
	static int lua_Delete(lua_State *L);
	static int lua_CustomMoveComplete(lua_State *L);

	ENT_DECL_GETSET(Parent);
	ENT_DECL_GETSET(Pos);
	ENT_DECL_GETSET(Origin);
	ENT_DECL_GETSET(WorldPos);
	ENT_DECL_GETSET(CameraPos);
	ENT_DECL_GETSET(CameraShift);
	ENT_DECL_GETSET(Angles);
	ENT_DECL_GETSET(OriginAngles);
	ENT_DECL_GETSET(WorldAngles);
	ENT_DECL_GETSET(AngleSpring);
	ENT_DECL_GETSET(TargetAngles);
	ENT_DECL_GETSET(Velocity);
	ENT_DECL_GETSET(Mins);
	ENT_DECL_GETSET(Maxs);
	ENT_DECL_GET(ActiveMove);
	ENT_DECL_GETSET(DesiredMove);
	ENT_DECL_GETSET(FloorPosition);
	ENT_DECL_GETSET(MotionSka);
	ENT_DECL_GETSET(Accel);
	ENT_DECL_GETSET(SnapTurnAngles);
	ENT_DECL_GETSET(GroundFriction);
	ENT_DECL_GETSET(AirFriction);
	ENT_DECL_GETSET(MaxGroundSpeed);
	ENT_DECL_GETSET(MaxAirSpeed);
	ENT_DECL_GETSET(SplineDistance);
	ENT_DECL_GETSET(MaxSplineBank);
	ENT_DECL_GETSET(SplineBankScale);
	ENT_DECL_GETSET(SplineBankLerp);
	ENT_DECL_GETSET(AutoDecelDistance);
	ENT_DECL_GETSET(MotionScale);
	ENT_DECL_GET(DistanceMoved);
	ENT_DECL_GETSET(MoveType);
	ENT_DECL_GETSET(SolidType);
	ENT_DECL_GETSET(OccupantType);
	ENT_DECL_GETSET(Flags);
	ENT_DECL_GETSET(NextThink);
	ENT_DECL_GETSET(ClassBits);
	ENT_DECL_GETSET(LightingFlags);
	ENT_DECL_GETSET(LightInteractionFlags);

	RAD_DECLARE_GET(zoneTag, ZoneTagRef) { 
		return m_zoneTag.lock(); 
	}

	RAD_DECLARE_GET(id, int) { 
		return m_id; 
	}

	RAD_DECLARE_GET(uid, int) {
		return m_uid;
	}

	RAD_DECLARE_GET(classname, const char*) { 
		return m_classname.c_str; 
	}

	RAD_DECLARE_GET(targetname, const char*) { 
		return m_targetname.empty ? 0 : (const char*)m_targetname.c_str; 
	}

	RAD_DECLARE_GET(world, World*);

	RAD_DECLARE_GET(scripted, bool) { 
		return m_scripted; 
	}

	RAD_DECLARE_GET(models, const DrawModel::Map*) { 
		return &m_models; 
	}

	RAD_DECLARE_GET(ps, PState*) { 
		return &const_cast<Entity*>(this)->m_ps; 
	}

	RAD_DECLARE_GET(lightingFlags, LightingFlags) {
		return m_lightingFlags;
	}

	RAD_DECLARE_SET(lightingFlags, LightingFlags) {
		m_lightingFlags = value;
	}

	RAD_DECLARE_GET(lightInteractionFlags, int) {
		return m_lightInteractionFlags;
	}

	RAD_DECLARE_SET(lightInteractionFlags, int) {
		m_lightInteractionFlags = value;
	}

	RAD_DECLARE_GET(gc, bool) { 
		return m_gc; 
	}

	RAD_DECLARE_SET(gc, bool) { 
		m_gc = value; 
	}

	RAD_DECLARE_GET(bspLeafs, const dBSPLeaf::PtrVec*) {
		return &m_bspLeafs;
	}

	RAD_DECLARE_GET(classbits, int) {
		return m_classbits;
	}

	enum SpawnState {
		S_LuaCreate,
		S_Native,
		S_LuaCoSpawn,
		S_LuaSpawn,
		S_Done,
		S_NativePostSpawn,
		S_PostSpawn,
		S_DonePost
	};

	typedef zone_map<Sound*, SoundRef, ZWorldT>::type SoundMap;

	int m_luaCallbackIdx;
	boost::array<int, kNumLuaCallbackBuckets> m_luaCallbacks;
	TickQueue<Entity> m_tasks;
	TickQueue<Entity> m_scriptTasks;
	DrawModel::Map m_models;
	dBSPLeaf::PtrVec m_bspLeafs;
	IntSet m_areas;
	dBSPLeaf *m_leaf;
	SoundMap m_sounds;
	ZoneTagWRef m_zoneTag;
	String m_targetname;
	String m_classname;
	LightingFlags m_lightingFlags;
	int m_frame;
	int m_spawnState;
	int m_id;
	int m_uid;
	int m_lightInteractionFlags;
	float m_nextLuaThink;
	float m_lastLuaThink;
	float m_nextTick;
	float m_lastTick;
	int m_markFrame;
	int m_classbits;
	bool m_scripted;
	bool m_gc;
};

///////////////////////////////////////////////////////////////////////////////

// Macros for making entities

#define E_DECL_BASE(_classname) \
private: \
	typedef _classname __BaseClassSpawn; \
	bool __baseSpawned

#define E_CONSTRUCT_BASE __baseSpawned(false)

#define E_SPAWN_BASE() \
	if (!__baseSpawned) \
	{ \
		int r = __BaseClassSpawn::Spawn(keys, time, flags); \
		if (r != pkg::SR_Success) return r; \
		__baseSpawned = true; \
	} ((void)0)

#define E_DECL_SPAWN(_api, _classname) \
	namespace spawn { class _api _classname { RADREFLECT_EXPOSE_PRIVATES(_classname) static void *Create(); }; }

#define PRIVATE_E_JOIN2(_var) #_var
#define PRIVATE_E_JOIN(_classname) PRIVATE_E_JOIN2(spawn::_classname)

#define E_EXPORT(_api, _classname) \
	RADREFLECT_BEGIN_CLASS_NAMESPACE(PRIVATE_E_JOIN(_classname), spawn, _classname) \
	RADREFLECT_CLASS_ATTRIBUTE(::world::EntityFactoryAttribute, ()) \
	RADREFLECT_STATICMETHOD(void*, Create) \
	RADREFLECT_END_NAMESPACE(_api, spawn, _classname)

} // world

RADREFLECT_DECLARE(RADENG_API, world::EntityFactoryAttribute)

#include <Runtime/PopPack.h>
