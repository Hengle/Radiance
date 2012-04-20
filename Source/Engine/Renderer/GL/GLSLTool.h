// GLSLTool.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Types.h"
#include "../CG/CGUtils.h"
#include "GLState.h"
#include <Runtime/Container/ZoneVector.h>
#include <iostream>
#include <Runtime/PushPack.h>

class Engine;

namespace r {

class Material;

class RADENG_CLASS GLSLTool : public cg::IncludeSource
{
public:

	typedef zone_vector<String, ZEngineT>::type StringVec;

	bool Assemble(
		Engine &engine,
		bool vertex,
		bool skinned,
		bool gles,
		const StringVec &textureTypes,
		int numTexCoords,
		int numColors,
		int numUColors,
		int numNormals,
		const Material *material,
		const GLState::MInputMappings *mapping,
		bool optimize,
		std::ostream &out
	);

	virtual bool AddInclude(const char *name, std::ostream &out) { return true; }
};

} // r

#include <Runtime/PopPack.h>
