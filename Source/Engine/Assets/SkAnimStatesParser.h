/*! \file SkAnimStatesParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../SkAnim/SkControllers.h"

#if defined(RAD_OPT_TOOLS)
#include "../Lua/LuaRuntime.h"
#endif

#include <Runtime/PushPack.h>

namespace asset  {

class RADENG_CLASS SkAnimStatesParser : public pkg::Sink<SkAnimStatesParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_SkAnimStates
	};

	SkAnimStatesParser();
	virtual ~SkAnimStatesParser();

	RAD_DECLARE_READONLY_PROPERTY(SkAnimStatesParser, states, const ska::AnimState::Map*);
	RAD_DECLARE_READONLY_PROPERTY(SkAnimStatesParser, valid, bool);

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

	RAD_DECLARE_GET(states, const ska::AnimState::Map*) { 
		return &m_states; 
	}

	RAD_DECLARE_GET(valid, bool) { 
		return m_valid; 
	}

#if defined(RAD_OPT_TOOLS)
	void ParseAnimVariant(lua_State *L, const pkg::Asset::Ref &asset, const lua::Variant::Map &map, const ska::AnimState &states, ska::Variant &variant);
	void ParseAnimState(lua_State *L, const pkg::Asset::Ref &asset, const lua::Variant::Map &map, ska::AnimState &states);
	static int lua_Compile(lua_State *L);
	lua::State::Ref InitLua(const pkg::Asset::Ref &asset);
#endif

	bool m_valid;
	ska::AnimState::Map m_states;
};

} // asset

#include <Runtime/PopPack.h>

