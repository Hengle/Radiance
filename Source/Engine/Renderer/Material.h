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

class MaterialLoader;
typedef boost::shared_ptr<MaterialLoader> MaterialLoaderRef;

} // asset

namespace r {

class RADENG_CLASS Material
{
public:

	enum Sort {
		kSort_Solid,
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

	RAD_DECLARE_PROPERTY(Material, sort, Sort, Sort);
	RAD_DECLARE_PROPERTY(Material, blendMode, BlendMode, BlendMode);
	RAD_DECLARE_PROPERTY(Material, depthFunc, DepthFunc, DepthFunc);
	RAD_DECLARE_PROPERTY(Material, doubleSided, bool, bool);
	RAD_DECLARE_PROPERTY(Material, depthWrite, bool, bool);
	RAD_DECLARE_PROPERTY(Material, shaderId, int, int);
	RAD_DECLARE_PROPERTY(Material, animated, bool, bool);
	RAD_DECLARE_PROPERTY(Material, time, float, float);
	RAD_DECLARE_PROPERTY(Material, timingMode, TimingMode, TimingMode);
	RAD_DECLARE_READONLY_PROPERTY(Material, shader, Shader::Ref);

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_PROPERTY(Material, shaderName, const char *, const char *);
#endif

	void SetColor(int num, int index, float *c);
	void Color(int num, int index, float *out) const;
	const WaveAnim &ColorWave(int num) const;
	WaveAnim &ColorWave(int num);

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
	void SampleColor(int num, float *out) const; // returns 4 floats

	// Apply rendering states
	// See RB for implementation.
	void BindStates(int flags=0, int blends=0);
	void BindTextures(asset::MaterialLoader *loader);

#if defined(RAD_OPT_PC_TOOLS)
	int CookShader(const char *path, Engine &engine, int pflags);
	static void BeginCook();
	static void EndCook();
#endif

private:

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
	RAD_DECLARE_GET(shader, Shader::Ref) { return m_shader->shader; }
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
	RAD_DECLARE_GET(time, float) { return m_time; }
	RAD_DECLARE_SET(time, float) { m_time = value; }
	RAD_DECLARE_GET(timingMode, TimingMode) { return m_timingMode; }
	RAD_DECLARE_SET(timingMode, const TimingMode&); // if this is not const TimingMode& we get an ICE!!!!! WTF!!!!

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

		boost::array<
			boost::array<WaveAnim::Type, kNumTCMods>,
		kMaterialTextureSource_MaxIndices> types;

		boost::array<int, kMaxTextures> uvIndices;

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

	boost::array<WaveAnim, kNumColors> m_colorWaves;
	
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

	boost::array<
		boost::array<
			boost::array<float, 4>,
		kNumColorIndices>,
	kNumColors> m_colors;
	
	boost::array<
		boost::array<float, 4>,
	kNumColors> m_sampledColor;

	boost::array<float, kMaterialTextureSource_MaxIndices> m_textureFPS;
	boost::array<U8, kMaterialTextureSource_MaxIndices> m_textureClamp;
	typedef boost::array<int, kMaterialTextureSource_MaxIndices> MTSInts;
	typedef boost::array<int, kMaterialTextureSource_MaxIndices> MTSU8;

	MTSInts m_ids;
	MTSU8 m_tcGen;
	MTSU8 m_tcUVIndex;
	Vec4 m_blend[3];

	float m_time;
	float m_blendTime[2];
	TimingMode m_timingMode;
	ShaderInstance::Ref m_shader;
#if defined(RAD_OPT_TOOLS)
	String m_shaderName;
#endif
	int m_shaderId;
	Sort m_sort;
	DepthFunc m_depthFunc;
	BlendMode m_blendMode;
	bool m_animated;
	bool m_doubleSided;
	bool m_depthWrite;
	U8 m_alphaVal;
};

} // namespace

#include <Runtime/PopPack.h>
#include "Material.inl"


