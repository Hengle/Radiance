// D_Asset.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include "../../Packages/PackagesDef.h"
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_Asset : public lua::SharedPtr
{
public:
	typedef boost::shared_ptr<D_Asset> Ref;

	static Ref New(const pkg::AssetRef &asset);

	RAD_DECLARE_READONLY_PROPERTY(D_Asset, asset, const pkg::AssetRef&);

protected:

	D_Asset(const pkg::AssetRef &asset) : m_asset(asset) {}
	virtual void PushElements(lua_State *L);

private:

	RAD_DECLARE_GET(asset, const pkg::AssetRef&) { return m_asset; }

	static int lua_Type(lua_State *L);
	static int lua_Name(lua_State *L);
	static int lua_Path(lua_State *L);

	pkg::AssetRef m_asset;
};

} // world

#include <Runtime/PopPack.h>
