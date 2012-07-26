// FontParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include <Runtime/File.h>
#include <Runtime/Font/Font.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS FontParser : public pkg::Sink<FontParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Font
	};

	typedef boost::shared_ptr<FontParser> Ref;

	FontParser();
	virtual ~FontParser();

	RAD_DECLARE_READONLY_PROPERTY(FontParser, font, font::Font*);

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

	RAD_DECLARE_GET(font, font::Font*) { return m_loaded ? &const_cast<FontParser*>(this)->m_font : 0; }

	font::Font m_font;
	file::MMapping::Ref m_mm;
	bool m_loaded;
};

} // asset

#include <Runtime/PopPack.h>
