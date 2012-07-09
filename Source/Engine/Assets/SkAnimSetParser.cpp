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

SkAnimSetParser::SkAnimSetParser() : m_load(false)
{
}

SkAnimSetParser::~SkAnimSetParser()
{
}

int SkAnimSetParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

#if defined(RAD_OPT_TOOLS)
	if ((m_skad||m_load) && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#else
	if (m_load && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#endif

	if (flags&P_Unload)
	{
		m_load = false;
		m_buf.Close();
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
)
{
	if (m_load)
		return SR_Success;

	if (!m_buf)
	{
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked)
		{
			Cooker::Ref cooker = asset->AllocateIntermediateCooker();
			CookStatus status = cooker->Status(0, P_TARGET_FLAGS(flags));

			if (status == CS_Ignore)
				return SR_CompilerError;

			if (status == CS_NeedRebuild)
			{
				COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
				int r = cooker->Cook(0, P_TARGET_FLAGS(flags));
				if (r != SR_Success)
					return r;
			}
			else
			{
				COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
			}

			String path(CStr(asset->path));
			path += ".bin";

			int media = file::AllMedia;
			int r = cooker->LoadFile( // load cooked data.
				path.c_str,
				0,
				media,
				m_buf,
				file::HIONotify()
			);

			if (r < SR_Success)
				return r;
		}
		else {
#endif
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bin";

		int media = file::AllMedia;
		int r = engine.sys->files->LoadFile(
			path.c_str,
			media,
			m_buf,
			file::HIONotify(),
			8,
			ska::ZSka
		);

		if (r < SR_Success)
			return r;
#if defined(RAD_OPT_TOOLS)
		}
#endif
	}

	if (m_buf->result == SR_Pending)
	{
		if (time.infinite)
			m_buf->WaitForCompletion();
		else
			return SR_Pending;
	}

	if (m_buf->result < SR_Success)
		return m_buf->result;

	int r = m_ska.Parse(m_buf->data->ptr, m_buf->data->size);
	m_load = r == SR_Success;
	return r;
}

#if defined(RAD_OPT_TOOLS)

int SkAnimSetParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	char path[256];
	char native[256];
	string::cpy(path, "9:/");
	string::cat(path, engine.sys->files->hddRoot.get());
	string::cat(path, "/");
	string::cat(path, s->c_str.get());
	if (!file::ExpandToNativePath(path, native, 256))
		return SR_MetaError;

	FILE *fp = fopen(native, "rt");
	if (fp == 0)
		return SR_MissingFile;

	tools::MapVec maps;
	char name[256];

	while (fgets(name, 256, fp) != 0)
	{
		for (char *c = name; *c; ++c)
		{
			if (*c < 20)
			{
				*c = 0;
				break;
			}
		}

		int media = file::AllMedia;
		file::HStreamInputBuffer ib;

		int r = engine.sys->files->OpenFileStream(
			name,
			media, 
			ib,
			file::HIONotify()
		);

		if (r < file::Success)
		{
			fclose(fp);
			return SR_MissingFile;
		}

		stream::InputStream is(ib->buffer);
		tools::MapRef map(new (ZTools) tools::Map());

		if (!tools::LoadMaxScene(is, *map, false))
		{
			fclose(fp);
			return SR_ParseError;
		}

		maps.push_back(map);
	}

	fclose(fp);

	m_skad = tools::CompileSkaData(
		asset->name,
		maps,
		0
	);

	return m_skad ? SR_Success : SR_ParseError;
}
#endif

void SkAnimSetParser::Register(Engine &engine)
{
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<SkAnimSetParser>();
}

} // asset
