/*! \file ShaderToolUtils.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup renderer
*/

#include RADPCH

#include "ShadertoolUtils.h"
#include "../COut.h"
#include "RendererDef.h"
#include <Runtime/StringBase.h>
#include <Runtime/File.h>
#include <stdio.h>

namespace tools {
namespace shader_utils {

bool OpenFile(
	Engine &e,
	const char *filename, 
	File &out
) {
	out.ib = e.sys->files->OpenInputBuffer(filename, ZTools);

	if (!out.ib) {
		COut(C_ErrMsgBox) << "ShaderTool: Error opening \"" << filename << "\"" << std::endl;
		return false;
	}

	out.sb.reset(new (r::ZRender) stream::basic_streambuf<char>(out.ib.get(), 0));
	out.is.reset(new std::istream(out.sb.get()));
	return out.is->good();
}

void Copy(
	std::istream &is,
	std::ostream &out,
	bool resetIn
) {
	enum { kMaxLineLength = Kilo*4 };
	char lineBuf[kMaxLineLength];

	if (resetIn)
		is.seekg(0);

	for (;;) {
		is.read(lineBuf, kMaxLineLength);
		out.write(lineBuf, is.gcount());
		if (is.bad() || is.fail())
			break;
	}
}

bool Inject(
	Engine &e,
	const char *filename,
	std::ostream &out
) {
	File f;
	if (!OpenFile(e, filename, f))
		return false;
	Copy(*f.is.get(), out);
	return true;
}

bool ParseInclude(const char *line, String &filename) {
	while (*line && *line != '#') {
		++line;
	}
	if (!*line)
		return false;
	const int z = string::len("#include ");
	if (string::len(line) < string::len("#include "))
		return false;
	if (string::ncmp(line, "#include ", z))
		return false;
	line += z;
	if (!*line)
		return false;
	// skip " or <
	++line;
	while (*line && *line != '"' && *line != '>') {
		filename += *line;
		++line;
	}
	return true;
}

bool ExpandIncludes(
	IncludeSource &isrc,
	std::istream &is,
	std::ostream &os
) {
	enum { kMaxLineLength = Kilo*4 };
	char lineBuf[kMaxLineLength];

	is.seekg(0);

	for(;;) {
		is.getline(lineBuf, kMaxLineLength, '\n');
		if (is.bad() || is.fail())
			break;

		String include;
		if (ParseInclude(lineBuf, include)) {
			if (!isrc.AddInclude(include.c_str, os))
				return false;
		} else {
			os << lineBuf << '\n';
		}
	}

	return true;
}

void SaveText(
	Engine &engine,
	const char *filename,
	const char *sz
) {
	FILE *fp = engine.sys->files->fopen(filename, "wb", file::kFileOptions_None, file::kFileMask_Base);
	if (fp) {
		fwrite(sz, 1, string::len(sz), fp);
		fclose(fp);
	}
}

} // shader_utils
} // tools

