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

void D_Light::PushElements(lua_State *L) {
	LUART_REGISTER_GETSET(L, DiffuseColor);
	LUART_REGISTER_GETSET(L, SpecularColor);
	LUART_REGISTER_GETSET(L, Pos);
	LUART_REGISTER_GETSET(L, Radius);
	LUART_REGISTER_GETSET(L, Brightness);
	LUART_REGISTER_GETSET(L, Style);
	LUART_REGISTER_GETSET(L, InteractionFlags);
	lua_pushcfunction(L, lua_Link);
	lua_setfield(L, -2, "Link");
	lua_pushcfunction(L, lua_Unlink);
	lua_setfield(L, -2, "Unlink");
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

LUART_GETSET(D_Light, DiffuseColor, Vec3, m_light->diffuseColor, SELF)
LUART_GETSET(D_Light, SpecularColor, Vec3, m_light->specularColor, SELF)
LUART_GETSET(D_Light, ShadowColor, Vec4, m_light->shadowColor, SELF)
LUART_GETSET(D_Light, Pos, Vec3, m_light->pos, SELF)
LUART_GET(D_Light, Radius, float, m_light->radius, SELF)
LUART_SET_CUSTOM(D_Light, Radius, SELF, float r = (float)luaL_checknumber(L, 2); self->m_light->radius = r; self->m_light->bounds = BBox(-Vec3(r,r,r), Vec3(r,r,r)))
LUART_GETSET(D_Light, Brightness, float, m_light->brightness, SELF)
LUART_GET(D_Light, Style, int, m_light->style.get(), SELF)
LUART_SET_CUSTOM(D_Light, Style, SELF, int s = luaL_checkinteger(L, 2); self->m_light->style = (Light::LightStyle::Enum)s)
LUART_GETSET(D_Light, InteractionFlags, int, m_light->interactionFlags, SELF)

} // world
