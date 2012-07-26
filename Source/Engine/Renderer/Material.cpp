// Material.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Material.h"
#include <Runtime/Math.h>
#include <Runtime/Stream.h>

#if defined(RAD_OPT_PC_TOOLS)
#include "../Packages/Packages.h"
#include <Runtime/File.h>
#endif

namespace r {

Material::ShaderInstance::Mutex Material::ShaderInstance::s_m;

#if defined(RAD_OPT_TOOLS)
Material::ShaderInstance::WRefList Material::ShaderInstance::s_shaders;
#endif
#if defined(RAD_OPT_PC_TOOLS)
Material::ShaderInstance::RefList Material::ShaderInstance::s_cookedShaders;
#endif
Material::ShaderInstance::WRefVec Material::ShaderInstance::s_cShaders;

Material::ShaderInstance::ShaderInstance()
{
	idx = -1;
#if defined(RAD_OPT_TOOLS)
	cooked = false;
	pflags = 0;
	for (int i = 0; i < MTS_Max; ++i)
		for (int k = 0; k < MTS_MaxIndices; ++k)
			for (int j = 0; j < NumTcMods; ++j)
				types[i][k][j] = WaveAnim::T_Identity;
#endif
}

Material::ShaderInstance::~ShaderInstance()
{
#if defined(RAD_OPT_TOOLS)
	if (idx < 0 && it != s_shaders.end())
	{
		Lock L(s_m);
		s_shaders.erase(it);
	}
	else if (idx >= 0 && !cooked)
#endif
	{
		Lock L(s_m);
		s_cShaders[idx].reset();
	}
}

#if defined(RAD_OPT_TOOLS)

bool Material::ShaderInstance::CanShare(const Material &m) const
{
	for (int i = 0; i < MTS_Max; ++i)
		for (int k = 0; k < MTS_MaxIndices; ++k)
			for (int j = 0; j < NumTcMods; ++j)
				if (m.Wave((MTSource)i, k, j, Material::S).type != types[i][k][j])
					return false;
	return true;
}

void Material::ShaderInstance::CopyWaveTypes(const Material &m)
{
	for (int i = 0; i < MTS_Max; ++i)
		for (int k = 0; k < MTS_MaxIndices; ++k)
			for (int j = 0; j < NumTcMods; ++j)
				types[i][k][j] = m.Wave((MTSource)i, k, j, Material::S).type;
}

#endif

Material::ShaderInstance::Ref Material::ShaderInstance::Find(Engine &engine, const Material &m)
{
	Lock L(s_m);
	
#if defined(RAD_OPT_TOOLS)
	if (m.shaderId < 0)
	{
		for (WRefList::const_iterator it = s_shaders.begin(); it != s_shaders.end(); ++it)
		{
			Ref r((*it).lock());
			RAD_ASSERT(r);
			if (r->shaderName == m.shaderName.get() && r->CanShare(m))
				return r;
		}
	}
	else
#endif
	{
		RAD_ASSERT(m.shaderId>=0);
		size_t idx = (size_t)m.shaderId.get()+1;
		if (s_cShaders.size() < idx+1)
			s_cShaders.resize(idx+1);
		return s_cShaders[idx].lock();
	}

	return Ref();
}

Material::ShaderInstance::Ref Material::ShaderInstance::FindOrCreate(Engine &engine, const Material &m)
{
	Lock L(s_m);
	
#if defined(RAD_OPT_TOOLS)
	if (m.shaderId < 0)
	{
		for (WRefList::const_iterator it = s_shaders.begin(); it != s_shaders.end(); ++it)
		{
			Ref r((*it).lock());
			RAD_ASSERT(r);
			if (r->shaderName == m.shaderName.get() && r->CanShare(m))
				return r;
		}

		Shader::Ref shader = Load(engine, false, m);

		if (!shader)
			return Ref();

		Ref r(new (ZEngine) ShaderInstance());
		r->shaderName = m.shaderName;
		r->shader = shader;
		r->CopyWaveTypes(m);
		
		s_shaders.push_back(r);
		r->it = --(s_shaders.end());

		return r;
	}
#endif
	
	RAD_ASSERT(m.shaderId>=0);
	size_t idx = (size_t)m.shaderId.get()+1;
	if (s_cShaders.size() < idx+1)
		s_cShaders.resize(idx+1);
	Ref r = s_cShaders[idx].lock();

	if (!r)
	{
		String path;
		path.Printf("Shaders/%d.bin", m.shaderId.get());

		file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(path.c_str, ZTools);
		if (!ib)
			return Ref();

		stream::InputStream is(*ib);

		Shader::Ref shader = LoadCooked(
			engine,
			path.c_str,
			is,
			false,
			m
		);

		if (shader)
		{
			r.reset(new (ZEngine) ShaderInstance());
			r->idx = (int)idx;
			r->shader = shader;

			s_cShaders[idx] = r;
		}
	}

	return r;
}

#if defined(RAD_OPT_PC_TOOLS)

int Material::ShaderInstance::Cook(const char *path, Engine &engine, const Material &m, int pflags)
{
	pflags &= pkg::P_AllTargets;
	
	for (RefList::const_iterator it = s_cookedShaders.begin(); it != s_cookedShaders.end(); ++it)
	{
		const Ref &r = *it;
		if (r->shaderName == m.shaderName.get() && r->CanShare(m) && r->pflags == pflags)
			return r->idx;
	}

	Ref r(new (ZEngine) ShaderInstance());
	r->shaderName = m.shaderName;
	r->CopyWaveTypes(m);
	r->pflags = pflags;
	r->idx = (int)s_cookedShaders.size();
	r->cooked = true;
	
	String spath;
	spath.Printf("%s/%d.bin", path, r->idx);

	FILE *fp = engine.sys->files->fopen(spath.c_str, "wb");
	
	if (!fp)
		return -1;

	file::FILEOutputBuffer ob(fp);
	stream::OutputStream os(ob);

	bool s = false;

	try
	{
		s = m.shader->CompileShaderSource(engine, os, pflags, m);
	}
	catch (exception&)
	{
	}
	
	fclose(fp);

	if (!s)
		return -1;
	
	s_cookedShaders.push_back(r);
	return r->idx;
}

int Material::CookShader(const char *path, Engine &engine, int pflags)
{
	return ShaderInstance::Cook(path, engine, *this, pflags);
}

void Material::BeginCook()
{
}

void Material::EndCook()
{
	ShaderInstance::s_cookedShaders.clear();
}

#endif

Material::Material() : 
m_sort(S_Solid), 
m_depthFunc(DT_Less),
m_alphaTest(AT_None),
m_alphaVal(0),
m_animated(false),
m_doubleSided(false),
m_depthWrite(true),
m_blendMode(BM_None),
m_shaderId(-1),
m_time(0.f),
m_timingMode(TM_Absolute)
{
	Clear();

	for (int i = 0; i < MTS_Max; ++i)
	{
		for (int k = 0; k < MTS_MaxIndices; ++k)
		{
			m_tcGen[i][k] = TcGen_Vertex;
			m_ids[i][k] = -1;
			m_textureFPS[i][k] = 0.f;
			m_textureClamp[i][k] = false;
		}
	}

	for (int i = 0; i < NumColors; ++i)
		for (int k = 0; k < NumColorIndices; ++k)
			for (int j = 0; j < 4; ++j)
				m_colors[i][k][j] = 1.f;

	for (int i = 0; i < NumColors; ++i)
		for (int k = 0; k < 4; ++k)
			m_sampledColor[i][k] = 1.f;
}

Material::~Material()
{
}

int Material::LoadShader(
	const xtime::TimeSlice &time,
	Engine &engine,
	int flags
)
{
	if (m_shader)
		return pkg::SR_Success;

	m_shader = ShaderInstance::FindOrCreate(engine, *this);

	return m_shader ? pkg::SR_Success : pkg::SR_ErrorGeneric;
}

int Material::TcModFlags(MTSource source, int index) const
{
	RAD_ASSERT(source < MTS_Max && index < MTS_MaxIndices);
	const WaveAnim (*wave)[NumTcMods][NumCoords] = &m_waves[source][index];
	int flags = 0;

	if ((*wave)[TcMod_Rotate]->type != WaveAnim::T_Identity)
		flags |= TcModFlag_Rotate;
	if ((*wave)[TcMod_Turb]->type != WaveAnim::T_Identity)
		flags |= TcModFlag_Turb;
	if ((*wave)[TcMod_Scale]->type != WaveAnim::T_Identity)
		flags |= TcModFlag_Scale;
	if ((*wave)[TcMod_Shift]->type != WaveAnim::T_Identity)
		flags |= TcModFlag_Shift;
	if ((*wave)[TcMod_Scroll]->type != WaveAnim::T_Identity)
		flags |= TcModFlag_Scroll;

	return flags;
}

void Material::Clear()
{
	for (int i = 0; i < MTS_Max; ++i)
	{
		for (int k = 0; k < MTS_MaxIndices; ++k)
		{
			for (int j = 0; j < NumTcMods; ++j)
			{
				m_waveOps[i][k][j] = 0;

				for (int p = 0; p < NumCoords; ++p)
				{
					for (int z = 0; z < 3; ++z)
					{
						m_waveSamples[i][k][j][p][z] = 0.f;
					}
				}
			}
		}
	}
}

void Material::RAD_IMPLEMENT_SET(timingMode)(const TimingMode &mode)
{
	if (mode != TM_Absolute)
		m_time = 0.f; // reset time.
	m_timingMode = mode;
}

void Material::Sample(float time, float dt)
{
	if (m_timingMode == TM_Absolute)
	{
		m_time = time;
	}
	else
	{
		m_time += dt;
	}

	for (int i = 0; i < MTS_Max; ++i)
	{
		for (int k = 0; k < MTS_MaxIndices; ++k)
		{
			for (int j = 0; j < NumTcMods; ++j)
			{
				int &ops = m_waveOps[i][k][j];

				if (j == TcMod_Turb)
				{ // turb only needs phase/amplitude
					float (*samples)[NumCoords][3] = &m_waveSamples[i][k][j];
					WaveAnim &S = m_waves[i][k][j][0];
					WaveAnim &T = m_waves[i][k][j][1];

					float sAmp = S.amplitude;
					float tAmp = T.amplitude;

					S.amplitude = 1.f;
					T.amplitude = 1.f;

					float sVal = S.Sample(time);
					float tVal = T.Sample(time);

					S.amplitude = sAmp;
					T.amplitude = tAmp;
					
					float sMod = (float)FloatToInt(sVal);
					float tMod = (float)FloatToInt(tVal);
					
					sVal = sVal - sMod; // clamp to range.
					tVal = tVal - tMod;

					(*samples)[0][0] = sVal*math::Constants<float>::PI()*2.0f;
					(*samples)[0][1] = S.amplitude;
					(*samples)[0][2] = S.phase;
					(*samples)[1][0] = tVal*math::Constants<float>::PI()*2.0f;
					(*samples)[1][1] = T.amplitude;
					(*samples)[1][2] = T.phase;
				}
				else
				{
					for (int p = 0; p < NumCoords; ++p)
					{
						const WaveAnim &wave = m_waves[i][k][j][p];
						float *samples = m_waveSamples[i][k][j][p];

						float val = wave.Sample(time);

						if (j == TcMod_Rotate)
						{ // sin/cos
							val *= math::Constants<float>::PI();
							if (val != samples[2])
							{
								samples[2] = val;
								math::SinAndCos(&samples[0], &samples[1], val);
								++ops;
							}
						}
						else
						{
							samples[0] = val;
							float temp = samples[1]+val*dt;
							float whole = (float)FloatToInt(temp);
							samples[1] = temp - whole;
							++ops;
						}
					}
				}
			}
		}
	}

	for (int i = 0; i < NumColors; ++i)
	{
		float lerp = 1.f - m_colorWaves[i].Sample(time);
		lerp = math::Clamp(lerp, 0.f, 1.f);
		for (int k = 0; k < 4; ++k)
			m_sampledColor[i][k] = math::Lerp(m_colors[i][ColorA][k], m_colors[i][ColorB][k], lerp);
	}
}

} // r
