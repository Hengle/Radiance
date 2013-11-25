/*! \file GameDef.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#pragma once

#include "../Types.h"
#include <Runtime/PushPack.h>

class App;
class GSLoadMap;
class GameCVars;
class Game;

#if defined(RAD_OPT_PC_TOOLS)
class IToolsCallbacks {
public:
	virtual void SwapBuffers() = 0;
	virtual void PlayFullscreenMovie(const char *path) = 0;
};
#endif

enum GameUIMode {
	kGameUIMode_PC,
	kGameUIMode_Mobile,
	kGameUIMode_Console
};

#include <Runtime/PopPack.h>

