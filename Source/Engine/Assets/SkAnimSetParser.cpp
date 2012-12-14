// SkAnimSetParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkAnimSetParser.h"
#include "../SkAnim/SkAnim.h"
#include "../Engine.h"
#include <Runtime/File.h>

using namespace pkg;

namespace asset {

SkAnimSetParser::SkAnimSetParser() : m_load(false) {
}

SkAnimSetParser::~SkAnimSetParser() {
}

int SkAnimSetParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

#if defined(RAD_OPT_TOOLS)
	if ((m_skad||m_load) && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#else
	if (m_load && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#endif

	if (flags&P_Unload) {
		m_load = false;
		m_mm.reset();
#if defined(RAD_OPT_TOOLS)
		m_skad.reset();
#endif
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && !(flags&P_FastPath))
		return Load(time, engine, asset, flags);
#endif

	return LoadCooked(time, engine, asset, flags);
}

int SkAnimSetParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (m_load)
		return SR_Success;

	if (!m_mm) {
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {
			Cooker::Ref cooker = asset->AllocateIntermediateCooker();
			CookStatus status = cooker->Status(0, P_TARGET_FLAGS(flags));

			if (status == CS_Ignore)
				return SR_CompilerError;

			if (status == CS_NeedRebuild) {
				COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
				int r = cooker->Cook(0, P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;
			} else {
				COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
			}

			String path(CStr(asset->path));
			path += ".bin";

			m_mm = cooker->MapFile(path.c_str, 0, ska::ZSka);
			if (!m_mm)
				return SR_FileNotFound;
		}
		else {
#endif
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bin";

		m_mm = engine.sys->files->MapFile(path.c_str, ska::ZSka);
		if (!m_mm)
			return SR_FileNotFound;
#if defined(RAD_OPT_TOOLS)
		}
#endif
	}

	int r = m_ska.Parse(m_mm->data, m_mm->size);
	m_load = r == SR_Success;
	return r;
}

#if defined(RAD_OPT_TOOLS)

int SkAnimSetParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	FILE *fp = engine.sys->files->fopen(
		s->c_str.get(), 
		"rt", 
		file::kFileOptions_None, 
		file::kFileMask_Base
	);

	if (!fp)
		return SR_FileNotFound;

	tools::SceneFileVec maps;
	char name[256];

	while (fgets(name, 256, fp) != 0) {

		for (char *c = name; *c; ++c) {
			if (*c < 20) {
				*c = 0;
				break;
			}
		}

		file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(name, ZTools);

		if (!ib) {
			fclose(fp);
			return SR_FileNotFound;
		}

		stream::InputStream is(*ib);
		tools::SceneFileRef map(new (ZTools) tools::SceneFile());

		if (!tools::LoadSceneFile(is, *map, false)) {
			fclose(fp);
			return SR_ParseError;
		}

		maps.push_back(map);
	}

	fclose(fp);

	s = asset->entry->KeyValue<String>("Compression", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	float compression = 1.f;
	sscanf(s->c_str, "%f", &compression);
	compression = std::max(0.f, std::min(compression, 1.f));

	m_skad = tools::CompileSkaData(
		asset->name,
		maps,
		0,
		compression
	);

	return m_skad ? SR_Success : SR_ParseError;
}
#endif

void SkAnimSetParser::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<SkAnimSetParser>();
}

} // asset
