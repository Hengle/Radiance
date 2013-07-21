/*! \file MapParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "MapParser.h"
#include "../World/World.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

MapParser::MapParser()
#if defined(RAD_OPT_TOOLS)
: m_state(S_None) 
#endif
{
}

MapParser::~MapParser() {
}

int MapParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const AssetRef &asset,
	int flags
) {
#if defined(RAD_OPT_TOOLS)
	if (flags&(P_Unload|P_Trim|P_Cancel)) {
		m_state = S_None;
		m_script.Reset();
		return SR_Success;
	}

	if (asset->cooked || (flags&P_FastPath))
		return SR_Success;

	if (!(flags&(P_Load|P_Parse)))
		return SR_Success;

	if (m_state == S_Done)
		return SR_Success;

	int r = Load(
		time,
		engine,
		asset,
		flags
	);

	if (r < SR_Success) {
		m_state = S_None;
		m_script.Reset();
	}

	return r;
#else
	RAD_ASSERT(asset->cooked);
	return SR_Success;
#endif
}

#if defined(RAD_OPT_TOOLS)

int MapParser::ParseEntity(tools::map_builder::EntSpawn &spawn) {
	spawn.keys.pairs.clear();
	spawn.brushes.clear();
	return ParseEntity(m_script, spawn);
}

int MapParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const AssetRef &asset,
	int flags
) {
	if (m_state == S_None) {
		const String *name = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
		if (!name || name->empty)
			return SR_MetaError;

		file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(
			name->c_str, 
			ZWorld,
			1*kMeg,
			file::kFileOptions_None,
			file::kFileMask_Base
		);

		if (!ib)
			return SR_FileNotFound;

		m_script.Bind(ib);

		int r = ParseCinematicCompressionMap(
			engine,
			asset,
			flags
		);

		if (r != SR_Success)
			return r;

		m_state = S_Done;
	}

	return SR_Success;
}

int MapParser::ParseCinematicCompressionMap(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	m_caMap.clear();

	const String *name = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!name || name->empty)
		return SR_MetaError;

	String file = file::SetFileExtension(name->c_str, ".actors");
	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(
		file.c_str, 
		ZWorld,
		1*kMeg,
		file::kFileOptions_None,
		file::kFileMask_Base
	);

	if (!ib)
		return SR_Success; // not a required file.

	Tokenizer script(ib);
	return ParseCinematicCompressionMap(script, m_caMap);
}

int MapParser::ParseEntity(
	Tokenizer &script,
	tools::map_builder::EntSpawn &spawn
) {
	String token, value, temp;

	if (!script.GetToken(token, Tokenizer::kTokenMode_CrossLine))
		return SR_End;
	if (token != "{")
		return SR_ParseError;

	for (;;) {
		if (!script.GetToken(token, Tokenizer::kTokenMode_CrossLine))
			return SR_ParseError;
		if (token == "}")
			break;
		if (token == "{") { // brush
			tools::SceneFile::Brush brush;
			int r = ParseBrush(script, brush);
			if (r != SR_Success)
				return r;
			if (!brush.windings->empty())
				spawn.brushes.push_back(brush);
			continue;
		}
		if (!script.GetToken(value, Tokenizer::kTokenMode_CrossLine))
			return SR_ParseError;

		// turn "\n" into '\n'
		const char *sz = value.c_str;
		temp.Clear();

		while (*sz) {
			if (sz[0] == '\\' && sz[1] == 'n') {
				temp += '\n';
				++sz;
			} else {
				temp += *sz;
			}
			++sz;
		}

		spawn.keys.pairs[token] = temp;
	}

	return SR_Success;
}

int MapParser::ParseBrush(
	Tokenizer &script,
	tools::SceneFile::Brush &brush
) {
	String token;
	tools::SceneFile::BrushPlane::Vec planes;

	for (;;) {
		if (!script.GetToken(token))
			return SR_ParseError;
		if (token == "}")
			break;

		script.UngetToken();

		Vec3 pts[3];

		for (int i = 0; i < 3; ++i) {
			float f[3];
			if (!script.Skip(1, Tokenizer::kTokenMode_SameLine))
				return SR_ParseError;

			for (int k = 0; k < 3; ++k) {
				if (!script.GetFloat(f[k], Tokenizer::kTokenMode_SameLine))
					return SR_ParseError;
			}

			if (!script.Skip(1, Tokenizer::kTokenMode_SameLine))
				return SR_ParseError;

			pts[i] = Vec3(f[0], f[1], f[2]);
		}

		if (!script.Skip(6, Tokenizer::kTokenMode_SameLine))
			return SR_ParseError;

		tools::SceneFile::BrushPlane bp;
		bp.plane = Plane(pts[2], pts[1], pts[0]); // <-- CCW

		// snap plane.
		for (int i = 0; i < 3; ++i) {
			if (math::Abs(bp.plane.Normal()[i]) > 0.999f) {
				Vec3 n = Vec3::Zero;
				n[i] = bp.plane.Normal()[i] > 0.f ? 1 : -1.f;
				bp.plane = Plane(n, bp.plane.D());
				break;
			}
		}

		planes.push_back(bp);
	}

	if (planes.size() < 4)
		return SR_ParseError;

	tools::SceneFile::Brush::FromPlanes(planes, brush);
	return SR_Success;
}

int MapParser::ParseCinematicCompressionMap(
	Tokenizer &script,
	tools::CinematicActorCompressionMap &caMap
) {
	String token;

	for (;;) {
		if (!script.GetToken(token))
			break;
		if (!script.IsNextToken("{"))
			return SR_InvalidFormat;

		String anim, value;
		tools::SkaCompressionMap animMap;

		for (;;) {
			if (!script.GetToken(anim))
				return SR_InvalidFormat;
			if (anim == "}")
				break;
			if (!script.IsNextToken("=", Tokenizer::kTokenMode_SameLine))
				return SR_InvalidFormat;
			if (!script.GetToken(value, Tokenizer::kTokenMode_SameLine))
				return SR_InvalidFormat;

			float fval;
			sscanf(value.c_str, "%f", &fval);
			animMap.insert(tools::SkaCompressionMap::value_type(anim, fval));
		}

		if (!animMap.empty())
			caMap.insert(tools::CinematicActorCompressionMap::value_type(token, animMap));
	}

	return SR_Success;
}

#endif

void MapParser::Register(Engine &engine) {
	static pkg::Binding::Ref r = engine.sys->packages->Bind<MapParser>();
}

} // asset

#endif // RAD_OPT_TOOLS
