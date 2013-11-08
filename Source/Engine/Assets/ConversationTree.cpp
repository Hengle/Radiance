/*! \file ConversationTree.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup game
*/

#include RADPCH
#include "ConversationTree.h"
#include "../Lua/LuaRuntime.h"
#include <Runtime/Stream/Stream.h>
#include <Runtime/EndianStream.h>

namespace asset {

namespace {
	enum {
		kBinTag = RAD_FOURCC_LE('C', 'O', 'N', 'V'),
		kBinVer1 = 1,
		kBinVer = 2,
	};
}

ConversationTree::ConversationTree() {
	m_dialogPool.Create(
		ZWorld,
		"conver_dialog",
		8
	);

	m_rootPool.Create(
		ZWorld,
		"conver_roots",
		8
	);

	m_nextUID = 0;
}

ConversationTree::~ConversationTree() {
	for(Root::PtrVec::iterator it = m_roots->begin(); it != m_roots->end(); ++it) {
		DeleteRoot(**it);
	}

	while (!m_dialogs.empty()) {
		DeleteDialog(*m_dialogs.begin()->second);
	}

	RAD_ASSERT(m_dialogPool.numUsedObjects == 0);
	RAD_ASSERT(m_rootPool.numUsedObjects == 0);
}

ConversationTree::Root *ConversationTree::NewRoot() {
	return m_rootPool.SafeConstruct();
}

void ConversationTree::DeleteRoot(Root &root) {
	for (Dialog::PtrVec::iterator it = root.dialog->begin(); it != root.dialog->end(); ++it) {
		UnrefDialog(**it);
	}
	m_rootPool.Destroy(&root);
}

ConversationTree::Dialog *ConversationTree::NewDialog() {
	Dialog *d = m_dialogPool.SafeConstruct();
	d->uid = m_nextUID++;
	m_dialogs[d->uid] = d;
	return d;
}

void ConversationTree::DeleteDialog(Dialog &dialog) {

	for (Dialog::PtrVec::iterator it = dialog.choices->begin(); it != dialog.choices->end(); ++it) {
		UnrefDialog(**it);
	}

	m_dialogs.erase(dialog.uid);
	m_dialogPool.Destroy(&dialog);
}

void ConversationTree::RefDialog(Dialog &dialog) {
	++dialog.refs;
}

void ConversationTree::UnrefDialog(Dialog &dialog) {
	RAD_ASSERT(dialog.refs > 0);
	--dialog.refs;
}

const ConversationTree::Dialog *ConversationTree::DialogForUID(int uid) const {
	Dialog::UIDMap::const_iterator it = m_dialogs.find(uid);
	if (it != m_dialogs.end())
		return it->second;
	return 0;
}

ConversationTree::Dialog *ConversationTree::DialogForUID(int uid) {
	Dialog::UIDMap::iterator it = m_dialogs.find(uid);
	if (it != m_dialogs.end())
		return it->second;
	return 0;
}

int ConversationTree::PushCopy(lua_State *L) const {
	if (m_roots->empty())
		return 0;

	lua_createtable(L, 0, 2); // outer table that is returned

	// push empty dialog table that can be linked up
	lua_createtable(L, (int)m_dialogs.size(), 0);

	for (Dialog::UIDMap::const_iterator it = m_dialogs.begin(); it != m_dialogs.end(); ++it) {
		const Dialog &dialog = *it->second;
		lua_pushinteger(L, dialog.uid);

		int numTableElems = 5;
		if (!dialog.action.empty)
			++numTableElems;
		if (!dialog.group.empty)
			++numTableElems;
		if (dialog.probability != 1.f)
			++numTableElems;

		lua_createtable(L, 0, numTableElems);
		lua_settable(L, -3);
	}

	int dialogTable = lua_gettop(L);

	for (Dialog::UIDMap::const_iterator it = m_dialogs.begin(); it != m_dialogs.end(); ++it) {
		const Dialog &dialog = *it->second;
		PushDialog(L, dialog, dialogTable);
		RAD_ASSERT(dialogTable == lua_gettop(L));
	}

	lua_createtable(L, 0, m_roots->size());
	for (size_t i = 0; i < m_roots->size(); ++i) {
		const Root &root = *m_roots[i];
		if (root.name.empty)
			continue;
		if (root.prompts->empty())
			continue;
		if (root.dialog->empty()) {
			if (!(root.flags&kChatFlag_AutoGenerate))
				continue;
		}
		
		lua_pushstring(L, root.name.c_str);
		PushRoot(L, root, dialogTable);
		lua_settable(L, -3);
	}
	lua_setfield(L, -3, "roots");

	RAD_ASSERT(dialogTable == lua_gettop(L));
	lua_setfield(L, -2, "dialogs");

	return 1;
}

void ConversationTree::PushRoot(lua_State *L, const Root &root, int dialogTable) const {

	lua_createtable(L, 0, 7);
	
	lua_pushstring(L, root.name.c_str);
	lua_setfield(L, -2, "name");

	// reply = {{prob = {0, 1}, "REPLY"}}
	lua_createtable(L, (int)root.prompts->size(), 0);

	for (size_t i = 0; i < root.prompts->size(); ++i) {
		const StringOption &prompt = root.prompts[i];
		if (prompt.text.empty)
			continue;
		
		lua_pushinteger(L, (int)(i+1));
		PushStringOption(L, prompt);
		lua_settable(L, -3);
	}

	lua_setfield(L, -2, "reply");

	// choices = {{prob = 0, {...}}}

	lua_createtable(L, (int)root.dialog->size(), 0);

	for (size_t i = 0; i < root.dialog->size(); ++i) {
		const Dialog &dialog = *root.dialog[i];
		lua_pushinteger(L, (int)(i+1));
		lua_pushinteger(L, dialog.uid);
		lua_gettable(L, dialogTable);
		lua_settable(L, -3);
	}

	lua_setfield(L, -2, "choices");

	if ((root.probability[0] != 1.f) ||
		(root.probability[1] != 1.f)) {
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, 1);
		lua_pushnumber(L, root.probability[0]);
		lua_settable(L, -3);
		lua_pushinteger(L, 2);
		lua_pushnumber(L, root.probability[1]);
		lua_settable(L, -3);

		lua_setfield(L, -2, "prob");
	}

