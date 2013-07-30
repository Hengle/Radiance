/*! \file Particles.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Particles.h"

namespace r {

ParticleEmitter::ParticleEmitter() : 
m_numParticles(0), 
m_skinFrame(-1), 
m_tickFrame(-1), 
m_allocBatch(-1),
m_particlesToEmit(0.f) {
	RAD_DEBUG_ONLY(m_init = false);
	m_particles[0] = m_particles[1] = 0;
}

ParticleEmitter::ParticleEmitter(
	const ParticleEmitterStyle &emitterStyle,
	const ParticleStyle &particleStyle
) {
	RAD_DEBUG_ONLY(m_init = false);
	Init(emitterStyle, particleStyle);
}

void ParticleEmitter::Init(
	const ParticleEmitterStyle &emitterStyle,
	const ParticleStyle &particleStyle
) {
	RAD_ASSERT(!m_init);
	RAD_DEBUG_ONLY(m_init = true);
		m_skinFrame = -1;
	m_tickFrame = -1;
	m_allocBatch = -1;
	m_numParticles = 0;
	m_particles[0] = m_particles[1] = 0;
	m_allocBatch = 0;
	
	m_emitterStyle.maxParticles = 0;
	UpdateStyle(emitterStyle);
	UpdateStyle(particleStyle);
}

void ParticleEmitter::UpdateStyle(const ParticleEmitterStyle &emitterStyle) {
	RAD_ASSERT(emitterStyle.maxParticles > 0);
	bool reallocate = m_emitterStyle.maxParticles != emitterStyle.maxParticles;

	m_emitterStyle = emitterStyle;
	m_emitterStyle.dir.Normalize();
	m_emitterStyle.dir.FrameVecs(m_up, m_left);

	m_volume = (m_emitterStyle.volume[0] > 0.f) || 
		(m_emitterStyle.volume[1] > 0.f) || (m_emitterStyle.volume[2] > 0.f);

	m_velocity = m_particleStyle.vel[0] > 0.f;
	m_cone = m_velocity && (m_emitterStyle.spread > 0.f);

	if (reallocate) {
		m_tickFrame = -1;

		Reset();
		m_batches.clear();

		int numParticlesToAllocate = emitterStyle.maxParticles;
		while (numParticlesToAllocate) {
			int maxSprites = math::Min<int>(numParticlesToAllocate, SpriteBatch::kMaxSprites);
			SpriteBatch::Ref batch(new (ZRender) SpriteBatch(sizeof(Particle), maxSprites, maxSprites));
			m_batches.push_back(batch);
			numParticlesToAllocate -= maxSprites;
		}
	}
}

void ParticleEmitter::UpdateStyle(const ParticleStyle &particleStyle) {

	m_particleStyle = particleStyle;

	m_particleStyle.sizeScaleX[1] -= m_particleStyle.sizeScaleX[0];
	m_particleStyle.sizeScaleY[1] -= m_particleStyle.sizeScaleY[0];

	if (m_particleStyle.mass[0] <= 0.f)
		m_particleStyle.mass[0] = 1.f;
	if (m_particleStyle.mass[1] <= 0.f)
		m_particleStyle.mass[1] = 1.f;

	for (int i = 0; i < 2; ++i) {
		m_particleStyle.xdriftPhase[i] = math::Clamp(m_particleStyle.xdriftPhase[i], 0.f, 1.f);
		m_particleStyle.ydriftPhase[i] = math::Clamp(m_particleStyle.ydriftPhase[i], 0.f, 1.f);
		m_particleStyle.zdriftPhase[i] = math::Clamp(m_particleStyle.zdriftPhase[i], 0.f, 1.f);
	}

	m_velocity = m_particleStyle.vel[0] > 0.f;
	m_cone = m_velocity && (m_emitterStyle.spread > 0.f);
}

void ParticleEmitter::Tick(float dt) {
	++m_tickFrame;

	m_particlesToEmit += m_emitterStyle.pps * dt;
	if (m_particlesToEmit >= 1.f) {
		float numToEmit = FloorFastFloat(m_particlesToEmit);
		m_particlesToEmit -= numToEmit;
		int iNumToEmit = FloatToInt(numToEmit);

		while (iNumToEmit-- > 0) {
			if (!SpawnParticle())
				break;
		}
	}

	for (Particle *p = m_particles[0]; p;) {
		Particle *n = p->next;
		if (!p->Move(dt, m_particleStyle)) {
			FreeParticle(p);
		}
		p = n;
	}
}

void ParticleEmitter::Skin() {
	if (m_skinFrame == m_tickFrame)
		return;
	m_skinFrame = m_tickFrame;

	for (BatchVec::const_iterator it = m_batches.begin(); it != m_batches.end(); ++it) {
		(*it)->Skin();
	}
}

void ParticleEmitter::Spawn(int numParticles) {
	while (numParticles-- > 0) {
		if (!SpawnParticle())
			break;
	}
}

void ParticleEmitter::Reset() {
	while (m_particles[0]) {
		FreeParticle(m_particles[0]);
	}
	m_particlesToEmit = 0.f;
}

ParticleEmitter::Particle *ParticleEmitter::SpawnParticle() {
	Particle *p = AllocateParticle();
	if (!p)
		return 0;

	p->time = 0.f;
	p->rotationDriftTime = 0.f;
	p->driftTime[0] = math::FastFloatRand(m_particleStyle.xdriftPhase[0], m_particleStyle.xdriftPhase[1]);
	p->driftTime[1] = math::FastFloatRand(m_particleStyle.ydriftPhase[0], m_particleStyle.ydriftPhase[1]);
	p->driftTime[2] = math::FastFloatRand(m_particleStyle.zdriftPhase[0], m_particleStyle.zdriftPhase[1]);
	p->scaleTime[0] = 0.f;
	p->scaleTime[1] = 0.f;
	p->state = Particle::kFadeIn;

	p->mass = math::FastFloatRand(m_particleStyle.mass[0], m_particleStyle.mass[1]);
	p->gravity = math::FastFloatRand(m_particleStyle.cgravity[0], m_particleStyle.cgravity[1]);
	p->drag = math::FastFloatRand(m_particleStyle.cdrag[0], m_particleStyle.cdrag[1]);
	
	p->doDrag = p->drag != 0.f;

	p->maxvel = math::FastFloatRand(m_particleStyle.maxvel[0], m_particleStyle.maxvel[1]);
	p->doMaxVel = p->maxvel > 0.f;

	p->fadein = math::FastFloatRand(m_particleStyle.fadein[0], m_particleStyle.fadein[1]);
	p->fadeout = math::FastFloatRand(m_particleStyle.fadeout[0], m_particleStyle.fadeout[1]);
	p->lifetime = math::FastFloatRand(m_particleStyle.lifetime[0], m_particleStyle.lifetime[1]);

	p->rot = math::FastFloatRand(m_particleStyle.rotation[0], m_particleStyle.rotation[1]);
	p->origRotate = p->rot;

	p->rotationRate = math::FastFloatRand(m_particleStyle.rotationRate[0], m_particleStyle.rotationRate[1]);
	p->rotationDrift = math::FastFloatRand(m_particleStyle.rotationDrift[0], m_particleStyle.rotationDrift[1]);
	p->rotationDrift *= math::Constants<float>::_2_PI();

	p->rotationDriftSpeed = math::FastFloatRand(m_particleStyle.rotationDriftTime[0], m_particleStyle.rotationDriftTime[1]);
	
	if (p->rotationDriftSpeed != 0.f)
		p->rotationDriftSpeed = 1.f/p->rotationDriftSpeed;

	p->doRotate = p->rotationRate != 0.f;
	p->doRotateDrift = p->rotationDrift != 0.f;

	p->force[0] = math::FastFloatRand(m_particleStyle.xforce[0], m_particleStyle.xforce[1]);
	p->force[1] = math::FastFloatRand(m_particleStyle.yforce[0], m_particleStyle.yforce[1]);
	p->force[2] = math::FastFloatRand(m_particleStyle.zforce[0], m_particleStyle.zforce[1]);

	p->drift[0] = math::FastFloatRand(m_particleStyle.xdrift[0], m_particleStyle.xdrift[1]);
	p->drift[1] = math::FastFloatRand(m_particleStyle.ydrift[0], m_particleStyle.ydrift[1]);
	p->drift[2] = math::FastFloatRand(m_particleStyle.zdrift[0], m_particleStyle.zdrift[1]);

	p->driftSpeed[0] = math::FastFloatRand(m_particleStyle.xdriftTime[0], m_particleStyle.xdriftTime[1]);
	p->driftSpeed[1] = math::FastFloatRand(m_particleStyle.ydriftTime[0], m_particleStyle.ydriftTime[1]);
	p->driftSpeed[2] = math::FastFloatRand(m_particleStyle.zdriftTime[0], m_particleStyle.zdriftTime[1]);
	
	if (p->driftSpeed[0] != 0.f)
		p->driftSpeed[0] = 1.f/p->driftSpeed[0];
	if (p->driftSpeed[1] != 0.f)
		p->driftSpeed[1] = 1.f/p->driftSpeed[1];
	if (p->driftSpeed[2] != 0.f)
		p->driftSpeed[2] = 1.f/p->driftSpeed[2];

	p->drift0 = p->drift[0] != 0.f;
	p->drift1 = p->drift[1] != 0.f;
	p->drift2 = p->drift[2] != 0.f;

	p->rgba = Vec4(
		m_particleStyle.rgba[0], 
		m_particleStyle.rgba[1], 
		m_particleStyle.rgba[2], 
		0.f
	);

	p->origSize[0] = math::FastFloatRand(m_particleStyle.sizeX[0], m_particleStyle.sizeX[1]);
	p->origSize[1] = math::FastFloatRand(m_particleStyle.sizeY[0], m_particleStyle.sizeY[1]);

	p->scaleSpeed[0] = math::FastFloatRand(m_particleStyle.sizeScaleXTime[0], m_particleStyle.sizeScaleXTime[1]);
	p->scaleSpeed[1] = math::FastFloatRand(m_particleStyle.sizeScaleYTime[0], m_particleStyle.sizeScaleYTime[1]);

	p->doScaleX = (m_particleStyle.sizeScaleX[1] != 0.f) && (p->scaleSpeed[0] > 0.f);
	p->doScaleY = (m_particleStyle.sizeScaleY[1] != 0.f) && (p->scaleSpeed[1] > 0.f);

	if (p->scaleSpeed[0] != 0.f)
		p->scaleSpeed[0] = 1.f/p->scaleSpeed[0];
	if (p->scaleSpeed[1] != 0.f)
		p->scaleSpeed[1] = 1.f/p->scaleSpeed[1];

	p->size[0] = p->origSize[0] * m_particleStyle.sizeScaleX[0];
	p->size[1] = p->origSize[1] * m_particleStyle.sizeScaleY[0];

	p->orgPos = m_emitterStyle.pos;

	if (m_volume) {
		if (m_emitterStyle.volume[0] > 0.f)
			p->orgPos[0] += -m_emitterStyle.volume[0] + math::FastFloatRand()*m_emitterStyle.volume[0]*2.f;
		if (m_emitterStyle.volume[1] > 0.f)
			p->orgPos[1] += -m_emitterStyle.volume[1] + math::FastFloatRand()*m_emitterStyle.volume[1]*2.f;
		if (m_emitterStyle.volume[2] > 0.f)
			p->orgPos[2] += -m_emitterStyle.volume[2] + math::FastFloatRand()*m_emitterStyle.volume[2]*2.f;
	}

	p->pos = p->orgPos;

	if (m_velocity) {

		p->vel = m_emitterStyle.dir;

		if (m_cone) {
			Vec3 up = m_up * (-m_emitterStyle.spread + math::FastFloatRand()*m_emitterStyle.spread*2.f);
			Vec3 left = m_left * (-m_emitterStyle.spread + math::FastFloatRand()*m_emitterStyle.spread*2.f);
			p->vel += up+left;
			p->vel.Normalize();
		}

		p->vel = p->vel * math::FastFloatRand(m_particleStyle.vel[0], m_particleStyle.vel[1]);

	} else {
		p->vel = Vec3::Zero;
	}

	return p;
}

ParticleEmitter::Particle *ParticleEmitter::AllocateParticle() {
	Particle *p = 0;
	
	const int kSize = m_batches.size();

	for (int i = 0; i < kSize; ++i) {
		int idx = m_allocBatch + i;
		if (idx >= kSize)
			idx -= kSize;

		p = (Particle*)m_batches[idx]->AllocateSprite();
		if (p) {
			m_allocBatch = i;
			p->batch = idx;
			p->prev = 0;
			p->next = m_particles[0];
			if (m_particles[0])
				m_particles[0]->prev = p;
			if (!m_particles[1])
				m_particles[1] = p;
			m_particles[0] = p;
			++m_numParticles;
			break;
		}
	}
	return p;
}

void ParticleEmitter::FreeParticle(Particle *p) {
	if (p->next)
		p->next->prev = p->prev;
	if (p->prev)
		p->prev->next = p->next;
	if (p == m_particles[0])
		m_particles[0] = p->next;
	if (p == m_particles[1])
		m_particles[1] = p->prev;

	m_allocBatch = p->batch;
	m_batches[p->batch]->FreeSprite(p);
	--m_numParticles;
}

///////////////////////////////////////////////////////////////////////////////

bool ParticleEmitter::Particle::Move(float dt, const ParticleStyle &style) {

	time += dt;
	switch (state) {
	case kFadeIn:
		if (time < fadein) {
			rgba[3] = style.rgba[3] * (time/fadein);
		} else {
			rgba[3] = style.rgba[3];
			time = 0.f;
			state = kMove;
		} break;
	case kMove:
		if (time >= lifetime) {
			time = 0.f;
			state = kFadeOut;
		} break;
	case kFadeOut:
		if (time >= fadeout) {
			rgba[3] = 0.f;
			return false;
		} else {
			rgba[3] = style.rgba[3] * (1.f - (time/fadeout));
		} break;
	}

	if (doRotate) {
		origRotate += rotationRate*dt;
		if (origRotate > math::Constants<float>::_2_PI()) {
			origRotate -= math::Constants<float>::_2_PI();
		} else if (origRotate < -math::Constants<float>::_2_PI()) {
			origRotate += math::Constants<float>::_2_PI();
		}
		rot = origRotate;
	}

	if (doRotateDrift) {
		rotationDriftTime += dt*rotationDriftSpeed;
		if (rotationDriftTime >= 1.f)
			rotationDriftTime -= 1.f;
		
		float multiplier = math::FastSin(-math::Constants<float>::PI() + rotationDriftTime*math::Constants<float>::_2_PI());
		rot = origRotate + multiplier*rotationDrift;
	}

	if (doScaleX) {
		scaleTime[0] += dt*scaleSpeed[0];
		if (scaleTime[0] >= 1.f)
			scaleTime[0] -= 1.f;
		float multiplier = math::FastSin(-math::Constants<float>::PI() + scaleTime[0]*math::Constants<float>::_2_PI());
		size[0] = origSize[0] * (style.sizeScaleX[0] + math::Abs(multiplier)*style.sizeScaleX[1]);
	}

	if (doScaleY) {
		scaleTime[1] += dt*scaleSpeed[1];
		if (scaleTime[1] >= 1.f)
			scaleTime[1] -= 1.f;
		float multiplier = math::FastSin(-math::Constants<float>::PI() + scaleTime[1]*math::Constants<float>::_2_PI());
		size[1] = origSize[1] * (style.sizeScaleY[0] + math::Abs(multiplier)*style.sizeScaleY[1]);
	}

	UpdatePos(dt);

	return true;
}

void ParticleEmitter::Particle::UpdatePos(float dt) {

	Vec3 force(this->force[0], this->force[1], this->force[2]-gravity);
	
	if (doDrag)
		force -= vel*drag;
	
	vel += (force/mass)*dt;

	if (doMaxVel) {
		float m = vel.Normalize();
		if (m > maxvel)
			m = maxvel;
		vel = vel*m;
	}

	orgPos += vel*dt;

	if (drift0||drift1||drift2) {

		if (drift0) {
			driftTime[0] += dt*driftSpeed[0];
			if (driftTime[0] >= 1.f)
				driftTime[0] -= 1.f;

			float multiplier = math::FastSin(-math::Constants<float>::PI() + driftTime[0]*math::Constants<float>::_2_PI());
			pos[0] = orgPos[0] + drift[0]*multiplier;
		} else {
			pos[0] = orgPos[0];
		}

		if (drift1) {
			driftTime[1] += dt*driftSpeed[1];
			if (driftTime[1] >= 1.f)
				driftTime[1] -= 1.f;

			float multiplier = math::FastSin(-math::Constants<float>::PI() + driftTime[1]*math::Constants<float>::_2_PI());
			pos[1] = orgPos[1] + drift[1]*multiplier;
		} else {
			pos[1] = orgPos[1];
		}

		if (drift2) {
			driftTime[2] += dt*driftSpeed[2];
			if (driftTime[2] >= 1.f)
				driftTime[2] -= 1.f;

			float multiplier = math::FastSin(-math::Constants<float>::PI() + driftTime[2]*math::Constants<float>::_2_PI());
			pos[2] = orgPos[2] + drift[2]*multiplier;
		} else {
			pos[2] = orgPos[2];
		}

	} else {
		pos = orgPos;
	}
}

} // r
