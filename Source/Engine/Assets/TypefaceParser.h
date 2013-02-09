// TypefaceParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../Renderer/Material.h"
#include <Runtime/Font/FontDef.h>
#include <Runtime/File.h>
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS TypefaceParser : public pkg::Sink<TypefaceParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Typeface
	};

	TypefaceParser();
	virtual ~TypefaceParser();

	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, font, font::Font*);
	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, fontAsset, const pkg::AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, material, r::Material*);
	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, materialAsset, const pkg::AssetRef&);
	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, width, int);
	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, height, int);
	RAD_DECLARE_READONLY_PROPERTY(TypefaceParser, valid, bool);

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

	RAD_DECLARE_GET(font, font::Font*) { 
		return m_font; 
	}

	RAD_DECLARE_GET(fontAsset, const pkg::AssetRef&) { 
		return m_fontRef; 
	}

	RAD_DECLARE_GET(material, r::Material*) { 
		return m_mat; 
	}

	RAD_DECLARE_GET(materialAsset, const pkg::AssetRef&) { 
		return m_matRef; 
	}

	RAD_DECLARE_GET(width, int) { 
		return m_width; 
	}

	RAD_DECLARE_GET(height, int) { 
		return m_height; 
	}

	RAD_DECLARE_GET(valid, bool) { 
		return m_font&&m_mat; 
	}

	pkg::Asset::Ref m_fontRef;
	pkg::Asset::Ref m_matRef;
	font::Font *m_font;
	r::Material *m_mat;
	int m_width;
	int m_height;
};

} // asset

#include <Runtime/PopPack.h>
