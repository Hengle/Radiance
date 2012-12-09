/*! \file FloorsDef.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Types.h"
#include <Runtime/PushPack.h>
#include <bitset>

namespace world {

class World;
class Floors;
class FloorPosition;

enum {
	kMaxFloors = 64,
	kMaxFloorTris = 2048,
	kMaxWaypoints = 2048
};

typedef std::bitset<kMaxFloors> FloorBits;
typedef std::bitset<kMaxFloorTris> FloorTriBits;
typedef std::bitset<kMaxWaypoints> WaypointBits;

} // world

#include <Runtime/PopPack.h>
