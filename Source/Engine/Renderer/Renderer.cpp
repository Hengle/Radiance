// Renderer.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../Zones.h"
#include "Renderer.h"
#include <math.h>

namespace r {

RAD_ZONE_DEF(RADENG_API, ZRender, "Renderer", ZEngine);
RAD_ZONE_DEF(RADENG_API, ZTextures, "Textures", ZRender);
RAD_ZONE_DEF(RADENG_API, ZVertexBuffers, "VertexBuffers", ZRender);
RAD_ZONE_DEF(RADENG_API, ZSkm, "Skm", ZRender);
RAD_ZONE_DEF(RADENG_API, ZFonts, "Fonts", ZRender);

bool VidMode::Div(int x, int y) const {
	RAD_ASSERT(x&&y);
	if (w <=0 || h <= 0) return false;
	float f = ((float)x)/((float)y);
	float z = ((float)w)/((float)h);
	return fabs(f-z) < 0.099999999999f;
}

} // r
