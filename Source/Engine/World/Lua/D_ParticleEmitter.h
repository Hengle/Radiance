/*! \file D_ParticleEmitter.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include "../../Renderer/Particles.h"
#include "../../Packages/Packages.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_ParticleEmitter : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<D_ParticleEmitter> Ref;

	static Ref New(
		const r::ParticleEmitter::Ref &emitter,
		const pkg::Asset::Ref &asset,
		const pkg::Asset::Ref &material
	);

	RAD_DECLARE_READONLY_PROPERTY(D_ParticleEmitter, particleEmitter, const r::ParticleEmitter::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(D_ParticleEmitter, asset, const pkg::Asset::Ref&);
	RAD_DECLARE_READONLY_PROPERTY(D_ParticleEmitter, material, const pkg::Asset::Ref&);

protected:

	D_ParticleEmitter(
		const r::ParticleEmitter::Ref &emitter,
		const pkg::Asset::Ref &asset,
		const pkg::Asset::Ref &material
	);

	virtual void PushElements(lua_State *L);

private:

	LUART_DECL_GETSET(Dir);
	LUART_DECL_GETSET(Volume);
	LUART_DECL_GETSET(Spread);
	LUART_DECL_GETSET(PPS);
	LUART_DECL_GETSET(MaxParticles);

	RAD_DECLARE_GET(particleEmitter, const r::ParticleEmitter::Ref&) {
		return m_emitter;
	}

	RAD_DECLARE_GET(asset, const pkg::Asset::Ref&) {
		return m_asset;
	}

	RAD_DECLARE_GET(material, const pkg::Asset::Ref&) {
		return m_material;
	}

	static int lua_Spawn(lua_State *L);
	static int lua_Reset(lua_State *L);

	r::ParticleEmitter::Ref m_emitter;
	pkg::Asset::Ref m_asset;
	pkg::Asset::Ref m_material;
};

}

#include <Runtime/PopPack.h>
