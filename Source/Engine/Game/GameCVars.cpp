/*! \file GameCVars.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "GameCVars.h"
#include "../Renderer/Common.h"

// cvar defaults
GameCVars::GameCVars(Game &game, CVarZone &zone) : 
r_showtris(zone, "r_showtris", false, false),
r_showportals(zone, "r_showportals", false, false),
r_showentitybboxes(zone, "r_showentitybboxes", false, false),
r_showworldbboxes(zone, "r_showworldbboxes", false, false),
r_showactorbboxes(zone, "r_showactorbboxes", false, false),
r_showwaypoints(zone, "r_showwaypoints", false, false),
r_showmovecmds(zone, "r_showmovecmds", false, false),
r_frustumcull(zone, "r_frustumcull", true, false),
r_lightscissor(zone, "r_lightscissor", true, false),
r_showlightscissor(zone, "r_showlightscissor", false, false),
r_throttle(zone, "r_throttle", true, false),
r_showlightpasses(zone, "r_showlightpasses", false, false),
r_showlightcounts(zone, "r_showlightcounts", false, false),
r_fly(zone, "r_fly", false, false),
r_lockvis(zone, "r_lockvis", false, false),
r_showfrustum(zone, "r_showfrustum", false, false),
r_viewunilightmatrix(zone, "r_viewunilightmatrix", false, false),
r_maxLightsPerPass(zone, "r_maxlightsperpass", r::kMaxLights, false),
r_showlights(zone, "r_showlights", false, false),
r_showunifiedlights(zone, "r_showunifiedlights", false, false) {
}

