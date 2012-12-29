// GLShader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../Shader.h"
#include "GLState.h"

#if defined(RAD_OPT_TOOLS)
	#include "../ShaderTool.h"
#endif

#include <Runtime/PushPack.h>

class Engine;

namespace r {

class GLShader : public Shader {
public:
	enum Backend {
		kBackend_GLSL
	};

	static Shader::Ref LoadCooked(
		const char *name,
		stream::InputStream &is,
		const Material &material
	);

#if defined(RAD_OPT_TOOLS)
	static Shader::Ref Load(
		Engine &engine, 
		const char *name,
		const Material &material,
		Backend backend
	);

	static const tools::shader_utils::ShaderCache::Ref &Cache();
#endif
};

} // r

#include <Runtime/PopPack.h>
