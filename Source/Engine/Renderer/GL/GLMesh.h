// GLMesh.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Sources.h"
#include "../Shader.h"
#include "GLState.h"
#include "GLVertexBuffer.h"
#include "GLVertexArray.h"

#include <Runtime/PushPack.h>

namespace r {

class RADENG_CLASS GLMesh
{
public:
	typedef boost::shared_ptr<GLMesh> Ref;
	typedef GLVertexBuffer::Ptr StreamPtr;

	GLMesh();
	~GLMesh();

	void AllocateSwapChains(int num);
	void SwapChain();

	int AllocateStream(StreamUsage usage, int vertexSize, int vertexCount, bool swapChain);
	StreamPtr::Ref Map(int stream);

	void MapSource(
		int stream, 
		MGSource source, 
		int index,
		int stride,
		int ofs
	);

	StreamPtr::Ref MapIndices(StreamUsage usage, int elemSize, int count);

	void BindAll(Shader *shader);
	void CompileArrayStates(Shader &shader);
	void FlushArrayStates(Shader *shader);

	void Bind(MGSource source, int index);
	void BindIndices(bool force=false);
	void Draw();

	void ResetStreamState();
	void Release();
	void Reserve(int numStreams);

private:

	struct Source
	{
		Source() : stream(-1) {}
		int stream;
		GLint count;
		GLenum type;
		GLsizei stride;
		GLuint ofs;
	};

	struct Indices
	{
		GLint count;
		GLenum type;
		GLenum usage;
		GLVertexBuffer::Ref vb;
	};
	
	struct Stream
	{
		typedef zone_vector<Stream, ZRenderT>::type Vec;
		GLVertexBuffer::Vec vbs;
		int size;
		int count;
	};

	int m_swapChain;
	int m_numVerts;
	int m_numSwapChains;
	Source m_sources[MGS_Max][MGS_MaxIndices];
	Stream::Vec m_streams;
	Indices m_i;
	GLShaderVertexArrayMap m_shaderStates;
	GLVertexArray::Vec *m_va;
	Shader *m_shader;
};

} // r

#include <Runtime/PopPack.h>

#include "GLMesh.inl"

