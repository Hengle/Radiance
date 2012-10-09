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

namespace details {

struct MBatchDrawLink;

struct MatRef;
typedef zone_map<int, MatRef, ZWorldT>::type MatRefMap;

struct MBatch;
typedef boost::shared_ptr<MBatch> MBatchRef;
typedef zone_map<int, MBatchRef, ZWorldT>::type MBatchIdMap;

} // details
} // world
