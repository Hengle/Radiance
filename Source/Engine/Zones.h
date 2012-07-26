// Zones.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Opts.h"
#include <Runtime/PushPack.h>

RAD_ZONE_DEC(RADENG_API, ZEngine);
RAD_ZONE_DEC(RADENG_API, ZAssets);
RAD_ZONE_DEC(RADENG_API, ZWorld);
RAD_ZONE_DEC(RADENG_API, ZSound);
RAD_ZONE_DEC(RADENG_API, ZMusic);
RAD_ZONE_DEC(RADENG_API, ZMeshes);
RAD_ZONE_DEC(RADENG_API, ZStringTables);

#if defined(RAD_OPT_TOOLS)
RAD_ZONE_DEC(RADENG_API, ZTools);
RAD_ZONE_DEC(RADENG_API, ZEditor);
#endif

namespace r {

RAD_ZONE_DEC(RADENG_API, ZRender);
RAD_ZONE_DEC(RADENG_API, ZTextures);
RAD_ZONE_DEC(RADENG_API, ZVertexBuffers);
RAD_ZONE_DEC(RADENG_API, ZSkm);
RAD_ZONE_DEC(RADENG_API, ZFonts);

}

namespace ska {
RAD_ZONE_DEC(RADENG_API, ZSka);
}

namespace lua{
RAD_ZONE_DEC(RADENG_API, ZLuaRuntime);
}

#include <Runtime/PopPack.h>
