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

#if defined(RAD_OPT_TOOLS)
RAD_ZONE_DEF(RADENG_API, ZTools, "Tools", ZRuntime);
RAD_ZONE_DEF(RADENG_API, ZEditor, "Editor", ZTools);
#endif
