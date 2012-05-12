// StringTableCooker.cpp
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "StringTableParser.h"
#include "StringTableCooker.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

StringTableCooker::StringTableCooker() : Cooker(1) {
}

StringTableCooker::~StringTableCooker() {
}

CookStatus StringTableCooker::Status(int flags, int allflags) {
	flags &= P_AllTargets;
	allflags &= P_AllTargets;

	if (flags == 0) // string tables only cook on generics pass.
		return CheckRebuild(flags, allflags);

	return CS_Ignore;
}

int StringTableCooker::Compile(int flags, int allflags) {

	// Make sure these are updated.
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.Root");

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		flags|P_Load|P_TargetDefault
	);

	if (r < SR_Success)
		return r;

	StringTableParser::Ref parser = StringTableParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	const String *s = asset->entry->KeyValue<String>("Source.Root", flags);
	if (!s)
		return SR_MetaError;

	WString path(string::Widen(asset->path));
	path += L".bin";

	BinFile::Ref fp = OpenWrite(path.c_str(), flags);
	if (!fp)
		return SR_IOError;

	if (!parser->stringTable->SaveBin(fp->ob))
		return SR_IOError;

	return SR_Success;
}

CookStatus StringTableCooker::CheckRebuild(int flags, int allflags) {
	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) ||
		CompareCachedFileTimeKey(flags, "Source.Root"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

void StringTableCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<StringTableCooker>();
}

} // asset
