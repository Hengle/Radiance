/*! \file Fog.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "WorldDraw.h"

namespace world {

FogVolume::FogVolume(
	WorldDraw &draw,
	const r::Mesh::Ref &m,
	const BBox &bounds,
	int matId
) : m_m(m), m_bounds(bounds), m_markFrame(-1) {
	m_matRef = draw.AddMaterialRef(matId);
}

} // world
