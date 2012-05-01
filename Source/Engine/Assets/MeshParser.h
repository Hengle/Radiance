// MeshParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../FileSystem/FileSystem.h"
#include "MeshBundle.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MeshParser : public pkg::Sink<MeshParser>
{
public:

	static void Register(Engine &engine);

	enum
	{
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Mesh
	};

	typedef boost::shared_ptr<MeshParser> Ref;

	MeshParser();
	virtual ~MeshParser();

	RAD_DECLARE_READONLY_PROPERTY(MeshParser, bundle, const DMeshBundle*);
	RAD_DECLARE_READONLY_PROPERTY(MeshParser, valid, bool);

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

#if defined(RAD_OPT_TOOLS)
	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);
#endif

	int LoadCooked(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	RAD_DECLARE_GET(valid, bool) { return m_valid; }

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(bundle, const DMeshBundle*) { return m_bundleData ? &m_bundleData->bundle : &m_bundle; }
	tools::DMeshBundleData::Ref m_bundleData;
#else
	RAD_DECLARE_GET(bundle, const DMeshBundle*) { return &m_bundle; }
#endif

	bool m_valid;
	DMeshBundle m_bundle;
	file::HBufferedAsyncIO m_buf;
};

} // asset

#include <Runtime/PopPack.h>
