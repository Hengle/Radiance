/*! \file Material.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "../Types.h"
#include "../Game/WaveAnim.h"
#include "../Packages/PackagesDef.h"
#include "../Engine.h"
#include "Common.h"
#include "Shader.h"
#include <Runtime/Thread/Locks.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/StreamDef.h>
#include <Runtime/PushPack.h>

namespace asset {

class MaterialParser;
class MaterialLoader;
typedef boost::shared_ptr<MaterialLoader> MaterialLoaderRef;

} // asset

namespace r {

class RADENG_CLASS Material : public boost::noncopyable
{
public:

	enum SkinMode {
		kSkinMode_Vertex,
		kSkinMode_Sprite,
		kNumSkinModes
	};

	enum Sort {
		kSort_Solid,
		kSort_Fog,
		kSort_Translucent,
		kSort_Translucent2,
		kSort_Translucent3,
		kSort_Translucent4,
		kSort_Translucent5,
		kNumSorts
	};

	enum DepthFunc {
		kDepthFunc_None,
		kDepthFunc_Less,
		kDepthFunc_LEqual,
		kDepthFunc_Greater,
		kDepthFunc_GEqual,
		kDepthFunc_Equal
	};

	enum BlendMode {
		kBlendMode_None,
		kBlendMode_Alpha,
		kBlendMode_InvAlpha,
		kBlendMode_Additive,
		kBlendMode_AddBlend,
		kBlendMode_Colorize,
		kBlendMode_InvColorizeD,
		kBlendMode_InvColorizeS
	};

	enum {
		// TcGen
		kTCGen_Vertex = 0,
		kTCGen_EnvMap,
		kTCGen_Projected,
		kNumTCGens,
		// TcMod
		RAD_FLAG_BIT(kTCModFlag_Rotate, 0),
		RAD_FLAG(kTCModFlag_Turb),
		RAD_FLAG(kTCModFlag_Scale),
		RAD_FLAG(kTCModFlag_Shift),
		RAD_FLAG(kTCModFlag_Scroll),
		kTCMod_Rotate = 0,
		kTCMod_Turb,
		kTCMod_Scale,
		kTCMod_Shift,
		kTCMod_Scroll,
		kNumTCMods,
		kNumTCModVals = 6,
		kColor0 = 0,
		kColor1,
		kNumColors,
		kColorA = 0,
		kColorB,
		kNumColorIndices
	};

	enum TexCoord {
		kTexCoord_S,
		kTexCoord_T,
		kNumTexCoordDimensions
	};

	enum TimingMode {
		kTimingMode_Absolute,
		kTimingMode_Relative
	};

	typedef boost::shared_ptr<Material> Ref;
	typedef boost::weak_ptr<Material> WRef;

	Material();
	~Material();

	RAD_DECLARE_PROPERTY(Material, skinMode, SkinMode, SkinMode);
	RAD_DECLARE_PROPERTY(Material, sort, Sort, Sort);
	RAD_DECLARE_PROPERTY(Material, blendMode, BlendMode, BlendMode);
	RAD_DECLARE_PROPERTY(Material, depthFunc, DepthFunc, DepthFunc);
	RAD_DECLARE_PROPERTY(Material, doubleSided, bool, bool);
	RAD_DECLARE_PROPERTY(Material, depthWrite, bool, bool);
	RAD_DECLARE_PROPERTY(Material, shaderId, int, int);
	RAD_DECLARE_PROPERTY(Material, animated, bool, bool);
	RAD_DECLARE_PROPERTY(Material, time, float, float);
	RAD_DECLARE_PROPERTY(Material, timingMode, TimingMode, TimingMode);
	RAD_DECLARE_PROPERTY(Material, castShadows, bool, bool);
	RAD_DECLARE_PROPERTY(Material, receiveShadows, bool, bool);
	RAD_DECLARE_PROPERTY(Material, selfShadow, bool, bool);
	RAD_DECLARE_PROPERTY(Material, specularExponent, float, float);
	RAD_DECLARE_READONLY_PROPERTY(Material, maxLights, int);
	RAD_DECLARE_READONLY_PROPERTY(Material, specularColorWave, WaveAnim*);
	RAD_DECLARE_READONLY_PROPERTY(Material, shader, const Shader::Ref&);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_PROPERTY(Material, shaderName, const char *, const char *);
#endif

	void SetColor(int num, int index, const Vec4 &color);
	Vec4 Color(int num, int index) const;
	WaveAnim &ColorWave(int num);

	void SetSpecularColor(int index, const Vec3 &color);
	Vec3 SpecularColor(int index) const;

	int TCUVIndex(int index) const;
	void SetTCUVIndex(int index, int uvIndex);

	int TCGen(int index) const;
	void SetTCGen(int index, int gen);
	
	WaveAnim &Wave(int index, int tcMod, TexCoord tc);
	const WaveAnim &Wave(int index, int tcMod, TexCoord tc) const;
	int TCModFlags(int index) const;

	void SetTextureId(int index, int id);
	int TextureId(int index) const;

	void SetTextureFPS(int index, float fps);
	float TextureFPS(int index) const;

	void SetClampTextureFrames(int index, bool clamp);
	bool ClampTextureFrames(int index) const;

	int LoadShader(
		const xtime::TimeSlice &time,
		Engine &engine,
		int flags
	);

	void ReleaseShader();

	void BlendTo(const Vec4 &rgba, float time);

	void Sample(float time, float dt);
	void Clear();

	// Returns 6 floats (st sampled + accumulated + ex)
	void Sample(int index, int tcMod, int &ops, float *out) const;
	Vec4 SampleColor(int num) const;
	Vec3 SampleSpecularColor() const;

	// Apply rendering states
	// See RB for implementation.
	void BindStates(int flags=0, int blends=0);
	void BindTextures(asset::MaterialLoader *loader);

	void InitDefaultSamples();

#if defined(RAD_OPT_PC_TOOLS)
	int CookShader(const char *path, Engine &engine, int pflags);
	static void BeginCook();
	static void EndCook();
#endif

private:

	friend class asset::MaterialParser;
	friend class asset::MaterialLoader;

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(shaderName, const char*) { 
		return m_shaderName.c_str; 
	}

	RAD_DECLARE_SET(shaderName, const char*) { 
		if (value) { 
			m_shaderName = value; 
		} else { 
			m_shaderName.Clear(); 
		} 
	}
#endif
	RAD_DECLARE_GET(skinMode, SkinMode) { return m_skinMode; }
	RAD_DECLARE_SET(skinMode, SkinMode) { m_skinMode = value; }
	RAD_DECLARE_GET(shader, const Shader::Ref&) { return m_shader->shader; }
	RAD_DECLARE_GET(sort, Sort) { return m_sort; }
	RAD_DECLARE_SET(sort, Sort) { m_sort = value; }
	RAD_DECLARE_GET(depthFunc, DepthFunc) { return m_depthFunc; }
	RAD_DECLARE_SET(depthFunc, DepthFunc) { m_depthFunc = value; }
	RAD_DECLARE_GET(animated, bool) { return m_animated; }
	RAD_DECLARE_SET(animated, bool) { m_animated = value; }
	RAD_DECLARE_GET(doubleSided, bool) { return m_doubleSided; }
	RAD_DECLARE_SET(doubleSided, bool) { m_doubleSided = value; }
	RAD_DECLARE_GET(depthWrite, bool) { return m_depthWrite; }
	RAD_DECLARE_SET(depthWrite, bool) { m_depthWrite = value; }
	RAD_DECLARE_GET(blendMode, BlendMode) { return m_blendMode; }
	RAD_DECLARE_SET(blendMode, BlendMode) { m_blendMode = value; }
	RAD_DECLARE_GET(shaderId, int) { return m_shaderId; }
	RAD_DECLARE_SET(shaderId, int) { m_shaderId = value; }
	RAD_DECLARE_GET(maxLights, int) { return m_maxLights; }
	RAD_DECLARE_GET(time, float) { return m_time; }
	RAD_DECLARE_SET(time, float) { m_time = value; }
	RAD_DECLARE_GET(timingMode, TimingMode) { return m_timingMode; }
	RAD_DECLARE_SET(timingMode, TimingMode);
	RAD_DECLARE_GET(castShadows, bool) { return m_castShadows; }
	RAD_DECLARE_SET(castShadows, bool) { m_castShadows = value; }
	RAD_DECLARE_GET(receiveShadows, bool) { return m_receiveShadows; }
	RAD_DECLARE_SET(receiveShadows, bool) { m_receiveShadows = value; }
	RAD_DECLARE_GET(selfShadow, bool) { return m_selfShadow; }
	RAD_DECLARE_SET(selfShadow, bool) { m_selfShadow = value; }
	RAD_DECLARE_GET(specularExponent, float) { return m_specularExponent; }
	RAD_DECLARE_SET(specularExponent, float) { m_specularExponent = value; }
	RAD_DECLARE_GET(specularColorWave, WaveAnim*) {
		return const_cast<WaveAnim*>(&m_specularWave);
	}

	typedef boost::array<int, kMaterialTextureSource_MaxIndices> MTSU8;

	// Basically I want materials that can to share a Shader::Ref to minimize
	// the cost of material changes (i.e. I can sort materials by shader and
	// not incur a GLSL/HLSL program change for every material).

	struct ShaderInstance {
		typedef boost::shared_ptr<ShaderInstance> Ref;
		typedef boost::weak_ptr<ShaderInstance> WRef;
		typedef zone_list<Ref, ZEngineT>::type RefList;
		typedef zone_list<WRef, ZEngineT>::type WRefList;
		typedef zone_vector<WRef, ZEngineT>::type WRefVec;

		static Ref Find(Engine &engine, const Material &m);
		static Ref FindOrCreate(Engine &engine, const Material &m);

		ShaderInstance();
		~ShaderInstance();

		// Implemented by RB
		static Shader::Ref LoadCooked(
			Engine &engine,
			const char *shaderName,
			stream::InputStream &is,
			const Material &material
		);

#if defined(RAD_OPT_TOOLS)

		// Implemented by RB
		static Shader::Ref Load(
			Engine &engine, 
			const Material &material
		);

		bool CanShare(const Material &m) const;
		void CopySharedData(const Material &m);

		SkinMode skinMode;

		boost::array<
			boost::array<WaveAnim::Type, kNumTCMods>,
		kMaterialTextureSource_MaxIndices> types;

		boost::array<int, kMaxTextures> uvIndices;

		MTSU8 tcGen;

		String shaderName;
		WRefList::iterator it;
		int pflags;
		static WRefList s_shaders;
		bool cooked;
#endif
		int idx;
		Shader::Ref shader;

		typedef boost::recursive_mutex Mutex;
		typedef boost::lock_guard<Mutex> Lock;

		static Mutex s_m;
		static WRefVec s_cShaders;

#if defined(RAD_OPT_PC_TOOLS)
		static int Cook(const char *path, Engine &engine, const Material &m, int pflags);
		static RefList s_cookedShaders;
#endif
	};

	boost::array<
		boost::array<
			boost::array<WaveAnim, kNumTexCoordDimensions>,
		kNumTCMods>,
	kMaterialTextureSource_MaxIndices> m_waves;

	boost::array<
		boost::array<int, kNumTCMods>, 
	kMaterialTextureSource_MaxIndices> m_waveOps;

	boost::array<
		boost::array<
			boost::array<
				boost::array<float, 3>,
			kNumTexCoordDimensions>,
		kNumTCMods>,
	kMaterialTextureSource_MaxIndices> m_waveSamples;

	boost::array<WaveAnim, kNumColors> m_colorWaves;

	boost::array<
		boost::array<Vec4, kNumColorIndices>,
	kNumColors> m_colors;
	
	boost::array<Vec4, kNumColors> m_sampledColor;

	boost::array<Vec3, kNumColorIndices> m_specularColors;
	Vec3 m_sampledSpecularColor;
	WaveAnim m_specularWave;

	boost::array<float, kMaterialTextureSource_MaxIndices> m_textureFPS;
	boost::array<U8, kMaterialTextureSource_MaxIndices> m_textureClamp;
	typedef boost::array<int, kMaterialTextureSource_MaxIndices> MTSInts;

	MTSInts m_ids;
	MTSU8 m_tcGen;
	MTSU8 m_tcUVIndex;
	Vec4 m_blend[3];

	float m_time;
	float m_blendTime[2];
	float m_specularExponent;
	TimingMode m_timingMode;
	ShaderInstance::Ref m_shader;
	int m_shaderId;
	int m_maxLights;
	SkinMode m_skinMode;
	Sort m_sort;
	DepthFunc m_depthFunc;
	BlendMode m_blendMode;
	bool m_animated;
	bool m_doubleSided;
	bool m_depthWrite;
	bool m_castShadows;
	bool m_receiveShadows;
	bool m_selfShadow;

#if defined(RAD_OPT_TOOLS)
	String m_shaderName;
#endif
};

} // namespace

#include <Runtime/PopPack.h>
#include "Material.inl"


