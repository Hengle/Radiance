/*! \file ConversationTree.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup game
*/

#pragma once

#include "../Types.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Container/StackVector.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Base/ObjectPool.h>

#if defined(RAD_OPT_PC_TOOLS)
#include <Runtime/StreamDef.h>
#endif

#include <Runtime/PushPack.h>

struct lua_State;

namespace asset {

class RADENG_CLASS ConversationTree {
public:
	typedef boost::shared_ptr<ConversationTree> Ref;
	
	enum RootFlags {
		RAD_FLAG(kRootFlag_Hidden),
		RAD_FLAG(kRootFlag_Locked)
	};
	
	struct StringOption {
		typedef stackify<zone_vector<StringOption, ZAssetsT>::type, 4> Vec;

		StringOption() {
			probability[0] = 1.f;
			probability[1] = 1.f;
		}

		String text;
		float probability[2];
	};

	struct Dialog {
		typedef stackify<zone_vector<Dialog*, ZAssetsT>::type, 4> PtrVec;

		Dialog() {
			probability = 1.f;
			refs = 0;
			uid = 0;
		}

		StringOption::Vec prompts;
		StringOption::Vec replies;
		PtrVec choices;
		String action;
		String condition;
		String name;
		float probability; // 0 -> 1
		int uid;
		int refs;
	};

	struct Root {
		typedef stackify<zone_vector<Root*, ZAssetsT>::type, 64> PtrVec;

		Root() {
			probability[0] = 1.f;
			probability[1] = 1.f;
			flags = 0;
		}

		StringOption::Vec prompts;
		Dialog::PtrVec dialog;
		String name;
		String group;
		float probability[2];
		int flags;
	};

	ConversationTree();
	~ConversationTree();

	RAD_DECLARE_READONLY_PROPERTY(ConversationTree, roots, Root::PtrVec*);

	Root *NewRoot();
	void DeleteRoot(Root &root);

	Dialog *NewDialog();
	void DeleteDialog(Dialog &dialog);

	void RefDialog(Dialog &dialog);
	void UnrefDialog(Dialog &dialog);

	const Dialog *DialogForUID(int uid) const;
	const Dialog *DialogForName(const char *name) const;

	Dialog *DialogForUID(int uid);
	Dialog *DialogForName(const char *name);

	void MapDialog(Dialog &dialog);
	void UnmapDialog(Dialog &dialog);

	int PushCopy(lua_State *L) const;

	// Save/Load streams will be adapted to EndianStreams internally.

#if defined(RAD_OPT_PC_TOOLS)
	bool SaveBin(stream::OutputStream &os) const;
#endif

	bool LoadBin(stream::InputStream &is);

private:

	typedef zone_map<int, Dialog*, ZAssetsT>::type UIDMap;
	typedef zone_map<String, Dialog*, ZAssetsT>::type StringMap;

	void PushRoot(lua_State *L, const Root &root) const;
	void PushDialog(lua_State *L, const Dialog &dialog) const;
	void PushStringOption(lua_State *L, const StringOption &opt) const;

#if defined(RAD_OPT_PC_TOOLS)
	bool SaveBinRoot(const Root &root, stream::OutputStream &os) const;
	bool SaveBinDialog(const Dialog &dialog, stream::OutputStream &os) const;
	bool SaveBinStringOption(const StringOption &opt, stream::OutputStream &os) const;
#endif

	Root *LoadBinRoot(stream::InputStream &is);
	bool LoadBinDialog(Dialog &dialog, stream::InputStream &is);
	bool LoadBinStringOption(StringOption &opt, stream::InputStream &is);

	RAD_DECLARE_GET(roots, Root::PtrVec*) {
		return &m_roots;
	}

	ObjectPool<Dialog> m_dialogPool;
	ObjectPool<Root> m_rootPool;
	mutable Root::PtrVec m_roots;
	UIDMap m_dialogs;
	StringMap m_names;
	int m_nextUID;
};

} // asset

#include <Runtime/PopPack.h>
