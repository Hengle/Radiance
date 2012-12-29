// GLShader.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "GLShader.h"
#include "GLSLShader.h"

namespace r {


Shader::Ref GLShader::LoadCooked(
	const char *name,
	stream::InputStream &is,
	const Material &material
) {
	U8 backend;
	is >> backend;

	switch (backend) {
	case kBackend_GLSL:
		return boost::static_pointer_cast<Shader>(
			GLSLShader::LoadCooked(
				name,
				is,
				material
			)
		);
	}

	return Shader::Ref();
}

#if defined(RAD_OPT_TOOLS)

Shader::Ref GLShader::Load(
	Engine &engine, 
	const char *name,
	const Material &material,
	Backend backend
) {
	switch (backend)
	{
	case kBackend_GLSL:
		return boost::static_pointer_cast<Shader>(
			GLSLShader::Load(
				engine, 
				name,
				material
			)
		);
	default:
		break;
	}

	return Shader::Ref();
}

const tools::shader_utils::ShaderCache::Ref &GLShader::Cache() {
	static tools::shader_utils::ShaderCache::Ref s_cache(new tools::shader_utils::ShaderCache());
	return s_cache;
}
#endif

} // r
