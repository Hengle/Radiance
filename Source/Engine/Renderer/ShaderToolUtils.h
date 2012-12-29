/*! \file ShaderToolUtils.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#pragma once

#include "../Engine.h"
#include <Runtime/Stream/STLStream.h>
#include <iostream>
#include <Runtime/PushPack.h>

namespace tools {
namespace shader_utils {

	// Various free functions for assembling shader files.

	typedef boost::shared_ptr<std::istream> istreamRef;
	typedef boost::shared_ptr<stream::basic_streambuf<char> > streambufRef;

	class IncludeSource {
	public:
		virtual bool AddInclude(const char *name, std::ostream &out) = 0;
	};

	struct File {
		file::MMFileInputBuffer::Ref ib;
		streambufRef sb;
		istreamRef is;
	};

	bool OpenFile(
		Engine &e,
		const char *filename, 
		File &out
	);

	bool Inject(
		Engine &e,
		const char *filename,
		std::ostream &out
	);

	void Copy(
		std::istream &is,
		std::ostream &out,
		bool resetIn=true
	);

	bool ParseInclude(const char *line, String &filename);

	bool ExpandIncludes(
		IncludeSource &isrc,
		std::istream &is,
		std::ostream &os
	);

	void SaveText(
		Engine &engine,
		const char *filename,
		const char *sz
	);

} // shader_utils
} // tools

#include <Runtime/PopPack.h>
