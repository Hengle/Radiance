// GLMesh.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {
	
inline GLMesh::GLMesh() : m_va(0), m_shader(0), m_numSwapChains(1), m_swapChain(0) {
}

inline GLMesh::~GLMesh() {
}

inline void GLMesh::AllocateSwapChains(int num) {
	RAD_ASSERT(m_numSwapChains == 1);
	RAD_ASSERT(num > 1);
	m_numSwapChains = num;
	m_shaderStates.AllocateSwapChains(num);
}

inline void GLMesh::SwapChain() {
	m_swapChain = (m_swapChain + 1) % m_numSwapChains;
}

inline void GLMesh::CompileArrayStates(Shader &shader) {
}

inline void GLMesh::FlushArrayStates(Shader *shader) {
	m_va = 0;

	if (!shader || m_shader == shader)
		m_shader = 0;

	if (shader) {
		m_shaderStates.Flush(shader->guid);
	} else {
		m_shaderStates.Clear();
	}
}

inline void GLMesh::BindIndices(bool force) {
	RAD_ASSERT(m_i.vb);
	gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, m_i.vb, force);
}

inline void GLMesh::Draw(int numTris) {
	RAD_ASSERT(m_i.vb);
	if (numTris < 0) {
		numTris = m_i.count;
	} else {
		numTris = numTris * 3;
	}
	gl.DrawElements(GL_TRIANGLES, numTris, m_i.type, 0);
	CHECK_GL_ERRORS();
}

inline void GLMesh::ResetStreamState() {
	gls.DisableAllMGSources();
}

inline void GLMesh::Reserve(int numStreams)  {
	m_streams.reserve(numStreams);
}

} // r
