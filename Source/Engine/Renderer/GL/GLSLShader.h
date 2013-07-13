// GLSLShader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../RendererDef.h"
#include "../Material.h"
#include "GLShader.h"
#include "GLState.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/ZoneHashMap.h>
#include <Runtime/PushPack.h>

class Engine;

namespace r {

///////////////////////////////////////////////////////////////////////////////

class GLSLShaderObj {
public:
	typedef boost::shared_ptr<GLSLShaderObj> Ref;
	GLSLShaderObj(GLenum type);
	~GLSLShaderObj();
	RAD_DECLARE_READONLY_PROPERTY(GLSLShaderObj, id, GLhandleARB);
private:
	RAD_DECLARE_GET(id, GLhandleARB);
	GLhandleARB m_name;
};

///////////////////////////////////////////////////////////////////////////////

class GLSLProgramObj {
public:
	typedef boost::shared_ptr<GLSLProgramObj> Ref;
	GLSLProgramObj();
	~GLSLProgramObj();
	RAD_DECLARE_READONLY_PROPERTY(GLSLProgramObj, id, GLhandleARB);
private:
	RAD_DECLARE_GET(id, GLhandleARB);
	GLhandleARB m_name;
};

///////////////////////////////////////////////////////////////////////////////

class GLSLShaderLink;
class RADENG_CLASS GLSLShader : public GLShader {
public:
	typedef boost::shared_ptr<GLSLShader> Ref;

#if defined(RAD_OPT_TOOLS)
	static Ref Load(
		Engine &engine, 
		const char *name,
		const Material &material
	);
#endif

	static Ref LoadCooked(
		const char *name,
		stream::InputStream &is,
		const Material &material
	);

	virtual int Usage(Shader::Pass p, MaterialTextureSource source) const;
	virtual int Usage(Shader::Pass p, MaterialGeometrySource source) const;
	virtual bool Requires(Shader::Pass p, MaterialTextureSource source, int index) const;
	virtual bool Requires(Shader::Pass p, MaterialGeometrySource source, int index) const;
	virtual int Outputs(Shader::Pass p) const;
	virtual bool HasPass(Shader::Pass p) const;
	virtual void Begin(Shader::Pass p, const Material &material);
	virtual void BindStates(const r::Shader::Uniforms &uniforms, bool sampleMaterialColor = true);
	virtual void End();

#if defined(RAD_OPT_PC_TOOLS)
	virtual bool CompileShaderSource(
		Engine &engine,
		stream::OutputStream &os, 
		int pflags,
		const Material &material
	);
#endif

protected:

	virtual RAD_DECLARE_GET(curPass, Pass) {
		return m_curPass;
	}

private:

	GLSLShader();

	friend class GLSLShaderLink;

	typedef zone_vector<int, ZRenderT>::type IntVec;

	struct Uniforms {
		boost::array<boost::array<GLint, Material::kNumTCMods+1>, kMaterialTextureSource_MaxIndices> tcMods;
		boost::array<
			boost::array<
				boost::array<float, Material::kNumTCModVals>,
			Material::kNumTCMods>,
		kMaterialTextureSource_MaxIndices> tcModVals;
		
		boost::array<GLint, kMaxTextures> textures;
		int matrixOps;
		int prMatrixOps;
		GLint mv;
		GLint pr;
		float mvfloats[16];
		float prfloats[16];
		GLint dcolor;
		Vec4 drgba;
		GLint scolor;
		Vec4 srgba;
		LightEnv lights;
		Vec3 eyePos;
		Mat4 tcPrjMat;
		Vec2 pfxVars;
		GLint lightPos[kMaxLights];
		GLint lightDiffuse[kMaxLights];
		GLint lightSpecular[kMaxLights];
		GLint eye;
		GLint tcPrj;
		GLint pfx;
		
#if defined(RAD_OPT_OGLES2)
		GLint mvp;
		float mvpfloats[16];
#endif
	};

	struct Pass {
		GLSLProgramObj::Ref p;
		Uniforms u;
		int outputs;
		int numReqVaryings;
		MaterialInputMappings m;
	};

	bool LoadPass(
		const char *name,
		stream::InputStream &is,
		const Material &material
	);

	bool MapInputs(Pass &p, const Material &material);

#if defined(RAD_OPT_TOOLS)
	bool Compile(
		Engine &e,
		const tools::shader_utils::Shader::Ref &shader,
		const Material &material
	);

	bool CompilePass(
		Engine &e,
		Shader::Pass pass,
		const tools::shader_utils::Shader::Ref &shader,
		const MaterialInputMappings &m,
		const tools::shader_utils::Shader::TexCoordMapping &tcMapping,
		int numVaryingFloats,
		const Material &material
	);

	int CalcNumShaderVaryings(
		Shader::Pass pass,
		const tools::shader_utils::Shader::Ref &shader,
		const MaterialInputMappings &m,
		const tools::shader_utils::Shader::TexCoordMapping &tcMapping,
		const Material &material
	);

#endif

	String ShaderLog(GLhandleARB s);
	String ProgramLog(GLhandleARB s);

	void BindAttribLocations(GLhandleARB p, const MaterialInputMappings &m);
	String m_name;
	Shader::Pass m_curPass;
	const r::Material *m_curMat;

	boost::array<Pass, Shader::kNumPasses> m_passes;
};

} // r

#include <Runtime/PopPack.h>
#include "GLSLShader.inl"
