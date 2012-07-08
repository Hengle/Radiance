// Material.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Game/WaveAnim.h"
#include "../Packages/PackagesDef.h"
#include "../Engine.h"
#include "Sources.h"
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

	enum Sort
	{
		S_Solid,
		S_Translucent,
		S_Translucent2,
		S_Translucent3,
		S_Translucent4,
		S_Translucent5,
		NumSorts
	};

	enum DepthFunc
	{
		DT_None,
		DT_Less,
		DT_LEqual,
		DT_Greater,
		DT_GEqual
	};

	enum AlphaTest
	{
		AT_None,
		AT_Less,
		AT_LEqual,
		AT_Greater,
		AT_GEqual
	};

	enum BlendMode
	{
		BM_None,
		BM_Alpha,
		BM_InvAlpha,
		BM_Additive,
		BM_AddBlend,
		BM_Colorize,
		BM_InvColorizeD,
		BM_InvColorizeS
	};

	enum
	{
		// TcGen
		TcGen_Vertex = 0,
		TcGen_EnvMap,
		NumTcGens,
		// TcMod
		RAD_FLAG_BIT(TcModFlag_Rotate, 0),
		RAD_FLAG(TcModFlag_Turb),
		RAD_FLAG(TcModFlag_Scale),
		RAD_FLAG(TcModFlag_Shift),
		RAD_FLAG(TcModFlag_Scroll),
		TcMod_Rotate = 0,
		TcMod_Turb,
		TcMod_Scale,
		TcMod_Shift,
		TcMod_Scroll,
		NumTcMods,
		NumTcModVals = 6,
		Color0 = 0,
		Color1,
		NumColors,
		ColorA = 0,
		ColorB,
		NumColorIndices
	};

	enum TexCoord
	{
		S,
		T,
		NumCoords
	};

	enum TimingMode
	{
		TM_Absolute,
		TM_Relative
	};

	typedef boost::shared_ptr<Material> Ref;
	typedef boost::weak_ptr<Material> WRef;

	Material();
	~Material();

	RAD_DECLARE_PROPERTY(Material, sort, Sort, Sort);
	RAD_DECLARE_PROPERTY(Material, blendMode, BlendMode, BlendMode);
	RAD_DECLARE_PROPERTY(Material, depthFunc, DepthFunc, DepthFunc);
	RAD_DECLARE_PROPERTY(Material, alphaTest, AlphaTest, AlphaTest);
	RAD_DECLARE_PROPERTY(Material, alphaVal, U8, U8);
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

	int TcGen(MTSource source, int index) const;
	void SetTcGen(MTSource source, int index, int gen);
	
	WaveAnim &Wave(MTSource source, int index, int tcMod, TexCoord tc);
	const WaveAnim &Wave(MTSource source, int index, int tcMod, TexCoord tc) const;
	int TcModFlags(MTSource source, int index) const;

	void SetTextureId(MTSource source, int index, int id);
	int TextureId(MTSource source, int index) const;

	void SetTextureFPS(MTSource source, int index, float fps);
	float TextureFPS(MTSource source, int index) const;

	void SetClampTextureFrames(MTSource, int index, bool clamp);
	bool ClampTextureFrames(MTSource, int index) const;

	int LoadShader(
		const xtime::TimeSlice &time,
		Engine &engine,
		int flags
	);

	void ReleaseShader();

	void Sample(float time, float dt);
	void Clear();

	// Returns 6 floats (st sampled + accumulated + ex)
	void Sample(MTSource source, int index, int tcMod, int &ops, float *out) const;
	void SampleColor(int num, float *out) const; // returns 4 floats

	// Apply rendering states
	// See RB for implementation.
	void BindStates(int flags=0, int blends=0);
	void BindTextures(const asset::MaterialLoaderRef &loader);


#if defined(RAD_OPT_PC_TOOLS)
	int CookShader(const char *path, Engine &engine, int pflags);
	static void BeginCook();
	static void EndCook();
#endif

private:

#if defined(RAD_OPT_TOOLS)
	RAD_DECLARE_GET(shaderName, const char*) { return m_shaderName.c_str; }
	RAD_DECLARE_SET(shaderName, const char*) { if (value) { m_shaderName = value; } else { m_shaderName.Clear(); } }
#endif
	RAD_DECLARE_GET(shader, Shader::Ref) { return m_shader->shader; }
	RAD_DECLARE_GET(sort, Sort) { return m_sort; }
	RAD_DECLARE_SET(sort, Sort) { m_sort = value; }
	RAD_DECLARE_GET(alphaTest, AlphaTest) { return m_alphaTest; }
	RAD_DECLARE_SET(alphaTest, AlphaTest) { m_alphaTest = value; }
	RAD_DECLARE_GET(alphaVal, U8) { return m_alphaVal; }
	RAD_DECLARE_SET(alphaVal, U8) { m_alphaVal = value; }
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

	struct ShaderInstance
	{
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
			bool skinned,
			const Material &material
		);

#if defined(RAD_OPT_TOOLS)

		// Implemented by RB
		static Shader::Ref Load(
			Engine &engine, 
			bool skinned,
			const Material &material
		);

		bool CanShare(const Material &m) const;
		void CopyWaveTypes(const Material &m);

		WaveAnim::Type types[MTS_Max][MTS_MaxIndices][NumTcMods];
		String shaderName;
		WRefList::iterator it;
		int pflags;
		static WRefList s_shaders;
		bool cooked;
#endif
		int idx;
		Shader::Ref shader;

		typedef boost::mutex Mutex;
		typedef boost::lock_guard<Mutex> Lock;

		static Mutex s_m;
		static WRefVec s_cShaders;

#if defined(RAD_OPT_PC_TOOLS)
		static int Cook(const char *path, Engine &engine, const Material &m, int pflags);
		static RefList s_cookedShaders;
#endif
	};

	WaveAnim m_waves[MTS_Max][MTS_MaxIndices][NumTcMods][NumCoords];
	WaveAnim m_colorWaves[NumColors];
	int m_tcGen[MTS_Max][MTS_MaxIndices];
	int m_ids[MTS_Max][MTS_MaxIndices];
	int m_waveOps[MTS_Max][MTS_MaxIndices][NumTcMods];
	float m_waveSamples[MTS_Max][MTS_MaxIndices][NumTcMods][NumCoords][3];
	float m_colors[NumColors][NumColorIndices][4];
	float m_sampledColor[NumColors][4];
	float m_textureFPS[MTS_Max][MTS_MaxIndices];
	bool m_textureClamp[MTS_Max][MTS_MaxIndices];
	float m_time;
	TimingMode m_timingMode;
	ShaderInstance::Ref m_shader;
#if defined(RAD_OPT_TOOLS)
	String m_shaderName;
#endif
	int m_shaderId;
	Sort m_sort;
	DepthFunc m_depthFunc;
	AlphaTest m_alphaTest;
	BlendMode m_blendMode;
	bool m_animated;
	bool m_doubleSided;
	bool m_depthWrite;
	U8 m_alphaVal;
};

} // namespace

#include <Runtime/PopPack.h>
#include "Material.inl"


