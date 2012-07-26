// MeshMaterialLoader.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MeshMaterialLoader.h"
#include "MeshParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

MeshMaterialLoader::MeshMaterialLoader() : m_state(S_None), m_matIdx(0)
{
}

MeshMaterialLoader::~MeshMaterialLoader()
{
}

int MeshMaterialLoader::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&(P_Load|P_Parse|P_Info|P_Trim))
	{
		if (flags&P_Trim)
		{
			if (m_state == S_Done)
				m_state = S_LoadMaterials;
		}
		else
		{
			if (m_state == S_Done)
				return SR_Success;
			if (m_state == S_None)
				m_state = S_LoadMaterials;
		}
	}

	if (flags&P_Unload)
	{
		m_matRefs.clear();
		m_umatRefs.clear();
		m_state = S_None;
		return SR_Success;
	}

	if (m_state == S_Error)
		return SR_ErrorGeneric;

	return Load(
		time,
		engine,
		asset,
		flags
	);
}

int MeshMaterialLoader::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	MeshParser::Ref parser = MeshParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	if (parser->bundle->meshes.size() == 0)
	{
		m_state = S_Error;
		return SR_InvalidFormat;
	}

	if (m_matRefs.empty())
		m_matRefs.resize(parser->bundle->meshes.size());
		
	RAD_ASSERT(m_state == S_LoadMaterials);
	for (; m_matIdx < (int)m_matRefs.size(); ++m_matIdx)
	{
		pkg::Asset::Ref &m = m_matRefs[m_matIdx];
		const DMesh &dm = parser->bundle->meshes[m_matIdx];

		if (!m)
		{
			m = engine.sys->packages->Resolve(dm.material, asset->zone);
			if (!m || m->type != asset::AT_Material)
			{
#if defined(RAD_OPT_TOOLS)
				if (!(flags&P_NoDefaultMedia))
					m = engine.sys->packages->Resolve("Sys/M_Missing", asset->zone);
				if (!m)
#endif
					return SR_FileNotFound;
			}
		}

		if (!time.remaining)
			return SR_Pending;

		RAD_ASSERT(m);
		int r = m->Process(
			time, 
			flags
		);

		if (r < SR_Success)
		{
			m_state = S_Error;
			return r;
		}

		if (r != SR_Success)
			return r;

		AddUMatRef(m);
	}

	m_matIdx = 0;
	++m_state;
	return SR_Success;
}

void MeshMaterialLoader::AddUMatRef(const pkg::Asset::Ref &m)
{
	for (int i = 0; i < (int)m_umatRefs.size(); ++i)
	{
		if (m_umatRefs[i].get() == m.get())
			return;
	}
	m_umatRefs.push_back(m);
}

void MeshMaterialLoader::Register(Engine &engine)
{
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<MeshMaterialLoader>();
}

} // asset
