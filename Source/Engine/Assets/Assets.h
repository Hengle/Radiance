// Assets.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Packages/PackagesDef.h"

class Engine;

namespace asset {

RADENG_API void RADENG_CALL RegisterParsers(Engine &engine);

#if defined(RAD_OPT_TOOLS)
RADENG_API void RADENG_CALL RegisterCookers(Engine &engine);
#endif

} // asset
