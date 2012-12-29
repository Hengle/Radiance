/*! \file Shader.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup renderer
*/

#include RADPCH
#include "Shader.h"

namespace r {

int Shader::s_guid = 0;
const Shader::Uniforms Shader::Uniforms::kDefault = Shader::Uniforms(Shader::Uniforms::defaultTag());

bool Shader::Requires(MaterialTextureSource source, int index) const {
	for (int i = 0; i < kNumPasses; ++i) {
		if (Requires((Shader::Pass)i, source, index))
			return true;
	}

	return false;
}

bool Shader::Requires(MaterialGeometrySource source, int index) const {
	for (int i = 0; i < kNumPasses; ++i) {
		if (Requires((Shader::Pass)i, source, index))
			return true;
	}

	return false;
}

} // r
