// E_ViewController.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "E_ViewController.h"
#include "../World.h"
#include <algorithm>

namespace world {

void E_ViewController::Blend::Init(float _in, float _out, float _hold)
{
	in[0] = 0.f;
	in[1] = _in;
	out[0] = 0.f;
	out[1] = _out;
	hold[0] = 0.f;
	hold[1] = _hold;
	
	if (_in > 0.f)
	{
		step = S_In;
		frac = 0.f;
	}
	else if (_hold != 0.f)
	{
		frac = 1.f;
		step = S_Hold;
	}
	else if (_out > 0.f)
	{
		frac = 1.f;
		step = S_Out;
	}
	else
	{
		frac = 0.f;
		step = S_Done;
	}
}

void E_ViewController::Blend::Tick(float dt)
{
	if (step == S_In)
	{
		in[0] += dt;
		if (in[0] >= in[1])
		{
			frac = 1.f;
			if (hold[1] == 0.f)
			{
				if (out[1] <= 0.f)
				{
					frac = 0.f;
					step = S_Done;
				}
				else
				{
					step = S_Out;
				}
			}
			else
			{
				step = S_Hold;
			}
		}
		else
		{
			frac = in[0] / in[1];
		}
	}
	else if (step == S_Hold)
	{
		if (hold[1] > 0.f)
		{
			hold[0] += dt;
			if (hold[0] >= hold[1])
			{
				if (out[1] <= 0.f)
				{
					frac = 0.f;
					step = S_Done;
				}
				else
				{
					step = S_Out;
				}
			}
		}
	}
	else if (step == S_Out)
	{
		out[0] += dt;
		if (out[0] >= out[1])
		{
			frac = 0.f;
			step = S_Done;
		}
		else
		{
			frac = 1.f - (out[0]/out[1]);
		}
	}
}

void E_ViewController::VecAnim::Start(
	float in, // blend in time
	float out, // blend out time
	float hold, // hold time, 0 for infinite
	float minDistance,
	float maxDistance,
	float distanceSpeed,
	float distanceLag,
	const Vec3 &minAngles,
	const Vec3 &maxAngles,
	float angleSpeed,      
	float angleLag,    
	bool useTargetPitch
)
{
	m_blend.Init(in, out, hold);
	m_distance[0] = minDistance;
	m_distance[1] = maxDistance;
	m_dspeed[0] = distanceSpeed;
	m_dspeed[1] = distanceLag;
	m_angles[0] = minAngles;
	m_angles[1] = maxAngles;
	m_aspeed[0] = angleSpeed;
	m_aspeed[1] = angleLag;
	m_time = 0.f;
	m_pitch = useTargetPitch;
	m_sync = false;
}

Vec3 E_ViewController::VecAnim::Tick(
	List &list,
	float dt,
	const Vec3 &targetPos,
	const Vec3 &targetAngles,
	float *_roll,
	bool sync
)
{
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();)
		{
			List::iterator next = it; ++next;

			VecAnim &x = *it;
			x.m_blend.Tick(dt);
			if (x.m_blend.step == Blend::S_Done)
				list.erase(it);
			
			it = next;
		}
	}

	if (list.empty()) // none left
		return targetPos;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block)
	{
		const VecAnim &x = *block;
		if (x.m_blend.frac == 1.f)
			break;
	}

	if (block == list.end())
		--block;

	// blend from blocking value forward.
	Vec3 pos;
	float roll;

	List::iterator it = block;

	for(;;)
	{
		VecAnim &x = *it;

		if (it == block)
		{
			pos = x.Tick(
				dt,
				targetPos,
				targetAngles,
				&roll,
				sync
			);
		}
		else
		{
			float blendRoll;

			Vec3 blendPos = x.Tick(
				dt,
				targetPos,
				targetAngles,
				&blendRoll,
				sync
			);

			pos = math::Lerp(pos, blendPos, x.m_blend.frac);
			roll = math::Lerp(roll, blendRoll, x.m_blend.frac);
		}

		if (it == list.begin())
			break;
		--it;
	}

	if (_roll)
		*_roll = roll;
	return pos;
}

