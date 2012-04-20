// GLShader.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "GLShader.h"
#include "GLSLShader.h"

namespace r {


Shader::Ref GLShader::LoadCooked(
	const char *name,
	stream::InputStream &is,
	bool skinned,
	const Material &material
)
{
	U8 backend;
	is >> backend;

	switch (backend)
	{
	case GLSL:
		return boost::static_pointer_cast<Shader>(
			GLSLShader::LoadCooked(
				name,
				is,
				skinned,
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
	bool skinned,
	const Material &material,
	Backend backend
)
{
	switch (backend)
	{
	case GLSL:
		return boost::static_pointer_cast<Shader>(
			GLSLShader::Load(
				engine, 
				name,
				skinned,
				material
			)
		);
	}

	return Shader::Ref();
}

const cg::ShaderCache::Ref &GLShader::Cache()
{
	static cg::ShaderCache::Ref s_cache(new cg::ShaderCache());
	return s_cache;
}
#endif

} // r
