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

class GLSLShaderObj
{
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

class GLSLProgramObj
{
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
class RADENG_CLASS GLSLShader : public GLShader
{
public:
	typedef boost::shared_ptr<GLSLShader> Ref;

#if defined(RAD_OPT_TOOLS)
	static Ref Load(
		Engine &engine, 
		const char *name,
		bool skinned,
		const Material &material
	);
#endif

	static Ref LoadCooked(
		const char *name,
		stream::InputStream &is,
		bool skinned,
		const Material &material
	);

	virtual int Usage(Shader::Pass p, MTSource source) const;
	virtual int Usage(Shader::Pass p, MGSource source) const;
	virtual bool Requires(Shader::Pass p, MTSource source, int index) const;
	virtual bool Requires(Shader::Pass p, MGSource source, int index) const;
	virtual int Outputs(Shader::Pass p) const;
	virtual bool HasPass(Shader::Pass p) const;
	virtual void Begin(Shader::Pass p, const Material &material);
	virtual void BindStates(bool sampleMaterialColor, const Vec4 &rgba);
	virtual void End();

#if defined(RAD_OPT_PC_TOOLS)
	virtual bool CompileShaderSource(
		Engine &engine,
		stream::OutputStream &os, 
		int pflags,
		const Material &material
	);
#endif

private:

	GLSLShader();

	friend class GLSLShaderLink;

	typedef zone_vector<int, ZRenderT>::type IntVec;

	struct Uniforms
	{
#if defined(RAD_OPT_OGLES2)
		GLint mvp;
		GLint color;
		int matrixOps;
		float rgba[4];
		float mvpfloats[16];
#endif
		GLint tcMods[GLState::MaxTextures][Material::NumTcMods+1];
		float tcModsVals[GLState::MaxTextures][Material::NumTcMods][Material::NumTcModVals];
		GLint textures[GLState::MaxTextures];
	};

	struct Pass
	{
		GLSLProgramObj::Ref p;
		Uniforms u;
		int outputs;
		GLState::MInputMappings m;
	};

	bool LoadPass(
		const char *name,
		stream::InputStream &is,
		bool skinned,
		const Material &material
	);

	bool MapInputs(Pass &p, const Material &material);

#if defined(RAD_OPT_TOOLS)
	bool CompilePass(
		Engine &e,
		Pass &p,
		bool skinned,
		const cg::Shader::Ref &s,
		cg::Shader::Channel channel,
		const Material &material
	);
#endif

	String ShaderLog(GLhandleARB s);
	String ProgramLog(GLhandleARB s);

	void BindAttribLocations(GLhandleARB p, const GLState::MInputMappings &m);

	String m_name;
	Shader::Pass m_curPass;
	const r::Material *m_curMat;
	Pass m_passes[GLShader::NumPasses];
};

} // r

#include <Runtime/PopPack.h>
#include "GLSLShader.inl"
