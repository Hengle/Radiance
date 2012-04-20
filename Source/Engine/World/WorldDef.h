// WorldDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once
#include "../Types.h"

namespace world {

enum UnloadDisposition
{
	UD_None,
	UD_Slot,
	UD_All
};

enum PauseFlags
{
	RAD_FLAG(PauseGame),
	RAD_FLAG(PauseUI),
	RAD_FLAG(PauseCinematics),
	PauseAll = PauseGame|PauseUI|PauseCinematics
};

class World;
class WorldLua;
class Zone;
class ZoneTag;

typedef boost::shared_ptr<World> WorldRef;
typedef boost::weak_ptr<World> WorldWRef;
typedef boost::shared_ptr<Zone> ZoneRef;
typedef boost::weak_ptr<Zone> ZoneWRef;
typedef boost::shared_ptr<ZoneTag> ZoneTagRef;
typedef boost::weak_ptr<ZoneTag> ZoneTagWRef;

} // world