Vec3 E_ViewController::VecAnim::Tick(
	float dt,
	const Vec3 &targetPos,
	const Vec3 &targetAngles,
	float *roll,
	bool sync
)
{
	sync = sync || !m_sync;
	m_sync = true;

	m_time += dt;
	float lerp = 0.f;

	if (m_dspeed[0] > 0.f)
		lerp = LerpSin(m_time/m_dspeed[0]);

	float distance = math::Lerp(m_distance[0], m_distance[1], lerp);
		
	lerp = 0.f;
	if (m_aspeed[0] > 0.f)
		lerp = LerpSin(m_time/m_aspeed[0]);
	
	Vec3 angles = LerpAngles(m_angles[0], m_angles[1], lerp);
	angles = WrapAngles(angles + targetAngles);

	// apply target angle frac lerp
	if (m_aspeed[1] > 0.f && !sync)
	{
		lerp = math::Clamp(m_aspeed[1]*dt, 0.f, 0.9999f);
		curAngles = LerpAngles(curAngles, angles, lerp);
	}
	else
	{
		curAngles = angles;
	}

	if (roll)
		*roll = curAngles[0];

	Quat q = QuatFromAngles(curAngles);
	Mat4 m = Mat4::Rotation(q);

	Vec3 vec = m * Vec3(1, 0, 0);

	if (m_dspeed[1] > 0.f && !sync)
	{
		// apply target pos frac lerp
		lerp = math::Clamp(m_dspeed[1]*dt, 0.f, 0.9999f);
		curPos = math::Lerp(curPos, targetPos, lerp);
	}
	else
	{
		curPos = targetPos;
	}

	return curPos - vec*distance;
}

Vec3 E_ViewController::Sway::Tick(List &list, float dt, const Vec3 &fwd)
{
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();)
		{
			List::iterator next = it; ++next;

			Sway &x = *it;
			x.blend.Tick(dt);
			if (x.blend.step == Blend::S_Done)
				list.erase(it);
			
			it = next;
		}
	}

	if (list.empty()) // none left
		return Vec3::Zero;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block)
	{
		const Sway &x = *block;
		if (x.blend.frac == 1.f)
			break;
	}

	if (block == list.end())
		--block;

	// blend from blocking value forward.
	Vec3 pos;

	List::iterator it = block;

	for(;;)
	{
		Sway &x = *it;

		if (it == block)
		{
			pos = x.Tick(
				dt,
				fwd
			);

			pos *= x.blend.frac; // fade-in
		}
		else
		{
			Vec3 blendPos = x.Tick(
				dt,
				fwd
			);

			pos = math::Lerp(pos, blendPos, x.blend.frac);
		}

		if (it == list.begin())
			break;
		--it;
	}

	return pos;
}

Vec3 E_ViewController::Sway::Tick(float dt, const Vec3 &fwd)
{
	if (freq == 0.f)
		freq = 1.f;

	time += dt;
	
	float as = aspeed;
	if (as != 0.f)
		as = 1.0f / as;

	angle += math::Mod(as*dt*math::Constants<float>::PI()*2.f, math::Constants<float>::PI()*2.f);

	float d = dist[0]+(dist[1]-dist[0])*LerpSin(time/freq);

	Quat q(fwd, angle);
	Vec3 up, left;
	
	FrameVecs(fwd, up, left);
	Vec3 shift = Mat4::Rotation(q) * left;

	Vec3 x = (left * scale[0]) * left.Dot(shift);
	Vec3 y = (up * scale[1]) * up.Dot(shift);

	shift = x + y;

	return shift * d;
}

float E_ViewController::Fov::Tick(List &list, float dt, float distance)
{
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();)
		{
			List::iterator next = it; ++next;

			Fov &x = *it;
			x.blend.Tick(dt);
			if (x.blend.step == Blend::S_Done)
				list.erase(it);
			
			it = next;
		}
	}

	if (list.empty()) // none left
		return 90.f;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block)
	{
		const Fov &x = *block;
		if (x.blend.frac == 1.f)
			break;
	}

	if (block == list.end())
		--block;

	// blend from blocking value forward.
	float fov;

	List::iterator it = block;

	for(;;)
	{
		Fov &x = *it;

		if (it == block)
		{
			fov = x.Tick(dt, distance);
		}
		else
		{
			float blendFov = x.Tick(dt, distance);
			fov = math::Lerp(fov, blendFov, x.blend.frac);
		}

		if (it == list.begin())
			break;
		--it;
	}

	return fov;
}

