// D_Asset.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "D_Asset.h"
#include "../../Packages/Packages.h"

namespace world {

D_Asset::Ref D_Asset::New(const pkg::AssetRef &asset)
{
	return Ref(new (ZWorld) D_Asset(asset));
}

void D_Asset::PushElements(lua_State *L)
{
	lua_pushcfunction(L, lua_Type);
	lua_setfield(L, -2, "Type");
	lua_pushcfunction(L, lua_Name);
	lua_setfield(L, -2, "Name");
	lua_pushcfunction(L, lua_Path);
	lua_setfield(L, -2, "Path");
}

int D_Asset::lua_Type(lua_State *L)
{
	Ref self = Get<D_Asset>(L, "D_Asset", 1, true);
	lua_pushinteger(L, self->asset->type.get());
	return 1;
}

int D_Asset::lua_Name(lua_State *L)
{
	Ref self = Get<D_Asset>(L, "D_Asset", 1, true);
	lua_pushstring(L, self->asset->name.get());
	return 1;
}

int D_Asset::lua_Path(lua_State *L)
{
	Ref self = Get<D_Asset>(L, "D_Asset", 1, true);
	lua_pushstring(L, self->asset->path.get());
	return 1;
}

} // world
