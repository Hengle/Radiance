// RBAssets.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../GL/GLTexture.h"

namespace asset {

void RB_RegisterParsers(Engine &engine)
{
	r::GLTextureAsset::Register(engine);
}

}
