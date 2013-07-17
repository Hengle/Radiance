// Mesh.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace r {

inline Mesh::Mesh() {
}

inline Mesh::~Mesh() {
}

inline void Mesh::Reserve(int numStreams) {
	m_imp.Reserve(numStreams);
}

inline void Mesh::AllocateSwapChains(int num) {
	m_imp.AllocateSwapChains(num);
}

inline void Mesh::SwapChain() {
	m_imp.SwapChain();
}

inline int Mesh::AllocateStream(StreamUsage usage, int vertexSize, int vertexCount, bool swapChain) {
	return m_imp.AllocateStream(usage, vertexSize, vertexCount, swapChain);
}

inline Mesh::StreamPtr::Ref Mesh::Map(int stream) {
	return m_imp.Map(stream);
}

inline void Mesh::MapSource(
	int stream, 
	MaterialGeometrySource source, 
	int index,
	int stride,
	int ofs,
	int count
) {
	m_imp.MapSource(stream, source, index, stride, ofs, count);
}

inline Mesh::StreamPtr::Ref Mesh::MapIndices(StreamUsage usage, int elemSize, int count) {
	return m_imp.MapIndices(usage, elemSize, count);
}

inline void Mesh::CompileArrayStates(Shader &shader) {
	m_imp.CompileArrayStates(shader);
}

inline void Mesh::FlushArrayStates(Shader *shader) {
	m_imp.FlushArrayStates(shader);
}

inline void Mesh::Bind(MaterialGeometrySource source, int index) {
	m_imp.Bind(source, index);
}

inline void Mesh::BindAll(MaterialGeometrySource source) {
	for (int i = 0; i < kMaterialGeometrySource_MaxIndices; ++i)
		Bind(source, i);
}

inline void Mesh::BindAll(Shader *shader) {
	m_imp.BindAll(shader);
}

inline void Mesh::BindIndices() {
	m_imp.BindIndices();
}

inline void Mesh::Draw(int numTris) {
	m_imp.Draw(numTris);
}

inline void Mesh::Release() {
	m_imp.Release();
}

} // r
