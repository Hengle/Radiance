// SkAnimStatesParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkAnimStatesParser.h"
#include "../SkAnim/SkAnim.h"
#include "../Engine.h"
#include <Runtime/File.h>
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

SkAnimStatesParser::SkAnimStatesParser() : m_valid(false)
{
}

SkAnimStatesParser::~SkAnimStatesParser()
{
}

int SkAnimStatesParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_valid && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&P_Unload)
	{
		m_states.clear();
		m_valid = false;
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked)
		return Load(time, engine, asset, flags);
#endif
	
	return LoadCooked(time, engine, asset, flags);
}

int SkAnimStatesParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	String path(CStr("Cooked/"));
	path += CStr(asset->path);
	path += ".bin";
	
	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(path.c_str, ska::ZSka);
	if (!ib)
		return SR_FileNotFound;
	
	stream::InputStream is(*ib);

	int r = SR_IOError;

	try {
		U16 numStates;
		is >> numStates;

		for (U16 i = 0; i < numStates; ++i) {
			ska::AnimState state;
			U16 temp;

			is >> state.name;
			is >> temp; state.loopCount[0] = (int)temp;
			is >> temp; state.loopCount[1] = (int)temp;
						
			U16 numVariants;
			is >> numVariants;

			for (U16 i = 0; i < numVariants; ++i) {
				
				ska::Variant v;
				is >> v.name;
				is >> v.timeScale[0];
				is >> v.timeScale[1];
				is >> temp; v.loopCount[0] = (int)temp;
				is >> temp; v.loopCount[1] = (int)temp;
				is >> v.weight;
				is >> v.in;
				is >> v.out;

				state.variants.push_back(v);
			}

			m_states.insert(ska::AnimState::Map::value_type(state.name, state));
		}

		r = SR_Success;
		m_valid = true;
	}
	catch (exception &) {
	}

	return r;
}

#if defined(RAD_OPT_TOOLS)

#define SELF "@self"
#define ASSET "@asset"

int SkAnimStatesParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	file::MMapping::Ref mm = engine.sys->files->MapFile(s->c_str, ska::ZSka);
	if (!mm) {
		COut(C_Error) << "SkAnimStatesParser: '" << asset->name.get() << "' failed to load '" << *s << "'" << std::endl;
		return SR_FileNotFound;
	}

	lua::State::Ref L = InitLua(asset);

	if (luaL_loadbuffer(
		L->L,
		(const char*)mm->data.get(),
		mm->size,
		s->c_str
	) != 0) {
		COut(C_Error) << "SkAnimStatesParser: '" << asset->name.get() << "' lua parse error '" << lua_tostring(L->L, -1) << "'" << std::endl;
		return SR_ParseError;
	}

	if (lua_pcall(L->L, 0, 1, 0) != 0) {
		COut(C_Error) << "SkAnimStatesParser: '" << asset->name.get() << "' lua runtime error '" << lua_tostring(L->L, -1) << "'" << std::endl;
		return SR_ParseError;
	}

	m_valid = true;
	
	return SR_Success;
}