	if (!root.group.empty) {
		lua_pushstring(L, root.group.c_str);
		lua_setfield(L, -2, "group");
	}

	lua_pushnumber(L, root.priority);
	lua_setfield(L, -2, "priority");

	lua_pushinteger(L, root.flags);
	lua_setfield(L, -2, "flags");
}

void ConversationTree::PushDialog(lua_State *L, const Dialog &dialog, int dialogTable) const {

	lua_pushinteger(L, dialog.uid);
	lua_gettable(L, dialogTable);

	lua_pushstring(L, dialog.name.c_str);
	lua_setfield(L, -2, "name");

	if (!dialog.choices->empty()) {
		lua_createtable(L, (int)dialog.choices->size(), 0);

		for (size_t i = 0; i < dialog.choices->size(); ++i) {
			const Dialog &child = *dialog.choices[i];
			lua_pushinteger(L, (int)(i+1));
			lua_pushinteger(L, child.uid);
			lua_gettable(L, dialogTable);
			lua_settable(L, -3);
		}

		lua_setfield(L, -2, "choices");

	}
	// prompt = {{prob = {0, 1}, "TEXT"}}
	lua_createtable(L, (int)dialog.prompts->size(), 0);
	
	for (size_t i = 0; i < dialog.prompts->size(); ++i) {
		const StringOption &prompt = dialog.prompts[i];
		if (prompt.text.empty)
			continue;
		
		lua_pushinteger(L, (int)(i+1));
		PushStringOption(L, prompt);
		lua_settable(L, -3);
	}

	lua_setfield(L, -2, "prompt");

	// reply = {{prob = {0, 1}, "REPLY"}
	lua_createtable(L, (int)dialog.replies->size(), 0);

	for (size_t i = 0; i < dialog.replies->size(); ++i) {
		const StringOption &prompt = dialog.replies[i];
		if (prompt.text.empty)
			continue;

		lua_pushinteger(L, (int)(i+1));
		PushStringOption(L, prompt);
		lua_settable(L, -3);
	}

	lua_setfield(L, -2, "reply");

	// action = "string"
	if (!dialog.action.empty) {
		lua_pushstring(L, dialog.action.c_str);
		lua_setfield(L, -2, "action");
	}

	// group = "string"
	if (!dialog.group.empty) {
		lua_pushstring(L, dialog.group.c_str);
		lua_setfield(L, -2, "group");
	}

	if (dialog.probability != 1.f) {
		lua_pushnumber(L, dialog.probability);
		lua_setfield(L, -2, "prob");
	}

	lua_pushinteger(L, dialog.flags);
	lua_setfield(L, -2, "flags");

	lua_pop(L, 1);
}

