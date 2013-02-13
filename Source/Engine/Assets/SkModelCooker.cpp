// SkModelCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkModelCooker.h"
#include "../Engine.h"
#include "../SkAnim/SkBuilder.h"
#include "SkAnimSetParser.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

SkModelCooker::SkModelCooker() : Cooker(6) {
}

SkModelCooker::~SkModelCooker() {
}

CookStatus SkModelCooker::CheckRebuild(int flags, int allflags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Mesh.Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

CookStatus SkModelCooker::Status(int flags, int allflags) {
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0) { 
		// generic only gets cooked if all platforms are identical
		if (MatchTargetKeys(allflags, allflags)==allflags)
			return CheckRebuild(flags, allflags);
		return CS_Ignore;
	}

	if (MatchTargetKeys(allflags, allflags)==allflags)
		return CS_Ignore; // generics are selected

	return CS_NeedRebuild;
}

int SkModelCooker::Compile(int flags, int allflags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Mesh.Source.File");

	const String *s = asset->entry->KeyValue<String>("AnimSet.Source", flags);
	if (!s || s->empty)
		return SR_MetaError;

	// CompileSkmData needs the ska info, so load it.

	Asset::Ref skaRef = engine->sys->packages->Resolve(s->c_str, asset->zone);
	if (!skaRef)
		return SR_FileNotFound;

	int r = skaRef->Process(
		xtime::TimeSlice::Infinite,
		flags|P_Parse|P_TargetDefault
	);

	if (r != SR_Success)
		return r;

	SkAnimSetParser *ska = SkAnimSetParser::Cast(skaRef);
	if (!ska || !ska->valid)
		return SR_ParseError;

	// AnimSet import index
	AddImport(s->c_str, flags);

	s = asset->entry->KeyValue<String>("AnimStates.Source", flags);
	if (!s)
		return SR_MetaError;

	// AnimStates import index
	AddImport(s->c_str, flags);

	// Load 3DX Mesh file
	s = asset->entry->KeyValue<String>("Mesh.Source.File", flags);
	if (!s)
		return SR_MetaError;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(s->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;
	
	stream::InputStream is (*ib);

	tools::SceneFile map;
	if (!tools::LoadSceneFile(is, map, false))
		return SR_ParseError;

	ib.reset();

	tools::SkmData::Ref skmd = tools::CompileSkmData(
		asset->name,
		map,
		0,
		ska::kSkinType_CPU,
		*ska->dska.get()
	);

	String path(CStr(asset->path));
	String path2(path);
	path2 += ".0.bin";

	// File 0 (discardable after load)
	{
		BinFile::Ref skmFile = OpenWrite(path2.c_str, flags);
		if (!skmFile)
			return SR_IOError;

		stream::OutputStream skmOut(skmFile->ob);
		if (skmOut.Write(skmd->skmData[0], (stream::SPos)skmd->skmSize[0], 0) != (stream::SPos)skmd->skmSize[0])
			return SR_IOError;
	}

	path2 = path;
	path2 += ".1.bin";

	// File 1 (persisted)
	{
		BinFile::Ref skmFile = OpenWrite(path2.c_str, flags);
		if (!skmFile)
			return SR_IOError;

		stream::OutputStream skmOut(skmFile->ob);
		if (skmOut.Write(skmd->skmData[1], (stream::SPos)skmd->skmSize[1], 0) != (stream::SPos)skmd->skmSize[1])
			return SR_IOError;
	}

	// add material imports

	for (ska::DMesh::Vec::const_iterator it = skmd->dskm.meshes.begin(); it != skmd->dskm.meshes.end(); ++it) {
		AddImport((*it).material, flags);
	}

	return SR_Success;
}

int SkModelCooker::MatchTargetKeys(int flags, int allflags) {
	return asset->entry->MatchTargetKeys<String>("Mesh.Source.File", flags, allflags);
}

void SkModelCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SkModelCooker>();
}

} // asset
