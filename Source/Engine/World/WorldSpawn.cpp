// WorldSpawn.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "World.h"
#include "../Game/Game.h"
#include "../App.h"
#include "../Assets/MaterialParser.h"
#include "ViewModel.h"

using namespace pkg;

namespace world {

int World::Spawn(
	const char *mapName,
	const bsp_file::BSPFile::Ref &bsp,
	const xtime::TimeSlice &time,
	int flags
)
{
	RAD_ASSERT(mapName);
	if (m_mapPath.empty())
		m_mapPath = mapName;

	if (m_spawnState == SS_Done)
		return SR_Success;

	if (!time.remaining)
		return SR_Pending;

	if (m_spawnState == SS_None)
		++m_spawnState;

	int r = SR_Pending;

	while (m_spawnState != SS_Done && time.remaining)
	{
		switch (m_spawnState)
		{
		case SS_SoundEmitter:
			r = SpawnSoundEntities(
				*bsp,
				time,
				flags
			);
			if (r == SR_Success)
				return SR_Pending; // pump game loop to start music.
			break;
		case SS_Draw:
			m_draw->Init(bsp);
			r = SR_Success;
			++m_spawnState;
			break;
		case SS_Materials:
			r = SpawnMaterials(
				*bsp,
				time,
				flags
			);
			break;
		case SS_Models:
			r = SpawnModels(
				*bsp,
				time,
				flags
			);
			break;
		case SS_Cinematics:
			r = m_cinematics->Spawn(
				bsp,
				time,
				flags
			);
			if (r == SR_Success)
				++m_spawnState;
			break;
		case SS_Ents:
			r = SpawnNonSoundEntities(
				*bsp,
				time,
				flags
			);
			break;
		case SS_Specials:
			r = SpawnSpecials(
				*bsp,
				time,
				flags
			);
			break;
		case SS_PostSpawn:
			r = PostSpawn(time, flags);
			if (r == SR_Success)
				r = m_draw->Precache();
			break;
		}

		if (r < SR_Success)
			break;
		
		r = SR_Pending;
	}

	return r;
}

int World::SpawnMaterials(
	const bsp_file::BSPFile &bsp,
	const xtime::TimeSlice &time,
	int flags
)
{
	int r = SR_Success;

	while (time.remaining)
	{
		if (m_spawnOfs >= bsp.numMaterials)
		{
			++m_spawnState;
			m_spawnOfs = 0;
			break;
		}

		r = SpawnMaterial(
			bsp, 
			m_spawnOfs, 
			time,
			flags
		);

		if (r != SR_Success)
			break;

		++m_spawnOfs;
		r = SR_Pending;
	}

	return r;
}

int World::SpawnMaterial(
	const bsp_file::BSPFile &bsp,
	U32 matNum,
	const xtime::TimeSlice &time,
	int flags
)
{
	if (!m_spawnAsset)
	{
		const char *name = bsp.String((bsp.Materials()+matNum)->string);
		m_spawnAsset = App::Get()->engine->sys->packages->Resolve(name, m_pkgZone);
#if defined(RAD_OPT_TOOLS)
		if (!m_spawnAsset || m_spawnAsset->type != asset::AT_Material)
			m_spawnAsset = App::Get()->engine->sys->packages->Resolve("Sys/M_Missing", m_pkgZone);
#endif
		RAD_VERIFY(m_spawnAsset);
		if (!time.remaining)
			return SR_Pending;
	}

	RAD_ASSERT(flags&P_Load);

	int r = m_spawnAsset->Process(
		time,
		flags
	);

	if (r == SR_Pending)
		return r;

	if (r != SR_Success)
	{
		COut(C_Warn) << "Failed to spawn material '" << m_spawnAsset->path.get() << "'" << std::endl;
		m_spawnAsset.reset();
	}
	else
	{
		m_spawnAsset->Process(
			xtime::TimeSlice::Infinite,
			P_Trim
		);
		COut(C_Debug) << "Spawned material '" << m_spawnAsset->path.get() << "'" << std::endl;
	}
	
	// can store invalid ref (to keep indices in sync with bsp file).
	m_bspMaterials.push_back(m_spawnAsset);
	m_spawnAsset.reset();
	return r;
}

int World::SpawnModels(
	const bsp_file::BSPFile &bsp,
	const xtime::TimeSlice &time,
	int flags
)
{
	while (time.remaining)
	{
		if (m_spawnOfs >= bsp.numModels)
		{
			++m_spawnState;
			m_spawnOfs = 0;
			return SR_Success;
		}

		SpawnModel(
			bsp,
			m_spawnOfs
		);

		++m_spawnOfs;
	}

	return SR_Pending;
}

void World::SpawnModel(
	const bsp_file::BSPFile &bsp,
	U32 modelNum
)
{
	const bsp_file::BSPModel *model = bsp.Models()+modelNum;

	// bsp vertex stream is interleaved : xyz + st[2] (2 pairs of uvs).
	r::Mesh::Ref mesh(new (r::ZRender) r::Mesh());

	int streamIndex = mesh->AllocateStream(
		r::SU_Static, 
		sizeof(bsp_file::BSPVertex),
		(int)model->numVerts
	);

	r::Mesh::StreamPtr::Ref vb = mesh->Map(streamIndex);
	memcpy(vb->ptr, bsp.Vertices() + model->firstVert, vb->size.get());
	vb.reset();

	// Map in vertex types into the stream

	mesh->MapSource(
		streamIndex,
		r::MGS_Vertices,
		0,
		sizeof(bsp_file::BSPVertex),
		0
	);

	if (model->numChannels > 0)
	{
		mesh->MapSource(
			streamIndex,
			r::MGS_TexCoords,
			0,
			sizeof(bsp_file::BSPVertex),
			sizeof(float)*3
		);
	}

	if (model->numChannels > 1)
	{
		mesh->MapSource(
			streamIndex,
			r::MGS_TexCoords,
			1,
			sizeof(bsp_file::BSPVertex),
			sizeof(float)*5
		);
	}

	// Upload model indices

	vb = mesh->MapIndices(r::SU_Static, sizeof(U16), (int)model->numIndices);
	memcpy(vb->ptr, bsp.Indices()+model->firstIndex, vb->size.get());
	vb.reset();

	m_staticMeshes.push_back(mesh);

	pkg::Asset::Ref material = m_bspMaterials[model->material];
	if (material)
		m_draw->AddStaticWorldMesh(mesh, material->id);
}

int World::SpawnSpecials(
	const bsp_file::BSPFile &bsp,
	const xtime::TimeSlice &time,
	int flags
)
{
	Keys keys;
	keys.pairs[String("classname")] = String("view_controller");
	Entity::Ref e = m_lua->CreateEntity(keys);
	if (e)
	{
		SetupEntity(e, m_nextEntId);

		int r = e->PrivateSpawn(
			keys,
			xtime::TimeSlice::Infinite,
			0
		);

		if (r != SR_Success)
			return r;

		++m_nextEntId;
		MapEntity(e);
	}

	++m_spawnState;
	return SR_Success;
}

void World::SetupEntity(const Entity::Ref &entity, int id)
{
	entity->m_id = id;
	ZoneTagRef zoneTag(new (ZWorld) world::ZoneTag(m_zone, *m_game));
	entity->m_zoneTag = zoneTag;
	m_zoneTags[(int)id] = zoneTag;
}

void World::MapEntity(const Entity::Ref &entity)
{
	m_ents.insert(Entity::IdMap::value_type(entity->m_id, entity));
	m_classnames.insert(Entity::StringMMap::value_type(String(entity->classname), entity));
	if (entity->targetname.get() != 0)
		m_targetnames.insert(Entity::StringMMap::value_type(String(entity->targetname), entity));
}

void World::UnmapEntity(const Entity::Ref &entity)
{
	m_lua->DeleteEntId(*entity);

	m_ents.erase(entity->m_id);
	m_classnames.erase(String(entity->classname));
	if (entity->targetname.get() != 0)
		m_targetnames.erase(String(entity->targetname));

	if (m_viewController.get() == entity.get())
		m_viewController.reset();
	if (m_playerPawn.get() == entity.get())
		m_playerPawn.reset();
	if (m_worldspawn.get() == entity.get())
		m_worldspawn.reset();

	m_zoneTags.erase(entity->m_id);
}

int World::SpawnSoundEntities(
	const bsp_file::BSPFile &bsp,
	const xtime::TimeSlice &time,
	int flags
)
{
	int r = SR_Success;

	while (time.remaining)
	{
		if (m_spawnOfs >= bsp.numEntities)
		{
			++m_spawnState;
			m_spawnOfs = 0;
			break;
		}

		r = SpawnSoundEmitter(
			bsp, 
			m_spawnOfs, 
			time,
			flags
		);

		if (r != SR_Success)
			break;

		++m_spawnOfs;
		r = SR_Pending;
	}

	return r;
}

int World::SpawnNonSoundEntities(
	const bsp_file::BSPFile &bsp,
	const xtime::TimeSlice &time,
	int flags
)
{
	int r = SR_Success;

	while (time.remaining)
	{
		if (m_spawnOfs >= bsp.numEntities)
		{
			++m_spawnState;
			m_spawnOfs = 0;
			break;
		}

		r = SpawnNonSoundEmitter(
			bsp, 
			m_spawnOfs, 
			time,
			flags
		);

		if (r != SR_Success)
			break;

		++m_spawnOfs;
		r = SR_Pending;
	}

	return r;
}

int World::SpawnEntity(
	Entity::Ref &entity,
	const Keys &keys,
	const xtime::TimeSlice &time
)
{
	if (!entity)
	{
		if (m_nextEntId >= MaxStaticEnts)
		{
			COut(C_Warn) << "MaxStaticEnts" << std::endl;
			return SR_ErrorGeneric;
		}

		entity = m_lua->CreateEntity(keys);
		if (!entity)
			return SR_ErrorGeneric;
		SetupEntity(entity, m_nextEntId++);
	}

	int r = entity->PrivateSpawn(
		keys,
		time,
		P_Load
	);

	if (r == SR_Pending)
		return r;
	if (r == SR_Success)
	{
		COut(C_Debug) << "Spawned '" << entity->classname.get() << "'" << std::endl;
		MapEntity(entity);
	}
	else
	{
		COut(C_Warn) << "Failed to spawn '" << entity->classname.get() << "'" << std::endl;
	}

	return r;
}

int World::SpawnTempEntity(
	Entity::Ref &entity,
	const Keys &keys,
	const xtime::TimeSlice &time
)
{
	if (!entity)
	{
		int i;
		for (i = 0; i < MaxTempEnts; ++i)
		{
			int id = (m_nextTempEntId+i)&(MaxTempEnts-1);
			if (m_ents.find(id+FirstTempEntId) == m_ents.end())
			{
				m_nextTempEntId = id;
				break;
			}
		}

		if (i == MaxTempEnts)
		{
			COut(C_Warn) << "MaxTempEnts" << std::endl;
			return SR_ErrorGeneric;
		}

		entity = m_lua->CreateEntity(keys);
		if (!entity)
			return SR_ErrorGeneric;
		SetupEntity(entity, m_nextTempEntId+FirstTempEntId);
		m_nextTempEntId = (m_nextTempEntId+1)&(MaxTempEnts-1);
	}

	int r = entity->PrivateSpawn(
		keys,
		time,
		P_Load
	);

	if (r == SR_Pending)
		return r;
	if (r == SR_Success)
	{
		COut(C_Debug) << "Spawned '" << entity->classname.get() << "'" << std::endl;
		MapEntity(entity);
	}
	else
	{
		COut(C_Warn) << "Failed to spawn '" << entity->classname.get() << "'" << std::endl;
	}

	return r;
}


int World::PostSpawnEntity(
	const Entity::Ref &entity,
	const xtime::TimeSlice &time
)
{
	return entity->PrivatePostSpawn(time, P_Load);
}

int World::SpawnEntity(
	const bsp_file::BSPFile &bsp,
	U32 entityNum,
	const xtime::TimeSlice &time,
	int flags
)
{
	if (!m_spawnEnt)
	{
		if (m_nextEntId >= MaxStaticEnts)
		{
			COut(C_Warn) << "MaxStaticEnts" << std::endl;
			return SR_ErrorGeneric;
		}

		int r = CreateEntity(bsp, entityNum);
		if (r != SR_Success) // don't fail on not finding a class factory.
			return SR_Success;
		RAD_ASSERT(m_spawnEnt);
		SetupEntity(m_spawnEnt, m_nextEntId++);
	}

	int r = m_spawnEnt->PrivateSpawn(
		m_spawnKeys,
		time,
		flags
	);

	if (r == SR_Pending)
		return r;
	if (r == SR_Success)
	{
		COut(C_Debug) << "Spawned '" << m_spawnEnt->classname.get() << "'" << std::endl;
		MapEntity(m_spawnEnt);
	}
	else
	{
		COut(C_Warn) << "Failed to spawn '" << m_spawnEnt->classname.get() << "'" << std::endl;
	}

	m_spawnEnt.reset();
	m_spawnKeys.pairs.clear();

	return r;
}

int World::SpawnSoundEmitter(
	const bsp_file::BSPFile &bsp,
	U32 entityNum,
	const xtime::TimeSlice &time,
	int flags
)
{
	if (!m_spawnEnt)
	{
		if (m_nextEntId >= MaxStaticEnts)
		{
			COut(C_Warn) << "MaxStaticEnts" << std::endl;
			return SR_ErrorGeneric;
		}

		m_spawnKeys = LoadEntityKeys(bsp, entityNum);
		const char *classname = m_spawnKeys.StringForKey("classname");
		if (classname && string::cmp(classname, "info_sound_emitter"))
			return SR_Success; // skip non sound emitters

		int r = CreateEntity(m_spawnKeys);
		if (r != SR_Success) // don't fail on not finding a class factory.
			return SR_Success;
		RAD_ASSERT(m_spawnEnt);
		SetupEntity(m_spawnEnt, m_nextEntId++);
	}

	int r = m_spawnEnt->PrivateSpawn(
		m_spawnKeys,
		time,
		flags
	);

	if (r == SR_Pending)
		return r;
	if (r == SR_Success)
	{
		COut(C_Debug) << "Spawned '" << m_spawnEnt->classname.get() << "'" << std::endl;
		MapEntity(m_spawnEnt);
	}
	else
	{
		COut(C_Warn) << "Failed to spawn '" << m_spawnEnt->classname.get() << "'" << std::endl;
	}

	m_spawnEnt.reset();
	m_spawnKeys.pairs.clear();

	return r;
}

int World::SpawnNonSoundEmitter(
	const bsp_file::BSPFile &bsp,
	U32 entityNum,
	const xtime::TimeSlice &time,
	int flags
)
{
	if (!m_spawnEnt)
	{
		if (m_nextEntId >= MaxStaticEnts)
		{
			COut(C_Warn) << "MaxStaticEnts" << std::endl;
			return SR_ErrorGeneric;
		}

		m_spawnKeys = LoadEntityKeys(bsp, entityNum);
		const char *classname = m_spawnKeys.StringForKey("classname");
		if (classname && !string::cmp(classname, "info_sound_emitter"))
			return SR_Success; // skip sound emitters

		int r = CreateEntity(m_spawnKeys);
		if (r != SR_Success) // don't fail on not finding a class factory.
			return SR_Success;
		RAD_ASSERT(m_spawnEnt);
		SetupEntity(m_spawnEnt, m_nextEntId++);
	}

	int r = m_spawnEnt->PrivateSpawn(
		m_spawnKeys,
		time,
		flags
	);

	if (r == SR_Pending)
		return r;
	if (r == SR_Success)
	{
		COut(C_Debug) << "Spawned '" << m_spawnEnt->classname.get() << "'" << std::endl;
		MapEntity(m_spawnEnt);
	}
	else
	{
		COut(C_Warn) << "Failed to spawn '" << m_spawnEnt->classname.get() << "'" << std::endl;
	}

	m_spawnEnt.reset();
	m_spawnKeys.pairs.clear();

	return r;
}

Keys World::LoadEntityKeys(const bsp_file::BSPFile &bsp, U32 entityNum)
{
	Keys keys;
	const bsp_file::BSPEntity *bspEnt = bsp.Entities()+entityNum;
	for (U32 i = 0; i < bspEnt->numStrings; ++i)
	{
		// 2 strings per iteration
		const char *key = bsp.String(bspEnt->firstString+(i*2));
		const char *value = bsp.String(bspEnt->firstString+(i*2)+1);
		keys.pairs.insert(Keys::Pairs::value_type(String(key), String(value)));

		if (!string::cmp(key, "classname") && !string::cmp(value, "worldspawn"))
		{ // insert mapname key
			keys.pairs.insert(Keys::Pairs::value_type(String("mappath"), m_mapPath));
		}
	}

	return keys;
}

int World::CreateEntity(const Keys &keys)
{
	m_spawnEnt = m_lua->CreateEntity(keys);
	return m_spawnEnt ? SR_Success : SR_ParseError;
}

int World::CreateEntity(const bsp_file::BSPFile &bsp, U32 entityNum)
{
	m_spawnKeys = LoadEntityKeys(bsp, entityNum);
	return CreateEntity(m_spawnKeys);
}

int World::PostSpawn(const xtime::TimeSlice &time, int flags)
{
	int r = SR_Success;

	for (Entity::IdMap::const_iterator it = m_ents.begin(); it != m_ents.end(); ++it)
	{
		bool pending = r == SR_Pending;
		r = it->second->PrivatePostSpawn(time, flags);
		if (r < SR_Success)
			return r;
		if (pending) // restore pending state.
			r = SR_Pending;
	}

	if (r == SR_Success)
		++m_spawnState;
	return r;
}

} // world
