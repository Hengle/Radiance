/*! \file MeshParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "MeshParser.h"
#include "../Engine.h"

using namespace pkg;

namespace asset {

MeshParser::MeshParser() : m_valid(false) {
}

MeshParser::~MeshParser() {
}

int MeshParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_valid && (flags&(P_Load|P_Parse|P_Info)))
		return SR_Success;

	if (flags&(P_Unload|P_Trim)) {
#if defined(RAD_OPT_TOOLS)
		m_bundleData.reset();
#endif
		m_mm.reset();
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
) {
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

			m_mm = cooker->MapFile(path.c_str, ZMeshes);
			if (!m_mm)
				return SR_FileNotFound;
		}
		else {
#endif
		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path += ".bin";

		m_mm = engine.sys->files->MapFile(path.c_str, ZMeshes);
		if (!m_mm)
			return SR_FileNotFound;
#if defined(RAD_OPT_TOOLS)
		}
#endif
	}

	int r = m_bundle.Parse(m_mm->data, m_mm->size);
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
) {
	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(s->c_str, ZTools);
	if (!ib)
		return SR_FileNotFound;
	
	stream::InputStream is(*ib);

	tools::SceneFileRef map(new (ZTools) tools::SceneFile());
	tools::SceneFileVec vec;

	if (!tools::LoadSceneFile(is, *map, true))
		return SR_ParseError;

	ib.reset();

	vec.push_back(map);
	m_bundleData = tools::CompileMeshBundle(asset->path, vec);
	if (m_bundleData)
		m_valid = true;
	return m_valid ? SR_Success : SR_CompilerError;
}

#endif
	
void MeshParser::Register(Engine &engine) {
	static pkg::Binding::Ref ref = engine.sys->packages->Bind<MeshParser>();
}

} // asset
