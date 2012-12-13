// SkAnimSetCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "SkAnimSetCooker.h"
#include "SkAnimSetParser.h"
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

	FILE *fp = engine->sys->files->fopen(
		s->c_str.get(), 
		"rt", 
		file::kFileOptions_None, 
		file::kFileMask_Base
	);

	if (fp == 0)
		return CS_NeedRebuild;

	char name[256];

	CookStatus r = CS_UpToDate;

	while (fgets(name, 256, fp) != 0) {

		for (char *c = name; *c; ++c) {
			if (*c < 20) {
				*c = 0;
				break;
			}
		}

		if (CompareCachedFileTime(flags, name, name)) {
			r = CS_NeedRebuild; // NOTE: no break here because we must cache all times
		}
	}

	fclose(fp);
	return r;
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

	SkAnimSetParser::Ref parser = SkAnimSetParser::Cast(asset);
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
