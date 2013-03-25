/*! \file MeshCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "MeshCooker.h"
#include "MeshBundle.h"
#include "../Engine.h"
#include "../Tools/SceneFile.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

MeshCooker::MeshCooker() : Cooker(5) {
}

MeshCooker::~MeshCooker() {
}

CookStatus MeshCooker::Status(int flags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int MeshCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s || s->empty)
		return SR_MetaError;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(s->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;

	stream::InputStream is(*ib);

	tools::SceneFileRef map(new (ZTools) tools::SceneFile());
	tools::SceneFileVec vec;

	if (!tools::LoadSceneFile(is, *map, true))
		return SR_ParseError;

	ib.reset();

	vec.push_back(map);
	tools::DMeshBundleData::Ref bundleData = tools::CompileMeshBundle(asset->path, vec);
	if (!bundleData)
		return SR_ParseError;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref file = OpenWrite(path.c_str);
	if (!file)
		return SR_IOError;

	stream::OutputStream out(file->ob);
	if (out.Write(bundleData->data, (stream::SPos)bundleData->size, 0) != (stream::SPos)bundleData->size)
		return SR_IOError;

	// add material imports
	for (DMesh::Vec::const_iterator it = bundleData->bundle.meshes.begin(); it != bundleData->bundle.meshes.end(); ++it) {
		AddImport((*it).material);
	}

	return SR_Success;
}

void MeshCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MeshCooker>();
}

} // asset
