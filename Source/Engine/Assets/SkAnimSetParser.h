// SkAnimSetParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../FileSystem/FileSystem.h"
#include "../SkAnim/SkAnim.h"

#if defined(RAD_OPT_TOOLS)
#include "../SkAnim/SkBuilder.h"
#endif

#include <Runtime/PushPack.h>

namespace asset  {

class SkAnimSetCooker;

class RADENG_CLASS SkAnimSetParser : public pkg::Sink<SkAnimSetParser>
{
public:

	static void Register(Engine &engine);

	enum
	{
		SinkStage = pkg::SS_Parser,
		AssetType = AT_SkAnimSet
	};

	typedef boost::shared_ptr<SkAnimSetParser> Ref;

	SkAnimSetParser();
	virtual ~SkAnimSetParser();

	RAD_DECLARE_READONLY_PROPERTY(SkAnimSetParser, dska, const ska::DSka*);
	RAD_DECLARE_READONLY_PROPERTY(SkAnimSetParser, valid, bool);

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

	friend class SkAnimSetCooker;

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(valid, bool) { return m_skad||m_load; }
	RAD_DECLARE_GET(dska, const ska::DSka*) { return m_skad ? &m_skad->dska : &m_ska; }
	tools::SkaData::Ref m_skad;
#else
	RAD_DECLARE_GET(valid, bool) { return m_load; }
	RAD_DECLARE_GET(dska, const ska::DSka*) { return &m_ska; }
#endif

	bool m_load;
	ska::DSka m_ska;
	file::HBufferedAsyncIO m_buf;
};

} // asset

#include <Runtime/PopPack.h>

