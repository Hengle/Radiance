/*! \file VtMaterialLoader.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "VtMaterialLoader.h"
#include "VtModelParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

VtMaterialLoader::VtMaterialLoader() : m_state(kS_None), m_matIdx(0) {
}

VtMaterialLoader::~VtMaterialLoader() {
}

int VtMaterialLoader::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&(P_Load|P_Parse|P_Info|P_Trim)) {
		if (flags&P_Trim) {
			if (m_state == kS_Done)
				m_state = kS_LoadMaterials;
		} else {
			if (m_state == kS_Done)
				return SR_Success;
			if (m_state == kS_None)
				m_state = kS_LoadMaterials;
		}
	}

	if (flags&P_Unload) {
		m_matRefs.clear();
		m_state = kS_None;
		return SR_Success;
	}

	if (m_state == kS_Error)
		return SR_ErrorGeneric;

	return Load(
		time,
		engine,
		asset,
		flags
	);
}

int VtMaterialLoader::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	VtModelParser *parser = VtModelParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	if (parser->dvtm->meshes.size() == 0) {
		m_state = kS_Error;
		return SR_InvalidFormat;
	}

	if (m_matRefs.empty())
		m_matRefs.resize(parser->dvtm->meshes.size());
		
	RAD_ASSERT(m_state == kS_LoadMaterials);
	for (; m_matIdx < (int)m_matRefs.size(); ++m_matIdx) {
		pkg::Asset::Ref &m = m_matRefs[m_matIdx];
		const ska::DVtMesh &dm = parser->dvtm->meshes[m_matIdx];

		if (!m) {
			m = engine.sys->packages->Resolve(dm.material, asset->zone);
			if (!m || m->type != asset::AT_Material) {
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

		if (r < SR_Success) {
			m_state = kS_Error;
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

void VtMaterialLoader::AddUMatRef(const pkg::Asset::Ref &m) {
	for (int i = 0; i < (int)m_umatRefs.size(); ++i) {
		if (m_umatRefs[i].get() == m.get())
			return;
	}
	m_umatRefs.push_back(m);
}

void VtMaterialLoader::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<VtMaterialLoader>();
}

} // asset
