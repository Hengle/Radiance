// MaterialParser.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

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

	file::HStreamInputBuffer buf;
	int media = file::AllMedia;
	int r = engine.sys->files->OpenFileStream(
		path.c_str,
		media,
		buf,
		file::HIONotify()
	);

	if (r != SR_Success)
		return r;

	stream::InputStream is(buf->buffer);

	try
	{
		U16 shaderId;
		U8  temp;
		float f;

		is >> shaderId;
		m_m.shaderId = shaderId;
		is >> temp; m_procedural = temp ? true : false;
		is >> temp; m_m.sort = (r::Material::Sort)temp;
		is >> temp; m_m.blendMode = (r::Material::BlendMode)temp;
		is >> temp; m_m.depthFunc = (r::Material::DepthFunc)temp;
		is >> temp; m_m.alphaTest = (r::Material::AlphaTest)temp;
		is >> temp; m_m.alphaVal = temp;
		is >> temp; m_m.doubleSided.set(temp?true:false);
		is >> temp; m_m.depthWrite.set(temp?true:false);

		m_m.animated = false;

		for (int i = 0; i < r::MTS_MaxIndices; ++i)
		{
			is >> temp;
			if (temp == 255)
			{
				m_m.SetTextureId(r::MTS_Texture, i, -1);
			}
			else
			{
				const Package::Entry::Import *imp = asset->entry->Resolve(temp);
				if (!imp)
					return SR_ParseError;
				int id = asset->entry->ResolveId(*imp);
				if (id < 0)
					return SR_MissingFile;
				m_m.SetTextureId(r::MTS_Texture, i, id);
			}

			is >> f;
			m_m.SetTextureFPS(r::MTS_Texture, i, f);
			
			is >> temp;
			m_m.SetClampTextureFrames(r::MTS_Texture, i, temp ? true : false);

			is >> temp;
			m_m.SetTcGen(r::MTS_Texture, i, temp);

			for (int k = 0; k < r::Material::NumTcMods; ++k)
			{
				WaveAnim &S = m_m.Wave(r::MTS_Texture, i, k, r::Material::S);
				WaveAnim &T = m_m.Wave(r::MTS_Texture, i, k, r::Material::T);

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

		for (int i = r::Material::Color0; i < r::Material::NumColors; ++i)
		{
			for (int k = r::Material::ColorA; k < r::Material::NumColorIndices; ++k)
			{
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

	}
	catch (exception&)
	{
		return SR_IOError;
	}

	m_loaded = true;

	return SR_Success;
}

#if defined(RAD_OPT_TOOLS)

const char *s_tcModNames[r::Material::NumTcMods] =
{
	"Rotate",
	"Turb",
	"Scale",
	"Shift",
	"Scroll"
};

int MaterialParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	for (int i = 0; i < r::MTS_Max; ++i)
		for (int k = 0; k < r::MTS_MaxIndices; ++k)
			m_m.SetTextureId((r::MTSource)i, k, -1);

	const String *s = asset->entry->KeyValue<String>("Source.Shader", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	m_m.shaderName = s->c_str;

	s = asset->entry->KeyValue<String>("Sort", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (*s == "Solid")
		m_m.sort = r::Material::S_Solid;
	else if (*s == "Translucent")
		m_m.sort = r::Material::S_Translucent;
	else if (*s == "Translucent2")
		m_m.sort = r::Material::S_Translucent2;
	else if (*s == "Translucent3")
		m_m.sort = r::Material::S_Translucent3;
	else if (*s == "Translucent4")
		m_m.sort = r::Material::S_Translucent4;
	else if (*s == "Translucent5")
		m_m.sort = r::Material::S_Translucent5;
	else
		return SR_MetaError;

	s = asset->entry->KeyValue<String>("AlphaTest", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (*s == "None")
		m_m.alphaTest = r::Material::AT_None;
	else if (*s == "Less")
		m_m.alphaTest = r::Material::AT_Less;
	else if (*s == "LEqual")
		m_m.alphaTest = r::Material::AT_LEqual;
	else if (*s == "Greater")
		m_m.alphaTest = r::Material::AT_Greater;
	else if (*s == "GEqual")
		m_m.alphaTest = r::Material::AT_GEqual;
	else
		return SR_MetaError;

	const int *n = asset->entry->KeyValue<int>("AlphaTestVal", P_TARGET_FLAGS(flags));
	if (!n)
		return SR_MetaError;

	m_m.alphaVal = (U8)std::max(std::min(*n, 255), 0);

	s = asset->entry->KeyValue<String>("BlendMode", P_TARGET_FLAGS(flags));
	if (!s)
		return SR_MetaError;

	if (*s == "None")
		m_m.blendMode = r::Material::BM_None;
	else if (*s == "Alpha")
		m_m.blendMode = r::Material::BM_Alpha;
	else if (*s == "InvAlpha")
		m_m.blendMode = r::Material::BM_InvAlpha;
	else if (*s == "Additive")
		m_m.blendMode = r::Material::BM_Additive;
	else if (*s == "AddBlend")
		m_m.blendMode = r::Material::BM_AddBlend;
	else if (*s == "Colorize")
		m_m.blendMode = r::Material::BM_Colorize;
	else if (*s == "InvColorizeD")
		m_m.blendMode = r::Material::BM_InvColorizeD;
	else if (*s == "InvColorizeS")
		m_m.blendMode = r::Material::BM_InvColorizeS;
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
		m_m.depthFunc = r::Material::DT_None;
	else if (*s == "Less")
		m_m.depthFunc = r::Material::DT_Less;
	else if (*s == "LEqual")
		m_m.depthFunc = r::Material::DT_LEqual;
	else if (*s == "Greater")
		m_m.depthFunc = r::Material::DT_Greater;
	else if (*s == "GEqual")
		m_m.depthFunc = r::Material::DT_GEqual;
	else
		return SR_MetaError;

	m_m.animated = false;

	b = asset->entry->KeyValue<bool>("ProceduralTextures", P_TARGET_FLAGS(flags));
	if (!b)
		return SR_MetaError;

	m_procedural = *b;

	String path;
	String z;

	for (int i = 0; i < r::MTS_MaxIndices; ++i)
	{
		path.Printf("Texture%d.Source.Texture", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		if (s->empty)
		{
			if (m_procedural && asset->zone != Z_Engine)
			{
				int id = engine.sys->packages->ResolveId("Sys/T_Procedural");
				if (id == -1)
					return SR_MissingFile;
				m_m.SetTextureId(r::MTS_Texture, i, id);
			}
			else
			{
				m_m.SetTextureId(r::MTS_Texture, i, -1);
			}
		}
		else
		{
			pkg::Package::Entry::Ref entry = engine.sys->packages->Resolve(s->c_str);
			if (!entry && !(flags&P_NoDefaultMedia))
				entry = engine.sys->packages->Resolve("Sys/T_Missing");
			if (!entry)
				return SR_MissingFile;
			if (entry->type != asset::AT_Texture)
				return SR_MetaError; // this must be a texture.
			m_m.SetTextureId(r::MTS_Texture, i, entry->id);
		}

		path.Printf("Texture%d.Source.FramesPerSecond", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;
		float fps;
		sscanf(s->c_str, "%f", &fps);
		m_m.SetTextureFPS(r::MTS_Texture, i, fps);

		path.Printf("Texture%d.Source.ClampTextureFrames", i+1);
		b = asset->entry->KeyValue<bool>(path.c_str, P_TARGET_FLAGS(flags));
		if (!b)
			return SR_MetaError;
		m_m.SetClampTextureFrames(r::MTS_Texture, i, *b);

		path.Printf("Texture%d.tcGen", i+1);
		s = asset->entry->KeyValue<String>(path.c_str, P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		if (*s == "Vertex")
			m_m.SetTcGen(r::MTS_Texture, i, r::Material::TcGen_Vertex);
		else if (*s == "EnvMap")
			m_m.SetTcGen(r::MTS_Texture, i, r::Material::TcGen_EnvMap);
		else
			return SR_MetaError;

		for (int k = 0; k < r::Material::NumTcMods; ++k)
		{
			path.Printf("Texture%d.tcMod.%s", i+1, s_tcModNames[k]);
			z = path + ".Type";

			WaveAnim &S = m_m.Wave(
				r::MTS_Texture,
				i,
				k,
				r::Material::S
			);

			WaveAnim &T = m_m.Wave(
				r::MTS_Texture,
				i,
				k,
				r::Material::T
			);

			s = asset->entry->KeyValue<String>(z.c_str, P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			if (*s == "Identity")
				S.type = T.type = WaveAnim::T_Identity;
			else if (*s == "Constant")
				S.type = T.type = WaveAnim::T_Constant;
			else if (*s == "Square")
				S.type = T.type = WaveAnim::T_Square;
			else if (*s == "Sawtooth")
				S.type = T.type = WaveAnim::T_Sawtooth;
			else if (*s == "Triangle")
				S.type = T.type = WaveAnim::T_Triangle;
			else if (*s == "Noise")
				S.type = T.type = WaveAnim::T_Noise;
			else
				return SR_MetaError;

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
	for (int i = r::Material::Color0; i < r::Material::NumColors; ++i)
	{
		for (int k = r::Material::ColorA; k < r::Material::NumColorIndices; ++k)
		{
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

		if (*s == "Identity")
			C.type = WaveAnim::T_Identity;
		else if (*s == "Constant")
			C.type =  WaveAnim::T_Constant;
		else if (*s == "Square")
			C.type = WaveAnim::T_Square;
		else if (*s == "Sawtooth")
			C.type = WaveAnim::T_Sawtooth;
		else if (*s == "Triangle")
			C.type = WaveAnim::T_Triangle;
		else if (*s == "Noise")
			C.type = WaveAnim::T_Noise;
		else
			return SR_MetaError;

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

void MaterialParser::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<MaterialParser>();
}

///////////////////////////////////////////////////////////////////////////////

MaterialLoader::MaterialLoader() : m_current(Unloaded), m_index(0)
{
}

MaterialLoader::~MaterialLoader()
{
}

pkg::Asset::Ref MaterialLoader::Texture(r::MTSource source, int index)
{
	RAD_ASSERT(source < r::MTS_Max && index < r::MTS_MaxIndices);
	return m_textures[source][index];
}

int MaterialLoader::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
)
{
	if (!(flags&(P_Load|P_Unload|P_Parse|P_Info|P_Cancel|P_Trim)))
		return SR_Success;

	if (flags&P_Cancel)
	{
		Cancel();
		return SR_Success;
	}

	if (flags&P_Load)
		flags &= ~P_Parse;
	if (flags&P_Parse)
		flags &= ~P_Info;
	
	if (m_current == Done && (flags&P_Load))
		return SR_Success;
	if ((m_current == Done || m_current == Parsed) && (flags&P_Parse))
		return SR_Success;
	if ((m_current == Done || m_current == Parsed || m_current == Info) && (flags&P_Info))
		return SR_Success;
	if ((m_current == Parsed || m_current == Info) && (flags&P_Load))
		m_current = Unloaded; // fully load.
	if (m_current == Info && (flags&P_Parse))
		m_current = Unloaded; // fully load.
	if (m_current == Unloaded && (flags&P_Unload))
		return SR_Success;
	if ((m_current == Unloaded || m_current == Info) && (flags&P_Trim))
		return SR_Success;

	if (flags&P_Unload)
	{
		Unload();
		return SR_Success;
	}

	MaterialParser::Ref parser = MaterialParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	if (m_current == Unloaded)
	{
		if (flags&(P_Parse|P_Load))
		{
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
			m_current = 0;

			if (!time.remaining)
				return SR_Pending;
		}
		else
		{ // skip shader loading
			RAD_ASSERT(flags&(P_Info|P_Trim));
			m_index = 0;
			m_current = 0;
		}
	}
	else if (flags&(P_Info|P_Trim))
	{
		m_index = 0;
		m_current = 0;
	}

	do
	{
		bool load = true;
		pkg::Asset::Ref &tex = m_textures[m_current][m_index];

		if (!tex)
		{
			int id = parser->material->TextureId((r::MTSource)m_current, m_index);
			if (id < 0)
			{
				load = false;
#if defined(RAD_OPT_TOOLS)
				if (flags&(P_Parse|P_Load))
				{
					if (!parser->procedural && parser->material->shader->Requires(r::Shader::P_Default, (r::MTSource)m_current, m_index))
						return SR_MissingFile;
				}
#endif
			}

			if (load)
			{
				tex = engine.sys->packages->Asset(id, asset->zone);
				
#if defined(RAD_OPT_TOOLS)
				if (!tex && (flags&(P_Parse|P_Load)))
				{
					if (parser->material->shader->Requires(r::Shader::P_Default, (r::MTSource)m_current, m_index))
						return SR_MissingFile;
				}
#endif
			}
		}

		if (tex)
		{
			int r = tex->Process(
				time,
				flags
			);

			if (r != SR_Success)
				return r;

			// flag animated based on texture bundle?
			TextureParser::Ref texParser = TextureParser::Cast(tex);
			if (!texParser)
				return SR_MetaError;
			if (texParser->numImages > 0 && (parser->material->TextureFPS((r::MTSource)m_current, m_index)>0.f))
				parser->material->animated = true;
		}

		++m_index;
		if (m_index >= r::MTS_MaxIndices)
		{
			m_index = 0;
			++m_current;
			if (m_current >= r::MTS_Max)
			{
				m_current = (flags&(P_Load|P_Trim)) ? Done : (flags&P_Parse) ? Parsed : Info;
				return SR_Success; // done loading.
			}
		}

	} while (time.remaining);

	return SR_Pending;
}

void MaterialLoader::Cancel()
{
	if (m_current <= Unloaded)
		return;

	m_current = Unloaded;

	for (int i = 0; i < r::MTS_Max; ++i)
	{
		for (int k = 0; k < r::MTS_MaxIndices; ++k)
		{
			if (!m_textures[i][k])
				break;
			m_textures[i][k]->Process(
				xtime::TimeSlice::Infinite,
				P_Cancel
			);
		}
	}
}

void MaterialLoader::Unload()
{
	for (int i = 0; i < r::MTS_Max; ++i)
		for (int k = 0; k < r::MTS_MaxIndices; ++k)
			m_textures[i][k].reset();

	m_current = Unloaded;
}

void MaterialLoader::Register(Engine &engine)
{
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<MaterialLoader>();
}

} // asset
