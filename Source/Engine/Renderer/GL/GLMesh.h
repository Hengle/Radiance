// GLMesh.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Common.h"
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
		MaterialGeometrySource source, 
		int index,
		int stride,
		int ofs,
		int count
	);

	StreamPtr::Ref MapIndices(StreamUsage usage, int elemSize, int count);

	void BindAll(Shader *shader);
	void CompileArrayStates(Shader &shader);
	void FlushArrayStates(Shader *shader);

	void Bind(MaterialGeometrySource source, int index);
	void BindIndices(bool force=false);
	void Draw(int firstTri, int numTris);

	void ResetStreamState();
	void Release();
	void Reserve(int numStreams);

private:

	int GenShaderGuid(Shader *shader);

	struct Source {
		Source() : stream(-1) {}
		int stream;
		GLint count;
		GLenum type;
		GLsizei stride;
		GLuint ofs;
	};

	struct Indices {
		GLint count;
		GLenum type;
		GLenum usage;
		GLVertexBuffer::Ref vb;
	};
	
	struct Stream {
		typedef zone_vector<Stream, ZRenderT>::type Vec;
		GLVertexBuffer::Vec vbs;
		int size;
		int count;
	};

	boost::array<
		boost::array<
			Source, 
			kMaterialGeometrySource_MaxIndices>, 
	kNumMaterialGeometrySources> m_sources;

	Stream::Vec m_streams;
	GLShaderVertexArrayMap m_shaderStates;

	int m_swapChain;
	int m_numVerts;
	int m_numSwapChains;
	Indices m_i;
	GLVertexArray::Vec *m_va;
	int m_shaderGuid;
};

} // r

#include <Runtime/PopPack.h>

#include "GLMesh.inl"

