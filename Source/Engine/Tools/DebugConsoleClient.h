/*! \file DebugConsoleClient.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "DebugConsoleCommon.h"
#include <Runtime/Stream.h>
#include <Runtime/Net/Socket.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Thread.h>
#include <Runtime/TimeDef.h>
#include <Runtime/PushPack.h>

namespace tools {

class DebugConsoleClient;
class DebugConsoleServerId {
public:
	typedef zone_vector<DebugConsoleServerId, ZToolsT>::type Vec;

	static bool VecEquals(const Vec &v1, const Vec& v2);
	static bool Contains(const DebugConsoleServerId &id, const Vec &v);
	static DebugConsoleServerId *Find(const DebugConsoleServerId &id, Vec &v);

	DebugConsoleServerId() : m_id(-1) {
		m_ip.s_addr = 0;
	}

	DebugConsoleServerId(const DebugConsoleServerId &id);

	RAD_DECLARE_READONLY_PROPERTY(DebugConsoleServerId, description, const char*);
	RAD_DECLARE_READONLY_PROPERTY(DebugConsoleServerId, id, int);
	RAD_DECLARE_READONLY_PROPERTY(DebugConsoleServerId, addr, const in_addr*);

	DebugConsoleServerId &operator = (const DebugConsoleServerId &id);

	// does not compare description
	bool operator == (const DebugConsoleServerId &other) const;

private:

	friend class DebugConsoleClient;

	RAD_DECLARE_GET(description, const char*) {
		return m_description.c_str;
	}

	RAD_DECLARE_GET(id, int) {
		return m_id;
	}

	RAD_DECLARE_GET(addr, const in_addr*) {
		return &m_ip;
	}

	String m_description;
	in_addr m_ip;
	int m_id;
	xtime::TimeVal m_expiry;
};

///////////////////////////////////////////////////////////////////////////////

//! A debug client connects to a debug server and can send commands.
class DebugConsoleClient : public boost::noncopyable {
public:
	typedef boost::shared_ptr<DebugConsoleClient> Ref;
	typedef zone_vector<String, ZToolsT>::type StringVec;

	~DebugConsoleClient();

	template <typename T>
	static Ref Connect(const DebugConsoleServerId &id);

	static void StaticStart();
	static void StaticStop();
	static void ProcessMessages();
	static const DebugConsoleServerId::Vec &ServerList();

	bool Exec(const char *cmd);

	StringVec GetCVarList();

protected:

	DebugConsoleClient(
		const net::Socket::Ref &socket, 
		const DebugConsoleServerId &id
	);

	virtual void HandleLogMessage(const String &msg) = 0;

private:

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;
	typedef zone_set<DebugConsoleClient*, ZToolsT>::type ClientSet;

	static void ExpireServers();
	static int ReadBroadcastPacket(void *buf, int maxLen, sockaddr_in &addr);
	static void HandleBroadcast(stream::InputStream &is, const sockaddr_in &addr);
	static void HandleLogMessage(stream::InputStream &is, const sockaddr_in &addr);
	static int ConnectClient(const DebugConsoleServerId &id);

	static net::Socket::Ref s_broadcast;
	static ClientSet s_clients;
	static DebugConsoleServerId::Vec s_servers;

	DebugConsoleServerId m_id;
	net::Socket::Ref m_sd;
};

} // tools

#include <Runtime/PopPack.h>
#include "DebugConsoleClient.inl"