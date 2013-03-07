/*! \file MaterialParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "MaterialParser.h"
#include "TextureParser.h"
#include "../Engine.h"
#include <Runtime/Stream.h>
#include <string.h>
#include <algorithm>
#undef min
#undef max

using namespace pkg;

namespace asset {

MaterialParser::MaterialParser() : 
m_loaded(false),
m_procedural(false)
{
}

MaterialParser::~MaterialParser()
{
}

int MaterialParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Trim)))
		return SR_Success;

	if (m_loaded && (flags&(P_Load|P_Parse|P_Info)))
		return SR_Success;

	if (flags&(P_Unload|P_Trim)) // nothing to do
		return SR_Success;


#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked)
		return Load(time, engine, asset, flags);
#endif
	
	return LoadCooked(time, engine, asset, flags);
}


int MaterialParser::LoadCooked(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	String path(CStr("Cooked/"));
	path += CStr(asset->path);
	path += ".bin";

	file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(path.c_str, r::ZRender);
	if (!ib)
		return SR_FileNotFound;

	stream::InputStream is(*ib);

	try {
		U16 shaderId;
		U8  temp;
		float f;

		is >> shaderId;
		m_m.shaderId = shaderId;
		is >> temp; m_procedural = temp ? true : false;
		is >> temp; m_m.sort = (r::Material::Sort)temp;
		is >> temp; m_m.blendMode = (r::Material::BlendMode)temp;
		is >> temp; m_m.depthFunc = (r::Material::DepthFunc)temp;
		is >> temp; m_m.doubleSided.set(temp?true:false);
		is >> temp; m_m.depthWrite.set(temp?true:false);

		m_m.animated = false;

		for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
			is >> temp;
			if (temp == 255) {
				m_m.SetTextureId(i, -1);
			} else {
				const Package::Entry::Import *imp = asset->entry->Resolve(temp);
				if (!imp)
					return SR_ParseError;
				int id = asset->entry->ResolveId(*imp);
				if (id < 0)
					return SR_FileNotFound;
				m_m.SetTextureId(i, id);
			}

			is >> temp;
			m_m.SetTCUVIndex(i, (int)temp);

			is >> f;
			m_m.SetTextureFPS(i, f);
			
			is >> temp;
			m_m.SetClampTextureFrames(i, temp ? true : false);

			is >> temp;
			m_m.SetTCGen(i, temp);

			for (int k = 0; k < r::Material::kNumTCMods; ++k) {
				WaveAnim &S = m_m.Wave(i, k, r::Material::kTexCoord_S);
				WaveAnim &T = m_m.Wave(i, k, r::Material::kTexCoord_T);

				is >> temp; S.type = (WaveAnim::Type)temp;
				is >> temp; T.type = (WaveAnim::Type)temp;
				
				is >> f; S.amplitude.set(f);
				is >> f; T.amplitude.set(f);
				is >> f; S.freq.set(f);
				is >> f; T.freq.set(f);
				is >> f; S.phase.set(f);
				is >> f; T.phase.set(f);
				is >> f; S.base.set(f);
				is >> f; T.base.set(f);

				m_m.animated = m_m.animated || S.type != WaveAnim::T_Identity || T.type != WaveAnim::T_Identity;
			}
		}

		for (int i = r::Material::kColor0; i < r::Material::kNumColors; ++i) {
			for (int k = r::Material::kColorA; k < r::Material::kNumColorIndices; ++k) {
				U8 rgba[4];
				is >> rgba[0];
				is >> rgba[1];
				is >> rgba[2];
				is >> rgba[3];

				float c[4];
				c[0] = rgba[0] / 255.f;
				c[1] = rgba[1] / 255.f;
				c[2] = rgba[2] / 255.f;
				c[3] = rgba[3] / 255.f;

				m_m.SetColor(i, k, c);
			}

			WaveAnim &C = m_m.ColorWave(i);
			is >> temp; C.type = (WaveAnim::Type)temp;
			
			is >> f; C.amplitude.set(f);
			is >> f; C.freq.set(f);
			is >> f; C.phase.set(f);
			is >> f; C.base.set(f);
			
			m_m.animated = m_m.animated || C.type != WaveAnim::T_Identity;
		}

	} catch (exception&) {
		return SR_IOError;
	}

	m_loaded = true;

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

const char *s_tcModNames[r::Material::kNumTCMods] = {
	"Rotate",
	"Turb",
	"Scale",
	"Shift",
	"Scroll"
};

int MaterialParser::SourceModifiedTime(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags,
	xtime::TimeDate &td
) {
	td = xtime::TimeDate::Zero();

	const bool *b = asset->entry->KeyValue<bool>("ProceduralTextures", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	bool procedural = *b;

	String path;
	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		path.Printf("Texture%d.Source.Texture", i+1);
		const String *s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		pkg::Asset::Ref texture;

		if (s->empty) {
			if (procedural && asset->zone != Z_Engine) {
				int id = engine.sys->packages->ResolveId("Sys/T_Procedural");
				if (id == -1)
					return SR_FileNotFound;
				texture = engine.sys->packages->Asset(id, asset->zone);
				RAD_ASSERT(texture);
				if (!texture)
					return SR_FileNotFound;
			} else {
				continue;
			}
		} else {
			pkg::Package::Entry::Ref entry = engine.sys->packages->Resolve(s->c_str);
			if (!entry && !(flags&P_NoDefaultMedia))
				entry = engine.sys->packages->Resolve("Sys/T_Missing");
			if (!entry)
				return SR_FileNotFound;
			if (entry->type != asset::AT_Texture)
				return SR_MetaError; // this must be a texture.
			texture = entry->Asset(asset->zone);
		}

		TextureParser *parser = TextureParser::Cast(texture);
		RAD_ASSERT(parser);

		xtime::TimeDate textureTd;

		int r = parser->SourceModifiedTime(engine, texture, flags, textureTd);
		if (r != SR_Success)
			return r;

		if (textureTd > td)
			td = textureTd; // choose newer.
	}

	return SR_Success;
}

int MaterialParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i)
		m_m.SetTextureId(i, -1);

	const String *s = asset->entry->KeyValue<String>("Source.Shader", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	m_m.shaderName = s->c_str;

	s = asset->entry->KeyValue<String>("Sort", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (*s == "Solid")
		m_m.sort = r::Material::kSort_Solid;
	else if (*s == "Translucent")
		m_m.sort = r::Material::kSort_Translucent;
	else if (*s == "Translucent2")
		m_m.sort = r::Material::kSort_Translucent2;
	else if (*s == "Translucent3")
		m_m.sort = r::Material::kSort_Translucent3;
	else if (*s == "Translucent4")
		m_m.sort = r::Material::kSort_Translucent4;
	else if (*s == "Translucent5")
		m_m.sort = r::Material::kSort_Translucent5;
	else
		return SR_MetaError;

	s = asset->entry->KeyValue<String>("BlendMode", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (*s == "None")
		m_m.blendMode = r::Material::kBlendMode_None;
	else if (*s == "Alpha")
		m_m.blendMode = r::Material::kBlendMode_Alpha;
	else if (*s == "InvAlpha")
		m_m.blendMode = r::Material::kBlendMode_InvAlpha;
	else if (*s == "Additive")
		m_m.blendMode = r::Material::kBlendMode_Additive;
	else if (*s == "AddBlend")
		m_m.blendMode = r::Material::kBlendMode_AddBlend;
	else if (*s == "Colorize")
		m_m.blendMode = r::Material::kBlendMode_Colorize;
	else if (*s == "InvColorizeD")
		m_m.blendMode = r::Material::kBlendMode_InvColorizeD;
	else if (*s == "InvColorizeS")
		m_m.blendMode = r::Material::kBlendMode_InvColorizeS;
	else
		return SR_MetaError;

	const bool *b = asset->entry->KeyValue<bool>("DoubleSided", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	m_m.doubleSided = *b;

	b = asset->entry->KeyValue<bool>("DepthWrite", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	m_m.depthWrite = *b;

	s = asset->entry->KeyValue<String>("DepthFunc", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (*s == "None")
		m_m.depthFunc = r::Material::kDepthFunc_None;
	else if (*s == "Less")
		m_m.depthFunc = r::Material::kDepthFunc_Less;
	else if (*s == "LEqual")
		m_m.depthFunc = r::Material::kDepthFunc_LEqual;
	else if (*s == "Greater")
		m_m.depthFunc = r::Material::kDepthFunc_Greater;
	else if (*s == "GEqual")
		m_m.depthFunc = r::Material::kDepthFunc_GEqual;
	else if (*s == "Equal")
		m_m.depthFunc = r::Material::kDepthFunc_Equal;
	else
		return SR_MetaError;

	m_m.animated = false;

	b = asset->entry->KeyValue<bool>("ProceduralTextures", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	m_procedural = *b;

	String path;
	String z;

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		path.Printf("Texture%d.Source.Texture", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		if (s->empty) {
			if (m_procedural && asset->zone != Z_Engine) {
				int id = engine.sys->packages->ResolveId("Sys/T_Procedural");
				if (id == -1)
					return SR_FileNotFound;
				m_m.SetTextureId(i, id);
			} else {
				m_m.SetTextureId(i, -1);
			}
		} else {
			pkg::Package::Entry::Ref entry = engine.sys->packages->Resolve(s->c_str);
			if (!entry && !(flags&P_NoDefaultMedia))
				entry = engine.sys->packages->Resolve("Sys/T_Missing");
			if (!entry)
				return SR_FileNotFound;
			if (entry->type != asset::AT_Texture)
				return SR_MetaError; // this must be a texture.
			m_m.SetTextureId(i, entry->id);
		}

		path.Printf("Texture%d.Source.FramesPerSecond", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;
		float fps;
		sscanf(s->c_str, "%f", &fps);
		m_m.SetTextureFPS(i, fps);

		path.Printf("Texture%d.Source.ClampTextureFrames", i+1);
		b = asset->entry->KeyValue<bool>(path.c_str, P_TARGET_FLAGS(flags));
		if (!b)
			return SR_MetaError;
		m_m.SetClampTextureFrames(i, *b);

		path.Printf("Texture%d.tcGen", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		if (*s == "Vertex") {
			m_m.SetTCGen(i, r::Material::kTCGen_Vertex);
		} else if (*s == "EnvMap") {
			m_m.SetTCGen(i, r::Material::kTCGen_EnvMap);
		} else {
			return SR_MetaError;
		}

		path.Printf("Texture%d.Source.UVChannel", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		int uvChannel = 0;
		sscanf(s->c_str, "%d", &uvChannel);
		--uvChannel; // was one based.
		if (uvChannel < 0 || uvChannel >= r::kMaterialTextureSource_MaxIndices)
			return SR_MetaError;

		m_m.SetTCUVIndex(i, uvChannel);

		for (int k = 0; k < r::Material::kNumTCMods; ++k) {
			path.Printf("Texture%d.tcMod.%s", i+1, s_tcModNames[k]);
			z = path + ".Type";

			WaveAnim &S = m_m.Wave(
				i,
				k,
				r::Material::kTexCoord_S
			);

			WaveAnim &T = m_m.Wave(
				i,
				k,
				r::Material::kTexCoord_T
			);

			s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			if (*s == "Identity") {
				S.type = T.type = WaveAnim::T_Identity;
			} else if (*s == "Constant") {
				S.type = T.type = WaveAnim::T_Constant;
			} else if (*s == "Square") {
				S.type = T.type = WaveAnim::T_Square;
			} else if (*s == "Sawtooth") {
				S.type = T.type = WaveAnim::T_Sawtooth;
			} else if (*s == "Triangle") {
				S.type = T.type = WaveAnim::T_Triangle;
			} else if (*s == "Noise") {
				S.type = T.type = WaveAnim::T_Noise;
			} else {
				return SR_MetaError;
			}

			m_m.animated = m_m.animated || S.type != WaveAnim::T_Identity;

			z = path + ".Amplitude";
			s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			float a, b;

			sscanf(s->c_str, "%f %f", &a, &b);
			S.amplitude = a;
			T.amplitude = b;

			z = path + ".Frequency";
			s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			sscanf(s->c_str, "%f %f", &a, &b);
			S.freq = a;
			T.freq = b;

			z = path + ".Phase";
			s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			sscanf(s->c_str, "%f %f", &a, &b);
			S.phase = a;
			T.phase = b;

			z = path + ".Base";
			s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			sscanf(s->c_str, "%f %f", &a, &b);
			S.base = a;
			T.base = b;
		}
	}
	// color
	for (int i = r::Material::kColor0; i < r::Material::kNumColors; ++i) {
		for (int k = r::Material::kColorA; k < r::Material::kNumColorIndices; ++k) {
			path.Printf("Color%d.%c", i, 'A'+k);
			s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			int r, g, b, a;
			float c[4];

			sscanf(s->c_str, "%d %d %d %d", &r, &g, &b, &a);
			c[0] = r/255.f;
			c[1] = g/255.f;
			c[2] = b/255.f;
			c[3] = a/255.f;

			for (int j = 0; j < 4; ++j)
				c[j] = std::max(std::min(c[j], 255.f), 0.f);

			m_m.SetColor(i, k, c);
		}

		path.Printf("Color%d.Gen", i);
		z = path + ".Type";

		s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		WaveAnim &C = m_m.ColorWave(i);

		if (*s == "Identity") {
			C.type = WaveAnim::T_Identity;
		} else if (*s == "Constant") {
			C.type =  WaveAnim::T_Constant;
		} else if (*s == "Square") {
			C.type = WaveAnim::T_Square;
		} else if (*s == "Sawtooth") {
			C.type = WaveAnim::T_Sawtooth;
		} else if (*s == "Triangle") {
			C.type = WaveAnim::T_Triangle;
		} else if (*s == "Noise") {
			C.type = WaveAnim::T_Noise;
		} else {
			return SR_MetaError;
		}

		m_m.animated = m_m.animated || C.type != WaveAnim::T_Identity;

		z = path + ".Amplitude";
		s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		float a;

		sscanf(s->c_str, "%f", &a);
		C.amplitude = a;
		
		z = path + ".Frequency";
		s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		sscanf(s->c_str, "%f", &a);
		C.freq = a;

		z = path + ".Phase";
		s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		sscanf(s->c_str, "%f", &a);
		C.phase = a;

		z = path + ".Base";
		s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		sscanf(s->c_str, "%f", &a);
		C.base = a;
	}

	m_loaded = true;

	return SR_Success;
}
#endif

void MaterialParser::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<MaterialParser>();
}

///////////////////////////////////////////////////////////////////////////////

MaterialLoader::MaterialLoader() : m_index(Unloaded) {
#if defined(RAD_OPT_TOOLS)
	m_shaderOnly = false;
#endif
}

MaterialLoader::~MaterialLoader() {
}

int MaterialLoader::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Cancel|P_Trim)))
		return SR_Success;

	if (flags&P_Cancel) {
		Cancel();
		return SR_Success;
	}

	if (flags&P_Load)
		flags &= ~P_Parse;
	if (flags&P_Parse)
		flags &= ~P_Info;
	
	if (m_index == Done && (flags&P_Load))
		return SR_Success;
	if ((m_index == Done || m_index == Parsed) && (flags&P_Parse))
		return SR_Success;
	if ((m_index == Done || m_index == Parsed || m_index == Info) && (flags&P_Info))
		return SR_Success;
	if ((m_index == Parsed || m_index == Info) && (flags&P_Load))
		m_index = Unloaded; // fully load.
	if (m_index == Info && (flags&P_Parse))
		m_index = Unloaded; // fully load.
	if (m_index == Unloaded && (flags&P_Unload))
		return SR_Success;
	if ((m_index == Unloaded || m_index == Info) && (flags&P_Trim))
		return SR_Success;

	if (flags&P_Unload) {
		Unload();
		return SR_Success;
	}

	MaterialParser *parser = MaterialParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	if (m_index == Unloaded) {
		if (flags&(P_Parse|P_Load)) {
			int r = parser->material->LoadShader(
				time,
				engine,
				flags
			);

			if (r == SR_Pending)
				return r;
			if (r != SR_Success)
				return r;

			m_index = 0;

			if (!time.remaining)
				return SR_Pending;

		} else { 
			// skip shader loading
			RAD_ASSERT(flags&(P_Info|P_Trim));
			m_index = 0;
		}
	} else if (flags&(P_Info|P_Trim)) {
		m_index = 0;
	}

	do {
		bool load = true;
		pkg::Asset::Ref &tex = m_textures[m_index];

		if (!tex) {
			int id = parser->material->TextureId(m_index);
			if (id < 0) {
				load = false;
#if defined(RAD_OPT_TOOLS)
				if (flags&(P_Parse|P_Load)) {
					if (!parser->procedural && parser->material->shader->Requires(r::kMaterialTextureSource_Texture, m_index))
						return SR_FileNotFound;
				}
#endif
			}

			if (load) {
				tex = engine.sys->packages->Asset(id, asset->zone);
				
#if defined(RAD_OPT_TOOLS)
				if (!tex && (flags&(P_Parse|P_Load))) {
					if (parser->material->shader->Requires(r::kMaterialTextureSource_Texture, m_index))
						return SR_FileNotFound;
				}
#endif
			}
		}

#if defined(RAD_OPT_TOOLS)
		if (!m_shaderOnly) {
#endif
		if (tex) {
			int r = tex->Process(
				time,
				flags
			);

			if (r != SR_Success)
				return r;

			// flag animated based on texture bundle?
			TextureParser *texParser = TextureParser::Cast(tex);
			if (!texParser)
				return SR_MetaError;
			if (texParser->numImages > 0 && (parser->material->TextureFPS(m_index)>0.f))
				parser->material->animated = true;
		}

#if defined(RAD_OPT_TOOLS)
		}
#endif

		++m_index;
		if (m_index >= r::kMaterialTextureSource_MaxIndices) {
			m_index = (flags&(P_Load|P_Trim)) ? Done : (flags&P_Parse) ? Parsed : Info;
			return SR_Success; // done loading.
		}

	} while (time.remaining);

	return SR_Pending;
}

void MaterialLoader::Cancel() {
	if (m_index <= Unloaded)
		return;

	m_index = Unloaded;

	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		if (!m_textures[i])
			break;
		m_textures[i]->Process(
			xtime::TimeSlice::Infinite,
			P_Cancel
		);
	}
}

void MaterialLoader::Unload() {
	for (int i = 0; i < r::kMaterialTextureSource_MaxIndices; ++i) {
		m_textures[i].reset();
	}

	m_index = Unloaded;
}

void MaterialLoader::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<MaterialLoader>();
}

} // asset
