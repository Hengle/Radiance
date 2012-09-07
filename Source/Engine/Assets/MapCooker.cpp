// MapCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "MapCooker.h"
#include "../World/World.h"
#include "../World/MapBuilder/MapBuilder.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

MapCooker::MapCooker() : Cooker(10) {
}

MapCooker::~MapCooker() {
}

CookStatus MapCooker::Status(int flags, int allflags) {
	// Maps only cook on the generics path
	flags &= P_AllTargets;

	if (flags == 0) {
		if (CompareVersion(flags) ||
			CompareModifiedTime(flags) ||
			CompareCachedFileTimeKey(flags, "Source.File"))
		{
			return CS_NeedRebuild;
		}

		// Check 3DX timestamp

		const String *mapPath = asset->entry->KeyValue<String>("Source.File", flags);
		if (!mapPath || mapPath->empty)
			return CS_NeedRebuild;

		file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(mapPath->c_str, ZTools);
		if (!ib)
			return CS_NeedRebuild;

		m_script.Bind(ib);
		ib.reset();

		int r;
		CookStatus status = CS_UpToDate;
		world::EntSpawn spawn;

		while ((r=ParseEntity(spawn))==SR_Success) {
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
	RAD_ASSERT((flags&P_AllTargets)==0);

	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	const String *mapPath = asset->entry->KeyValue<String>("Source.File", flags);
	if (!mapPath || mapPath->empty)
		return SR_MetaError;

//	cout.get() << "********" << std::endl << "Loading :" << mapPath->c_str() << std::endl;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer(mapPath->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;
	
	m_script.Bind(ib);
	ib.reset();

	::tools::MapBuilder mapBuilder(*engine.get());

	int r;
	world::EntSpawn spawn;
	while ((r=ParseEntity(spawn))==SR_Success) {
		if (!mapBuilder.LoadEntSpawn(spawn))
			return SR_ParseError;

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
	}

	r = r == SR_End ? SR_Success : r;
	if (r != SR_Success)
		return r;

	m_script.Reset();

//	cout.get() << "Compiling..." << std::endl;

	if (!mapBuilder.Compile())
		return SR_CompilerError;

	String path(CStr(asset->path));
	path += ".bsp";
	BinFile::Ref fp = OpenWrite(path.c_str, flags);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);
	if (mapBuilder.bspFileBuilder->Write(os) != pkg::SR_Success)
		return SR_IOError;

	world::bsp_file::BSPFile::Ref bspFile = mapBuilder.bspFile;
	RAD_ASSERT(bspFile);

	for (U32 i = 0; i < bspFile->numMaterials; ++i) { 
		// import materials.
		AddImport(bspFile->String((bspFile->Materials()+i)->string), flags);
	}

	return SR_Success;
}

int MapCooker::ParseEntity(world::EntSpawn &spawn) {
	spawn.keys.pairs.clear();
	return ParseScript(spawn);
}

int MapCooker::ParseScript(world::EntSpawn &spawn) {
	String token, value, temp;

	if (!m_script.GetToken(token, Tokenizer::kTokenMode_CrossLine))
		return SR_End;
	if (token != "{")
		return SR_ParseError;

	for (;;) {
		if (!m_script.GetToken(token, Tokenizer::kTokenMode_CrossLine))
			return SR_ParseError;
		if (token == "}")
			break;
		if (!m_script.GetToken(value, Tokenizer::kTokenMode_CrossLine))
			return SR_ParseError;

		// turn "\n" into '\n'
		const char *sz = value.c_str;
		temp.Clear();

		while (*sz) {
			if (sz[0] == '\\' && sz[1] == 'n') {
				temp += '\n';
				++sz;
			} else {
				temp += *sz;
			}
			++sz;
		}

		spawn.keys.pairs[token] = temp;
	}

	return SR_Success;
}

void MapCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<MapCooker>();
}

} // asset
