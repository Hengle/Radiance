/*! \file DebugConsoleServer.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once
#include "DebugConsoleCommon.h"
#include <Runtime/Stream.h>
#include <Runtime/Net/Socket.h>
#include <Runtime/Thread.h>
#include <Runtime/Container/ZoneSet.h>
#include <Runtime/Container/ZoneMap.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

class CVarZone;

namespace tools {

//! DebugConsoleServer allows RPC into the Console.
class DebugConsoleServer : public boost::noncopyable {
public:
	typedef boost::shared_ptr<DebugConsoleServer> Ref;

	~DebugConsoleServer();

	static Ref Start(const char *description, CVarZone *cvars);
	static void BroadcastLogMessage(const char *msg);

	void ProcessClients();
	void SetDescription(const char *description);

	RAD_DECLARE_READONLY_PROPERTY(DebugConsoleServer, sessionId, int);
	
private:

	typedef zone_map<int, DebugConsoleServer*, ZToolsT>::type Map;

	typedef boost::recursive_mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	struct Client {
		typedef boost::shared_ptr<Client> Ref;
		typedef zone_vector<Ref, ZToolsT>::type Vec;

		Client(const net::Socket::Ref &_sd, const in_addr _addr) : sd(_sd), addr(_addr) {}

		net::Socket::Ref sd;
		in_addr addr;
	};

	friend class SessionServer;

	//////////////////////////////////////////////////////////////////////////////

	class SessionServer : public thread::Thread {
	public:
		SessionServer();
		~SessionServer();

		bool Register(DebugConsoleServer *sv);
		void Unregister(DebugConsoleServer *sv);
		void BroadcastLogMessage(const char *msg);

	private:
		
		bool StartListening();
		void Accept();
		void DoBroadcast();
		void SendLogMessages();
		void ConnectClient(int sd, const in_addr &addr);
		
		virtual int ThreadProc();

		Map m_servers;
		Mutex m_m;
		net::Socket::Ref m_listen;
		net::Socket::Ref m_broadcast;
		int m_nextSessionId;
		volatile int m_numActiveServers;
	};

	RAD_DECLARE_GET(sessionId, int) {
		return m_sessionId;
	}

	DebugConsoleServer(const char *description, CVarZone *cvars);
	void Stop();
	void DisconnectClients();
	void ProcessClientCmds();
	int ProcessClient(const Client &client);
	int HandleClientCmd(const Client &client);
	void Register(const Client::Ref &client);
	
	int NetMsg_Cmd(const Client &client, stream::InputStream &is);
	int NetMsg_GetCVarList(const Client &client);
	
	int m_sessionId;
	CVarZone *m_cvars;
	Client::Vec m_clients;
	String m_description;
	Mutex m_m;

	static SessionServer s_ss;
};

} // tools

#include <Runtime/PopPack.h>
