/*! \file Mesh.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup renderer
*/

#pragma once

#include "../Types.h"
#include "../Packages/PackagesDef.h"
#include "../Assets/MeshMaterialLoader.h"
#include "Common.h"
#include "Shader.h"
#include <Runtime/Container/ZoneVector.h>

#if defined(RAD_OPT_GL)
#include "GL/GLMesh.h"
#endif

#include <Runtime/PushPack.h>

namespace r {

//! Renderable Mesh.
/*! A mesh serves as a container for vertex buffers and the mappings of r::MaterialGeometrySource descriptors
	into those buffers. The mesh itself is implemented in the rendering backend.
	\sa r::GLMesh
*/
class RADENG_CLASS Mesh {
public:
#if defined(RAD_OPT_GL)
	typedef GLMesh RB;
#endif

	typedef boost::shared_ptr<Mesh> Ref;
	typedef zone_vector<Ref, ZEngineT>::type Vec;
	typedef RB::StreamPtr StreamPtr;

	Mesh();
	~Mesh();

	//! Allocates swap chain storage.
	/*! For dynamic mesh data it can be beneficial to place SU_Stream or SU_Dynamic vertex
		data into a swap chain that is then alternated each frame.

		This method allocates storage for the requested number of swap chains. NOTE that this
		method does not actually allocate stream data. Streams that are intended to be part
		of the swap chain must declare that intention when they call AllocateStream().

		By default there is only 1 swap chain and all streams use this.
	*/
	void AllocateSwapChains(int num);

	//! Swaps any swap chain streams to the next buffer in the chain.
	void SwapChain();

	int AllocateStream(StreamUsage usage, int vertexSize, int vertexCount, bool swapChain = false);
	StreamPtr::Ref Map(int stream);

	void MapSource(
		int stream, 
		MaterialGeometrySource s, 
		int index,
		int stride,
		int ofs
	);

	StreamPtr::Ref MapIndices(StreamUsage usage, int elemSize, int count);

	void Bind(MaterialGeometrySource source, int index);
	void BindAll(MaterialGeometrySource source);
	void BindIndices();

	//! Binds all array states for this mesh, including indices. 
	/*! When used in conjunction with CompileArrayStates() will use
		driver side array state caching.
	*/
	void BindAll(Shader *shader);

	//! Compiles the mesh array states assocated the specified shader into a state block.
	/*!	This method is used in conjunction with a material shader and
		normal mesh stream state methods to capture and compile array states.

		The array states will automatically be used by subsequent BindAll() method
		calls on the mesh.

		To draw a mesh using vertex array states:

		\verbatim

		Material->BindStates();
		Material->BindTextures(...);
		Material->Shader->Begin(...);

		Mesh->BindAll(Material->Shader);
		Material->Shader->BindStates();
		Mesh->CompileArrayStates(Material->Shader);
		Mesh->Draw();

		Material->Shader->End();

		\endverbatim

		The first time the mesh is drawn with a shader its array states will be captured.
	*/

	void CompileArrayStates(Shader &shader);

	//! Releases array states associated with the specified shader.
	/*! If the specified shader is NULL, all array states are flushed. */
	void FlushArrayStates(Shader *shader);

	void Draw();
	void Release();

	void Reserve(int numStreams);

private:

	RB m_imp;
};

//! Defines a group of meshes and their associated materials.
class RADENG_CLASS MeshBundle {
public:
	typedef boost::shared_ptr<MeshBundle> Ref;

	static Ref New(const pkg::AssetRef &bundle);

	MeshBundle() {}
	~MeshBundle() {}

	const r::Mesh::Ref &Mesh(int idx) { 
		return m_meshes[idx]; 
	}
	
	pkg::AssetRef MaterialAsset(int idx) {
		RAD_ASSERT(m_loader);
		return m_loader->MaterialAsset(idx);
	}

	RAD_DECLARE_READONLY_PROPERTY(MeshBundle, asset, const pkg::AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(MeshBundle, materialLoader, asset::MeshMaterialLoader*);
	RAD_DECLARE_READONLY_PROPERTY(MeshBundle, numMeshes, int);

private:

	RAD_DECLARE_GET(asset, const pkg::AssetRef&) { 
		return m_asset; 
	}

	RAD_DECLARE_GET(materialLoader, asset::MeshMaterialLoader*) {
		return m_loader;
	}

	RAD_DECLARE_GET(numMeshes, int) { 
		return (int)m_meshes.size(); 
	}

	Mesh::Vec m_meshes;
	pkg::AssetRef m_asset;
	asset::MeshMaterialLoader *m_loader;
};

} // r


#include <Runtime/PopPack.h>
#include "Mesh.inl"
