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

#if defined(RAD_OPT_TOOLS)
RAD_ZONE_DEC(RADENG_API, ZTools);
RAD_ZONE_DEC(RADENG_API, ZEditor);
#endif

#include <Runtime/PopPack.h>