void SkAnimStatesParser::ParseAnimVariant(lua_State *L, const pkg::Asset::Ref &asset, const lua::Variant::Map &map, const ska::AnimState &state, ska::Variant &v) {
	lua::Variant::Map::const_iterator it = map.find(CStr("loop"));
	if (it != map.end()) {
		const lua::Variant::Map *vars = static_cast<const lua::Variant::Map*>(it->second);
		if (!vars) {
			luaL_error(L, "AnimState '%s':'%s':'%s':loop expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("1"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':'%s':loop expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *low = static_cast<const lua_Number*>(it->second);
		if (!low) {
			luaL_error(L, "AnimState '%s':'%s':'%s':loop expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("2"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':'%s':loop expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *high = static_cast<const lua_Number*>(it->second);
		if (!high) {
			luaL_error(L, "AnimState '%s':'%s':'%s':loop expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		v.loopCount[0] = (int)*low;
		v.loopCount[1] = (int)*high;

		if (v.loopCount[0] < 0)
			v.loopCount[0] = 0;
		if (v.loopCount[1] < 0)
			v.loopCount[1] = 0;
		if (v.loopCount[0] > v.loopCount[1])
			v.loopCount[1] = v.loopCount[0];
	} else {
		v.loopCount[0] = v.loopCount[1] = 1;
	}

	it = map.find(CStr("timeScale"));
	if (it != map.end()) {
		const lua::Variant::Map *vars = static_cast<const lua::Variant::Map*>(it->second);
		if (!vars) {
			luaL_error(L, "AnimState '%s':'%s':'%s':timeScale expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("1"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':'%s':timeScale expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *low = static_cast<const lua_Number*>(it->second);
		if (!low) {
			luaL_error(L, "AnimState '%s':'%s':'%s':timeScale expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("2"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':'%s':timeScale expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *high = static_cast<const lua_Number*>(it->second);
		if (!high) {
			luaL_error(L, "AnimState '%s':'%s':'%s':timeScale expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		v.timeScale[0] = (float)*low;
		v.timeScale[1] = (float)*high;

		if (v.timeScale[0] < 0.f)
			v.timeScale[0] = 0.f;
		if (v.timeScale[1] < 0.f)
			v.timeScale[1] = 0.f;
		if (v.timeScale[0] > v.timeScale[1])
			v.timeScale[1] = v.timeScale[0];
	} else {
		v.timeScale[0] = v.timeScale[1] = 1.f;
	}

	it = map.find(CStr("xfade"));
	if (it != map.end()) {
		const lua::Variant::Map *vars = static_cast<const lua::Variant::Map*>(it->second);
		if (!vars) {
			luaL_error(L, "AnimState '%s':'%s':'%s':xfade expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("1"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':'%s':xfade expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *in = static_cast<const lua_Number*>(it->second);
		if (!in) {
			luaL_error(L, "AnimState '%s':'%s':'%s':xfade expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("2"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':'%s':xfade expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *out = static_cast<const lua_Number*>(it->second);
		if (!out) {
			luaL_error(L, "AnimState '%s':'%s':'%s':xfade expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		v.in = (float)*in;
		v.out = (float)*out;

		if (v.out < 0.f)
			v.out = 0.f;
	} else {
		v.in = v.out = 1.f;
	}

	it = map.find(CStr("weight"));
	if (it != map.end()) {
		const lua_Number *n = static_cast<const lua_Number*>(it->second);
		if (!n) {
			luaL_error(L, "AnimState '%s':'%s':'%s':weight expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				v.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		v.weight = (float)*n;
		if (v.weight < 0.f)
			v.weight = 0.f;
	} else {
		v.weight = 1.f;
	}
}

void SkAnimStatesParser::ParseAnimState(lua_State *L, const pkg::Asset::Ref &asset, const lua::Variant::Map &map, ska::AnimState &state) {
	state.variants.reserve(map.size());

	lua::Variant::Map::const_iterator it = map.find(CStr("loop"));
	if (it != map.end()) {
		const lua::Variant::Map *vars = static_cast<const lua::Variant::Map*>(it->second);
		if (!vars) {
			luaL_error(L, "AnimState '%s':'%s':loop expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("1"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':loop expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *low = static_cast<const lua_Number*>(it->second);
		if (!low) {
			luaL_error(L, "AnimState '%s':'%s':loop expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		it = vars->find(CStr("2"));
		if (it == vars->end()) {
			luaL_error(L, "AnimState '%s':'%s':loop expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		const lua_Number *high = static_cast<const lua_Number*>(it->second);
		if (!high) {
			luaL_error(L, "AnimState '%s':'%s':loop expected number, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		state.loopCount[0] = (int)*low;
		state.loopCount[1] = (int)*high;

		if (state.loopCount[0] < 0)
			state.loopCount[0] = 0;
		if (state.loopCount[1] < 0)
			state.loopCount[1] = 0;
		if (state.loopCount[0] > state.loopCount[1])
			state.loopCount[1] = state.loopCount[0];
	} else {
		state.loopCount[0] = state.loopCount[1] = 1;
	}

	for (lua::Variant::Map::const_iterator it = map.begin(); it != map.end(); ++it) {
		ska::Variant v;

		if (it->first == CStr("loop"))
			continue; // special keyword

		v.name = it->first;

		const lua::Variant::Map *vars = static_cast<const lua::Variant::Map*>(it->second);
		if (!vars) {
			luaL_error(L, "AnimState '%s':'%s':'%s' expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				state.name.c_str.get(),
				it->first.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		ParseAnimVariant(L, asset, *vars, state, v);
		state.variants.push_back(v);
	}
}

int SkAnimStatesParser::lua_Compile(lua_State *L) {

	lua_getfield(L, LUA_REGISTRYINDEX, SELF);
	SkAnimStatesParser *self = (SkAnimStatesParser*)lua_touserdata(L, -1);
	lua_pop(L, 1);
	RAD_VERIFY(self);
	lua_getfield(L, LUA_REGISTRYINDEX, ASSET);
	const pkg::Asset::Ref &asset = *(const pkg::Asset::Ref*)lua_touserdata(L, -1);
	lua_pop(L, 1);

	lua::Variant::Map map;
	lua::ParseVariantTable(L, map, true);

	for (lua::Variant::Map::const_iterator it = map.begin(); it != map.end(); ++it) {
		const lua::Variant::Map *vars = static_cast<const lua::Variant::Map*>(it->second);
		if (!vars) {
			luaL_error(L, "AnimState '%s':'%s' expected table, (Function %s, File %s, Line %d)",
				asset->name.get(),
				it->first.c_str.get(),
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		}

		ska::AnimState s;
		s.name = it->first;
		self->ParseAnimState(L, asset, *vars, s);
		self->m_states.insert(ska::AnimState::Map::value_type(s.name, s));
	}

	return 0;
}

lua::State::Ref SkAnimStatesParser::InitLua(const pkg::Asset::Ref &asset) {
	lua::State::Ref state(new (ska::ZSka) lua::State("SkAnimStates"));
	lua_State *L = state->L;

	luaL_Reg r[] = {
		{ "Compile", lua_Compile },
		{ 0, 0 }
	};

	lua::RegisterGlobals(L, 0, r);
	lua_pushlightuserdata(L, this);
	lua_setfield(L, LUA_REGISTRYINDEX, SELF);
	lua_pushlightuserdata(L, (void*)&asset);
	lua_setfield(L, LUA_REGISTRYINDEX, ASSET);

	return state;
}

#endif

void SkAnimStatesParser::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<SkAnimStatesParser>();
}

} // asset