float E_ViewController::Fov::Tick(float dt, float distance)
{
	if (freq == 0.f)
		freq = 1.f;

	time += dt;

	if (fov[0] != 0.f) // not distance controlled
	{
		return fov[0] - fov[1] + (fov[2]+fov[1])*LerpSin(time/freq);
	}

	if (dist[0] >= dist[1]) // invalid?
		return dfov[0];

	// distance controlled.
	if (distance < dist[0])
		distance = dist[0];
	if (distance > dist[1])
		distance = dist[1];

	float f = (distance-dist[0])/(dist[1]-dist[0]);
	f = math::Lerp(dfov[0], dfov[1], f);

	f = f - fov[1] + (fov[2]+fov[1])*LerpSin(time/freq);
	return f;
}

E_ViewController::E_ViewController() : 
E_CONSTRUCT_BASE,
m_mode(M_Fixed),
m_pos(Vec3::Zero),
m_angles(Vec3::Zero),
m_targetPos(Vec3::Zero),
m_targetLook(Vec3::Zero),
m_sync(true)
{
	m_blendTime[0] = m_blendTime[1] = 0.f;
	m_fovShift[0] = m_fovShift[1] = m_fovShift[2] = m_fovShift[3] = 0.f;
}

E_ViewController::~E_ViewController()
{
}

int E_ViewController::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
)
{
	E_SPAWN_BASE();
	return pkg::SR_Success;
}

void E_ViewController::Tick(
	int frame,
	float dt, 
	const xtime::TimeSlice &time
)
{
	Entity::Ref target = m_target.lock();
	if (!target || m_mode == M_Fixed)
	{
		Vec3 fwd = Mat4::Rotation(QuatFromAngles(m_angles)) * Vec3(1, 0, 0);
		world->camera->pos = m_pos + Sway::Tick(m_sways, dt, fwd);
		world->camera->angles = m_angles;

		float fov = m_fovShift[1];
	
		if (m_fovShift[3] > 0.f)
		{
			m_fovShift[2] += dt;
			if (m_fovShift[2] < m_fovShift[3])
			{
				fov = math::Lerp(m_fovShift[0], m_fovShift[1], LerpSin((m_fovShift[2]/m_fovShift[3])*0.5f));
			}
			else
			{
				m_fovShift[3] = 0.f;
				fov = m_fovShift[1];
			}
		}
		
		fov += Fov::Tick(m_fovs, dt, 0.f);
		fov = math::Clamp(fov, 0.f, 360.f);

		world->camera->fov = fov;
		return;
	}

	// HACK:
	// ScreenView entities set worldPos but we don't want to track that, we want
	// the "origin".

	float roll;

	Vec3 mPos = VecAnim::Tick(
		m_anims[TM_Distance],
		dt,
		target->ps->cameraPos,
		target->ps->worldAngles,
		&roll,
		m_sync
	);

	Vec3 mLook = VecAnim::Tick(
		m_anims[TM_Look],
		dt,
		target->ps->cameraPos,
		target->ps->worldAngles,
		0,
		m_sync
	);

	Vec3 lookVec = mLook-mPos;
	Vec3 fwd(lookVec);
	fwd.Normalize();
	Vec3 mAngles = LookAngles(fwd);
	mAngles[0] = roll;

	if (m_blendTime[1] > 0.f)
	{
		m_blendTime[0] += dt;
		if (m_blendTime[0] > m_blendTime[1])
			m_blendTime[0] = m_blendTime[1];
		float frac = m_blendTime[0]/m_blendTime[1];
		mPos = math::Lerp(m_pos, mPos, math::Sin(frac*math::Constants<float>::PI_OVER_2()));
		mAngles = LerpAngles(m_angles, mAngles, frac);
		if (m_blendTime[0] >= m_blendTime[1])
			m_blendTime[1] = 0.f; // done lerping
	}

	mPos += Sway::Tick(m_sways, dt, fwd);
		
	float fov = m_fovShift[1];
	
	if (m_fovShift[3] > 0.f)
	{
		m_fovShift[2] += dt;
		if (m_fovShift[2] < m_fovShift[3])
		{
			fov = math::Lerp(m_fovShift[0], m_fovShift[1], LerpSin((m_fovShift[2]/m_fovShift[3])*0.5f));
		}
		else
		{
			m_fovShift[3] = 0.f;
			fov = m_fovShift[1];
		}
	}
	
	fov += Fov::Tick(m_fovs, dt, lookVec.Magnitude());
	fov = math::Clamp(fov, 0.f, 360.f);
	
	world->camera->pos = mPos;
	world->camera->angles = mAngles;
	world->camera->fov = fov;
	world->camera->quatMode = false;
	m_sync = false;
}

