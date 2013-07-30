/*! \file ParticleParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "ParticleParser.h"
#include "ParticleMaterialLoader.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

ParticleMaterialLoader::ParticleMaterialLoader() : m_loaded(false) {
}

ParticleMaterialLoader::~ParticleMaterialLoader() {
}

int ParticleMaterialLoader::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_loaded && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&P_Unload) {
		m_material.reset();
		return SR_Success;
	}

	return Load(time, engine, asset, flags);
}

int ParticleMaterialLoader::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!m_material) {
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {
			ParticleParser *parser = ParticleParser::Cast(asset);
			m_material = engine.sys->packages->Resolve(parser->material, asset->zone);
			if (!m_material || (m_material->type != asset::AT_Material)) {
				return SR_FileNotFound;
			}
		} else {
#endif
			const Package::Entry::Import *imp = asset->entry->Resolve(0);
			if (!imp)
				return SR_ParseError;
			m_material = asset->entry->Resolve(*imp, asset->zone);
			if (!m_material || (m_material->type != asset::AT_Material)) {
				return SR_FileNotFound;
			}
#if defined(RAD_OPT_TOOLS)
		}
#endif
	}

	if (!time.remaining)
		return SR_Pending;

	int r = m_material->Process(time, flags);
	if (r == SR_Success)
		m_loaded = true;
	return r;
}


void ParticleMaterialLoader::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<ParticleMaterialLoader>();
}

} // asset
