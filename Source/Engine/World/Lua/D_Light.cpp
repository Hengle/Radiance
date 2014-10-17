/*! \file D_Light.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "D_Light.h"

namespace world {

D_Light::Ref D_Light::New(const Light::Ref &light) {
	return Ref(new (ZWorld) D_Light(light));
}

D_Light::D_Light(const Light::Ref &light) : m_light(light) {
}

D_Light::~D_Light() {
	m_light->Unlink();
}

void D_Light::PushElements(lua_State *L) {
	LUART_REGISTER_GETSET(L, DiffuseColor);
	LUART_REGISTER_GETSET(L, SpecularColor);
	LUART_REGISTER_GETSET(L, Pos);
	LUART_REGISTER_GETSET(L, Radius);
	LUART_REGISTER_GETSET(L, Intensity);
	LUART_REGISTER_GETSET(L, IntensityScale);
	LUART_REGISTER_GETSET(L, ShadowWeight);
	LUART_REGISTER_GETSET(L, Style);
	LUART_REGISTER_GETSET(L, InteractionFlags);
	lua_pushcfunction(L, lua_Link);
	lua_setfield(L, -2, "Link");
	lua_pushcfunction(L, lua_Unlink);
	lua_setfield(L, -2, "Unlink");
	lua_pushcfunction(L, lua_FadeTo);
	lua_setfield(L, -2, "FadeTo");
	lua_pushcfunction(L, lua_AnimateIntensity);
	lua_setfield(L, -2, "AnimateIntensity");
	lua_pushcfunction(L, lua_AnimateDiffuseColor);
	lua_setfield(L, -2, "AnimateDiffuseColor");
	lua_pushcfunction(L, lua_AnimateSpecularColor);
	lua_setfield(L, -2, "AnimateSpecularColor");
}

#define SELF D_Light::Ref self = lua::SharedPtr::Get<D_Light>(L, "D_Light", 1, true)

int D_Light::lua_Link(lua_State *L) {
	SELF;
	self->m_light->Link();
	return 0;
}

int D_Light::lua_Unlink(lua_State *L) {
	SELF;
	self->m_light->Unlink();
	return 0;
}

int D_Light::lua_FadeTo(lua_State *L) {
	SELF;
	self->m_light->FadeTo(
		(float)luaL_checknumber(L, 2), 
		(float)luaL_checknumber(L, 3)
	);
	return 0;
}

int D_Light::lua_AnimateIntensity(lua_State *L) {
	SELF;
	Light::IntensityStep::Vec vec;
	ParseIntensitySteps(L, 2, vec);
	self->m_light->AnimateIntensity(
		vec,
		!!lua_toboolean(L, 3)
	);
	return 0;
}

int D_Light::lua_AnimateDiffuseColor(lua_State *L) {
	SELF;
	Light::ColorStep::Vec vec;
	ParseColorSteps(L, 2, vec);
	self->m_light->AnimateDiffuseColor(
		vec,
		!!lua_toboolean(L, 3)
	);
	return 0;
}

int D_Light::lua_AnimateSpecularColor(lua_State *L) {
	SELF;
	Light::ColorStep::Vec vec;
	ParseColorSteps(L, 2, vec);
	self->m_light->AnimateSpecularColor(
		vec,
		!!lua_toboolean(L, 3)
	);
	return 0;
}

void D_Light::ParseColorSteps(lua_State *L, int tableIndex, Light::ColorStep::Vec &steps) {
	lua_pushnil(L);
	while (lua_next(L, tableIndex) != 0) {
		luaL_checktype(L, -1, LUA_TTABLE);

		Light::ColorStep step;
		lua_getfield(L, -1, "color");
		step.color = lua::Marshal<Vec3>::Get(L, -1, true);
		lua_getfield(L, -2, "time");
		step.time = (double)luaL_checknumber(L, -1);
		
		lua_pop(L, 3);

		steps.push_back(step);
	}
}

void D_Light::ParseIntensitySteps(lua_State *L, int tableIndex, Light::IntensityStep::Vec &steps) {

	lua_pushnil(L);
	while (lua_next(L, tableIndex) != 0) {
		luaL_checktype(L, -1, LUA_TTABLE);

		Light::IntensityStep step;
		lua_getfield(L, -1, "intensity");
		step.intensity = (float)luaL_checknumber(L, -1);
		lua_getfield(L, -2, "time");
		step.time = (double)luaL_checknumber(L, -1);
		
		lua_pop(L, 3);

		steps.push_back(step);
	}
}

LUART_GETSET(D_Light, DiffuseColor, Vec3, m_light->diffuseColor, SELF)
LUART_GETSET(D_Light, SpecularColor, Vec3, m_light->specularColor, SELF)
LUART_GETSET(D_Light, ShadowColor, Vec4, m_light->shadowColor, SELF)
LUART_GETSET(D_Light, Pos, Vec3, m_light->pos, SELF)
LUART_GET(D_Light, Radius, float, m_light->radius, SELF)
LUART_SET_CUSTOM(D_Light, Radius, SELF, float r = (float)luaL_checknumber(L, 2); self->m_light->radius = r; self->m_light->bounds = BBox(-Vec3(r,r,r), Vec3(r,r,r)))
LUART_GETSET(D_Light, Intensity, float, m_light->intensity, SELF)
LUART_GETSET(D_Light, IntensityScale, float, m_light->intensityScale, SELF)
LUART_GETSET(D_Light, ShadowWeight, float, m_light->shadowWeight, SELF)
LUART_GET(D_Light, Style, int, m_light->style.get(), SELF)
LUART_SET_CUSTOM(D_Light, Style, SELF, int s = (int)luaL_checkinteger(L, 2); self->m_light->style = (Light::LightStyle::Enum)s)
LUART_GETSET(D_Light, InteractionFlags, int, m_light->interactionFlags, SELF)

} // world
