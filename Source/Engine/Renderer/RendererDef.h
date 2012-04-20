// RenderDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Runtime/InterfaceDef.h>
#include <Runtime/Interface/ComponentManagerDef.h>

namespace r {

RAD_ZONE_DEC(RADENG_API, ZRender);
RAD_ZONE_DEC(RADENG_API, ZTextures);
RAD_ZONE_DEC(RADENG_API, ZVertexBuffers);
RAD_ZONE_DEC(RADENG_API, ZSkm);
RAD_ZONE_DEC(RADENG_API, ZFonts);

enum Info { Version = (1 << 16) | (0) }; // 1.0
RAD_DECLARE_INTERFACE(HRenderer, IRenderer);
RAD_DECLARE_INTERFACE(HContext, IContext);
RAD_DECLARE_EXPORT_COMPONENTS_FN(RADENG_API, ExportRBackendComponents);

} // r
