// Shader.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "Sources.h"
#include <Runtime/StreamDef.h>

class Engine;

namespace r {

class Material;

class Shader
{
public:

	enum Pass
	{
		P_First,
		P_Default = P_First,
		NumPasses
	};

	enum OutputFlags
	{
		RAD_FLAG(OF_Color),
		RAD_FLAG(OF_Depth)
	};

	typedef boost::shared_ptr<Shader> Ref;

	Shader() : m_guid(s_guid++) {}
	virtual ~Shader() {}

	virtual int Usage(Pass p, MTSource source) const = 0;
	virtual int Usage(Pass p, MGSource source) const = 0;
	virtual bool Requires(Pass p, MTSource source, int index) const = 0;
	virtual bool Requires(Pass p, MGSource source, int index) const = 0;
	virtual int Outputs(Pass p) const = 0;
	virtual bool HasPass(Pass p) const = 0;

	// Bind the shader, and required textures
	virtual void Begin(Pass p, const Material &material) = 0;
	// Bind the geometry states
	virtual void BindStates(bool sampleMaterialColor = true, const Vec4 &rgba = Vec4(1, 1, 1, 1)) = 0;
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
