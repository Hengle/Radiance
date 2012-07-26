// MusicParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/File.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MusicParser : public pkg::Sink<MusicParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Music
	};

	typedef boost::shared_ptr<MusicParser> Ref;

	MusicParser();
	virtual ~MusicParser();

	RAD_DECLARE_READONLY_PROPERTY(MusicParser, file, const file::MMFile::Ref&);

protected:

	RAD_DECLARE_GET(file, const file::MMFile::Ref&) { 
		return m_file; 
	}
	
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

	file::MMFile::Ref m_file;
};

} // asset

#include <Runtime/PopPack.h>
