// MaterialParser.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "AssetTypes.h"
#include "../Packages/Packages.h"
#include "../Renderer/Material.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS MaterialParser : public pkg::Sink<MaterialParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_Material
	};

	typedef boost::shared_ptr<MaterialParser> Ref;

	MaterialParser();
	virtual ~MaterialParser();

	RAD_DECLARE_READONLY_PROPERTY(MaterialParser, material, r::Material*);
	RAD_DECLARE_READONLY_PROPERTY(MaterialParser, procedural, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialParser, valid, bool);

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

	RAD_DECLARE_GET(material, r::Material*) { return &const_cast<MaterialParser*>(this)->m_m; }
	RAD_DECLARE_GET(procedural, bool) { return m_procedural; }
	RAD_DECLARE_GET(valid, bool) { return m_loaded; }

	r::Material m_m;
	bool m_procedural;
	bool m_loaded;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS MaterialLoader : public pkg::Sink<MaterialLoader> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Load,
		AssetType = AT_Material
	};


	typedef boost::shared_ptr<MaterialLoader> Ref;

	MaterialLoader();
	virtual ~MaterialLoader();

	pkg::Asset::Ref Texture(r::MTSource source, int index);

	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, loaded, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, parsed, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, info, bool);

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

private:

	void Unload();
	void Cancel();

	enum {
		Unloaded = -1,
		Info = -2,
		Parsed = -3,
		Done = -4
	};

	RAD_DECLARE_GET(loaded, bool) { return m_current == Done; }
	RAD_DECLARE_GET(parsed, bool) { return m_current == Done || m_current == Parsed; }
	RAD_DECLARE_GET(info, bool) { return m_current == Done || m_current == Parsed || m_current == Info; }

	int m_current;
	int m_index;
	pkg::Asset::Ref m_textures[r::MTS_Max][r::MTS_MaxIndices];
};

} // asset

#include <Runtime/PopPack.h>
