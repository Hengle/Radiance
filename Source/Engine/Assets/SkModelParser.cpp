// SkModelParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkModelParser.h"
#include "../SkAnim/SkAnim.h"
#include "../Engine.h"
#include <Runtime/Base/SIMD.h>

using namespace pkg;

namespace asset {

SkModelParser::SkModelParser() : m_state(S_None)
{
}

SkModelParser::~SkModelParser()
{
}

int SkModelParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

#if defined(RAD_OPT_TOOLS)
	if ((m_skmd||m_state==S_Done) && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#else
#pragma message("TODO: Implement P_Trim")
	// This is unfinished. SkMesh has 2 files, one is persistent, mesh weights, verts (for cpu skin), bone indices etc.
	// The other is meant to be free'd after the model is created. BUT we can't free the transient data
	// (texture coords, indices, etc) because they are references repeatedly by new instances of SkMesh. What I should
	// do is have SkModelParser move its transient data into those vertex buffers and expose them as part of its interface.
	// Work todo in the future, right now it's not going to save enough memory to care too much.
	//
	// Technically this is broken because the entire concept of this asset sink parser mechanism is to expose the "static" side
	// of the data in whatever form it should take place. TextureParser does this correctly, it moves the data into a GLTexture
	// as a later sink stage.
	//
	// So we need to make a GLSkModelParser that moves the SkModelParser transient data into some VB's and then when we get a P_Trim
	// we can properly release this data.
	if ((m_state==S_Done) && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#endif

	if (flags&P_Unload)
	{
		m_state = S_None;
		m_ska.reset();
		m_skaRef.reset();
		m_states.reset();
		m_statesRef.reset();
		m_buf[0].Close();
		m_buf[1].Close();
#if defined(RAD_OPT_TOOLS)
		m_skmd.reset();
		m_cooker.reset();
#endif
		return SR_Success;
	}

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked && !(flags&P_FastPath))
		return Load(time, engine, asset, flags);
#endif

	return LoadCooked(time, engine, asset, flags);	
}

int SkModelParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (m_state == S_None)
	{
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked)
		{
			const String *s = asset->entry->KeyValue<String>("AnimSet.Source", P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			m_skaRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
			if (!m_skaRef)
				return SR_MissingFile;

			int r = m_skaRef->Process(
				xtime::TimeSlice::Infinite,
				flags
			);

			if (r != SR_Success)
				return r;

			m_ska = SkAnimSetParser::Cast(m_skaRef);
			if (!m_ska || !m_ska->valid)
				return SR_ParseError;

			s = asset->entry->KeyValue<String>("AnimStates.Source", P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			m_statesRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
			if (!m_statesRef)
				return SR_MissingFile;

			r = m_statesRef->Process(
				xtime::TimeSlice::Infinite,
				flags
			);

			if (r != SR_Success)
				return r;

			m_states = SkAnimStatesParser::Cast(m_statesRef);
			if (!m_states || !m_states->valid)
				return SR_ParseError;
		}
		else {
#endif
		const Package::Entry::Import *import = asset->entry->Resolve(0);
		if (!import)
			return SR_MetaError;

		m_skaRef = asset->entry->Resolve(*import, asset->zone);
		if (!m_skaRef)
			return SR_MetaError;

		int r = m_skaRef->Process(
			xtime::TimeSlice::Infinite,
			flags
		);

		if (r != SR_Success)
			return r;

		m_ska = SkAnimSetParser::Cast(m_skaRef);
		if (!m_ska || !m_ska->valid)
			return SR_ParseError;

		import = asset->entry->Resolve(1);
		if (!import)
			return SR_MetaError;

		m_statesRef = asset->entry->Resolve(*import, asset->zone);
		if (!m_statesRef)
			return SR_MissingFile;

		r = m_statesRef->Process(
			xtime::TimeSlice::Infinite,
			flags
		);

		if (r != SR_Success)
			return r;

		m_states = SkAnimStatesParser::Cast(m_statesRef);
		if (!m_states || !m_states->valid)
			return SR_ParseError;
#if defined(RAD_OPT_TOOLS)
		}
#endif

		m_state = S_Load0;
	}

	if (m_state == S_Load0)
	{
		if (!m_buf[0])
		{
#if defined(RAD_OPT_TOOLS)
			if (!asset->cooked)
			{
				m_cooker = asset->AllocateIntermediateCooker();
				CookStatus status = m_cooker->Status(0, P_TARGET_FLAGS(flags));

				if (status == CS_Ignore)
					return SR_CompilerError;

				if (status == CS_NeedRebuild)
				{
					COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
					int r = m_cooker->Cook(0, P_TARGET_FLAGS(flags));
					if (r != SR_Success)
						return r;
				}
				else
				{
					COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
				}

				String path(CStr(asset->path));
				path += ".0.bin";

				int media = file::AllMedia;
				int r = m_cooker->LoadFile( // load cooked data.
					path.c_str,
					0,
					media,
					m_buf[0],
					file::HIONotify(),
					8,
					ska::ZSka
				);

				if (r < SR_Success)
					return r;
			}
			else {
#endif
			String path(CStr("Cooked/"));
			path += CStr(asset->path);
			path += ".0.bin";

			int media = file::AllMedia;
			int r = engine.sys->files->LoadFile(
				path.c_str,
				media,
				m_buf[0],
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

		if (m_buf[0]->result == SR_Pending)
		{
			if (time.infinite)
				m_buf[0]->WaitForCompletion();
			else
				return SR_Pending;
		}

		if (m_buf[0]->result < SR_Success)
			return m_buf[0]->result;

		m_state = S_Load1;
	}

	if (m_state == S_Load1)
	{
		if (!m_buf[1])
		{
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked)
		{
			String path(CStr(asset->path));
			path += ".1.bin";

			int media = file::AllMedia;
			int r = m_cooker->LoadFile( // load cooked data.
				path.c_str,
				0,
				media,
				m_buf[1],
				file::HIONotify(),
				SIMDDriver::Alignment,
				r::ZSkm
			);

			m_cooker.reset();

			if (r < SR_Success)
				return r;
		}
		else {
#endif
			String path(CStr("Cooked/"));
			path += asset->path.get();
			path += ".1.bin";

			int media = file::AllMedia;
			int r = engine.sys->files->LoadFile(
				path.c_str,
				media,
				m_buf[1],
				file::HIONotify(),
				SIMDDriver::Alignment,
				r::ZSkm
			);

			if (r < SR_Success)
				return r;
#if defined(RAD_OPT_TOOLS)
			}
#endif
		}

		if (m_buf[1]->result == SR_Pending)
		{
			if (time.infinite)
				m_buf[1]->WaitForCompletion();
			else
				return SR_Pending;
		}

		if (m_buf[1]->result < SR_Success)
			return m_buf[1]->result;

		const void *data[2];
		AddrSize size[2];

		data[0] = m_buf[0]->data->ptr;
		data[1] = m_buf[1]->data->ptr;
		size[0] = m_buf[0]->data->size;
		size[1] = m_buf[1]->data->size;

		int r = m_dskm.Parse(data, size, ska::SkinCpu);
		if (r < SR_Success)
			return r;

		m_state = S_Done;
	}

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

int SkModelParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	const String *s = asset->entry->KeyValue<String>("AnimSet.Source", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	m_skaRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
	if (!m_skaRef)
		return SR_MissingFile;

	int r = m_skaRef->Process(
		xtime::TimeSlice::Infinite,
		flags
	);

	if (r != SR_Success)
		return r;

	m_ska = SkAnimSetParser::Cast(m_skaRef);
	if (!m_ska || !m_ska->valid)
		return SR_ParseError;

	s = asset->entry->KeyValue<String>("AnimStates.Source", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	m_statesRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
	if (!m_statesRef)
		return SR_MissingFile;

	r = m_statesRef->Process(
		xtime::TimeSlice::Infinite,
		flags
	);

	if (r != SR_Success)
		return r;

	m_states = SkAnimStatesParser::Cast(m_statesRef);
	if (!m_states || !m_states->valid)
		return SR_ParseError;

	s = asset->entry->KeyValue<String>("Mesh.Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	int media = file::AllMedia;
	file::HStreamInputBuffer ib;

	r = engine.sys->files->OpenFileStream(
		s->c_str,
		media,
		ib,
		file::HIONotify()
	);

	if (r < file::Success)
		return r;

	stream::InputStream is(ib->buffer);

	tools::Map map;
	if (!tools::LoadMaxScene(is, map, false))
		return SR_ParseError;

	ib.Close();

	m_skmd = tools::CompileSkmData(
		asset->name,
		map,
		0,
		ska::SkinCpu,
		*m_ska->dska.get()
	);

	return m_skmd ? SR_Success : SR_ParseError;
}

bool SkModelParser::RAD_IMPLEMENT_GET(valid) 
{
	if (m_skmd)
		return m_ska && m_ska->valid && m_states && m_states->valid; 
	return m_buf[1] && m_ska && m_ska->valid && m_states && m_states->valid;
}

#endif

void SkModelParser::Register(Engine &engine)
{
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<SkModelParser>();
}

} // asset
