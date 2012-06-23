// MeshParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "MeshParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

MeshParser::MeshParser() : m_valid(false)
{
}

MeshParser::~MeshParser()
{
}

int MeshParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_valid && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (flags&P_Unload)
	{
#if defined(RAD_OPT_TOOLS)
		m_bundleData.reset();
#endif
		m_buf.Close();
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && !(flags&P_FastPath))
		return Load(time, engine, asset, flags);
#endif

	return LoadCooked(time, engine, asset, flags);
}

int MeshParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
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
			r::ZRender
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

	int r = m_bundle.Parse(m_buf->data->ptr, m_buf->data->size);
	if (r < SR_Success)
		return r;

	m_valid = true;
	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

int MeshParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	int media = file::AllMedia;
	file::HStreamInputBuffer ib;

	int r = engine.sys->files->OpenFileStream(
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
	m_bundleData = tools::CompileMeshBundle(asset->path, vec);
	if (m_bundleData)
		m_valid = true;
	return m_valid ? SR_Success : SR_CompilerError;
}

#endif
	
void MeshParser::Register(Engine &engine)
{
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<MeshParser>();
}

} // asset
