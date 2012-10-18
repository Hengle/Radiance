// EditorContentAssetThumbDimensionCache.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorContentAssetThumbDimensionCache.h"
#include "../../../App.h"
#include "../../../Engine.h"
#include "../../../COut.h"
#include <Runtime/File.h>
#include <Runtime/Stream.h>
#include <Runtime/PushSystemMacros.h>

namespace tools {
namespace editor {

namespace {
enum {
	kId = RAD_FOURCC_LE('T', 'H', 'S', 'C'),
	kVersion = 1
};
}

ContentAssetThumbDimensionCache *ContentAssetThumbDimensionCache::Get() {
	static ContentAssetThumbDimensionCache s_cache;
	return &s_cache;
}

ContentAssetThumbDimensionCache::ContentAssetThumbDimensionCache() : m_enableSaves(true) {
}

ContentAssetThumbDimensionCache::~ContentAssetThumbDimensionCache() {
}

void ContentAssetThumbDimensionCache::Load() {
	App::Get()->engine->sys->files->CreateDirectory("@r:/Temp");
	FILE *fp = App::Get()->engine->sys->files->fopen("@r:/Temp/thumbsizes.dat", "rb");
	if (!fp) {
		COut(C_Error) << "WARNING: unable to open Temp/thumbsizes.dat" << std::endl;
		return;
	}

	file::FILEInputBuffer ib(fp);
	stream::InputStream is(ib);

	U32 id;
	U32 version;

	if (!(is.Read(&id) && is.Read(&version))) {
		fclose(fp);
		return;
	}
		
	if (id != kId || version != kVersion) {
		fclose(fp);
		return;
	}

	U32 num;
	if (!is.Read(&num)) {
		fclose(fp);
		return;
	}

	App *app = App::Get();

	for (U32 i = 0; i < num; ++i) {
		Info info;
		String s;

		if (!is.Read(&s)) {
			fclose(fp);
			return;
		}
		
		if (!info.modified.Read(is)) {
			fclose(fp);
			return;
		}
		
		if (!is.Read(&info.width)) {
			fclose(fp);
			return;
		}
		
		if (!is.Read(&info.height)) {
			fclose(fp);
			return;
		}

		int id = app->engine->sys->packages->ResolveId(s.c_str);
		if (id == -1)
			continue;

		m_map[id] = info;
	}

	fclose(fp);
}

void ContentAssetThumbDimensionCache::Save() const {
	if (!m_enableSaves)
		return;

	FILE *fp = App::Get()->engine->sys->files->fopen("@r:/Temp/thumbsizes.dat", "wb");
	if (!fp) {
		COut(C_Error) << "ERROR: unable to open Temp/thumbsizes.dat" << std::endl;
		return;
	}

	file::FILEOutputBuffer ob(fp);
	stream::OutputStream os(ob);

	os.Write((U32)kId);
	os.Write((U32)kVersion);

	App *app = App::Get();

	U32 count = 0;
	for (Map::const_iterator it = m_map.begin(); it != m_map.end(); ++it) {
		const Info &info = it->second;

		pkg::Package::Entry::Ref e = app->engine->sys->packages->FindEntry(it->first);
		if (e)
			++count;
	}

	os.Write(count);
	
	for (Map::const_iterator it = m_map.begin(); it != m_map.end(); ++it) {
		const Info &info = it->second;

		pkg::Package::Entry::Ref e = app->engine->sys->packages->FindEntry(it->first);
		if (!e)
			continue;

		os.Write(CStr(e->path));
		info.modified.Write(os);
		os.Write(info.width);
		os.Write(info.height);
	}

	fclose(fp);
}

void ContentAssetThumbDimensionCache::Delete(int id) {
	m_map.erase(id);
}

const ContentAssetThumbDimensionCache::Info *ContentAssetThumbDimensionCache::Find(int id) const {
	Map::const_iterator it = m_map.find(id);
	if (it != m_map.end())
		return &it->second;
	return 0;
}

void ContentAssetThumbDimensionCache::Update(int id, const Info &info) {
	m_map[id] = info;
}

} // editor
} // tools
