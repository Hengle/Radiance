// UnitTests.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../UTCommon.h"
#include <string>
#include <Engine/App.h>
#include <Engine/Game/Game.h>
#include <Engine/Game/GSLoadMap.h>
#include <Engine/Game/GSPlay.h>
#include <Engine/World/Lua/D_SkModel.h>

#if defined(RAD_OPT_WIN)
#if !defined(RAD_TARGET_GOLDEN)
#include <VLD/vld.h> // VLD only in non-golden builds.
#endif
#endif

namespace ut
{
    void TaskManagerTest();
}

namespace
{
	int s_argc = 0;
	const char **s_argv = 0;
}

int __Argc() { return s_argc; }
const char **__Argv() { return s_argv; }
bool __PostQuitPending() { return false; }

#if defined(RAD_OPT_TOOLS)

thread::Id __QMainThreadId()
{
	return 0;
}

#endif

namespace world {
	
D_SkModel::Ref D_SkModel::New(const r::SkMesh::Ref &mesh)
{
	return Ref();
}
	
} // world

App *App::New()
{
	return 0;
}

Game::Ref Game::New()
{
	return Ref();
}

Game::Tickable::Ref GSLoadMap::New(int mapId, int slot, bool play, bool loadScreen)
{
	return Game::Tickable::Ref();
}

Game::Tickable::Ref GSPlay::New()
{
	return Game::Tickable::Ref();
}

int main(int argc, const char **argv)
{
	s_argc = argc;
	s_argv = (const char **)argv;

	rt::Initialize();

    INIT();
	std::string testToRun;

	if (argc > 1) { testToRun = argv[1]; }

    rt::Finalize();

    END();
}
