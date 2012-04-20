// GLShader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../Shader.h"
#include "GLState.h"

#if defined(RAD_OPT_TOOLS)
	#include "../CG/CGShader.h"
#endif

#include <Runtime/PushPack.h>

class Engine;

namespace r {

class GLShader : public Shader
{
public:
	enum Backend
	{
		ARB,
		GLSL
	};

	static Shader::Ref LoadCooked(
		const char *name,
		stream::InputStream &is,
		bool skinned,
		const Material &material
	);

#if defined(RAD_OPT_TOOLS)
	static Shader::Ref Load(
		Engine &engine, 
		const char *name,
		bool skinned,
		const Material &material,
		Backend backend
	);

	static const cg::ShaderCache::Ref &Cache();
#endif
};

} // r

#include <Runtime/PopPack.h>
