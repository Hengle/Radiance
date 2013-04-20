/*! \file VtModelParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "VtModelParser.h"
#include "SkAnimSetParser.h"
#include "../SkAnim/SkAnim.h"
#include "../Engine.h"
#include <Runtime/Base/SIMD.h>

using namespace pkg;

namespace asset {

VtModelParser::VtModelParser() : m_state(kS_None), m_states(0) {
}

VtModelParser::~VtModelParser() {
}

int VtModelParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

#pragma message("TODO: Implement P_Trim")
	// This is unfinished. VtMesh has 2 files, one is persistent, mesh uv's, indices etc.
	// The other is meant to be free'd after the model is created. BUT we can't free the transient data
	// (texture coords, indices, etc) because they are references repeatedly by new instances of SkMesh. What I should
	// do is have VtModelParser move its transient data into those vertex buffers and expose them as part of its interface.
	// Work todo in the future, right now it's not going to save enough memory to care too much.
	//
	// Technically this is broken because the entire concept of this asset sink parser mechanism is to expose the "static" side
	// of the data in whatever form it should take place. TextureParser does this correctly, it moves the data into a GLTexture
	// as a later sink stage.
	//
	// So we need to make a GLVtModelParser that moves the VtModelParser transient data into some VB's and then when we get a P_Trim
	// we can properly release this data.

#if defined(RAD_OPT_TOOLS)
	if ((m_vtmd||(m_state==kS_Done)) && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#else
	if ((m_state==kS_Done) && (flags&(P_Load|P_Parse|P_Info|P_Trim)))
		return SR_Success;
#endif

	if (flags&P_Unload) {
		m_state = kS_None;
		m_states = 0;
		m_statesRef.reset();
		m_mm[0].reset();
		m_mm[1].reset();
#if defined(RAD_OPT_TOOLS)
		m_vtmd.reset();
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

int VtModelParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (m_state == kS_None) {
#if defined(RAD_OPT_TOOLS)
		if (!asset->cooked) {
			const String *s = asset->entry->KeyValue<String>("AnimStates.Source", P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			m_statesRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
			if (!m_statesRef)
				return SR_FileNotFound;

			int r = m_statesRef->Process(
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

		m_statesRef = asset->entry->Resolve(*import, asset->zone);
		if (!m_statesRef)
			return SR_FileNotFound;

		int r = m_statesRef->Process(
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

		m_state = kS_Load0;
	}

	if (m_state == kS_Load0) {
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

		m_state = kS_Load1;
	}

	if (m_state == kS_Load1) {
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

		int r = m_dvtm.Parse(data, size);
		if (r < SR_Success)
			return r;

		m_state = kS_Done;
	}

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

int VtModelParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	const String *s = asset->entry->KeyValue<String>("AnimStates.Source", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	m_statesRef = engine.sys->packages->Resolve(s->c_str, asset->zone);
	if (!m_statesRef)
		return SR_FileNotFound;

	int r = m_statesRef->Process(
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

	tools::SceneFile mesh;
	if (!tools::LoadSceneFile(is, mesh, true))
		return SR_ParseError;

	ib.reset();

	if (mesh.worldspawn->models.size() != 1) {
		COut(C_Error) << "ERROR: 3DX file should only contain 1 model, it contains " << mesh.worldspawn->models.size() << ". File: '" << *s << "'" << std::endl;
		return SR_ParseError;
	}

	s = asset->entry->KeyValue<String>("Anims.Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	StringVec animSources;
	SkAnimSetParser::LoadToolsFile(s->c_str, engine, &animSources, 0);

	if (animSources.empty())
		return SR_ParseError;

	tools::SceneFileVec anims;
	for (StringVec::const_iterator it = animSources.begin(); it != animSources.end(); ++it) {
		ib = engine.sys->files->OpenInputBuffer((*it).c_str, ZTools);
		if (!ib)
			return SR_FileNotFound;
		stream::InputStream is(*ib);
		tools::SceneFileRef x(new (tools::Z3DX) tools::SceneFile());
		if (!tools::LoadSceneFile(is, *x, true))
			return SR_ParseError;
		anims.push_back(x);
	}

	m_vtmd = tools::CompileVtmData(
		asset->path,
		mesh,
		anims,
		0
	);

	return m_vtmd ? SR_Success : SR_ParseError;
}

bool VtModelParser::RAD_IMPLEMENT_GET(valid)  {
	if (m_vtmd)
		return m_states && m_states->valid; 
	return m_state == kS_Done;
}

#endif

void VtModelParser::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<VtModelParser>();
}

} // asset
