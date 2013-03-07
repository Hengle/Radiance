/*! \file Assets.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "../Packages/PackagesDef.h"

class Engine;

namespace asset {

RADENG_API void RADENG_CALL RegisterParsers(Engine &engine);

#if defined(RAD_OPT_TOOLS)
RADENG_API void RADENG_CALL RegisterCookers(Engine &engine);
#endif

} // asset
