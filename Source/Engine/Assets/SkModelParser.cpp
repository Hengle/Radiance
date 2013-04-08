/*! \file SkModelParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "SkModelParser.h"
#include "../SkAnim/SkAnim.h"
#include "../Engine.h"
#include <Runtime/Base/SIMD.h>

using namespace pkg;

namespace asset {

SkModelParser::SkModelParser() : m_state(S_None), m_ska(0), m_states(0) {
}

SkModelParser::~SkModelParser() {
}

int SkModelParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
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

	if (flags&P_Unload) {
		m_state = S_None;
		m_ska = 0;
		m_skaRef.reset();
		m_states = 0;
		m_statesRef.reset();
		m_mm[0].reset();
		m_mm[1].reset();
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
) {
	if (m_state == S_None) {
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {
			const String *s = asset->entry->KeyValue<String>("AnimSet.Source", P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			m_skaRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
			if (!m_skaRef)
				return SR_FileNotFound;

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
				return SR_FileNotFound;

			r = m_statesRef->Process(
				xtime::TimeSlice::Infinite,
				flags
			);

			if (r != SR_Success)
				return r;

			m_states = SkAnimStatesParser::Cast(m_statesRef);
			if (!m_states || !m_states->valid)
				return SR_ParseError;
		} else {
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
			return SR_FileNotFound;

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

	if (m_state == S_Load0) {
		if (!m_mm[0]) {
#if defined(RAD_OPT_TOOLS)
			if (!asset->cooked) {
				m_cooker = asset->AllocateIntermediateCooker();
				CookStatus status = m_cooker->Status(P_TARGET_FLAGS(flags));

				if (status == CS_Ignore)
					return SR_CompilerError;

				if (status == CS_NeedRebuild) {
					COut(C_Info) << asset->path.get() << " is out of date, rebuilding..." << std::endl;
					int r = m_cooker->Cook(P_TARGET_FLAGS(flags));
					if (r != SR_Success)
						return r;
				} else {
					COut(C_Info) << asset->path.get() << " is up to date, using cache." << std::endl;
				}

				String path(CStr(asset->path));
				path += ".0.bin";

				m_mm[0] = m_cooker->MapFile(path.c_str, r::ZSkm);
				if (!m_mm[0])
					return SR_FileNotFound;
			}
			else {
#endif
			String path(CStr("Cooked/"));
			path += CStr(asset->path);
			path += ".0.bin";

			m_mm[0] = engine.sys->files->MapFile(path.c_str, r::ZSkm);
			if (!m_mm[0])
				return SR_FileNotFound;
#if defined(RAD_OPT_TOOLS)
			}
#endif
		}

		m_state = S_Load1;
	}

	if (m_state == S_Load1) {
		if (!m_mm[1]) {
#if defined(RAD_OPT_TOOLS)
			if (!asset->cooked)
			{
				String path(CStr(asset->path));
				path += ".1.bin";

				m_mm[1] = m_cooker->MapFile(path.c_str, r::ZSkm);
				m_cooker.reset();

				if (!m_mm[1])
					return SR_FileNotFound;
			}
			else {
#endif
			String path(CStr("Cooked/"));
			path += asset->path.get();
			path += ".1.bin";

			m_mm[1] = engine.sys->files->MapFile(path.c_str, r::ZSkm);
			if (!m_mm[1])
				return SR_FileNotFound;
#if defined(RAD_OPT_TOOLS)
			}
#endif
		}

		const void *data[2];
		AddrSize size[2];

		data[0] = m_mm[0]->data;
		data[1] = m_mm[1]->data;
		size[0] = m_mm[0]->size;
		size[1] = m_mm[1]->size;

		int r = m_dskm.Parse(data, size, ska::kSkinType_CPU);
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
) {
	const String *s = asset->entry->KeyValue<String>("AnimSet.Source", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	m_skaRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
	if (!m_skaRef)
		return SR_FileNotFound;

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
		return SR_FileNotFound;

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

	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(s->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;

	stream::InputStream is(*ib);

	tools::SceneFile map;
	if (!tools::LoadSceneFile(is, map, true))
		return SR_ParseError;

	ib.reset();

	if (map.worldspawn->models.size() != 1) {
		COut(C_Error) << "ERROR: 3DX file should only contain 1 model, it contains " << map.worldspawn->models.size() << ". File: '" << *s << "'" << std::endl;
		return SR_ParseError;
	}

	m_skmd = tools::CompileSkmData(
		asset->name,
		map,
		0,
		ska::kSkinType_CPU,
		*m_ska->dska.get()
	);

	return m_skmd ? SR_Success : SR_ParseError;
}

bool SkModelParser::RAD_IMPLEMENT_GET(valid)  {
	if (m_skmd)
		return m_ska && m_ska->valid && m_states && m_states->valid; 
	return m_mm[1] && m_ska && m_ska->valid && m_states && m_states->valid;
}

#endif

void SkModelParser::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<SkModelParser>();
}

} // asset
