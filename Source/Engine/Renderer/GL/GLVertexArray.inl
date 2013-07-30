/*! \file GLVertexArray.inl
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup renderer
*/

namespace r {

inline GLVertexArray::GLVertexArray() : m_initialized(false)
{
	RAD_ASSERT(gl.vbos&&gl.vaos);
	gl.GenVertexArrays(1, &m_id);
	CHECK_GL_ERRORS();
}

inline GLVertexArray::~GLVertexArray()
{
	RAD_ASSERT(gl.vbos&&gl.vaos);
	gl.DeleteVertexArrays(1, &m_id);
	CHECK_GL_ERRORS();
}

inline GLuint GLVertexArray::RAD_IMPLEMENT_GET(id)
{
	return m_id;
}

inline bool GLVertexArray::RAD_IMPLEMENT_GET(initialized)
{
	return m_initialized;
}

inline GLShaderVertexArrayMap::GLShaderVertexArrayMap() : m_numSwaps(1)
{
}

inline void GLShaderVertexArrayMap::AllocateSwapChains(int num)
{
	RAD_ASSERT(m_numSwaps==1);
	RAD_ASSERT(num > 1);
	RAD_ASSERT(m_vaMap.empty());
	m_numSwaps = num;
}

inline void GLShaderVertexArrayMap::Clear()
{
	m_vaMap.clear();
	m_numSwaps = 1;
}

inline GLVertexArray::Vec *GLShaderVertexArrayMap::Find(int shaderGuid) const
{
	GLVertexArray::Vec *v = 0;
	Map::iterator it = m_vaMap.find(shaderGuid);
	if (it != m_vaMap.end())
		v = &it->second;
	return v;
}

inline GLVertexArray::Vec *GLShaderVertexArrayMap::Create(int shaderGuid, int swapChain)
{
	RAD_ASSERT(swapChain < m_numSwaps);

	GLVertexArray::Vec *v;

	Map::iterator it = m_vaMap.find(shaderGuid);
	if (it != m_vaMap.end())
	{
		v = &it->second;
	}
	else
	{
		std::pair<Map::iterator, bool> x = m_vaMap.insert(Map::value_type(shaderGuid, GLVertexArray::Vec()));
		RAD_ASSERT(x.second);
		v = &x.first->second;
		v->resize(m_numSwaps);
	}

	GLVertexArray::Ref r(new (ZRender) GLVertexArray());
	v->at(swapChain) = r;

	return v;
}

inline void GLShaderVertexArrayMap::Flush(int shaderGuid)
{
	m_vaMap.erase(shaderGuid);
}

} // r
