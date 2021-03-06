// GLSLShader.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {

///////////////////////////////////////////////////////////////////////////////

inline GLSLShaderObj::GLSLShaderObj(GLenum type) {
	RAD_ASSERT(gl.ARB_shader_objects&&gl.ARB_vertex_shader&&gl.ARB_fragment_shader);
	m_name = gl.CreateShaderObjectARB(type);
	CHECK_GL_ERRORS();
}

inline GLSLShaderObj::~GLSLShaderObj() {
	if (m_name) {
		gl.DeleteObjectARB(m_name);
		CHECK_GL_ERRORS();
	}
}

inline GLhandleARB GLSLShaderObj::RAD_IMPLEMENT_GET(id) {
	return m_name;
}

///////////////////////////////////////////////////////////////////////////////

inline GLSLProgramObj::GLSLProgramObj() {
	RAD_ASSERT(gl.ARB_shader_objects&&gl.ARB_vertex_shader&&gl.ARB_fragment_shader);
	m_name = gl.CreateProgramObjectARB();
	CHECK_GL_ERRORS();
}

inline GLSLProgramObj::~GLSLProgramObj() {
	if (m_name) {
#if defined(RAD_OPT_OGLES)
		glDeleteProgram(m_name);
#else
		gl.DeleteObjectARB(m_name);
#endif
		CHECK_GL_ERRORS();
	}
}

inline GLhandleARB GLSLProgramObj::RAD_IMPLEMENT_GET(id) {
	return m_name;
}

///////////////////////////////////////////////////////////////////////////////

inline GLSLShader::GLSLShader() : m_curMat(0), m_curPass(kNumPasses) {
}

inline int GLSLShader::Usage(Shader::Pass p, MaterialTextureSource source) const {
	return HasPass(p) ? m_passes[p].m.numMTSources[source] : 0;
}

inline int GLSLShader::Usage(Shader::Pass p, MaterialGeometrySource source) const {
	return HasPass(p) ? m_passes[p].m.numMGSources[source] : 0;
}

inline int GLSLShader::Outputs(Shader::Pass p) const {
	return HasPass(p) ? m_passes[p].outputs : 0;
}

inline bool GLSLShader::HasPass(Shader::Pass p) const {
	return m_passes[p].p;
}

} // r
