// Assets.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Assets.h"
#include "TextureParser.h"
#include "MapAsset.h"
#include "MaterialParser.h"
#include "SkAnimSetParser.h"
#include "SkAnimStatesParser.h"
#include "SkModelParser.h"
#include "SkMaterialLoader.h"
#include "SoundParser.h"
#include "SoundLoader.h"
#include "MeshParser.h"
#include "MusicParser.h"
#include "MeshMaterialLoader.h"
#include "FontParser.h"
#include "TypefaceParser.h"
#include "StringTableParser.h"

#if defined(RAD_OPT_TOOLS)
#include "MapParser.h"
#endif

#define ASSET_STRINGS
#include "AssetTypes.h"

namespace asset {

#if defined(RAD_OPT_TOOLS)
RADENG_API const char * RADENG_CALL TypeString(Type type) {
	RAD_ASSERT(type < AT_Max);
	return s_strings[type];
}

RADENG_API Type RADENG_CALL TypeFromName(const char *str) {
	RAD_ASSERT(str);

	for (int i = 0; i < AT_Max; ++i) {
		if (!string::cmp(s_strings[i], str)) 
			return (Type)i;
	}

	return AT_Max;
}
#endif

void RB_RegisterParsers(Engine&);

RADENG_API void RADENG_CALL RegisterParsers(Engine &engine) {
	TextureParser::Register(engine);
#if defined(RAD_OPT_TOOLS)
	MapParser::Register(engine);
#endif
	MapAsset::Register(engine);
	MaterialParser::Register(engine);
	MaterialLoader::Register(engine);
	SkAnimSetParser::Register(engine);
	SkAnimStatesParser::Register(engine);
	SkModelParser::Register(engine);
	SkMaterialLoader::Register(engine);
	SoundParser::Register(engine);
	SoundLoader::Register(engine);
	MeshParser::Register(engine);
	MeshMaterialLoader::Register(engine);
	MusicParser::Register(engine);
	FontParser::Register(engine);
	TypefaceParser::Register(engine);
	StringTableParser::Register(engine);
	RB_RegisterParsers(engine);
}

} // asset
