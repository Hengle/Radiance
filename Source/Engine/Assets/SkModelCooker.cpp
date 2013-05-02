/*! \file SkModelCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "SkModelCooker.h"
#include "../Engine.h"
#include "../SkAnim/SkBuilder.h"
#include "SkAnimSetParser.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

SkModelCooker::SkModelCooker() : Cooker(9) {
}

SkModelCooker::~SkModelCooker() {
}

CookStatus SkModelCooker::Status(int flags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Mesh.Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int SkModelCooker::Compile(int flags) {
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
	AddImport(s->c_str);

	s = asset->entry->KeyValue<String>("AnimStates.Source", flags);
	if (!s)
		return SR_MetaError;

	// AnimStates import index
	AddImport(s->c_str);

	// Load 3DX Mesh file
	s = asset->entry->KeyValue<String>("Mesh.Source.File", flags);
	if (!s)
		return SR_MetaError;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(s->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;
	
	stream::InputStream is (*ib);

	tools::SceneFile mesh;
	if (!tools::LoadSceneFile(is, mesh, true))
		return SR_ParseError;

	ib.reset();

	if (mesh.worldspawn->models.size() != 1) {
		COut(C_Error) << "ERROR: 3DX file should only contain 1 model, it contains " << mesh.worldspawn->models.size() << ". File: '" << *s << "'" << std::endl;
		return SR_ParseError;
	}

	tools::SkmData::Ref skmd = tools::CompileSkmData(
		asset->name,
		mesh,
		0,
		ska::kSkinType_CPU,
		*ska->dska.get()
	);

	if (!skmd) {
		COut(C_Error) << "ERROR: " << asset->name.get() << " does not contain a skeletal animated mesh." << std::endl;
		return SR_CompilerError;
	}

	String path(CStr(asset->path));
	String path2(path);
	path2 += ".0.bin";

	// File 0 (discardable after load)
	{
		BinFile::Ref skmFile = OpenWrite(path2.c_str);
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
		BinFile::Ref skmFile = OpenWrite(path2.c_str);
		if (!skmFile)
			return SR_IOError;

		stream::OutputStream skmOut(skmFile->ob);
		if (skmOut.Write(skmd->skmData[1], (stream::SPos)skmd->skmSize[1], 0) != (stream::SPos)skmd->skmSize[1])
			return SR_IOError;
	}

	// add material imports

	for (ska::DSkMesh::Vec::const_iterator it = skmd->dskm.meshes.begin(); it != skmd->dskm.meshes.end(); ++it) {
		AddImport((*it).material);
	}

	return SR_Success;
}

void SkModelCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SkModelCooker>();
}

} // asset
