// GLSLTool.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../Shader.h"
#include "../ShaderTool.h"
#include "../ShaderToolUtils.h"
#include <iostream>
#include <Runtime/PushPack.h>

class Engine;

namespace r {
class Material;
struct MaterialInputMappings;
}

namespace tools {
namespace shader_utils {

//! Assembles and compiles a GLSL shader from a tool shader object.
class RADENG_CLASS GLSLTool : public IncludeSource {
public:

	RAD_BEGIN_FLAGS
		RAD_FLAG(kAssemble_VertexShader),
		RAD_FLAG(kAssemble_PixelShader),
		RAD_FLAG(kAssemble_GLES),
		RAD_FLAG(kAssemble_Optimize),
		kAssemble_None = 0
	RAD_END_FLAGS(AssembleFlags)

	bool Assemble(
		Engine &engine,
		const r::Material &material,
		const Shader::Ref &shader,
		const r::MaterialInputMappings &mapping,
		r::Shader::Pass pass,
		AssembleFlags flags,
		std::ostream &out
	);

	virtual bool AddInclude(const char *name, std::ostream &out) { 
		return true; 
	}

};

} // shader_utils
} // tools

RAD_IMPLEMENT_FLAGS(tools::shader_utils::GLSLTool::AssembleFlags)

#include <Runtime/PopPack.h>
