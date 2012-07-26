// CGUtils.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Engine.h"
#include <Runtime/Stream/STLStream.h>
#include <iostream>
#include <Runtime/PushPack.h>

namespace cg {

	typedef boost::shared_ptr<std::istream> istreamRef;
	typedef boost::shared_ptr<stream::basic_streambuf<char> > streamBufRef;

	class IncludeSource
	{
	public:
		virtual bool AddInclude(const char *name, std::ostream &out) = 0;
	};

	struct File
	{
		file::MMFileInputBuffer::Ref ib;
		streamBufRef sb;
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

} // cg

#include <Runtime/PopPack.h>
