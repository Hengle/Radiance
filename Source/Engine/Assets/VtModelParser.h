/*! \file VtModelParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../SkAnim/SkControllers.h"
#include "SkAnimStatesParser.h"
#include <Runtime/File.h>
#include <Runtime/PushPack.h>

#if defined(RAD_OPT_TOOLS)
#include "../SkAnim/SkBuilder.h"
#endif

class Engine;

namespace asset  {

class RADENG_CLASS VtModelParser : public pkg::Sink<VtModelParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_VtModel
	};

	VtModelParser();
	virtual ~VtModelParser();

	RAD_DECLARE_READONLY_PROPERTY(VtModelParser, dvtm, const ska::DVtm*);
	RAD_DECLARE_READONLY_PROPERTY(VtModelParser, states, const ska::AnimState::Map*);
	RAD_DECLARE_READONLY_PROPERTY(VtModelParser, valid, bool);

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

	RAD_DECLARE_GET(dvtm, const ska::DVtm*) { 
		return &m_dvtm; 
	}
	RAD_DECLARE_GET(states, const ska::AnimState::Map*) { 
		return m_states->states; 
	}

	RAD_DECLARE_GET(valid, bool) {
		return m_state == kS_Done;
	}

	enum {
		kS_None,
		kS_Load0,
		kS_Load1,
		kS_Done
	};

	ska::DVtm m_dvtm;
	pkg::Asset::Ref m_statesRef;
	SkAnimStatesParser *m_states;
	file::MMapping::Ref m_mm[2];
	int m_state;
};

} // asset

#include <Runtime/PopPack.h>

