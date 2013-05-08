/*! \file Material.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

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

Material::ShaderInstance::ShaderInstance() {
	idx = -1;
#if defined(RAD_OPT_TOOLS)
	cooked = false;
	pflags = 0;
	skinMode = kNumSkinModes;

	for (int i = 0 ;i < kMaxTextures; ++i)
		uvIndices[i] = 0;

	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		for (int k = 0; k < kNumTCMods; ++k) {
			types[i][k] = WaveAnim::T_Identity;
		}
	}
#endif
}

Material::ShaderInstance::~ShaderInstance() {
	Lock L(s_m);
#if defined(RAD_OPT_TOOLS)
	if (idx < 0 && it != s_shaders.end()) {
		s_shaders.erase(it);
	} else if (idx >= 0 && !cooked)
#endif
	{
		s_cShaders[idx].reset();
	}
}

#if defined(RAD_OPT_TOOLS)

bool Material::ShaderInstance::CanShare(const Material &m) const {
	
	if (skinMode != m.skinMode)
		return false;

	for (int i = 0; i < kMaxTextures; ++i) {
		if (uvIndices[i] != m.TCUVIndex(i))
			return false;
	}

	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		for (int k = 0; k < kNumTCMods; ++k) {
			if (m.Wave(i, k, Material::kTexCoord_S).type != types[i][k]) {
				return false;
			}
		}
	}
	return true;
}

void Material::ShaderInstance::CopySharedData(const Material &m) {

	skinMode = m.skinMode;

	for (int i = 0; i < kMaxTextures; ++i)
		uvIndices[i] = m.TCUVIndex(i);

	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		for (int k = 0; k < kNumTCMods; ++k) {
			types[i][k] = m.Wave(i, k, Material::kTexCoord_S).type;
		}
	}
}

#endif

Material::ShaderInstance::Ref Material::ShaderInstance::Find(Engine &engine, const Material &m) {
	Lock L(s_m);
	
#if defined(RAD_OPT_TOOLS)
	if (m.shaderId < 0) {
		for (WRefList::const_iterator it = s_shaders.begin(); it != s_shaders.end(); ++it) {
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

Material::ShaderInstance::Ref Material::ShaderInstance::FindOrCreate(Engine &engine, const Material &m) {
	Lock L(s_m);
	
#if defined(RAD_OPT_TOOLS)
	if (m.shaderId < 0) {
		for (WRefList::const_iterator it = s_shaders.begin(); it != s_shaders.end(); ++it) {
			Ref r((*it).lock());
			if (r && (r->shaderName == m.shaderName.get()) && r->CanShare(m))
				return r;
		}

		Shader::Ref shader = Load(engine, m);

		if (!shader)
			return Ref();

		Ref r(new (ZEngine) ShaderInstance());
		r->shaderName = m.shaderName;
		r->shader = shader;
		r->CopySharedData(m);
		
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

	if (!r) {
		String path;
		path.Printf("Shaders/%d.bin", m.shaderId.get());

		file::MMFileInputBuffer::Ref ib = engine.sys->files->OpenInputBuffer(path.c_str, ZRender);
		if (!ib)
			return Ref();

		stream::InputStream is(*ib);

		Shader::Ref shader = LoadCooked(
			engine,
			path.c_str,
			is,
			m
		);

		if (shader) {
			r.reset(new (ZEngine) ShaderInstance());
			r->idx = (int)idx;
			r->shader = shader;
			s_cShaders[idx] = r;
		}
	}

	return r;
}

#if defined(RAD_OPT_PC_TOOLS)

int Material::ShaderInstance::Cook(const char *path, Engine &engine, const Material &m, int pflags) {
	Lock L(s_m);

	pflags &= pkg::P_AllTargets;
	
	for (RefList::const_iterator it = s_cookedShaders.begin(); it != s_cookedShaders.end(); ++it) {
		const Ref &r = *it;
		if (r->shaderName == m.shaderName.get() && r->CanShare(m) && r->pflags == pflags)
			return r->idx;
	}

	Ref r(new (ZEngine) ShaderInstance());
	r->shaderName = m.shaderName;
	r->CopySharedData(m);
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

	try {
		s = m.shader->CompileShaderSource(engine, os, pflags, m);
	} catch (exception&) {
	}
	
	fclose(fp);

	if (!s)
		return -1;
	
	s_cookedShaders.push_back(r);
	return r->idx;
}

int Material::CookShader(const char *path, Engine &engine, int pflags) {
	return ShaderInstance::Cook(path, engine, *this, pflags);
}

void Material::BeginCook() {
}

void Material::EndCook() {
	ShaderInstance::s_cookedShaders.clear();
}

#endif

Material::Material() : 
m_skinMode(kSkinMode_Vertex),
m_sort(kSort_Solid), 
m_depthFunc(kDepthFunc_Less),
m_animated(false),
m_doubleSided(false),
m_depthWrite(true),
m_lit(false),
m_castShadows(false),
m_receiveShadows(false),
m_selfShadow(false),
m_blendMode(kBlendMode_None),
m_shaderId(-1),
m_time(0.f),
m_timingMode(kTimingMode_Absolute),
m_specularExponent(32.f) {
	Clear();

	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		m_tcGen[i] = kTCGen_Vertex;
		m_ids[i] = -1;
		m_textureFPS[i] = 0.f;
		m_textureClamp[i] = 0;
		m_tcUVIndex[i] = 0;
	}

	for (int i = 0; i < kNumColors; ++i)
		for (int k = 0; k < kNumColorIndices; ++k)
			for (int j = 0; j < 4; ++j)
				m_colors[i][k][j] = 1.f;

	for (int i = 0; i < kNumColors; ++i)
		for (int k = 0; k < 4; ++k)
			m_sampledColor[i][k] = 1.f;

	for (int i = 0; i < kNumColorIndices; ++i)
		m_specularColors[i] = Vec3(1.f, 1.f, 1.f);

	m_sampledSpecularColor = Vec3(1.f, 1.f, 1.f);

	m_blend[0] = m_blend[1] = m_blend[3] = Vec4(1,1,1,1);
	m_blendTime[0] = m_blendTime[1] = -1.f;
}

Material::~Material() {
}

int Material::LoadShader(
	const xtime::TimeSlice &time,
	Engine &engine,
	int flags
) {
	if (m_shader)
		return pkg::SR_Success;

	m_shader = ShaderInstance::FindOrCreate(engine, *this);
	return m_shader ? pkg::SR_Success : pkg::SR_ErrorGeneric;
}

int Material::TCModFlags(int index) const {
	const boost::array<boost::array<WaveAnim, kNumTexCoordDimensions>, kNumTCMods> &wave = m_waves[index];
	int flags = 0;

	if (wave[kTCMod_Rotate][kTexCoord_S].type != WaveAnim::T_Identity)
		flags |= kTCModFlag_Rotate;
	if (wave[kTCMod_Turb][kTexCoord_S].type != WaveAnim::T_Identity)
		flags |= kTCModFlag_Turb;
	if (wave[kTCMod_Scale][kTexCoord_S].type != WaveAnim::T_Identity)
		flags |= kTCModFlag_Scale;
	if (wave[kTCMod_Shift][kTexCoord_S].type != WaveAnim::T_Identity)
		flags |= kTCModFlag_Shift;
	if (wave[kTCMod_Scroll][kTexCoord_S].type != WaveAnim::T_Identity)
		flags |= kTCModFlag_Scroll;

	return flags;
}

void Material::Clear() {
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		for (int k = 0; k < kNumTCMods; ++k) {
			m_waveOps[i][k] = 0;

			for (int p = 0; p < kNumTexCoordDimensions; ++p) {
				for (int z = 0; z < 3; ++z) {
					m_waveSamples[i][k][p][z] = 0.f;
				}
			}
		}
	}
}

void Material::RAD_IMPLEMENT_SET(timingMode)(TimingMode mode) {
	if (mode != kTimingMode_Absolute)
		m_time = 0.f; // reset time.
	m_timingMode = mode;
}

void Material::Sample(float time, float dt) {
	if (m_timingMode == kTimingMode_Absolute) {
		m_time = time;
	} else {
		m_time += dt;
	}

	if (m_blendTime[1] > 0.f) {
		m_blendTime[0] += dt;
		if (m_blendTime[0] >= m_blendTime[1]) {
			m_blendTime[0] = m_blendTime[1] = -1.f;
			m_blend[0] = m_blend[2];
		} else {
			m_blend[0] = math::Lerp(m_blend[1], m_blend[2], m_blendTime[0] / m_blendTime[1]);
		}
	}
		
	for (int i = 0; i < kMaterialTextureSource_MaxIndices; ++i) {
		for (int k = 0; k < kNumTCMods; ++k) {
			int &ops = m_waveOps[i][k];

			if (k == kTCMod_Turb) { 
				// turb only needs phase/amplitude
				boost::array<boost::array<float, 3>, kNumTexCoordDimensions> &samples = m_waveSamples[i][k];
				WaveAnim &S = m_waves[i][k][kTexCoord_S];
				WaveAnim &T = m_waves[i][k][kTexCoord_T];

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

				samples[0][0] = sVal*math::Constants<float>::_2_PI();
				samples[0][1] = S.amplitude;
				samples[0][2] = S.phase;
				samples[1][0] = tVal*math::Constants<float>::_2_PI();
				samples[1][1] = T.amplitude;
				samples[1][2] = T.phase;
			} else {
				for (int p = 0; p < kNumTexCoordDimensions; ++p) {
					const WaveAnim &wave = m_waves[i][k][p];
					boost::array<float, 3> &samples = m_waveSamples[i][k][p];

					float val = wave.Sample(time);

					if (k == kTCMod_Rotate) { 
						// sin/cos [-pi/pi]
						if ((val > 1.f) || (val < -1.f)) {
							val = val - FloorFastFloat(val); // [-1, 1]
						}
						val *= math::Constants<float>::PI();
						if ((ops == 0) || (val != samples[2])) {
							samples[2] = val;
							samples[0] = math::FastSin(val);
							samples[1] = math::FastCos(val);
							++ops;
						}
					} else {
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

	for (int i = 0; i < kNumColors; ++i) {
		float lerp = 1.f - m_colorWaves[i].Sample(time);
		lerp = math::Clamp(lerp, 0.f, 1.f);
		m_sampledColor[i] = m_blend[0] * math::Lerp(m_colors[i][kColorA], m_colors[i][kColorB], lerp);
	}

	{
		float lerp = 1.f - m_specularWave.Sample(time);
		lerp = math::Clamp(lerp, 0.f, 1.f);
		m_sampledSpecularColor = math::Lerp(m_specularColors[kColorA], m_specularColors[kColorB], lerp);
	}
}

void Material::BlendTo(const Vec4 &rgba, float time) {
	if (time <= 0.f) {
		m_blendTime[0] = m_blendTime[1] = -1.f;
		m_blend[0] = rgba;
	} else {
		m_blendTime[0] = 0.f;
		m_blendTime[1] = time;
		m_blend[1] = m_blend[0];
		m_blend[2] = rgba;
	}
}

} // r
