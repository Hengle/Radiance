/*! \file MaterialParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

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

	MaterialParser();
	virtual ~MaterialParser();

	RAD_DECLARE_READONLY_PROPERTY(MaterialParser, material, r::Material*);
	RAD_DECLARE_READONLY_PROPERTY(MaterialParser, procedural, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialParser, valid, bool);

#if defined(RAD_OPT_TOOLS)
	int SourceModifiedTime(
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags,
		xtime::TimeDate &td
	);
#endif

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

	MaterialLoader();
	virtual ~MaterialLoader();

	pkg::Asset::Ref Texture(int index) {
		return m_textures[index];
	}

	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, loaded, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, parsed, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, info, bool);
	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, width, int);
	RAD_DECLARE_READONLY_PROPERTY(MaterialLoader, height, int);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_PROPERTY(MaterialLoader, shaderOnly, bool, bool);
#endif

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

	RAD_DECLARE_GET(loaded, bool) { 
		return m_index == Done; 
	}

	RAD_DECLARE_GET(parsed, bool) { 
		return m_index == Done || m_index == Parsed; 
	}

	RAD_DECLARE_GET(info, bool) { 
		return m_index == Done || m_index == Parsed || m_index == Info; 
	}

	RAD_DECLARE_GET(width, int) {
		return m_width;
	}

	RAD_DECLARE_GET(height, int) {
		return m_height;
	}

	
	int m_index;
	int m_width;
	int m_height;
	boost::array<pkg::Asset::Ref, r::kMaterialTextureSource_MaxIndices> m_textures;

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(shaderOnly, bool) {
		return m_shaderOnly;
	}

	RAD_DECLARE_SET(shaderOnly, bool) {
		m_shaderOnly = value;
	}

	bool m_shaderOnly;
#endif
};

///////////////////////////////////////////////////////////////////////////////

class MaterialBundle {
public:
	MaterialBundle() : loader(0), material(0) {
	}

	MaterialBundle(const pkg::Asset::Ref &asset) {
		Bind(asset);
	}

	void Bind(const pkg::Asset::Ref &asset);

	void Reset() {
		asset.reset();
		loader = 0;
		material = 0;
	}

	pkg::Asset::Ref asset;
	MaterialLoader *loader;
	r::Material *material;
};

} // asset

#include <Runtime/PopPack.h>
