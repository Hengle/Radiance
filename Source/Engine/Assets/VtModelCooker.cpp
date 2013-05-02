/*! \file VtModelCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "VtModelCooker.h"
#include "SkAnimSetParser.h"
#include "../Engine.h"
#include "../SkAnim/SkBuilder.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

VtModelCooker::VtModelCooker() : Cooker(2) {
}

VtModelCooker::~VtModelCooker() {
}

CookStatus VtModelCooker::Status(int flags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Mesh.Source.File") ||
		CompareCachedFileTimeKey(flags, "Anims.Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int VtModelCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Mesh.Source.File");
	CompareCachedFileTimeKey(flags, "Anims.Source.File");

	const String *s = asset->entry->KeyValue<String>("AnimStates.Source", flags);
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

	if (mesh.worldspawn->models.size() != 1) {
		COut(C_Error) << "ERROR: 3DX file should only contain 1 model, it contains " << mesh.worldspawn->models.size() << ". File: '" << *s << "'" << std::endl;
		return SR_ParseError;
	}

	ib.reset();

	s = asset->entry->KeyValue<String>("Anims.Source.File", flags);
	if (!s)
		return SR_MetaError;

	StringVec animSources;
	SkAnimSetParser::LoadToolsFile(s->c_str, *engine, &animSources, 0);

	if (animSources.empty())
		return SR_ParseError;

	tools::SceneFileVec anims;
	for (StringVec::const_iterator it = animSources.begin(); it != animSources.end(); ++it) {
		ib = engine->sys->files->OpenInputBuffer((*it).c_str, ZTools);
		if (!ib)
			return SR_FileNotFound;
		stream::InputStream is(*ib);
		tools::SceneFileRef x(new (tools::Z3DX) tools::SceneFile());
		if (!tools::LoadSceneFile(is, *x, true))
			return SR_ParseError;
		anims.push_back(x);
	}

	tools::VtmData::Ref vtmd = tools::CompileVtmData(
		asset->name,
		mesh,
		anims,
		0
	);

	if (!vtmd)
		return SR_CompilerError;

	String path(CStr(asset->path));
	String path2(path);
	path2 += ".0.bin";

	// File 0 (discardable after load)
	{
		BinFile::Ref skmFile = OpenWrite(path2.c_str);
		if (!skmFile)
			return SR_IOError;

		stream::OutputStream skmOut(skmFile->ob);
		if (skmOut.Write(vtmd->vtmData[0], (stream::SPos)vtmd->vtmSize[0], 0) != (stream::SPos)vtmd->vtmSize[0])
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
		if (skmOut.Write(vtmd->vtmData[1], (stream::SPos)vtmd->vtmSize[1], 0) != (stream::SPos)vtmd->vtmSize[1])
			return SR_IOError;
	}

	// add material imports

	for (ska::DVtMesh::Vec::const_iterator it = vtmd->dvtm.meshes.begin(); it != vtmd->dvtm.meshes.end(); ++it) {
		AddImport((*it).material);
	}

	return SR_Success;
}

void VtModelCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<VtModelCooker>();
}

} // asset
