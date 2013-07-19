/*! \file SkAnimSetCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "SkAnimSetCooker.h"
#include "SkAnimSetParser.h"
#include <Runtime/Tokenizer.h>
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

SkAnimSetCooker::SkAnimSetCooker() : Cooker(7) {
}

SkAnimSetCooker::~SkAnimSetCooker() {
}

CookStatus SkAnimSetCooker::CheckRebuildFiles(int flags) {
	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s)
		return CS_Error;

	StringVec sources;
	int r = SkAnimSetParser::LoadToolsFile(
		s->c_str,
		*engine.get(),
		&sources,
		0
	);

	if (r != SR_Success)
		return CS_NeedRebuild; // will fail

	CookStatus x = CS_UpToDate;

	for (StringVec::const_iterator it = sources.begin(); it != sources.end(); ++it) {
		if (CompareCachedFileTime(flags, (*it).c_str, (*it).c_str)) {
			x = CS_NeedRebuild; // cache all file times.
		}
	}

	return x;
}

CookStatus SkAnimSetCooker::Status(int flags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;

	return CheckRebuildFiles(flags);
}

int SkAnimSetCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");
	CheckRebuildFiles(flags);

	int r = asset->Process(
		xtime::TimeSlice::Infinite, 
		flags|P_Load|P_TargetDefault
	);

	if (r < SR_Success)
		return r;

	SkAnimSetParser *parser = SkAnimSetParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref fp = OpenWrite(path.c_str);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);

	if (os.Write(parser->m_skad->skaData, (stream::SPos)parser->m_skad->skaSize, 0) != (stream::SPos)parser->m_skad->skaSize)
		return SR_IOError;

	return SR_Success;
}

void SkAnimSetCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SkAnimSetCooker>();
}

} // asset
