/*! \file GLVertexArray.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup renderer
*/

#pragma once

#include "../RendererDef.h"
#include "GLState.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/PushPack.h>

namespace r {

class GLVertexArray
{
public:
	typedef GLVertexArrayRef Ref;
	typedef zone_vector<Ref, ZRenderT>::type Vec;

	GLVertexArray();
	~GLVertexArray();

	RAD_DECLARE_READONLY_PROPERTY(GLVertexArray, id, GLuint);
	RAD_DECLARE_READONLY_PROPERTY(GLVertexArray, initialized, bool);

private:

	friend class GLState;

	RAD_DECLARE_GET(id, GLuint);
	RAD_DECLARE_GET(initialized, bool);

	GLuint m_id;
	bool m_initialized;
};

class GLShaderVertexArrayMap
{
public:
	typedef GLShaderVertexArrayMapRef Ref;

	GLShaderVertexArrayMap();
	void AllocateSwapChains(int num);
	void Clear();
	GLVertexArray::Vec *Find(int shaderGuid) const;
	GLVertexArray::Vec *Create(int shaderGuid, int swapChain);
	void Flush(int shaderGuid);

private:

	typedef zone_map<int, GLVertexArray::Vec, ZRenderT>::type Map;

	int m_numSwaps;
	mutable Map m_vaMap;
};

} // r

#include <Runtime/PopPack.h>
#include "GLVertexArray.inl"
