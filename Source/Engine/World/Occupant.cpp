/*! \file Occupant.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Occupant.h"
#include "World.h"

namespace world {

void MBatchOccupant::Link() {
	m_world->LinkOccupant(*this, this->bounds);
}

void MBatchOccupant::Unlink() {
	m_world->UnlinkOccupant(*this);
}

}