void ConversationTree::PushStringOption(lua_State *L, const StringOption &opt) const {
	lua_createtable(L, 1, 1);
	if (opt.probability[0] != 1.f ||
		opt.probability[1] != 1.f) {
		lua_createtable(L, 2, 0);
		lua_pushinteger(L, 1);
		lua_pushnumber(L, opt.probability[0]);
		lua_settable(L, -3);
		lua_pushinteger(L, 2);
		lua_pushnumber(L, opt.probability[1]);
		lua_settable(L, -3);
		lua_setfield(L, -2, "prob");
	}
	lua_pushinteger(L, 1);
	lua_pushstring(L, opt.text.c_str);
	lua_settable(L, -3);
}

#if defined(RAD_OPT_PC_TOOLS)
bool ConversationTree::SaveBin(stream::OutputStream &_os) const {
	stream::LittleOutputStream os(_os.Buffer());

	if (!(os.Write((U32)kBinTag) && os.Write((U32)kBinVer)))
		return false;

	if (!(os.Write((S32)m_nextUID)))
		return false;

	if (!os.Write((U32)m_dialogs.size()))
		return false;

	// keys.
	for (Dialog::UIDMap::const_iterator it = m_dialogs.begin(); it != m_dialogs.end(); ++it) {
		if (!os.Write((S32)it->first))
			return false;
	}

	for (Dialog::UIDMap::const_iterator it = m_dialogs.begin(); it != m_dialogs.end(); ++it) {
		if (!SaveBinDialog(*(it->second), os))
			return false;
	}

	if (!os.Write((U32)m_roots->size()))
		return false;

	for (Root::PtrVec::const_iterator it = m_roots->begin(); it != m_roots->end(); ++it) {
		if (!SaveBinRoot(**it, os))
			return false;
	}

	return true;
}

bool ConversationTree::SaveBinRoot(const Root &root, stream::OutputStream &os) const {
	if (!os.Write(root.name))
		return false;
	if (!os.Write(root.group))
		return false;
	if (!(os.Write(root.probability[0]) && os.Write(root.probability[1])))
		return false;
	if (!os.Write(root.flags))
		return false;
	if (!os.Write(root.priority))
		return false;

	if (!os.Write((U32)root.prompts->size()))
		return false;
	
	for (StringOption::Vec::const_iterator it = root.prompts->begin(); it != root.prompts->end(); ++it) {
		if (!SaveBinStringOption(*it, os))
			return false;
	}

	if (!os.Write((U32)root.dialog->size()))
		return false;

	for (Dialog::PtrVec::const_iterator it = root.dialog->begin(); it != root.dialog->end(); ++it) {
		if (!os.Write((S32)(*it)->uid))
			return false;
	}

	return true;
}

bool ConversationTree::SaveBinDialog(const Dialog &dialog, stream::OutputStream &os) const {
	if (!os.Write((S32)dialog.uid))
		return false;
	if (!os.Write(dialog.name))
		return false;
	if (!os.Write(dialog.action))
		return false;
	if (!os.Write(dialog.group))
		return false;
	if (!os.Write(dialog.probability))
		return false;
	if (!os.Write((S32)dialog.flags))
		return false;
	
	if (!os.Write((U32)dialog.prompts->size()))
		return false;

	for (StringOption::Vec::const_iterator it = dialog.prompts->begin(); it != dialog.prompts->end(); ++it) {
		if (!SaveBinStringOption(*it, os))
			return false;
	}

	if (!os.Write((U32)dialog.replies->size()))
		return false;

	for (StringOption::Vec::const_iterator it = dialog.replies->begin(); it != dialog.replies->end(); ++it) {
		if (!SaveBinStringOption(*it, os))
			return false;
	}

	if (!os.Write((U32)dialog.choices->size()))
		return false;

	for (Dialog::PtrVec::const_iterator it = dialog.choices->begin(); it != dialog.choices->end(); ++it) {
		if (!os.Write((S32)(*it)->uid))
			return false;
	}

	return true;
}

bool ConversationTree::SaveBinStringOption(const StringOption &opt, stream::OutputStream &os) const {
	if (!os.Write(opt.probability[0]))
		return false;
	if (!os.Write(opt.probability[1]))
		return false;
	if (!os.Write(opt.text))
		return false;
	return true;
}

#endif

