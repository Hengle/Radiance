/*! \file T_ParticleEmitterPrecache.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "T_ParticleEmitterPrecache.h"
#include "../../Renderer/Particles.h"
#include "../../Assets/ParticleParser.h"
#include "../../Assets/ParticleMaterialLoader.h"
#include "D_ParticleEmitter.h"

namespace world {

T_ParticleEmitterPrecache::T_ParticleEmitterPrecache(World *world, const pkg::AssetRef &asset)
: T_Precache(world, asset) {
}

int T_ParticleEmitterPrecache::PushResult(lua_State *L) {
	if (result != pkg::SR_Success)
		return 0;

	r::ParticleEmitterStyle emitterStyle;
	emitterStyle.dir = Vec3(0.f,0.f,1.f);
	emitterStyle.maxParticles = 0;
	emitterStyle.pos = Vec3::Zero;
	emitterStyle.pps = 0.f;
	emitterStyle.spread = 0.f;
	emitterStyle.volume = Vec3::Zero;

	asset::ParticleParser *parser = asset::ParticleParser::Cast(asset);
	if (!parser)
		return 0;

	r::ParticleEmitter::Ref emitter(
		new (ZWorld) r::ParticleEmitter(
			emitterStyle,
			*parser->particleStyle
		)
	);

	asset::ParticleMaterialLoader *loader = asset::ParticleMaterialLoader::Cast(asset);
	
	// P_Trim
	asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Trim
	);

	D_ParticleEmitter::Ref r(D_ParticleEmitter::New(emitter, asset, loader->material));
	if (r) {
		r->Push(L);
		return 1;
	}

	return 0;
}

} // world
