/*! \file D_Light.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include "../Light.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_Light : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<D_Light> Ref;

	static Ref New(const Light::Ref &light);

	RAD_DECLARE_READONLY_PROPERTY(D_Light, light, const Light::Ref&);

protected:

	virtual void PushElements(lua_State *L);

private:

	LUART_DECL_GETSET(DiffuseColor);
	LUART_DECL_GETSET(SpecularColor);
	LUART_DECL_GETSET(ShadowColor);
	LUART_DECL_GETSET(Pos);
	LUART_DECL_GETSET(Radius);
	LUART_DECL_GETSET(Brightness);
	LUART_DECL_GETSET(Style);
	LUART_DECL_GETSET(InteractionFlags);

	static int lua_Link(lua_State *L);
	static int lua_Unlink(lua_State *L);

	D_Light(const Light::Ref &light);

	RAD_DECLARE_GET(light, const Light::Ref&) {
		return m_light;
	}

	Light::Ref m_light;

};

} // world

#include <Runtime/PopPack.h>
