/*! \file FontParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/


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
