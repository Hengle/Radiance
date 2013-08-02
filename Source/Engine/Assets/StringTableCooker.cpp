/*! \file StringTableCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "StringTableParser.h"
#include "StringTableCooker.h"
#include <Runtime/Stream.h>
#include "../Engine.h"

using namespace pkg;

namespace asset {

StringTableCooker::StringTableCooker() : Cooker(2) {
}

StringTableCooker::~StringTableCooker() {
}

CookStatus StringTableCooker::CheckRebuildFiles(int flags) {
	const String *root = asset->entry->KeyValue<String>("Source.Root", flags);
	if (!root)
		return CS_Error;

	CookStatus x = CS_UpToDate;

	for (int i = StringTable::LangId_First; i < StringTable::LangId_MAX; ++i) {
		const String kPath(*root + CStr(StringTable::Langs[i]));
		if (CompareCachedFileTime(flags, kPath.c_str, kPath.c_str)) {
			x = CS_NeedRebuild; // cache all file times.
		}
	}

	return x;
}

CookStatus StringTableCooker::Status(int flags) {
	if (CompareVersion(flags) || CompareModifiedTime(flags))
		return CS_NeedRebuild;
	return CheckRebuildFiles(flags);
}

int StringTableCooker::Compile(int flags) {

	// Make sure these are updated.
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CheckRebuildFiles(flags);

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		flags|P_Load|P_TargetDefault
	);

	if (r < SR_Success)
		return r;

	StringTableParser *parser = StringTableParser::Cast(asset);
	if (!parser)
		return SR_ParseError;

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref fp = OpenWrite(path.c_str);
	if (!fp)
		return SR_IOError;

	if (!parser->stringTable->SaveBin(fp->ob))
		return SR_IOError;

	return SR_Success;
}

void StringTableCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<StringTableCooker>();
}

} // asset
