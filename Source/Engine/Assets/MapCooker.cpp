// MapCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MapCooker.h"
#include "../World/World.h"
#include "../World/MapBuilder/MapBuilder.h"
#include "MapParser.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

MapCooker::MapCooker() : Cooker(28), m_parsing(false), m_ui(0), m_parser(0) {
}

MapCooker::~MapCooker() {
}

CookStatus MapCooker::Status(int flags, int allflags) {
	// Maps only cook on the generics path
	flags &= P_AllTargets;

	if (flags == 0) {
		if (CompareVersion(flags) ||
			CompareModifiedTime(flags) ||
			CompareCachedFileTimeKey(flags, "Source.File")) {
			return CS_NeedRebuild;
		}

		// Check 3DX timestamp

		const String *mapPath = asset->entry->KeyValue<String>("Source.File", flags);
		if (!mapPath || mapPath->empty)
			return CS_NeedRebuild;

		String actorPath = file::SetFileExtension(mapPath->c_str, ".actors");
		if (CompareCachedFileTime(flags, "Source.File.Actors", actorPath.c_str, true, true))
			return CS_NeedRebuild;

		file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(mapPath->c_str, ZTools);
		if (!ib)
			return CS_NeedRebuild;

		m_script.Bind(ib);
		ib.reset();

		int r;
		CookStatus status = CS_UpToDate;
		tools::map_builder::EntSpawn spawn;

		while ((r=MapParser::ParseEntity(m_script, spawn))==SR_Success) {
			const char *sz = spawn.keys.StringForKey("classname");
			if (!sz)
				continue;
			if (!string::cmp(sz, "static_mesh_scene")) {
				sz = spawn.keys.StringForKey("file");
				if (sz) {
					String path(sz);
					path += ".3dx";
					if (CompareCachedFileTime(flags, path.c_str, path.c_str)) {
						status = CS_NeedRebuild;
						break;
					}
				}
			}
		}

		if (r != SR_Success && r != SR_End)
			status = CS_NeedRebuild; // force error

		return status;
	}

	return CS_Ignore;
}

int MapCooker::Compile(int flags, int allflags) {

	if (m_ui) // progress indicator makes cooker a tickable object
		return TickCompile(flags, allflags);

	int r;
	while((r=TickCompile(flags, allflags)) == SR_Pending) {
		if (m_mapBuilder)
			m_mapBuilder->WaitForCompletion();
	}

	return r;
}

int MapCooker::TickCompile(int flags, int allflags) {
	RAD_ASSERT((flags&P_AllTargets)==0);

	if (m_parsing) {
		int r = m_mapBuilder->result;
		if (r != SR_Success)
			return r;

		tools::map_builder::EntSpawn spawn;
		r = MapParser::ParseEntity(m_script, spawn);
		if ((r != SR_Success) && (r != SR_End))
			return r;

		if (r == SR_End) {
			m_parsing = false;
			r = ParseCinematicCompressionMap(flags);
			if (r != SR_Success)
				return r;
			m_mapBuilder->SetCinematicActorCompression(m_caMap);
			if (!m_mapBuilder->SpawnCompile())
				return SR_ParseError;
			m_script.Reset();
		} else {
			RAD_ASSERT(r == SR_Success);
			// For static_mesh_scene, cache the 3DX filetime
			const char *sz = spawn.keys.StringForKey("classname");
			if (sz && !string::cmp(sz, "static_mesh_scene")) {
				sz = spawn.keys.StringForKey("file");
				if (sz) {
					String path(sz);
					path += ".3dx";
					CompareCachedFileTime(flags, path.c_str, path.c_str);
				}
			}

			if (!m_mapBuilder->LoadEntSpawn(spawn))
				return SR_ParseError;

			return SR_Pending;
		}
	}

	if (m_mapBuilder) {
		int r = m_mapBuilder->result;
		if (r != SR_Success)
			return r;

		String path(CStr(asset->path));
		path += ".bsp";
		BinFile::Ref fp = OpenWrite(path.c_str, flags);
		if (!fp)
			return SR_IOError;

		stream::OutputStream os(fp->ob);
		if (m_mapBuilder->bspFileBuilder->Write(os) != pkg::SR_Success)
			return SR_IOError;

		world::bsp_file::BSPFile::Ref bspFile = m_mapBuilder->bspFile;
		RAD_ASSERT(bspFile);

		for (U32 i = 0; i < bspFile->numMaterials; ++i) { 
			// import materials.
			AddImport(bspFile->String((bspFile->Materials()+i)->string), flags);
		}

		return SR_Success;
	}

	m_parsing = true;
	
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *mapPath = asset->entry->KeyValue<String>("Source.File", flags);
	if (!mapPath || mapPath->empty)
		return SR_MetaError;

	String actorPath = file::SetFileExtension(mapPath->c_str, ".actors");
	CompareCachedFileTime(flags, "Source.File.Actors", actorPath.c_str, true, true);

	//	cout.get() << "********" << std::endl << "Loading :" << mapPath->c_str() << std::endl;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(mapPath->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;
	
	m_script.Bind(ib);
	ib.reset();
		
	m_mapBuilder.reset(new (ZTools) tools::map_builder::MapBuilder(*engine.get()));
	m_mapBuilder->SetProgressIndicator(m_ui);

	return SR_Pending;
}

void MapCooker::SetProgressIndicator(tools::UIProgress *ui) {
	m_ui = ui;
}

int MapCooker::ParseCinematicCompressionMap(int flags) {
	m_caMap.clear();

	const String *name = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!name || name->empty)
		return SR_MetaError;

	String file = file::SetFileExtension(name->c_str, ".actors");
	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(
		file.c_str, 
		ZWorld,
		1*Meg,
		file::kFileOptions_None,
		file::kFileMask_Base
	);

	if (!ib)
		return SR_Success; // not a required file.

	Tokenizer script(ib);
	return MapParser::ParseCinematicCompressionMap(script, m_caMap);
}

void MapCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MapCooker>();
}

} // asset
