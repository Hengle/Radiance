/*! \file ConversationTreeCooker.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "ConversationTreeCooker.h"
#include "ConversationTreeParser.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

ConversationTreeCooker::ConversationTreeCooker() : Cooker(1) {
}

ConversationTreeCooker::~ConversationTreeCooker() {
}

CookStatus ConversationTreeCooker::Status(int flags) {
	if (CompareVersion(flags) ||
		CompareModifiedTime(flags) || 
		CompareCachedFileTimeKey(flags, "Source.File"))
		return CS_NeedRebuild;
	return CS_UpToDate;
}

int ConversationTreeCooker::Compile(int flags) {
	// Make sure these get updated
	CompareVersion(flags);
	CompareModifiedTime(flags);
	CompareCachedFileTimeKey(flags, "Source.File");

	int r = asset->Process(
		xtime::TimeSlice::Infinite,
		flags|P_Load|P_TargetDefault
	);

	if (r != SR_Success)
		return r;

	ConversationTreeParser *parser = ConversationTreeParser::Cast(asset);
	if (!parser)
		return SR_MetaError;

	const String *stringTable = asset->entry->KeyValue<String>("Source.StringTable", flags);
	if (!stringTable)
		return SR_MetaError;

	AddImport(stringTable->c_str);

	String path(CStr(asset->path));
	path += ".bin";

	BinFile::Ref fp = OpenWrite(path.c_str);
	if (!fp)
		return SR_IOError;

	stream::OutputStream os(fp->ob);
	if (!parser->conversationTree->SaveBin(os))
		return SR_IOError;

	return SR_Success;
}

void ConversationTreeCooker::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->BindCooker<ConversationTreeCooker>();
}

} // asset