void E_ViewController::SetTargetMode(
	TargetMode mode,
	float in, // blend in time
	float out, // blend out time
	float hold, // hold time, 0 for infinite
	float minDistance, // minimum distance
	float maxDistance, // maximum distance
	float distanceSpeed, // speed to lerp between the min/max distance
	float distanceLag, // fraction time blend from current position to target distance
	const Vec3 &minAngles, // Zero angles specifies looking down +X
	const Vec3 &maxAngles,
	float angleSpeed,      // speed to lerp between min/max angles
	float angleLagTime,    // fraction time to blend between current angles and desired angles
	bool useTargetPitch
)
{
	RAD_ASSERT(mode < TM_Max);

	VecAnim anim;
	anim.Start(
		in,
		out,
		hold,
		minDistance,
		maxDistance,
		distanceSpeed,
		distanceLag,
		minAngles,
		maxAngles,
		angleSpeed,
		angleLagTime,
		useTargetPitch
	);

	if (mode == TM_Distance)
		m_mode = M_Target;

	m_anims[mode].push_front(anim);
}

void E_ViewController::SetCamera(
	const Vec3 &pos,
	const Vec3 &angles
)
{
	m_mode = M_Fixed;
	m_pos = pos;
	m_angles = angles;
}

void E_ViewController::SetCameraSway(
	float in, // blend in time
	float out, // blend out time
	float hold, // hold time, 0 for infinite
	float minDistance,
	float maxDistance,
	float frequency,
	float angleSpeed,
	const Vec2 &scale
)
{
	Sway sway;
	sway.blend.Init(in, out, hold);
	sway.time = 0.f;
	sway.dist[0] = minDistance;
	sway.dist[1] = maxDistance;
	sway.freq = frequency;
	sway.aspeed = angleSpeed;
	sway.angle = 0.f;
	sway.scale = scale;

	m_sways.push_front(sway);
}

void E_ViewController::SetCameraFov(
	float in, // blend in time
	float out, // blend out time
	float hold, // hold time, 0 for infinite
	float _fov,
	float plus,
	float minus,
	float frequency,
	float minFov,
	float maxFov,
	float minDistance,
	float maxDistance
)
{
	Fov fov;

	fov.blend.Init(in, out, hold);
	fov.fov[0] = _fov;
	fov.fov[1] = plus;
	fov.fov[2] = minus;
	fov.freq = frequency;
	fov.dfov[0] = minFov;
	fov.dfov[1] = maxFov;
	fov.dist[0] = minDistance;
	fov.dist[1] = maxDistance;
	fov.time = 0.f;

	m_fovs.push_front(fov);
}
	
void E_ViewController::LerpCameraFovShift(float fov, float time)
{
	float curFOV = m_fovShift[1];
	
	if (m_fovShift[3] > 0.f)
	{
		if (m_fovShift[2] < m_fovShift[3])
		{
			curFOV = math::Lerp(m_fovShift[0], m_fovShift[1], LerpSin((m_fovShift[2]/m_fovShift[3])*0.5f));
		}
	}

	m_fovShift[0] = curFOV;
	m_fovShift[1] = fov;
	m_fovShift[2] = 0.f;
	m_fovShift[3] = time;
}

void E_ViewController::BlendToTarget(float time)
{
	m_blendTime[0] = 0.f;
	m_blendTime[1] = time;
}

int E_ViewController::lua_SetTargetMode(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	int mode = (int)luaL_checkinteger(L, 2);
	float in = (float)luaL_checknumber(L, 3);
	float out = (float)luaL_checknumber(L, 4);
	float hold = (float)luaL_checknumber(L, 5);
	float mind = (float)luaL_checknumber(L, 6);
	float maxd = (float)luaL_checknumber(L, 7);
	float dspeed = (float)luaL_checknumber(L, 8);
	float dlag = (float)luaL_checknumber(L, 9);
	Vec3 minA = lua::Marshal<Vec3>::Get(L, 10, true);
	Vec3 maxA = lua::Marshal<Vec3>::Get(L, 11, true);
	float aspeed = (float)luaL_checknumber(L, 12);
	float alag = (float)luaL_checknumber(L, 13);
	bool pitch = lua_toboolean(L, 14) ? true : false;

	self->SetTargetMode(
		(TargetMode)mode,
		in,
		out,
		hold,
		mind,
		maxd,
		dspeed,
		dlag,
		minA,
		maxA,
		aspeed,
		alag,
		pitch
	);

	return 0;
}

