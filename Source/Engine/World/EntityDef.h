// EntityDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once
#include "../Types.h"
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneSet.h>

namespace world {

class Entity;
struct EntSpawn;

typedef boost::shared_ptr<Entity> EntityRef;
typedef boost::weak_ptr<Entity> EntityWRef;
typedef zone_list<EntityWRef, ZWorldT>::type EntityWRefList;
typedef zone_set<int, ZWorldT>::type EntityIdSet;

} // world
