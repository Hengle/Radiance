/*! \file Occupant.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "WorldDef.h"
#include "MBatchDraw.h"

namespace world {

class RADENG_CLASS MBatchOccupant : public boost::noncopyable {
public:

	MBatchOccupant(World &world, LightingFlags lightingFlags, int lightInteractionFlags) 
		: m_markFrame(-1), 
		m_world(&world), 
		m_leaf(0),
		m_lightingFlags(lightingFlags),
		m_lightInteractionFlags(lightInteractionFlags)
	{}

	virtual ~MBatchOccupant() {}

	RAD_DECLARE_READONLY_PROPERTY(MBatchOccupant, visible, bool);
	RAD_DECLARE_READONLY_PROPERTY(MBatchOccupant, bounds, const BBox&);
	RAD_DECLARE_READONLY_PROPERTY(MBatchOccupant, batches, const MBatchDraw::RefVec *);
	RAD_DECLARE_READONLY_PROPERTY(MBatchOccupant, world, World*);
	RAD_DECLARE_PROPERTY(MBatchOccupant, lightInteractionFlags, int, int);
	RAD_DECLARE_PROPERTY(MBatchOccupant, lightingFlags, LightingFlags, LightingFlags);

	void Link();
	void Unlink();

protected:

	virtual RAD_DECLARE_GET(visible, bool) = 0;
	virtual RAD_DECLARE_GET(bounds, const BBox&) = 0;
	virtual RAD_DECLARE_GET(batches, const MBatchDraw::RefVec*) = 0;

private:

	RAD_DECLARE_GET(world, World*) {
		return m_world;
	}

	RAD_DECLARE_GET(lightingFlags, LightingFlags) {
		return m_lightingFlags;
	}

	RAD_DECLARE_SET(lightingFlags, LightingFlags) {
		m_lightingFlags = value;
	}

	RAD_DECLARE_GET(lightInteractionFlags, int) {
		return m_lightInteractionFlags;
	}

	RAD_DECLARE_SET(lightInteractionFlags, int) {
		m_lightInteractionFlags = value;
	}

	friend class World;
	friend class WorldDraw;

	IntSet m_areas;
	dBSPLeaf::PtrVec m_bspLeafs;
	World *m_world;
	dBSPLeaf *m_leaf;
	int m_markFrame;
	int m_lightInteractionFlags;
	LightingFlags m_lightingFlags;
};

}
