// MapAsset.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "MapAsset.h"
#include "../App.h"
#include "../Engine.h"
#include "../World/World.h"
#include "../Sound.h"
#include <Runtime/Base/SIMD.h>

#if defined(RAD_OPT_TOOLS)
#include "MapParser.h"
#endif

using namespace pkg;

namespace asset {

MapAsset::MapAsset() : m_game(0), m_slot(0), m_spawning(false)
{
}

MapAsset::~MapAsset()
{
}

int MapAsset::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (flags&P_Unload)
	{
#if defined(RAD_OPT_TOOLS)
		m_mapBuilder.reset();
#endif
		m_bspFile.reset();
		m_bspData.Close();
		m_world.reset();
		m_spawning = false;
		return SR_Success;
	}

	if (!(flags&P_Load))
		return SR_Success;

	RAD_ASSERT(m_game);

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && !(flags&P_FastPath))
	{
		int r = SpawnTool(
			time,
			engine,
			asset,
			flags
		);

		if (r < SR_Success)
		{
			m_world.reset();
			m_mapBuilder.reset();
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
)
{
	if (m_world)
	{
		if (m_spawning)
		{
			int r = m_world->Spawn(
				asset->path,
				m_bspFile,
				time,
				flags
			);

			if (r == SR_Success)
			{
#pragma message ("TODO: split bsp into persistant/non-persistant files to save memory like i do for ska's.")
				// Cinematics data references data in-place in bsp file.
//				m_bspData.Close();
				m_spawning = false;
			}

			return r;
		}

		return SR_Success;
	}


	if (!m_bspData)
	{
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked)
		{
			Cooker::Ref cooker = asset->AllocateIntermediateCooker();
			CookStatus status = cooker->Status(0, P_TARGET_FLAGS(flags));

			if (status == CS_Ignore)
				return SR_CompilerError;

			if (status == CS_NeedRebuild)
			{
				COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
				int r = cooker->Cook(0, P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;
			}
			else
			{
				COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
			}

			String path(CStr(asset->path));
			path += ".bsp";

			int media = file::AllMedia;
			int r = cooker->LoadFile( // load cooked data.
				path.c_str,
				0,
				media,
				m_bspData,
				file::HIONotify(),
				SIMDDriver::Alignment,
				ZWorld
			);

			if (r < SR_Success)
				return r;
		}
		else {
#endif
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bsp";

		int media = file::AllMedia;
		int r = engine.sys->files->LoadFile(
			path.c_str,
			media,
			m_bspData,
			file::HIONotify(),
			SIMDDriver::Alignment,
			ZWorld
		);

		if (r < SR_Success)
			return r;
#if defined(RAD_OPT_TOOLS)
		}
#endif
	}

	if (m_bspData->result == file::Pending)
	{
		if (time.infinite)
			m_bspData->WaitForCompletion();
		else
			return SR_Pending;
	}

	if (m_bspData->result < file::Success)
		return m_bspData->result.get();

	SoundContext::Ref sound = App::Get()->engine->sys->soundDevice->CreateContext();

	m_world = world::World::New(*m_game, m_slot, sound, asset->zone);
	int r = m_world->Init();
	if (r < SR_Success)
		return r;

	m_bspFile.reset(new (ZWorld) world::bsp_file::BSPFileParser());
	r = m_bspFile->Parse(m_bspData->data->ptr, m_bspData->data->size);
	if (r < SR_Success)
		return r;

	m_spawning = true;

	return SR_Pending;
}

#if defined(RAD_OPT_TOOLS)
int MapAsset::SpawnTool(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (m_world)
	{
		if (m_spawning)
		{
			RAD_ASSERT(m_mapBuilder);
			
			int r = m_world->Spawn(
				asset->path,
				m_mapBuilder->bspFile,
				time,
				flags
			);

			if (r == SR_Success)
			{
				m_mapBuilder.reset();
				m_spawning = false;
			}
			return r;
		}
		return SR_Success;
	}

	m_mapBuilder.reset(new ::tools::MapBuilder(engine));

	// we don't do this inside any timeslice.
	MapParser::Ref parser(MapParser::Cast(asset));
	if (!parser)
		return SR_ParseError;

	world::EntSpawn spawn;
	int r;
	while ((r=parser->ParseEntity(spawn)) == SR_Success)
	{
		if (!m_mapBuilder->LoadEntSpawn(spawn))
			return SR_ParseError;
	}

	r = r == MapParser::SR_End ? SR_Success : r;

	if (r == SR_Success)
	{
		if (!m_mapBuilder->Compile())
			return SR_CompilerError;

		SoundContext::Ref sound = App::Get()->engine->sys->soundDevice->CreateContext();

		m_world = world::World::New(*m_game, m_slot, sound, asset->zone);

		r = m_world->Init();
		if (r < SR_Success)
			return r;

		r = m_world->Spawn(
			asset->path,
			m_mapBuilder->bspFile,
			time,
			flags
		);
		
		if (r == SR_Success)
		{
			m_mapBuilder.reset();
		}
		else
		{
			m_spawning = true;
		}
	}

	return r;
}
#endif

void MapAsset::Register(Engine &engine)
{
	static pkg::Binding::Ref r = engine.sys->packages->Bind<MapAsset>();
}

} // asset

