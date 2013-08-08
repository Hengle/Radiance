// T_Precache.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../World.h"
#include "T_Precache.h"
#include "T_SkModelPrecache.h"
#include "T_VtModelPrecache.h"
#include "T_MeshPrecache.h"
#include "T_SoundPrecache.h"
#include "T_MaterialPrecache.h"
#include "T_TypefacePrecache.h"
#include "T_StringTablePrecache.h"
#include "T_ConversationTreePrecache.h"
#include "T_ParticleEmitterPrecache.h"
#include "D_Asset.h"
#include "../../Assets/TypefaceParser.h"
#include "../../Packages/Packages.h"
#include "../../Engine.h"
#if defined(RAD_OPT_PC_TOOLS)
#include "../../Tools/Editor/EditorMainWindow.h"
#endif

namespace world {

T_Precache::Ref T_Precache::New(
	World *world, 
	Engine &e, 
	const char *asset, 
	pkg::Zone zone,
	bool create,
	int extra
) {
	pkg::Asset::Ref r = e.sys->packages->Resolve(asset, zone);
	if (!r)
		return Ref();
	return New(world, r, create, extra);
}

T_Precache::Ref T_Precache::New(
	World *world, 
	const pkg::AssetRef &asset, 
	bool create,
	int extra
) {
	if (create) {
		switch (asset->type.get()) {
		case asset::AT_SkModel:
			return Ref(new (ZWorld) T_SkModelPrecache(world, asset));
		case asset::AT_VtModel:
			return Ref(new (ZWorld) T_VtModelPrecache(world, asset));
		case asset::AT_Mesh:
			return Ref(new (ZWorld) T_MeshPrecache(world, asset));
		case asset::AT_Sound:
		case asset::AT_Music:
			return Ref(new (ZWorld) T_SoundPrecache(world, asset, extra));
		case asset::AT_Material:
			return Ref(new (ZWorld) T_MaterialPrecache(world, asset));
		case asset::AT_Typeface:
			return Ref(new (ZWorld) T_TypefacePrecache(world, asset));
		case asset::AT_StringTable:
			return Ref(new (ZWorld) T_StringTablePrecache(world, asset));
		case asset::AT_ConversationTree:
			return Ref(new (ZWorld) T_ConversationTreePrecache(world, asset));
		case asset::AT_Particle:
			return Ref(new (ZWorld) T_ParticleEmitterPrecache(world, asset));
		default:
			break;
		}
	}

	return Ref(new (ZWorld) T_Precache(world, asset));
}

T_Precache::T_Precache(World *world, const pkg::AssetRef &asset) : 
m_r(pkg::SR_Pending), 
m_world(world), 
m_asset(asset) {
}

int T_Precache::Tick(Entity &e, float dt, const xtime::TimeSlice &time, int flags) {
	if (!m_asset) {
		m_r = pkg::SR_FileNotFound;
		return TickPop;
	}

#if defined(RAD_OPT_PC_TOOLS)
	const int kLoadFlags = (tools::editor::MainWindow::Get()->lowQualityPreview) ? pkg::P_FastCook : 0;
#endif

	m_r = m_asset->Process(
		time,
		pkg::P_Load|pkg::P_FastPath
#if defined(RAD_OPT_PC_TOOLS)
		| kLoadFlags
#endif
	);

	if (m_r == pkg::SR_Success) { 
		if (m_asset->type == asset::AT_Material) { // for material tick
			m_world->draw->AddMaterial(m_asset->id);
		} else if (m_asset->type == asset::AT_Typeface) {
			asset::TypefaceParser *parser = asset::TypefaceParser::Cast(m_asset);
			if (parser && parser->valid)
				m_world->draw->AddMaterial(parser->materialAsset->id); // for material tick.
		}
	}

	return (m_r <= pkg::SR_Success) ? TickPop : TickNext;
}

int T_Precache::PushResult(lua_State *L) {
	if (m_r != pkg::SR_Success)
		return 0;

	D_Asset::Ref r = D_Asset::New(asset);

	// P_Trim
	asset->Process(
		xtime::TimeSlice::Infinite,
		pkg::P_Trim
	);

	if (r) {
		r->Push(L);
		return 1;
	}
	return 0;
}

} // world
