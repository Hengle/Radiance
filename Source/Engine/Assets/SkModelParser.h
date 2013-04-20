/*! \file SkModelParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../SkAnim/SkControllers.h"
#include "SkAnimSetParser.h"
#include "SkAnimStatesParser.h"
#include <Runtime/File.h>

#if defined(RAD_OPT_TOOLS)
#include "../SkAnim/SkBuilder.h"
#endif

#include <Runtime/PushPack.h>

class Engine;

namespace asset  {

class RADENG_CLASS SkModelParser : public pkg::Sink<SkModelParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_SkModel
	};

	SkModelParser();
	virtual ~SkModelParser();

	RAD_DECLARE_READONLY_PROPERTY(SkModelParser, dska, const ska::DSka*);
	RAD_DECLARE_READONLY_PROPERTY(SkModelParser, dskm, const ska::DSkm*);
	RAD_DECLARE_READONLY_PROPERTY(SkModelParser, skinType, ska::SkinType);
	RAD_DECLARE_READONLY_PROPERTY(SkModelParser, states, const ska::AnimState::Map*);
	RAD_DECLARE_READONLY_PROPERTY(SkModelParser, valid, bool);

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

	RAD_DECLARE_GET(dska, const ska::DSka*) { 
		return m_ska->dska; 
	}
	RAD_DECLARE_GET(states, const ska::AnimState::Map*) { 
		return m_states->states; 
	}

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(valid, bool);
	RAD_DECLARE_GET(skinType, ska::SkinType) { 
		return m_skmd ? m_skmd->skinType : ska::kSkinType_CPU; 
	}
	RAD_DECLARE_GET(dskm, const ska::DSkm*) { 
		return m_skmd ? &m_skmd->dskm : &m_dskm; 
	}
#else
	RAD_DECLARE_GET(valid, bool) { 
		return m_state == S_Done;
	}
	RAD_DECLARE_GET(dskm, const ska::DSkm*) { 
		return &m_dskm; 
	}
	RAD_DECLARE_GET(skinType, ska::SkinType) { 
		return ska::kSkinType_CPU;
	}
#endif

	enum {
		S_None,
		S_Load0,
		S_Load1,
		S_Done
	};

#if defined(RAD_OPT_TOOLS)
	tools::SkmData::Ref m_skmd;
	pkg::Cooker::Ref m_cooker;
#endif

	pkg::Asset::Ref m_skaRef;
	pkg::Asset::Ref m_statesRef;
	SkAnimSetParser *m_ska;
	ska::DSkm m_dskm;
	SkAnimStatesParser *m_states;
	file::MMapping::Ref m_mm[2];
	int m_state;
};

} // asset

#include <Runtime/PopPack.h>

