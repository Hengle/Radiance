/*! \file ParticleParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../Renderer/Particles.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS ParticleMaterialLoader : public pkg::Sink<ParticleMaterialLoader> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Process,
		AssetType = AT_Particle
	};

	ParticleMaterialLoader();
	virtual ~ParticleMaterialLoader();

	RAD_DECLARE_READONLY_PROPERTY(ParticleMaterialLoader, material, const pkg::Asset::Ref&);

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	RAD_DECLARE_GET(material, const pkg::Asset::Ref&) {
		return m_material;
	}

	pkg::Asset::Ref m_material;
	bool m_loaded;
};

} // asset

#include <Runtime/PopPack.h>
