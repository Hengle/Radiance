/*! \file SkAnimSetParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "SkAnimSetParser.h"
#include "../SkAnim/SkAnim.h"
#include "../Engine.h"
#include <Runtime/File.h>
#include <Runtime/Tokenizer.h>

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
			CookStatus status = cooker->Status(P_TARGET_FLAGS(flags));

			if (status == CS_Ignore)
				return SR_CompilerError;

			if (status == CS_NeedRebuild) {
				COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
				int r = cooker->Cook(P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;
			} else {
				COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
			}

			String path(CStr(asset->path));
			path += ".bin";

			m_mm = cooker->MapFile(path.c_str, ska::ZSka);
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

int SkAnimSetParser::LoadToolsFile(
	const char *path,
	Engine &engine,
	StringVec *sources,
	tools::SkaCompressionMap *compression
) {
	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(
		path, 
		ZTools, 
		1*kMeg,
		file::kFileOptions_None, 
		file::kFileMask_Base
	);

	if (!ib)
		return SR_FileNotFound;

	Tokenizer script(ib);
	ib.reset();

	String token;
	const String kFiles("files");
	const String kCompression("compression");

	while (script.GetToken(token)) {
		if (token == kFiles) {
			if (!script.IsNextToken("{"))
				return SR_InvalidFormat;
			for (;;) {
				if (!script.GetToken(token))
					return SR_InvalidFormat;
				if (token == "}")
					break;

				if (sources)
					sources->push_back(token);
			}
		} else if (token == kCompression) {
			if (!script.IsNextToken("{"))
				return SR_InvalidFormat;
			for (;;) {
				if (!script.GetToken(token))
					return SR_InvalidFormat;
				if (token == "}")
					break;

				if (!script.IsNextToken("=", Tokenizer::kTokenMode_SameLine))
					return SR_InvalidFormat;
				
				String level;
				if (!script.GetToken(level, Tokenizer::kTokenMode_SameLine))
					return SR_InvalidFormat;

				if (compression) {
					if (compression->find(token) != compression->end())
						return SR_InvalidFormat;

					float flevel;
					sscanf(level.c_str, "%f", &flevel);
					flevel = std::max(0.f, std::min(1.f, flevel));
					compression->insert(tools::SkaCompressionMap::value_type(token, flevel));
				}
			}
		} else {
			return SR_InvalidFormat;
		}
	}

	return SR_Success;
}

int SkAnimSetParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	StringVec sourceVec;
	tools::SkaCompressionMap compression;

	int r = LoadToolsFile(
		s->c_str,
		engine,
		&sourceVec,
		&compression
	);

	if (r != SR_Success)
		return r;

	tools::SceneFileVec sources;

	for (StringVec::const_iterator it = sourceVec.begin(); it != sourceVec.end(); ++it) {
	
		file::MMFileInputBuffer::Ref scene = engine.sys->files->OpenInputBuffer((*it).c_str, ZTools);

		if (!scene) {
			COut(C_Error) << "ERROR: Unable to open file '" << *it << "'" << std::endl;
			return SR_FileNotFound;
		}

		stream::InputStream is(*scene);
		tools::SceneFileRef source(new (ZTools) tools::SceneFile());

		if (!tools::LoadSceneFile(is, *source, false))
			return SR_ParseError;

		if (source->worldspawn->models.size() != 1) {
			COut(C_Error) << "ERROR: 3DX file should only contain 1 model, it contains " << source->worldspawn->models.size() << ". File: '" << *it << "'" << std::endl;
			return SR_ParseError;
		}

		sources.push_back(source);
	}

	m_skad = tools::CompileSkaData(
		asset->name,
		sources,
		0,
		&compression
	);

	return m_skad ? SR_Success : SR_ParseError;
}
#endif

void SkAnimSetParser::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<SkAnimSetParser>();
}

} // asset
