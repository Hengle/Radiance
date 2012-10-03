// WorldDrawDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>

namespace world {

class World;
class WorldDraw;
class RB_WorldDraw;
class MBatchDraw;
typedef zone_vector<MBatchDraw*, ZWorldT>::type MBatchDrawPtrVec;

namespace details {

class MBatch;
typedef boost::shared_ptr<MBatch> MBatchRef;
typedef zone_map<int, MBatchRef, ZWorldT>::type MBatchIdMap;

} // details
} // world
