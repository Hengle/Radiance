/*! \file GameCVars.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Game.h"

// cvar defaults
Game::CVars::CVars(Game &game, CVarZone &zone) 
: r_showtris(zone, "r_showtris", false, false) {
}
