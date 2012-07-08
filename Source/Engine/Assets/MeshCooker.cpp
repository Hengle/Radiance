// MeshCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "MeshCooker.h"
#include "MeshBundle.h"
#include "../Engine.h"
#include "../Tools/MaxScene.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

MeshCooker::MeshCooker() : Cooker(0)
{
}

MeshCooker::~MeshCooker()
{
}

CookStatus MeshCooker::CheckRebuild(int flags, int allflags)
{
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus MeshCooker::Status(int flags, int allflags)
{
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0)
	{ // generic only gets cooked if all platforms are identical
		if (MatchTargetKeys(allflags, allflags)==allflags)
			return CheckRebuild(flags, allflags);
		return CS_Ignore;
	}

	if (MatchTargetKeys(allflags, allflags)==allflags)
		return CS_Ignore; // generics are selected

	// only build ipad if different from iphone
	if ((flags&P_TargetIPad) && (allflags&P_TargetIPhone))
	{
		if (MatchTargetKeys(P_TargetIPad, P_TargetIPhone))
			return CS_Ignore;
	}

	return CheckRebuild(flags, allflags);
}

int MeshCooker::Compile(int flags, int allflags)
{
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s || s->empty)
		return SR_MetaError;

	int media = file::AllMedia;
	file::HStreamInputBuffer ib;

	int r = engine->sys->files->OpenFileStream(
		s->c_str,
		media,
		ib,
		file::HIONotify()
	);

	if (r < file::Success)
		return r;

	stream::InputStream is(ib->buffer);

	tools::MapRef map(new (ZTools) tools::Map());
	tools::MapVec vec;

	if (!tools::LoadMaxScene(is, *map, false))
		return SR_ParseError;

	ib.Close();

	vec.push_back(map);
	tools::DMeshBundleData::Ref bundleData = tools::CompileMeshBundle(asset->path, vec);
	if (!bundleData)
		return SR_ParseError;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref file = OpenWrite(path.c_str, flags);
	if (!file)
		return SR_IOError;

	stream::OutputStream out(file->ob);
	if (out.Write(bundleData->data, (stream::SPos)bundleData->size, 0) != (stream::SPos)bundleData->size)
		return SR_IOError;

	// add material imports
	for (DMesh::Vec::const_iterator it = bundleData->bundle.meshes.begin(); it != bundleData->bundle.meshes.end(); ++it)
	{
		AddImport((*it).material, flags);
	}

	return SR_Success;
}

int MeshCooker::MatchTargetKeys(int flags, int allflags)
{
	return asset->entry->MatchTargetKeys<String>("Source.File", flags, allflags);
}

void MeshCooker::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MeshCooker>();
}

} // asset
