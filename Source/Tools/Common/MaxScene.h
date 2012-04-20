// MaxScene.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/Stream.h>
#include "Map.h"

// throws exceptions on error.
void LoadMaxScene(stream::InputStream &stream, Map &map);
