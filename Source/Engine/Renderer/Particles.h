/*! \file Particles.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "Sprites.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace r {

struct ParticleStyle {
// NOTE: this structure is serialized, see ParticleParser.cpp

	// particle mass
	float mass[2];
	// initial velocity
	float vel[2];
	// max velocity
	float maxvel[2];
	// gravity constant
	float cgravity[2];
	// drag
	float cdrag[2];
	// fadein/out/lifetime
	float fadein[2];
	float fadeout[2];
	float lifetime[2];
	// rotation rate in degrees per second
	float rotation[2];
	float rotationRate[2];
	float rotationDrift[2];
	float rotationDriftTime[2];
	// external constant x/y/z force
	float xforce[2];
	float yforce[2];
	float zforce[2];
	// drift
	float xdrift[2];
	float ydrift[2];
	float zdrift[2];
	// drift phase
	float xdriftPhase[2];
	float ydriftPhase[2];
	float zdriftPhase[2];
	// drift cycle times
	float xdriftTime[2];
	float ydriftTime[2];
	float zdriftTime[2];
	// size
	float sizeX[2];
	float sizeY[2];
	float sizeScaleX[2];
	float sizeScaleY[2];
	float sizeScaleXTime[2];
	float sizeScaleYTime[2];
	// color
	float rgba[4];
};

struct ParticleEmitterStyle {
	Vec3 dir;
	Vec3 pos;
	Vec3 volume;
	float spread; // 0 == dir, 1 == 180 from dir (random spread)
	float pps;
	int maxParticles;
};

// A particle emitter manages the physics 
// and lifetime of particles of a particular style.

class ParticleEmitter : public boost::noncopyable {
public:
	typedef boost::shared_ptr<ParticleEmitter> Ref;

	ParticleEmitter();
	ParticleEmitter(
		const ParticleEmitterStyle &emitterStyle,
		const ParticleStyle &particleStyle
	);

	void Init(
		const ParticleEmitterStyle &emitterStyle,
		const ParticleStyle &particleStyle
	);

	void UpdateStyle(const ParticleEmitterStyle &emitterStyle);
	void UpdateStyle(const ParticleStyle &particleStyle);

	SpriteBatch &Batch(int idx);
	void Skin();
	void Tick(float dt);
	void Spawn(int numParticles); // spawn particles
	void Reset(); // unspawns all particles.

	RAD_DECLARE_READONLY_PROPERTY(ParticleEmitter, numParticles, int);
	RAD_DECLARE_READONLY_PROPERTY(ParticleEmitter, numBatches, int);
	RAD_DECLARE_READONLY_PROPERTY(ParticleEmitter, emitterStyle, const ParticleEmitterStyle*);
	RAD_DECLARE_READONLY_PROPERTY(ParticleEmitter, particleStyle, const ParticleStyle*);

private:

	RAD_DECLARE_GET(numParticles, int) {
		return m_numParticles;
	}

	RAD_DECLARE_GET(numBatches, int) {
		return (int)m_batches.size();
	}

	RAD_DECLARE_GET(emitterStyle, ParticleEmitterStyle*) {
		return const_cast<ParticleEmitterStyle*>(&m_emitterStyle);
	}

	RAD_DECLARE_GET(particleStyle, ParticleStyle*) {
		return const_cast<ParticleStyle*>(&m_particleStyle);
	}

	typedef zone_vector<SpriteBatch::Ref, ZRenderT>::type BatchVec;

	class Particle : public Sprite {
	public:
		Particle *next, *prev;
		int batch;

		enum State {
			kFadeIn,
			kMove,
			kFadeOut
		};

		// returns false when particle expires.
		bool Move(float dt, const ParticleStyle &style); 

		Vec3 vel;
		Vec3 orgPos;
		float time;
		float rotationDriftTime;
		float driftTime[3];
		float scaleTime[2];

		float mass;
		float gravity;
		float drag;
		float maxvel;
		float fadein;
		float fadeout;
		float lifetime;
		float rotationRate;
		float rotationDrift;
		float rotationDriftSpeed;
		float force[3];
		float drift[3];
		float driftSpeed[3];
		float scaleSpeed[2];
		float origSize[2];
		float origRotate;
		State state;

		unsigned drift0 : 1;
		unsigned drift1 : 1;
		unsigned drift2 : 1;
		unsigned doDrag : 1;
		unsigned doRotate : 1;
		unsigned doRotateDrift : 1;
		unsigned doScaleX : 1;
		unsigned doScaleY : 1;
		unsigned doMaxVel : 1;

	private:

		void UpdatePos(float dt);
	};

	Particle *SpawnParticle();
	Particle *AllocateParticle();
	void FreeParticle(Particle *p);

	ParticleEmitterStyle m_emitterStyle;
	ParticleStyle m_particleStyle;
	Vec3 m_up;
	Vec3 m_left;
	BatchVec m_batches;
	Particle *m_particles[2];
	float m_particlesToEmit;
	int m_numParticles;
	int m_tickFrame;
	int m_skinFrame;
	int m_allocBatch;

	unsigned m_cone : 1;
	unsigned m_volume : 1;
	unsigned m_velocity : 1;

	RAD_DEBUG_ONLY(bool m_init);
};

} // r

#include <Runtime/PopPack.h>
#include "Particles.inl"
