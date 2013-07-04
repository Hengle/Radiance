// RGLBackend.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Renderer.h"
#include <Runtime/PushPack.h>

namespace r {

class RBackend;

class GLRenderer : public IRenderer {
public:

	virtual void Initialize(int version);

	virtual void ClearBackBuffer();
	virtual void CommitStates();
	virtual void UnbindStates();

};

} // r

#include <Runtime/PopPack.h>
