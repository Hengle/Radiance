// E_ViewController.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "E_ViewController.h"
#include "../World.h"
#include "../../Game/GameCVars.h"
#include <algorithm>

namespace world {

void E_ViewController::Blend::Init(float _in, float _out, float _hold) {
	in[0] = 0.f;
	in[1] = _in;
	out[0] = 0.f;
	out[1] = _out;
	hold[0] = 0.f;
	hold[1] = _hold;
	
	if (_in > 0.f) {
		step = kStep_In;
		frac = 0.f;
	} else if (_hold != 0.f) {
		frac = 1.f;
		step = kStep_Hold;
	} else if (_out > 0.f) {
		frac = 1.f;
		step = kStep_Out;
	} else {
		frac = 0.f;
		step = kStep_Done;
	}
}

void E_ViewController::Blend::FadeIn(float time) {
	in[0] = frac * time;
	in[1] = time;
	out[1] = 0.f;
	hold[1] = -1.f;
	step = kStep_In;
}

void E_ViewController::Blend::FadeOut(float time) {
	out[0] = (1.f - frac) * time;
	out[1] = time;
	step = kStep_Out;
}

void E_ViewController::Blend::Tick(float dt) {
	if (step == kStep_In) {
		in[0] += dt;
		if (in[0] >= in[1]) {
			frac = 1.f;
			if (hold[1] == 0.f) {
				if (out[1] <= 0.f) {
					frac = 0.f;
					step = kStep_Done;
				} else {
					step = kStep_Out;
				}
			} else {
				step = kStep_Hold;
			}
		} else {
			frac = in[0] / in[1];
		}
	} else if (step == kStep_Hold) {
		if (hold[1] > 0.f) {
			hold[0] += dt;
			if (hold[0] >= hold[1]) {
				if (out[1] <= 0.f) {
					frac = 0.f;
					step = kStep_Done;
				} else {
					step = kStep_Out;
				}
			}
		}
	} else if (step == kStep_Out) {
		out[0] += dt;
		if (out[0] >= out[1]) {
			frac = 0.f;
			step = kStep_Done;
		} else {
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
) {
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
) {
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();) {
			List::iterator next = it; ++next;

			VecAnim &x = *it;
			x.m_blend.Tick(dt);
			if (x.m_blend.step == Blend::kStep_Done)
				list.erase(it);
			
			it = next;
		}
	}

	if (list.empty()) // none left
		return targetPos;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block) {
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

	for(;;) {
		VecAnim &x = *it;

		if (it == block) {
			pos = x.Tick(
				dt,
				targetPos,
				targetAngles,
				&roll,
				sync
			);
		} else {
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
) {
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
	if (m_aspeed[1] > 0.f && !sync) {
		lerp = math::Clamp(m_aspeed[1]*dt, 0.f, 0.9999f);
		curAngles = LerpAngles(curAngles, angles, lerp);
	} else {
		curAngles = angles;
	}

	if (roll)
		*roll = curAngles[0];

	Vec3 vec = ForwardFromAngles(curAngles);

	if (m_dspeed[1] > 0.f && !sync) {
		// apply target pos frac lerp
		lerp = math::Clamp(m_dspeed[1]*dt, 0.f, 0.9999f);
		curPos = math::Lerp(curPos, targetPos, lerp);
	} else {
		curPos = targetPos;
	}

	return curPos + vec*distance;
}

Vec3 E_ViewController::Sway::Tick(List &list, float dt, const Vec3 &fwd) {
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();) {
			List::iterator next = it; ++next;

			Sway &x = *it;
			x.blend.Tick(dt);
			if (x.blend.step == Blend::kStep_Done)
				list.erase(it);
			
			it = next;
		}
	}

	if (list.empty()) // none left
		return Vec3::Zero;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block) {
		const Sway &x = *block;
		if (x.blend.frac == 1.f)
			break;
	}

	if (block == list.end())
		--block;

	// blend from blocking value forward.
	Vec3 pos;

	List::iterator it = block;

	for(;;) {
		Sway &x = *it;

		if (it == block) {
			pos = x.Tick(
				dt,
				fwd
			);

			pos *= x.blend.frac; // fade-in
		} else {
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

Vec3 E_ViewController::Sway::Tick(float dt, const Vec3 &fwd) {
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
	
	fwd.FrameVecs(up, left);
	Vec3 shift = Mat4::Rotation(q) * left;

	Vec3 x = (left * scale[0]) * left.Dot(shift);
	Vec3 y = (up * scale[1]) * up.Dot(shift);

	shift = x + y;

	return shift * d;
}

float E_ViewController::FOV::Tick(List &list, float dt, float distance) {
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();) {
			List::iterator next = it; ++next;

			FOV &x = *it;
			x.blend.Tick(dt);
			if (x.blend.step == Blend::kStep_Done)
				list.erase(it);
			
			it = next;
		}
	}

	if (list.empty()) // none left
		return 90.f;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block) {
		const FOV &x = *block;
		if (x.blend.frac == 1.f)
			break;
	}

