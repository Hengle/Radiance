// MaxScene.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <Runtime/StreamDef.h>
#include "Map.h"

namespace tools {

bool LoadMaxScene(stream::InputStream &stream, Map &map, bool smooth);

} // tools