bool ConversationTree::LoadBin(stream::InputStream &_is) {
	stream::LittleInputStream is(_is.Buffer());

	U32 tag, ver;
	if (!(is.Read(&tag) && is.Read(&ver)))
		return false;
	if (tag != kBinTag || ver > kBinVer)
		return false;

	S32 nextUID;
	if (!is.Read(&nextUID))
		return false;
	m_nextUID = (int)nextUID;

	S32 numUIDs;
	if (!is.Read(&numUIDs))
		return false;

	for (S32 i = 0; i < numUIDs; ++i) {
		S32 uid;
		if (!is.Read(&uid))
			return false;
		Dialog *d = m_dialogPool.SafeConstruct();
		d->uid = (int)uid;
		m_dialogs[d->uid] = d;
	}

	for (S32 i = 0; i < numUIDs; ++i) {
		S32 uid;
		if (!is.Read(&uid))
			return false;
		Dialog *d = m_dialogs[uid];
		RAD_ASSERT(d);
		if (!LoadBinDialog(*d, is, ver))
			return false;
	}

	U32 numRoots;
	if (!is.Read(&numRoots))
		return false;

	for (U32 i = 0; i < numRoots; ++i) {
		Root *r = LoadBinRoot(is, ver);
		if (!r)
			return false;
		m_roots->push_back(r);
	}

	return true;
}

ConversationTree::Root *ConversationTree::LoadBinRoot(stream::InputStream &is, int version) {
	Root *r = NewRoot();
	if (!is.Read(&r->name)) {
		DeleteRoot(*r);
		return 0;
	}

	if (!is.Read(&r->group)) {
		DeleteRoot(*r);
		return 0;
	}

	if (!(is.Read(&r->probability[0]) && is.Read(&r->probability[1]))) {
		DeleteRoot(*r);
		return 0;
	}

	if (!is.Read(&r->flags)) {
		DeleteRoot(*r);
		return 0;
	}

	if (version > kBinVer1) {
		if (!is.Read(&r->priority)) {
			DeleteRoot(*r);
			return 0;
		}
	}

	U32 count;
	if (!is.Read(&count)) {
		DeleteRoot(*r);
		return 0;
	}

	for (U32 i = 0; i < count; ++i) {
		StringOption x;
		if (!LoadBinStringOption(x, is, version)) {
			DeleteRoot(*r);
			return 0;
		}
		r->prompts->push_back(x);
	}

	if (!is.Read(&count)) {
		DeleteRoot(*r);
		return 0;
	}

	for (U32 i = 0; i < count; ++i) {
		S32 uid;
		if (!is.Read(&uid))
			return false;
		Dialog *d = m_dialogs[uid];
		RAD_ASSERT(d);
		RefDialog(*d);
		r->dialog->push_back(d);
	}
	
	return r;
}

bool ConversationTree::LoadBinDialog(Dialog &dialog, stream::InputStream &is, int version) {
	// UID read by LoadBin function

	if (!is.Read(&dialog.name))
		return false;

	if (!is.Read(&dialog.action)) {
		return false;
	}

	if (!is.Read(&dialog.group)) {
		return false;
	}

	if (!is.Read(&dialog.probability)) {
		return false;
	}

	if (version > kBinVer1) {
		S32 n;
		if (!is.Read(&n)) {
			return false;
		}
		dialog.flags = n;
	}

	U32 count;
	if (!is.Read(&count)) {
		return false;
	}

	for (U32 i = 0; i < count; ++i) {
		StringOption x;
		if (!LoadBinStringOption(x, is, version)) {
			return false;
		}
		dialog.prompts->push_back(x);
	}

	if (!is.Read(&count)) {
		return false;
	}

	for (U32 i = 0; i < count; ++i) {
		StringOption x;
		if (!LoadBinStringOption(x, is, version)) {
			return false;
		}
		dialog.replies->push_back(x);
	}

	if (!is.Read(&count)) {
		return false;
	}

	for (U32 i = 0; i < count; ++i) {
		S32 uid;
		if (!is.Read(&uid))
			return false;
		Dialog *d = m_dialogs[uid];
		RAD_ASSERT(d);
		RefDialog(*d);
		dialog.choices->push_back(d);
	}

	return true;
}

bool ConversationTree::LoadBinStringOption(StringOption &opt, stream::InputStream &is, int version) {
	if (!is.Read(&opt.probability[0]))
		return false;
	if (!is.Read(&opt.probability[1]))
		return false;
	if (!is.Read(&opt.text))
		return false;
	return true;
}

} // asset
