// T_SkModelPrecache.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "LuaTask.h"
#include "../../Packages/PackagesDef.h"
#include <Runtime/PushPack.h>

class Engine;

namespace world {

class World;

class RADENG_CLASS T_Precache : public LuaTask
{
public:
	typedef boost::shared_ptr<T_Precache> Ref;

	static Ref New(
		World *world, 
		Engine &e, 
		const char *asset, 
		pkg::Zone zone,
		bool create, // if true creates objects after data is loaded
		int extra // extra data (not used on all types)
	);
	static Ref New(
		World *world, 
		const pkg::AssetRef &asset,
		bool create, // if true creates objects after data is loaded
		int extra
	);

	virtual int Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags);

	RAD_DECLARE_READONLY_PROPERTY(T_Precache, result, int);
	RAD_DECLARE_READONLY_PROPERTY(T_Precache, asset, const pkg::AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(T_Precache, world, World*);

protected:

	T_Precache(World *world, const pkg::AssetRef &asset);

	virtual RAD_DECLARE_GET(complete, bool) { return m_r <= pkg::SR_Success; }

	// default implementation wraps the asset in a D_Asset
	// object.
	virtual int PushResult(lua_State *L);

private:

	RAD_DECLARE_GET(result, int) { return m_r; }
	RAD_DECLARE_GET(asset, const pkg::AssetRef&) { return m_asset; }
	RAD_DECLARE_GET(world, World*) { return m_world; }

	int m_extra;
	int m_r;
	World *m_world;
	pkg::AssetRef m_asset;
};

} // world

#include <Runtime/PopPack.h>
