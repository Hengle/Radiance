/*! \file GameCVars.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once
#include "../Types.h"
#include "../CVars.h"
#include <Runtime/PushPack.h>

class Game;
class GameCVars {
public:
	CVarBool r_showtris;

	void AddRef(const CVar::Ref &cvar) {
		m_vec.push_back(cvar);
	}

private:
	typedef zone_vector<CVar::Ref, ZEngineT>::type CVarVec;
	friend class Game;

	GameCVars(Game &game, CVarZone &zone);

	CVarVec m_vec;
};

#include <Runtime/PopPack.h>
