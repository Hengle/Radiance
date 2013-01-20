// MapAsset.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MapAsset.h"
#include "../App.h"
#include "../Engine.h"
#include "../World/World.h"
#include "../Sound/Sound.h"
#include <Runtime/Base/SIMD.h>

#if defined(RAD_OPT_TOOLS)
#include "MapParser.h"
#include "MapCooker.h"
#endif

using namespace pkg;

namespace asset {

MapAsset::MapAsset() : m_game(0), m_slot(0), m_spawning(false) {
#if defined(RAD_OPT_TOOLS)
	m_ui = 0;
	m_debugUI = 0;
#endif
}

MapAsset::~MapAsset() {
}

int MapAsset::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (flags&P_Unload) {
#if defined(RAD_OPT_TOOLS)
		m_mapBuilder.reset();
		m_parser.reset();
#endif
		m_bspFile.reset();
		m_bspData.reset();
		m_world.reset();
		m_spawning = false;
		return SR_Success;
	}

	if (!(flags&(P_Load|P_Parse)))
		return SR_Success;

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && (!(flags&P_FastPath) || m_debugUI)) {
		int r = SpawnTool(
			time,
			engine,
			asset,
			flags
		);

		if (r < SR_Success) {
			m_world.reset();
			m_mapBuilder.reset();
			m_parser.reset();
		}

		return r;
	}
#endif

	return SpawnCooked(
		time,
		engine,
		asset,
		flags
	);
}

int MapAsset::SpawnCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (m_world) {
		if (m_spawning) {
			int r = m_world->Spawn(
				asset->path,
				m_bspFile,
				time,
				flags
			);

			if (r == SR_Success) {
#pragma message ("TODO: split bsp into persistant/non-persistant files to save memory like i do for ska's.")
				// Cinematics data references data in-place in bsp file.
//				m_bspData.Close();
				m_spawning = false;
			}

			return r;
		}

		return SR_Success;
	}


	if (!m_bspData) {
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {
			if (m_cooker) {
				int r = m_cooker->Cook(0, P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;

				// load BSP file

				String path(CStr(asset->path));
				path += ".bsp";

				m_bspData = m_cooker->MapFile(path.c_str, 0, ZWorld);
				if (!m_bspData)
					return SR_FileNotFound;
				m_cooker.reset();

			} else {
				m_cooker = asset->AllocateIntermediateCooker();
				CookStatus status = m_cooker->Status(0, P_TARGET_FLAGS(flags));

				if (status == CS_Ignore)
					return SR_CompilerError;

				if (status == CS_NeedRebuild) {
					COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
					static_cast<MapCooker*>(m_cooker.get())->SetProgressIndicator(m_ui);
					int r = m_cooker->Cook(0, P_TARGET_FLAGS(flags));
					if (r != SR_Success)
						return r;
				} else {
					COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
				}

				String path(CStr(asset->path));
				path += ".bsp";

				m_bspData = m_cooker->MapFile(path.c_str, 0, ZWorld);
				if (!m_bspData)
					return SR_FileNotFound;
				m_cooker.reset();
			}
		}
		else {
#endif
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bsp";

		m_bspData = engine.sys->files->MapFile(path.c_str, ZWorld);
		if (!m_bspData)
			return SR_FileNotFound;

#if defined(RAD_OPT_TOOLS)
		}
#endif
	}

#if defined(RAD_OPT_TOOLS)
	if (flags&P_Parse)
		return SR_Success; // done, not spawning world.
#endif

	RAD_ASSERT(m_game);

	SoundContext::Ref sound = SoundContext::New(App::Get()->engine->sys->alDriver);

	m_world = world::World::New(*m_game, m_slot, sound, asset->zone);
	int r = m_world->Init();
	if (r < SR_Success)
		return r;

	m_bspFile.reset(new (ZWorld) world::bsp_file::BSPFileParser());
	r = m_bspFile->Parse(m_bspData->data, m_bspData->size);
	if (r < SR_Success)
		return r;

	m_spawning = true;

	return SR_Pending;
}

#if defined(RAD_OPT_TOOLS)

void MapAsset::SetProgressIndicator(tools::UIProgress *ui) {
	m_ui = ui;
}

void MapAsset::SetDebugUI(tools::map_builder::DebugUI *ui) {
	m_debugUI = ui;
}

void MapAsset::DebugDraw(float time, float dt, const QRect &viewport) {
	if (m_mapBuilder)
		m_mapBuilder->DebugDraw(time, dt, viewport);
}

void MapAsset::OnDebugMenu(const QVariant &data) {
	if (m_mapBuilder)
		m_mapBuilder->OnDebugMenu(data);
}

int MapAsset::SpawnTool(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (m_parser) {

		int r = m_mapBuilder->result;
		if (r != SR_Success)
			return r;

		tools::map_builder::EntSpawn spawn;
		r = m_parser->ParseEntity(spawn);
		if (r != SR_Success) {
			if (r == MapParser::SR_End) {
				r = SR_Pending;
				m_parser.reset();
				if (!m_mapBuilder->SpawnCompile())
					return SR_ParseError;
			}

			return r;
		}

		if (!m_mapBuilder->LoadEntSpawn(spawn))
			return SR_ParseError;
		return SR_Pending;
	}

	if (m_mapBuilder) {

		int r = m_mapBuilder->result;
		if (r != SR_Success) {
			if (r == SR_Pending) {
				if (time.infinite)
					m_mapBuilder->WaitForCompletion();
			}
		}
				
		return r;
	}

	m_mapBuilder.reset(new (ZTools) tools::map_builder::MapBuilder(engine));

	m_mapBuilder->SetProgressIndicator(m_ui);
	m_mapBuilder->SetDebugUI(m_debugUI);
	
	m_parser = MapParser::Cast(asset);
	if (!m_parser)
		return SR_ParseError;

	m_mapBuilder->SetCinematicActorCompression(*m_parser->caMap.get());

	for (;;) {
		tools::map_builder::EntSpawn spawn;
		int r = m_parser->ParseEntity(spawn);
		if (r != SR_Success) {
			if (r == MapParser::SR_End) {
				m_parser.reset();
				if (!m_mapBuilder->SpawnCompile())
					return SR_ParseError;
				return SR_Pending;
			}
			return r;
		}

		if (!m_mapBuilder->LoadEntSpawn(spawn))
			return SR_ParseError;

		r = m_mapBuilder->result;

		if (r != SR_Pending) {
			if (r == SR_Success) {
				if (!time.remaining)
					return SR_Pending;
				// else spawn next entity.
			} else {
				return r; // error code.
			}
		} else {
			if (!time.infinite)
				return SR_Pending;
			m_mapBuilder->WaitForCompletion();
		}
	}

	return SR_Pending;
}
#endif

void MapAsset::Register(Engine &engine) {
	static pkg::Binding::Ref r = engine.sys->packages->Bind<MapAsset>();
}

} // asset

