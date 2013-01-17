// Zones.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Zones.h"

RAD_ZONE_DEF(RADENG_API, ZEngine, "Engine", ZRuntime);
RAD_ZONE_DEF(RADENG_API, ZAssets, "Assets", ZRuntime);
RAD_ZONE_DEF(RADENG_API, ZWorld, "World", ZEngine);
RAD_ZONE_DEF(RADENG_API, ZSound, "Sound", ZEngine);
RAD_ZONE_DEF(RADENG_API, ZMusic, "Music", ZEngine);
RAD_ZONE_DEF(RADENG_API, ZMeshes, "Meshes", ZEngine);
RAD_ZONE_DEF(RADENG_API, ZStringTables, "StringTables", ZEngine);

#if !defined(RAD_OPT_SHIP)
RAD_ZONE_DEF(RADENG_API, ZTools, "Tools", ZRuntime);
#endif

#if defined(RAD_OPT_TOOLS)
RAD_ZONE_DEF(RADENG_API, ZEditor, "Editor", ZTools);
#endif

namespace r {

RAD_ZONE_DEF(RADENG_API, ZRender, "Renderer", ZEngine);
RAD_ZONE_DEF(RADENG_API, ZTextures, "Textures", ZRender);
RAD_ZONE_DEF(RADENG_API, ZVertexBuffers, "VertexBuffers", ZRender);
RAD_ZONE_DEF(RADENG_API, ZSkm, "Skm", ZRender);
RAD_ZONE_DEF(RADENG_API, ZFonts, "Fonts", ZRender);

}

namespace ska {

RAD_ZONE_DEF(RADENG_API, ZSka, "SkModels", ZEngine);

}

namespace lua {

RAD_ZONE_DEF(RADENG_API, ZLuaRuntime, "Lua Runtime", ZEngine);

}