	if (block == list.end())
		--block;

	// blend from blocking value forward.
	float fov;

	List::iterator it = block;

	for(;;) {
		FOV &x = *it;

		if (it == block) {
			fov = x.Tick(dt, distance);
		} else {
			float blendFov = x.Tick(dt, distance);
			fov = math::Lerp(fov, blendFov, x.blend.frac);
		}

		if (it == list.begin())
			break;
		--it;
	}

	return fov;
}

float E_ViewController::FOV::Tick(float dt, float distance) {
	if (freq == 0.f)
		freq = 1.f;

	time += dt;

	if (fov[0] != 0.f) {
		// not distance controlled
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

int E_ViewController::LookTarget::s_nextId(0);

Vec3 E_ViewController::LookTarget::Tick(
	List &list,
	const Vec3 &pos, 
	const Vec3 &fwd,
	const Vec3 &targetFwd,
	float dt
) {
	// Tick / Expire blends
	{
		for (List::iterator it = list.begin(); it != list.end();) {
			List::iterator next = it; ++next;

			LookTarget &x = *it;

			Vec3 tfwd = x.target - pos;
			tfwd.Normalize();

			float d = tfwd.Dot(targetFwd);
			bool valid = (x.dotCull > 0.f) ? (d >= x.dotCull) : true;

			if (!x.init) {
				x.init = true;
				x.valid = valid;
				if (valid) {
					x.band.Init(0.f, 0.f, -1.f);
				} else {
					x.band.Init(0.f, 0.f, 0.f);
				}
			} else {
				if (x.valid != valid) {
					x.valid = valid;
					if (valid) {
						x.band.FadeIn(1.f);
					} else {
						if (d < 0.f) {
							x.band.Init(0.f, 0.f, 0.f);
						} else {
							x.band.FadeOut(1.f);
						}
					}
				}
			}

			x.band.Tick(dt);
			x.blend.Tick(dt);
			if ((x.blend.step == Blend::kStep_Done) && (x.frac < 0.001f))
				list.erase(it);
			it = next;
		}
	}

	if (list.empty()) // none left
		return fwd;

	// find blocking value
	List::iterator block = list.begin();

	for (; block != list.end(); ++block) {
		const LookTarget &x = *block;

		float smooth = ((x.blend.step == Blend::kStep_In) || (x.blend.step == Blend::kStep_Hold)) ?
			x.smooth[0] : x.smooth[1];

		float frac = 0.f;

		if (smooth <= 0.f) {
			frac = x.blend.frac*x.band.frac;
		} else {
			float z = math::Clamp(smooth*dt, 0.f, 0.99999f);
			frac = math::Lerp(x.frac, x.blend.frac*x.band.frac, z);
		}

		if ((frac*x.weight) >= 0.999f)
			break;
	}

	if (block == list.end())
		--block;

	// blend from blocking value forward.
	Vec3 nfwd(fwd);
	
	List::iterator it = block;

	for(;;) {
		LookTarget &x = *it;

		float smooth = ((x.blend.step == Blend::kStep_In) || (x.blend.step == Blend::kStep_Hold)) ?
			x.smooth[0] : x.smooth[1];

		if (smooth <= 0.f) {
			x.frac = x.blend.frac*x.band.frac;
		} else {
			float z = math::Clamp(smooth*dt, 0.f, 0.99999f);
			x.frac = math::Lerp(x.frac, x.blend.frac*x.band.frac, z);
		}

		if (x.frac > 0.f) {
			Vec3 tfwd = x.target - pos;
			tfwd.Normalize();
			nfwd = math::Lerp(nfwd, tfwd, x.frac*x.weight);
			nfwd.Normalize();
		}

		if (it == list.begin())
			break;
		--it;
	}

	return nfwd;
}


E_ViewController::E_ViewController() : 
E_CONSTRUCT_BASE,
m_mode(kMode_Fixed),
m_pos(Vec3::Zero),
m_angles(Vec3::Zero),
m_targetPos(Vec3::Zero),
m_targetLook(Vec3::Zero),
m_sync(true) {
	m_blendTime[0] = m_blendTime[1] = 0.f;
	m_fovShift[0] = m_fovShift[1] = m_fovShift[2] = m_fovShift[3] = 0.f;
}

E_ViewController::~E_ViewController() {
}

int E_ViewController::Spawn(
	const Keys &keys,
	const xtime::TimeSlice &time,
	int flags
) {
	E_SPAWN_BASE();
	return pkg::SR_Success;
}

void E_ViewController::Tick(
	int frame,
	float dt, 
	const xtime::TimeSlice &time
) {
	Entity::Ref target = m_target.lock();
	if (!target || m_mode == kMode_Fixed) {
		TickFixedMode(frame, dt, target);
		return;
	}

	switch (m_mode) {
	case kMode_Distance:
		TickDistanceMode(frame, dt, target);
		break;
	case kMode_Rail:
		TickRailMode(frame, dt, target);
		break;
	}
}

void E_ViewController::TickFixedMode(int frame, float dt, const Entity::Ref &target) {
	if (!world->cvars->r_fly.value) {
		Vec3 fwd = Mat4::Rotation(QuatFromAngles(m_angles)) * Vec3(1, 0, 0);
		world->camera->pos = m_pos + Sway::Tick(m_sways, dt, fwd);
		world->camera->angles = m_angles;
		world->camera->fov = TickFOV(frame, dt, 0.f);;
	}
}

void E_ViewController::TickDistanceMode(int frame, float dt, const Entity::Ref &target) {
	float roll;

	Vec3 mPos = VecAnim::Tick(
		m_anims[kTargetMode_Distance],
		dt,
		target->ps->cameraPos,
		target->ps->worldAngles,
		&roll,
		m_sync
	);

	Vec3 mLook = VecAnim::Tick(
		m_anims[kTargetMode_Look],
		dt,
		target->ps->cameraPos,
		target->ps->worldAngles,
		0,
		m_sync
	);

	Vec3 lookVec = mLook-mPos;
	Vec3 fwd(lookVec);
	fwd.Normalize();

	fwd = LookTarget::Tick(m_looks, mPos, fwd, fwd, dt);

	Vec3 mAngles = LookAngles(fwd);
	mAngles[0] = roll;

	if (m_blendTime[1] > 0.f) {
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
		
	float fov = TickFOV(frame, dt, lookVec.Magnitude());
	
	if (!world->cvars->r_fly.value) {
		world->camera->pos = mPos;
		world->camera->angles = mAngles;
		world->camera->fov = fov;
		world->camera->quatMode = false;
	}

	m_sync = false;
}

void E_ViewController::TickRailMode(int frame, float dt, const Entity::Ref &target) {

	if (m_sync) {
		m_rail.tm = 0;
	}

	Vec3 vTarget = VecAnim::Tick(
		m_anims[kTargetMode_Look],
		dt,
		target->ps->cameraPos,
		target->ps->worldAngles,
		0,
		m_sync
	);

	Vec3 targetFwd = ForwardFromAngles(target->ps->worldAngles);
	
	UpdateRailTarget(target->ps->cameraPos, targetFwd);
	if (!m_rail.tm)
		return;

	if (m_sync) {
		m_rail.pos = m_rail.tm->t;
		m_rail.rot = m_rail.tm->r;
		m_rail.fov = m_rail.tm->fov;
	}

#if defined(WORLD_DEBUG_DRAW)
	{
		BBox bounds(
			vTarget - Vec3(8, 8, 8),
			vTarget + Vec3(8, 8, 8)
		);
		world->draw->DebugAddViewControllerBBox(bounds);
	}
#endif

	Vec3 fwd = vTarget - m_rail.pos;
	float distance = fwd.Normalize();

	if (!m_sync) {
		// large change?
		Vec3 v = m_rail.tm->t - m_rail.pos;
		
		if ((m_rail.fwd.Dot(fwd) < 0.1f) || (v.Magnitude() > 220.f)) {
			// snap
			m_rail.pos = m_rail.tm->t;
			m_rail.rot = m_rail.tm->r;
			m_rail.fov = m_rail.tm->fov;
			fwd = vTarget - m_rail.pos;
			distance = fwd.Normalize();
			m_rail.fwd = fwd;
		}
	}

	if (m_sync) {
		m_rail.fwd = fwd;
	} else {
		// blend to new pos/look
		if (m_rail.trackLag > 0.f) {
			float lerp = math::Clamp(m_rail.trackLag*dt, 0.f, 0.9999f);
			m_rail.pos = math::Lerp(m_rail.pos, m_rail.tm->t, lerp);
			m_rail.rot = math::Slerp(m_rail.rot, m_rail.tm->r, lerp);
			m_rail.fov = math::Lerp(m_rail.fov, m_rail.tm->fov, lerp);
		} else {
			m_rail.pos = m_rail.tm->t;
			m_rail.rot = m_rail.tm->r;
			m_rail.fov = m_rail.tm->fov;
		}

		if (m_rail.turnLag > 0.f) {
			float lerp = math::Clamp(m_rail.turnLag*dt, 0.f, 0.9999f);
			m_rail.fwd = math::Lerp(m_rail.fwd, fwd, lerp);
		} else {
			m_rail.fwd = fwd;
		}
	}

	float fov = m_rail.fov;

	if (m_rail.cinematicFOV) {
		fov += TickFOV(frame, dt, -1.f);
	} else {
		fov = TickFOV(frame, dt, distance);
	}

	// special case (unrestricted camera)
	if (m_rail.clamp[1] >= 180.f && m_rail.clamp[2] >= 180.f) {
		m_rail.lookFwd = LookTarget::Tick(m_looks, m_rail.pos, m_rail.fwd, fwd, dt);
		Vec3 pos = m_rail.pos + Sway::Tick(m_sways, dt, m_rail.lookFwd);

		Vec3 angles = LookAngles(m_rail.lookFwd);
		Vec3 camAngles = AnglesFromQuat(m_rail.rot);
		angles[0] = camAngles[0]; // always bank
//		COut(C_Debug) << "Fwd: " << m_rail.lookFwd[0] << ", " << m_rail.lookFwd[1] << ", " << m_rail.lookFwd[2] << std::endl;
//		COut(C_Debug) << "Angles: " << angles[0] << ", " << angles[1] << ", " << angles[2] << std::endl;
//		COut(C_Debug) << "Pos (" << pos[0] << ", " << pos[1] << ", " << pos[2] << ") Angles (" << camAngles[0] << ", " << camAngles[1] << ", " << camAngles[2] << ")" << std::endl;
		if (!world->cvars->r_fly.value) {
			world->camera->pos = pos;
			world->camera->angles = angles;
			world->camera->fov = fov;
			world->camera->quatMode = false;
		}
	// special case (fully restricted camera)
	} else if (m_rail.clamp[1] == 0.f && m_rail.clamp[2] == 0.f) {
		Vec3 qFwd = ForwardFromQuat(m_rail.rot);

		m_rail.lookFwd = LookTarget::Tick(m_looks, m_rail.pos, qFwd, qFwd, dt);
		Vec3 pos = m_rail.pos + Sway::Tick(m_sways, dt, m_rail.lookFwd);

		Vec3 fAngles = LookAngles(m_rail.lookFwd);
		fAngles[0] = AnglesFromQuat(m_rail.rot)[0]; // bank

		if (!world->cvars->r_fly.value) {
			world->camera->pos = pos;
			world->camera->fov = fov;
			world->camera->angles = fAngles;
			world->camera->quatMode = false;
		}
	} else {
		// we can drift +/- clamp angles from the cinematic camera orientation.
		Vec3 qAngles = LookAngles(ForwardFromQuat(m_rail.rot));
		Vec3 fAngles = LookAngles(m_rail.fwd);
		Vec3 deltaAngles = DeltaAngles(qAngles, fAngles);

		for (int i = 1; i < 3; ++i) {
			// shortest path.
			float d = deltaAngles[i];

			if (d > m_rail.clamp[i])
				fAngles[i] = qAngles[i] + m_rail.clamp[i];
			if (d < -m_rail.clamp[i])
				fAngles[i] = qAngles[i] - m_rail.clamp[i];

			if (fAngles[i] < 0.f)
				fAngles[i] += 360.f;
			if (fAngles[i] >= 360.f)
				fAngles[i] -= 360.f;
		}

		Vec3 clampFwd = ForwardFromAngles(fAngles);

		// blend look targets
		m_rail.lookFwd = LookTarget::Tick(m_looks, m_rail.pos, clampFwd, clampFwd, dt);
		Vec3 pos = m_rail.pos + Sway::Tick(m_sways, dt, m_rail.lookFwd);

		Vec3 angles = LookAngles(m_rail.lookFwd);
		angles[0] = AnglesFromQuat(m_rail.rot)[0]; // bank

		if (!world->cvars->r_fly.value) {
			world->camera->pos = pos;
			world->camera->angles = angles;
			world->camera->fov = fov;
			world->camera->quatMode = false;
		}
	}

	m_sync = false;
}

void E_ViewController::ClassifyRail(const Vec3 &target, const Vec3 &targetFwd) {

	// classify distance gradiants as +/- (towards or away from target)
	const bsp_file::BSPFile *bspFile = world->bspFile;

	for (S32 i = 0; i < m_rail.track->numTMs; ++i) {
		const bsp_file::BSPCameraTM *tm = bspFile->CameraTMs() + m_rail.track->firstTM + i;

		Vec3 v = target - tm->t;
				
		float d = v.MagnitudeSquared();
		float dd = m_rail.distanceSq - d;
		float abs = math::Abs(dd);

		v.Normalize();

		m_rail.metrics[i].d = d;
		m_rail.metrics[i].dd = dd;
		m_rail.metrics[i].abs = abs;
		m_rail.metrics[i].dot = targetFwd.Dot(v);

		m_rail.metrics[i].absPolarity = (dd <= 0.f) ? kPolarity_Plus : kPolarity_Minus;

		if (i > 0) {
			if (m_rail.metrics[i].d > m_rail.metrics[i-1].d) {
				m_rail.metrics[i].relPolarity = kPolarity_Minus;
			} else if (m_rail.metrics[i].d < m_rail.metrics[i-1].d) {
				m_rail.metrics[i].relPolarity = kPolarity_Plus;
			} else {
				m_rail.metrics[i].relPolarity = kPolarity_Zero;
			}

			if (i == 1) {
				m_rail.metrics[0].relPolarity = m_rail.metrics[1].relPolarity;
			}
		} else {
			m_rail.metrics[i].relPolarity = kPolarity_Zero;
		}
	}
}

int E_ViewController::UpdateCameraTM(
	const Vec3 &target, 
	const Vec3 &targetFwd,
	int curIdx,
	bool behind
) {
	const bsp_file::BSPFile *bspFile = world->bspFile;
	const float kFrontAngle = 0.0f;
	
	if (curIdx >= 0) {
		if (behind && (m_rail.metrics[curIdx].dot < kFrontAngle))
			curIdx = -1;
	}

	if (curIdx < 0) {

		if (behind) {
			// behind camera does not respect the startMode, we want the best point behind the player period

			// algorithm:
			// find the point that is closest to the player.
			// move pos->start, and pos->end, looking for a point that is behind and
			// fits the best
			int closest = -1;
			float closestDist = std::numeric_limits<float>::max();


			for (S32 i = 0; i < m_rail.track->numTMs; ++i) {
				if (m_rail.metrics[i].d < closestDist) {
					closest = i;
					closestDist = m_rail.metrics[i].d;
				}
			}

			RAD_ASSERT(closest != -1);

			// try moving forward, everything should lead away from the player
			// (pos->end)

			int best = -1;
			int best2 = -1;
			float bestDot = -1.f;

			for (S32 i = closest+1; i < m_rail.track->numTMs; ++i) {
				if (m_rail.metrics[i].relPolarity != kPolarity_Minus)
					break;
				if (m_rail.metrics[i].dd < 0.f)
					break; // distance is a hard limit here
				if (m_rail.metrics[i].absPolarity != kPolarity_Minus)
					break;
				if (m_rail.metrics[i].dot > bestDot) {
					best2 = i;
					bestDot = m_rail.metrics[i].dot;
				}
				if (m_rail.metrics[i].dot < kFrontAngle)
					continue;
				best = i;
			}

			if (best == -1) { // found no valid points going in this direction, try pos->start
						
				for (S32 i = closest-1; i >= 0; --i) {
					if (m_rail.metrics[i].relPolarity != kPolarity_Plus) // NOTE: inverted polarity (moving backwards)
						break;
					if (m_rail.metrics[i].dd < 0.f)
						break; // distance is a hard limit here
					if (m_rail.metrics[i].absPolarity != kPolarity_Minus)
						break;
					if (m_rail.metrics[i].dot > bestDot) {
						best2 = i;
						bestDot = m_rail.metrics[i].dot;
					}
					if (m_rail.metrics[i].dot < kFrontAngle)
						continue;
					best = i;
				}

				if (best < 0)
					best = best2;
				if (best < 0)
					best = closest;
			}

			return best;
		}

		// in both cases, we are looking for a polarity switch from Plus to Minus or Minus to Plus (we stop previous)

		if (m_rail.startMode == kRailStart_End) {
			// from start of track, find the point closest to the target
			// before a polarity change.
			// NOTE: polarity is reversed because we are looping through the track backwards
			// (i.e.) points moving away going front->back are moving towards when going back->front

			int best = m_rail.track->numTMs-1;
			const Polarity kPolarity = m_rail.metrics[best].absPolarity;
						
			for (S32 i = m_rail.track->numTMs-2; i >= 0; --i) {
				if (m_rail.metrics[i].absPolarity != kPolarity)
					break;
				
				best = i;
			}

			return best;

		}
		
		// from start of track, find first polarity change

		int best = 0;
		const Polarity kPolarity = m_rail.metrics[best].absPolarity;

		
		for (S32 i = 1; i < m_rail.track->numTMs; ++i) {
			if (m_rail.metrics[i].absPolarity != kPolarity) // transition
				break;
						
			best = i;
		}

		return best;
	}
	
	// do we even need to move (max distance only camera)?
	if (!m_rail.strict) {
		if (m_rail.metrics[curIdx].dd >= 0.f)
			return curIdx; // nope
	}

	// default: always move towards distance, never go closer
	Polarity plus = kPolarity_Plus;
	Polarity minus = kPolarity_Minus;

	if (m_rail.strict && (m_rail.metrics[curIdx].dd >= 0.f)) {
		// we are too close to the target, move away
		plus = kPolarity_Minus;
		minus = kPolarity_Plus;
	}

	// try and follow the gradiant to a better spot.
	int best = curIdx;
	int best2 = curIdx;
	float bestDot = -1.f;

	for (int i = curIdx+1; i < m_rail.track->numTMs; ++i) {
		if (m_rail.metrics[i].relPolarity == minus) // not moving towards target
			break;
		if (m_rail.metrics[i].absPolarity != plus)
			break; // we've moved too close (cam doesn't go closer than specific distance)
		if (m_rail.metrics[i].relPolarity == plus) {
			if (behind && (m_rail.metrics[i].dot > bestDot)) {
				bestDot = m_rail.metrics[i].dot;
				best2 = i;
			}
			if (behind && (m_rail.metrics[i].dot < kFrontAngle))
				continue;
			best = i;
		}
	}

	if (best != curIdx)
		return best;
	
	// try gradiant going backwards
	for (int i = curIdx-1; i >= 0; --i) {
		const Polarity kRelPolarity = ReversePolarity(m_rail.metrics[i].relPolarity);
		if (kRelPolarity == minus) // not moving towards target
			break;
		if (m_rail.metrics[i].absPolarity != plus)
			break; // we've moved too close (cam doesn't go closer than specific distance)
		if (kRelPolarity == plus) {
			if (behind && (m_rail.metrics[i].dot > bestDot)) {
				bestDot = m_rail.metrics[i].dot;
				best2 = i;
			}
			if (behind && (m_rail.metrics[i].dot < kFrontAngle))
				continue;
			best = i;
		}
	}

	if (best == curIdx)
		best = best2;

	return best;
}


void E_ViewController::UpdateRailTarget(const Vec3 &target, const Vec3 &targetFwd) {

	// generate rail gradiants
	ClassifyRail(target, targetFwd);

	const bsp_file::BSPFile *bspFile = world->bspFile;

	if (m_rail.startMode == kRailStart_Front) {
		const bsp_file::BSPCameraTM *tm = bspFile->CameraTMs() + m_rail.track->firstTM;
		Vec3 t = target - tm->t;
		t.Normalize();

		// who's on front?
		if (targetFwd.Dot(t) < 0.f) {
			m_rail.startMode = kRailStart_Begin;
		} else {
			m_rail.startMode = kRailStart_End;
		}
	} else if (m_rail.startMode == kRailStart_Back) {
		const bsp_file::BSPCameraTM *tm = bspFile->CameraTMs() + m_rail.track->firstTM;
		Vec3 t = target - tm->t;
		t.Normalize();

		// who's on back?
		if (targetFwd.Dot(t) > 0.f) {
			m_rail.startMode = kRailStart_Begin;
		} else {
			m_rail.startMode = kRailStart_End;
		}
	}

	int curIdx = -1;

	if (m_rail.tm) {
		curIdx = (int)(m_rail.tm-bspFile->CameraTMs() - m_rail.track->firstTM);
	}

	curIdx = UpdateCameraTM(target, targetFwd, curIdx, m_rail.stayBehind >= 0.f);
	if (curIdx < 0)
		curIdx = 0;

	m_rail.tm = bspFile->CameraTMs() + m_rail.track->firstTM + curIdx;
}

float E_ViewController::TickFOV(int frame, float dt, float distance) {
	float fov = m_fovShift[1];
	
	if (m_fovShift[3] > 0.f) {
		m_fovShift[2] += dt;
		if (m_fovShift[2] < m_fovShift[3]) {
			fov = math::Lerp(m_fovShift[0], m_fovShift[1], LerpSin((m_fovShift[2]/m_fovShift[3])*0.5f));
		} else {
			m_fovShift[3] = 0.f;
			fov = m_fovShift[1];
		}
	}
	
	if (distance >= 0.f)
		fov += FOV::Tick(m_fovs, dt, distance);
	fov = math::Clamp(fov, 0.f, 360.f);

	return fov;
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
) {
	RAD_ASSERT(mode < kNumTargetModes);

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

	if (mode == kTargetMode_Distance) {
		m_mode = kMode_Distance;
	}

	m_anims[mode].push_front(anim);
}

void E_ViewController::SetFixedCamera(
	const Vec3 &pos,
	const Vec3 &angles
) {
	m_mode = kMode_Fixed;
	m_pos = pos;
	m_angles = angles;
}

void E_ViewController::SetRailMode(
	const char *cinematicName,
	float distance,
	bool strict,
	float trackingLag,
	float lookAtLag,
	float stayBehind,
	RailStartMode startMode,
	bool useCinematicFOV, // true = uses embedded fov from cinematic
	const Vec3 &angleClamp // how much camera can "turn" from its rail-track to look at target
) {
	if ((m_mode == kMode_Rail) && (m_rail.name == CStr(cinematicName))) {
		m_rail.distance = distance;
		m_rail.distanceSq = distance*distance;
		m_rail.strict = strict;
		m_rail.trackLag = trackingLag;
		m_rail.turnLag = lookAtLag;
		m_rail.stayBehind = stayBehind;
		m_rail.cinematicFOV = useCinematicFOV;
		m_rail.clamp = angleClamp;
		return; // already doing this.
	}

	const bsp_file::BSPCinematic *cinematic = world->cinematics->FindCinematic(cinematicName);
	if (!cinematic)
		return;

	// find camera track to use.
	for (S32 i = 0; i < cinematic->numTriggers; ++i) {
		const bsp_file::BSPCinematicTrigger *trigger = world->bspFile->CinematicTriggers() + cinematic->firstTrigger + i;
		if (trigger->camera < 0)
			continue;
		m_rail.track = world->bspFile->CameraTracks() + trigger->camera;
		m_rail.metrics = (RailMetric*)zone_realloc(ZWorld, m_rail.metrics, sizeof(RailMetric)*m_rail.track->numTMs);
		m_rail.distance = distance;
		m_rail.distanceSq = distance*distance;
		m_rail.strict = strict;
		m_rail.trackLag = trackingLag;
		m_rail.turnLag = lookAtLag;
		m_rail.stayBehind = stayBehind;
		m_rail.startMode = startMode;
		m_rail.cinematicFOV = useCinematicFOV;
		m_rail.clamp = angleClamp;
		m_rail.name = cinematicName;
		m_rail.lastBehindTime = 0.f;
		m_rail.tm = 0;
		// found a camera.
		m_sync = true;
		m_mode = kMode_Rail;
		break;
	}
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
) {
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

void E_ViewController::SetCameraFOV(
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
) {
	FOV fov;

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
	
void E_ViewController::LerpCameraFOVShift(float fov, float time) {
	float curFOV = m_fovShift[1];
	
	if (m_fovShift[3] > 0.f) {
		if (m_fovShift[2] < m_fovShift[3]) {
			curFOV = math::Lerp(m_fovShift[0], m_fovShift[1], LerpSin((m_fovShift[2]/m_fovShift[3])*0.5f));
		}
	}

	m_fovShift[0] = curFOV;
	m_fovShift[1] = fov;
	m_fovShift[2] = 0.f;
	m_fovShift[3] = time;
}

void E_ViewController::BlendToTarget(float time) {
	m_blendTime[0] = 0.f;
	m_blendTime[1] = time;
}

int E_ViewController::BlendToLookTarget(
	const Vec3 &target, 
	float in, 
	float out, 
	float hold, 
	float weight,
	float inSmooth,
	float outSmooth,
	float dotCull
) {
	LookTarget t;
	t.target = target;
	t.blend.Init(in, out, hold);
	t.weight = weight;
	t.frac = 0.f;
	t.dotCull = dotCull;
	t.smooth[0] = inSmooth;
	t.smooth[1] = outSmooth;
	t.id = LookTarget::s_nextId++;
	t.init = false;
	t.valid = false;
	t.band.Init(0.f, 0.f, 0.f);
	m_looks.push_front(t);

	return t.id;
}

void E_ViewController::FadeOutLookTarget(int id, float time) {
	for (LookTarget::List::iterator it = m_looks.begin(); it != m_looks.end(); ++it) {
		LookTarget &t = *it;
		if (t.id == id) {
			if (t.blend.step != Blend::kStep_Done &&
				t.blend.step != Blend::kStep_Out) {
				t.blend.step = Blend::kStep_Out;
				t.blend.out[1] = time;
			}
			break;
		}
	}
}

void E_ViewController::FadeOutLookTargets(float time) {
	if (time > 0.f) {
		for (LookTarget::List::iterator it = m_looks.begin(); it != m_looks.end(); ++it) {
			LookTarget &t = *it;
			if (t.blend.step != Blend::kStep_Done &&
				t.blend.step != Blend::kStep_Out) {
				t.blend.step = Blend::kStep_Out;
				t.blend.out[1] = time;
			}
		}
	} else {
		m_looks.clear();
	}
}

int E_ViewController::lua_SetTargetMode(lua_State *L) {
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

int E_ViewController::lua_SetRailMode(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	const char *name = luaL_checkstring(L, 2);
	float distance = (float)luaL_checknumber(L, 3);
	bool strict = lua_toboolean(L, 4) != 0;
	float trackLag = (float)luaL_checknumber(L, 5);
	float lookLag  = (float)luaL_checknumber(L, 6);
	float stayBehind = (float)luaL_checknumber(L, 7);
	RailStartMode startMode = (RailStartMode)luaL_checkinteger(L, 8);
	bool useFOV = lua_toboolean(L, 9) != 0;
	Vec3 clamp = lua::Marshal<Vec3>::Get(L, 10, true);

	self->SetRailMode(
		name,
		distance,
		strict,
		trackLag,
		lookLag,
		stayBehind,
		startMode,
		useFOV,
		clamp
	);

	return 0;
}

int E_ViewController::lua_SetFixedCamera(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	Vec3 pos = lua::Marshal<Vec3>::Get(L, 2, true);
	Vec3 angles = lua::Marshal<Vec3>::Get(L, 3, true);

	self->SetFixedCamera(pos, angles);

	return 0;
}

int E_ViewController::lua_SetCameraSway(lua_State *L) {
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

int E_ViewController::lua_SetCameraFOV(lua_State *L) {
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

	self->SetCameraFOV(
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
	
int E_ViewController::lua_LerpCameraFOVShift(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	self->LerpCameraFOVShift(
		(float)luaL_checknumber(L, 2),
		(float)luaL_checknumber(L, 3)
	);
							 
	return 0;
}

int E_ViewController::lua_BlendToTarget(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	float time = (float)luaL_checknumber(L, 2);

	self->BlendToTarget(time);

	return 0;
}

int E_ViewController::lua_BlendToLookTarget(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	int id = self->BlendToLookTarget(
		lua::Marshal<Vec3>::Get(L, 2, true),
		(float)luaL_checknumber(L, 3),
		(float)luaL_checknumber(L, 4),
		(float)luaL_checknumber(L, 5),
		(float)luaL_checknumber(L, 6),
		(float)luaL_checknumber(L, 7),
		(float)luaL_checknumber(L, 8),
		(float)luaL_checknumber(L, 9)
	);
	lua_pushinteger(L, id);
	return 1;
}

int E_ViewController::lua_FadeOutLookTarget(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	self->FadeOutLookTarget(
		(int)luaL_checkinteger(L, 2),
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int E_ViewController::lua_FadeOutLookTargets(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	self->FadeOutLookTargets((float)luaL_checknumber(L, 2));
	return 0;
}

int E_ViewController::lua_Sync(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	self->m_sync = true;
	return 0;
}

ENT_GET_WEAK_ENT(E_ViewController, Target, m_target);

int E_ViewController::LUART_SETFN(Target)(lua_State *L) {
	E_ViewController *self = static_cast<E_ViewController*>(WorldLua::EntFramePtr(L, 1, true));
	Entity *ent = WorldLua::EntFramePtr(L, 2, false);
	Entity::Ref target = ent ? ent->shared_from_this() : Entity::Ref();
	
	Entity::Ref curTarget = self->m_target.lock();

	bool changed = (curTarget && !target) || (!curTarget && target) || (target && target.get() != curTarget.get());
	self->m_target = target;
	self->m_sync = self->m_sync || (changed && target);
	return 0;
}

void E_ViewController::PushCallTable(lua_State *L) {
	Entity::PushCallTable(L);
	lua_pushcfunction(L, lua_SetTargetMode);
	lua_setfield(L, -2, "SetTargetMode");
	lua_pushcfunction(L, lua_SetRailMode);
	lua_setfield(L, -2, "SetRailMode");
	lua_pushcfunction(L, lua_SetFixedCamera);
	lua_setfield(L, -2, "SetFixedCamera");
	lua_pushcfunction(L, lua_SetCameraSway);
	lua_setfield(L, -2, "SetCameraSway");
	lua_pushcfunction(L, lua_SetCameraFOV);
	lua_setfield(L, -2, "SetCameraFOV");
	lua_pushcfunction(L, lua_LerpCameraFOVShift);
	lua_setfield(L, -2, "LerpCameraFOVShift");
	lua_pushcfunction(L, lua_BlendToTarget);
	lua_setfield(L, -2, "BlendToTarget");
	lua_pushcfunction(L, lua_BlendToLookTarget);
	lua_setfield(L, -2, "BlendToLookTarget");
	lua_pushcfunction(L, lua_FadeOutLookTarget);
	lua_setfield(L, -2, "FadeOutLookTarget");
	lua_pushcfunction(L, lua_FadeOutLookTargets);
	lua_setfield(L, -2, "FadeOutLookTargets");
	lua_pushcfunction(L, lua_Sync);
	lua_setfield(L, -2, "Sync");
	LUART_REGISTER_GETSET(L, Target);
}

} // world
