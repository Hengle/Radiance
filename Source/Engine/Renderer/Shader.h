/*! \file Shader.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "../Types.h"
#include "Common.h"
#include <Runtime/StreamDef.h>

class Engine;

namespace r {

class Material;

class Shader {
public:

	enum Pass {
		kPass_First,
		kPass_Default, // blend is controlled by material
		kPass_Diffuse1,
		kPass_Diffuse2,
		kPass_Diffuse3,
		kPass_Diffuse4,
		kPass_Specular1,
		kPass_Specular2,
		kPass_Specular3,
		kPass_Specular4,
		kPass_DiffuseSpecular1,
		kPass_DiffuseSpecular2,
		kPass_DiffuseSpecular3,
		kPass_DiffuseSpecular4,
		kPass_Fullbright,
#if defined(RAD_OPT_PC_TOOLS)
		kPass_Preview, // material preview
#endif
		kNumPasses
	};

	enum Output {
		kOutput_First,
		kOutput_Color = kOutput_First,
		kOutput_Depth,
		kNumOutputs
	};

	struct Uniforms {
		LightEnv lights;
		Vec4 blendColor;

		struct defaultTag {};

		Uniforms() {}
		Uniforms(const Uniforms &u) : lights(u.lights), blendColor(u.blendColor) {}
		explicit Uniforms(const defaultTag&) : blendColor(1.f, 1.f, 1.f, 1.f) {
			lights.numLights = 0;
		}
		explicit Uniforms(const Vec4 &color) : blendColor(color) {
			lights.numLights = 0;
		}

		static const Uniforms kDefault;
	};

	typedef boost::shared_ptr<Shader> Ref;

	Shader() : m_guid(s_guid++) {}
	virtual ~Shader() {}

	bool Requires(MaterialTextureSource source, int index) const;
	bool Requires(MaterialGeometrySource source, int index) const;

	virtual int Usage(Pass p, MaterialTextureSource source) const = 0;
	virtual int Usage(Pass p, MaterialGeometrySource source) const = 0;
	virtual bool Requires(Pass p, MaterialTextureSource source, int index) const = 0;
	virtual bool Requires(Pass p, MaterialGeometrySource source, int index) const = 0;
	virtual int Outputs(Pass p) const = 0;
	virtual bool HasPass(Pass p) const = 0;

	// Bind the shader, and required textures
	virtual void Begin(Pass p, const Material &material) = 0;
	// Bind the geometry states
	virtual void BindStates(
		const Uniforms &uniforms = Uniforms::kDefault,
		bool sampleMaterialColor = true
	) = 0;
	virtual void End() = 0;

#if defined(RAD_OPT_PC_TOOLS)
	virtual bool CompileShaderSource(
		Engine &engine,
		stream::OutputStream &os, 
		int pflags,
		const Material &material
	) = 0;
#endif

	RAD_DECLARE_READONLY_PROPERTY(Shader, guid, int);

private:

	RAD_DECLARE_GET(guid, int) { return m_guid; }

	int m_guid;

	static int s_guid;
};

} // r
