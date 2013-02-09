// SkAnimSetCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkAnimSetCooker.h"
#include "SkAnimSetParser.h"
#include <Runtime/Tokenizer.h>
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

SkAnimSetCooker::SkAnimSetCooker() : Cooker(5) {
}

SkAnimSetCooker::~SkAnimSetCooker() {
}

CookStatus SkAnimSetCooker::CheckRebuild(int flags, int allflags) {
	if (CompareVersion(flags) || 
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;

	return CheckRebuildFiles(flags, allflags);
}

CookStatus SkAnimSetCooker::CheckRebuildFiles(int flags, int allflags) {
	const String *s = asset->entry->KeyValue<String>("Source.File", flags);
	if (!s)
		return CS_NeedRebuild;

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

CookStatus SkAnimSetCooker::Status(int flags, int allflags) {
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0) { 
		// only build generics if all platforms are identical to eachother.
		if (MatchTargetKeys(allflags, allflags)==allflags)
			return CheckRebuild(flags, allflags);
		return CS_Ignore;
	}

	if (MatchTargetKeys(allflags, allflags)==allflags)
		return CS_Ignore;

	// only build ipad if different from iphone
	if ((flags&P_TargetIPad) && (allflags&P_TargetIPhone)) {
		if (MatchTargetKeys(P_TargetIPad, P_TargetIPhone))
			return CS_Ignore;
	}

	return CheckRebuild(flags, allflags);
}

int SkAnimSetCooker::Compile(int flags, int allflags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");
	CheckRebuildFiles(flags, allflags);

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

	BinFile::Ref fp = OpenWrite(path.c_str, flags);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);

	if (os.Write(parser->m_skad->skaData, (stream::SPos)parser->m_skad->skaSize, 0) != (stream::SPos)parser->m_skad->skaSize)
		return SR_IOError;

	return SR_Success;
}

int SkAnimSetCooker::MatchTargetKeys(int flags, int allflags) {
	return asset->entry->MatchTargetKeys<String>("Source.File", flags, allflags);
}

void SkAnimSetCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<SkAnimSetCooker>();
}

} // asset
