// CGUtils.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "CGUtils.h"
#include "../../COut.h"
#include "../RendererDef.h"
#include <Runtime/StringBase.h>
#include <Runtime/File.h>
#include <stdio.h>

namespace cg {

bool OpenFile(
	Engine &e,
	const char *filename, 
	File &out
)
{
	int media = file::AllMedia;
	file::Result r = e.sys->files->OpenFileStream(
		filename,
		media,
		out.ib,
		file::HIONotify(),
		Kilo
	);

	if (r != file::Success)
	{
		COut(C_ErrMsgBox) << "GLSLTool: Error opening \"" << filename << "\"" << std::endl;
		return false;
	}

	out.sb.reset(new (r::ZRender) stream::basic_streambuf<char>(&out.ib->buffer.get(), 0));
	out.is.reset(new std::istream(out.sb.get()));
	return out.is->good();
}

void Copy(
	std::istream &is,
	std::ostream &out,
	bool resetIn
)
{
	enum { MaxLineLength = Kilo*4 };
	char lineBuf[MaxLineLength];

	if (resetIn)
		is.seekg(0);

	for (;;)
	{
		is.read(lineBuf, MaxLineLength);
		out.write(lineBuf, is.gcount());
		if (is.bad() || is.fail())
			break;
	}
}

bool Inject(
	Engine &e,
	const char *filename,
	std::ostream &out
)
{
	File f;
	if (!OpenFile(e, filename, f))
		return false;
	Copy(*f.is.get(), out);
	return true;
}

bool ParseInclude(const char *line, String &filename)
{
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
	while (*line && *line != '"' && *line != '>')
	{
		filename += *line;
		++line;
	}
	return true;
}

bool ExpandIncludes(
	IncludeSource &isrc,
	std::istream &is,
	std::ostream &os
)
{
	enum { MaxLineLength = Kilo*4 };
	char lineBuf[MaxLineLength];

	is.seekg(0);

	for(;;)
	{
		is.getline(lineBuf, MaxLineLength, '\n');
		if (is.bad() || is.fail())
			break;

		String include;
		if (ParseInclude(lineBuf, include))
		{
			if (!isrc.AddInclude(include.c_str, os))
				return false;
		}
		else
		{
			os << lineBuf << '\n';
		}
	}

	return true;
}

void SaveText(
	Engine &engine,
	const char *filename,
	const char *sz
)
{
	String path(CStr("9:/") + engine.sys->files->hddRoot.get());
	path += '/';
	path += filename;
	char nativePath[file::MaxFilePathLen+1];
	if (file::ExpandToNativePath(path.c_str, nativePath, file::MaxFilePathLen+1))
	{
		FILE *fp = fopen(nativePath, "wb");
		if (fp)
		{
			fwrite(sz, 1, string::len(sz), fp);
			fclose(fp);
		}
	}
}

} // cg

#endif // RAD_OPT_TOOLS

