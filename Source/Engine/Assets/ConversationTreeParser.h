/*! \file ConversationTreeParser.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup assets
*/

#pragma once

#include "AssetTypes.h"
#include "StringTableParser.h"
#include "../Packages/Packages.h"
#include "ConversationTree.h"
#include <Runtime/PushPack.h>

class Engine;

namespace asset {

class RADENG_CLASS ConversationTreeParser : public pkg::Sink<ConversationTreeParser> {
public:

	static void Register(Engine &engine);

	enum {
		SinkStage = pkg::SS_Parser,
		AssetType = AT_ConversationTree
	};

	ConversationTreeParser();
	virtual ~ConversationTreeParser();

#if defined(RAD_OPT_PC_TOOLS)

	static int Save(
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	RAD_DECLARE_READONLY_PROPERTY(ConversationTreeParser, conversationTree, ConversationTree*);
#else
	RAD_DECLARE_READONLY_PROPERTY(ConversationTreeParser, conversationTree, const ConversationTree*);
#endif

	RAD_DECLARE_READONLY_PROPERTY(ConversationTreeParser, stringTable, const StringTable*);
	RAD_DECLARE_READONLY_PROPERTY(ConversationTreeParser, stringTableAsset, const pkg::Asset::Ref&);

protected:

	virtual int Process(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	int Load(
		const xtime::TimeSlice &time,
		Engine &engine,
		const pkg::Asset::Ref &asset,
		int flags
	);

	#if defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_GET(conversationTree, ConversationTree*) {
		return m_conversation.get();
	}
#else
	RAD_DECLARE_GET(conversationTree, const ConversationTree*) {
		return m_conversation.get();
	}
#endif

	RAD_DECLARE_GET(stringTable, const StringTable*) {
		return m_stringParser->stringTable;
	}

	RAD_DECLARE_GET(stringTableAsset, const pkg::Asset::Ref&) {
		return m_stringTable;
	}

	pkg::Asset::Ref m_stringTable;
	ConversationTree::Ref m_conversation;
	StringTableParser *m_stringParser;
};

} // asset

#include <Runtime/PopPack.h>
