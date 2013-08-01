/*! \file D_ParticleEmitter.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "D_ParticleEmitter.h"

namespace world {

D_ParticleEmitter::Ref D_ParticleEmitter::New(
	const r::ParticleEmitter::Ref &emitter,
	const pkg::Asset::Ref &asset,
	const pkg::Asset::Ref &material
) {
	D_ParticleEmitter::Ref r(new (ZWorld) D_ParticleEmitter(emitter, asset, material));
	return r;
}

D_ParticleEmitter::D_ParticleEmitter(
	const r::ParticleEmitter::Ref &emitter,
	const pkg::Asset::Ref &asset,
	const pkg::Asset::Ref &material
) : m_emitter(emitter), m_asset(asset), m_material(material) {
}

void D_ParticleEmitter::PushElements(lua_State *L) {
	LUART_REGISTER_GETSET(L, Volume);
	LUART_REGISTER_GETSET(L, Spread);
	LUART_REGISTER_GETSET(L, PPS);
	LUART_REGISTER_GETSET(L, MaxParticles);
	lua_pushcfunction(L, lua_Spawn);
	lua_setfield(L, -2, "Spawn");
	lua_pushcfunction(L, lua_Reset);
	lua_setfield(L, -2, "Reset");
}

#define SELF D_ParticleEmitter::Ref self = lua::SharedPtr::Get<D_ParticleEmitter>(L, "D_ParticleEmitter", 1, true)
#define SETFIELD(_type, _field) _type x = lua::Marshal<_type>::Get(L, 2, true); r::ParticleEmitterStyle s = *self->m_emitter->emitterStyle; s._field = x; self->m_emitter->UpdateStyle(s)
#define GETSET(_name, _type, _field) \
	LUART_GET(D_ParticleEmitter, _name, _type, m_emitter->emitterStyle->_field, SELF) \
	LUART_SET_CUSTOM(D_ParticleEmitter, _name, SELF, SETFIELD(_type, _field))

GETSET(Volume, Vec3, volume)
GETSET(Spread, float, spread)
GETSET(PPS, float, pps)
GETSET(MaxParticles, int, maxParticles)

int D_ParticleEmitter::lua_Spawn(lua_State *L) {
	SELF;
	self->m_emitter->Spawn(luaL_checkinteger(L, 2));
	return 0;
}

int D_ParticleEmitter::lua_Reset(lua_State *L) {
	SELF;
	self->m_emitter->Reset();
	return 0;
}

}
