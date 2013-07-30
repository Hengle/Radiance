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

class RADENG_CLASS ParticleParser : public pkg::Sink<ParticleParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Particle
	};

	ParticleParser();
	virtual ~ParticleParser();

	RAD_DECLARE_READONLY_PROPERTY(ParticleParser, particleStyle, const r::ParticleStyle*);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_READONLY_PROPERTY(ParticleParser, material, const char*);
#endif

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

#if defined(RAD_OPT_TOOLS)
	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
#endif

	int LoadCooked(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	RAD_DECLARE_GET(particleStyle, const r::ParticleStyle*) {
		return &m_style;
	}

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(material, const char*) {
		return m_material.c_str;
	}
#endif

	r::ParticleStyle m_style;
#if defined(RAD_OPT_TOOLS)
	String m_material;
#endif
	bool m_loaded;
};

} // asset

#include <Runtime/PopPack.h>
