// Sources.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"

namespace r {

enum StreamUsage
{
	SU_Static,
	SU_Dynamic,
	SU_Stream
};

enum MTSource // Material Texture Sources
{
	MTS_First,
	MTS_Texture = MTS_First,
	MTS_Framebuffer,
	MTS_Max,
	MTS_MaxIndices = 6
};

enum MGSource // Material Geometry Sources
{
	MGS_First,
	MGS_Vertices = MGS_First,
	MGS_Normals,
	MGS_Tangents,
	MGS_TexCoords,
	MGS_Max,
	MGS_MaxIndices = 2
};

} // r
