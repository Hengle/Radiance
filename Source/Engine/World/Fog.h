/*! \file Fog.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "MBatchDraw.h"
#include "../Renderer/Mesh.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace world {
class WorldDraw;

class FogVolume {
	typedef boost::shared_ptr<FogVolume> Ref;
	typedef zone_vector<Ref, ZWorldT>::type Vec;

	FogVolume(
		WorldDraw &draw,
		const r::Mesh::Ref &m,
		const BBox &bounds,
		int matId
	);

private:

	friend class WorldDraw;

	r::Mesh::Ref m_m;
	BBox m_bounds;
	details::MatRef *m_matRef;
	int m_markFrame;
};

} // world

#include <Runtime/PopPack.h>
