/*! \file ConversationTreeParser.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#include RADPCH
#include "ConversationTreeParser.h"
#include "../Engine.h"
#include <Runtime/Stream.h>

using namespace pkg;

namespace asset {

ConversationTreeParser::ConversationTreeParser() : m_stringParser(0) {
}

ConversationTreeParser::~ConversationTreeParser() {
}

int ConversationTreeParser::Process(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (!(flags&(P_Load|P_Unload|P_Parse)))
		return SR_Success;

	if (m_conversation && (flags&(P_Load|P_Parse)))
		return SR_Success;

	if (flags&P_Unload) {
		m_conversation.reset();
		m_stringParser = 0;
		m_stringTable.reset();
		return SR_Success;
	}

	return Load(time, engine, asset, flags);
}

int ConversationTreeParser::Load(
	const xtime::TimeSlice &time,
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {

	file::MMFileInputBuffer::Ref ib;

#if defined(RAD_OPT_TOOLS)
	if (!asset->cooked) {

		if (!m_stringTable) {
			const String *s = asset->entry->KeyValue<String>("Source.StringTable", P_TARGET_FLAGS(flags));
			if (!s)
				return SR_MetaError;

			m_stringTable = engine.sys->packages->Resolve(s->c_str, asset->zone);
			if (!m_stringTable)
				return SR_FileNotFound;
		}

		if (!m_stringParser) {
			int r = m_stringTable->Process(time, flags);
			if (r != SR_Success)
				return r;

			m_stringParser = StringTableParser::Cast(m_stringTable);
			if (!m_stringParser)
				return SR_MetaError;
		}

		const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));
		if (!s)
			return SR_MetaError;

		ib = engine.sys->files->OpenInputBuffer(
			s->c_str,
			ZAssets,
			kMeg
		);
	} else {
#endif
		if (!m_stringTable) {
			const Package::Entry::Import *imp = asset->entry->Resolve(0);
			if (!imp)
				return SR_ParseError;
			m_stringTable = asset->entry->Resolve(*imp, asset->zone);
			if (!m_stringTable)
				return SR_FileNotFound;
		}

		if (!m_stringParser) {
			int r = m_stringTable->Process(time, flags);
			if (r != SR_Success)
				return r;

			m_stringParser = StringTableParser::Cast(m_stringTable);
			if (!m_stringParser)
				return SR_MetaError;
		}

		String path(CStr("Cooked/"));
		path += CStr(asset->path);
		path +=  ".bin";

		ib = engine.sys->files->OpenInputBuffer(
			path.c_str,
			ZAssets,
			kMeg
		);
#if defined(RAD_OPT_TOOLS)
	}
#endif

	if (!ib) {
#if defined(RAD_OPT_TOOLS)
		if (flags&P_Create) {
			// create a new conversation tree.
			m_conversation.reset(new (ZAssets) ConversationTree());
			return SR_Success;
		}
#endif
		return SR_FileNotFound;
	}

	stream::InputStream is(*ib);

	m_conversation.reset(new (ZAssets) ConversationTree());
	if (!m_conversation->LoadBin(is)) {
		m_conversation.reset();
		return SR_CorruptFile;
	}

	return SR_Success;
}

#if defined(RAD_OPT_PC_TOOLS)

int ConversationTreeParser::Save(
	Engine &engine,
	const pkg::Asset::Ref &asset,
	int flags
) {
	if (asset->type != AT_ConversationTree)
		return SR_ErrorGeneric;

	const String *s = asset->entry->KeyValue<String>("Source.File", P_TARGET_FLAGS(flags));

	FILE *fp = engine.sys->files->fopen(s->c_str, "wb");
	if (!fp)
		return SR_FileNotFound;

	ConversationTreeParser *parser = ConversationTreeParser::Cast(asset);
	if (!parser)
		return SR_MetaError;

	file::FILEOutputBuffer ob(fp);
	stream::OutputStream os(ob);

	if (!parser->conversationTree->SaveBin(os))
		return SR_IOError;

	return SR_Success;
}

#endif

void ConversationTreeParser::Register(Engine &engine) {
	static pkg::Binding::Ref binding = engine.sys->packages->Bind<ConversationTreeParser>();
}

} // asset