int E_ViewController::lua_SetCamera(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	Vec3 pos = lua::Marshal<Vec3>::Get(L, 2, true);
	Vec3 angles = lua::Marshal<Vec3>::Get(L, 3, true);

	self->SetCamera(pos, angles);

	return 0;
}

int E_ViewController::lua_SetCameraSway(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	float in = (float)luaL_checknumber(L, 2);
	float out = (float)luaL_checknumber(L, 3);
	float hold = (float)luaL_checknumber(L, 4);
	float mind = (float)luaL_checknumber(L, 5);
	float maxd = (float)luaL_checknumber(L, 6);
	float freq = (float)luaL_checknumber(L, 7);
	float aspeed = (float)luaL_checknumber(L, 8);
	Vec2 scale = lua::Marshal<Vec2>::Get(L, 9, true);

	self->SetCameraSway(
		in,
		out,
		hold,
		mind,
		maxd,
		freq,
		aspeed,
		scale
	);

	return 0;
}

int E_ViewController::lua_SetCameraFov(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	float in = (float)luaL_checknumber(L, 2);
	float out = (float)luaL_checknumber(L, 3);
	float hold = (float)luaL_checknumber(L, 4);
	float fov = (float)luaL_checknumber(L, 5);
	float plus = (float)luaL_checknumber(L, 6);
	float minus = (float)luaL_checknumber(L, 7);
	float freq = (float)luaL_checknumber(L, 8);
	float minf = (float)luaL_checknumber(L, 9);
	float maxf = (float)luaL_checknumber(L, 10);
	float mind = (float)luaL_checknumber(L, 11);
	float maxd = (float)luaL_checknumber(L, 12);

	self->SetCameraFov(
		in,
		out,
		hold,
		fov,
		plus,
		minus,
		freq,
		minf,
		maxf,
		mind,
		maxd
	);

	return 0;
}
	
int E_ViewController::lua_LerpCameraFovShift(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	self->LerpCameraFovShift(
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3)
	);
							 
	return 0;
}

int E_ViewController::lua_BlendToTarget(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	float time = (float)luaL_checknumber(L, 2);

	self->BlendToTarget(time);

	return 0;
}

int E_ViewController::lua_Sync(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	self->m_sync = true;
	return 0;
}

ENT_GET_WEAK_ENT(E_ViewController, Target, m_target);

int E_ViewController::LUART_SETFN(Target)(lua_State *L)
{
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	Entity *ent = WorldLua::EntFramePtr(L, 2, false);
	Entity::Ref target = ent ? ent->shared_from_this() : Entity::Ref();
	
	Entity::Ref curTarget = self->m_target.lock();

	bool changed = (curTarget && !target) || (!curTarget && target) || (target && target.get() != curTarget.get());
	self->m_target = target;
	self->m_sync = self->m_sync || (changed && target);
	return 0;
}

void E_ViewController::PushCallTable(lua_State *L)
{
	Entity::PushCallTable(L);
	lua_pushcfunction(L, lua_SetTargetMode);
	lua_setfield(L, -2, "SetTargetMode");
	lua_pushcfunction(L, lua_SetCamera);
	lua_setfield(L, -2, "SetCamera");
	lua_pushcfunction(L, lua_SetCameraSway);
	lua_setfield(L, -2, "SetCameraSway");
	lua_pushcfunction(L, lua_SetCameraFov);
	lua_setfield(L, -2, "SetCameraFov");
	lua_pushcfunction(L, lua_LerpCameraFovShift);
	lua_setfield(L, -2, "LerpCameraFovShift");
	lua_pushcfunction(L, lua_BlendToTarget);
	lua_setfield(L, -2, "BlendToTarget");
	lua_pushcfunction(L, lua_Sync);
	lua_setfield(L, -2, "Sync");
	LUART_REGISTER_GETSET(L, Target);
}

} // world
