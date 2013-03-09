/*! \file DebugConsoleServer.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "DebugConsoleServer.h"
#include "../Console.h"
#include "../CVars.h"
#include "../COut.h"
#include <Runtime/Time.h>

namespace tools {

///////////////////////////////////////////////////////////////////////////////

DebugConsoleServer::SessionServer DebugConsoleServer::s_ss;

DebugConsoleServer::DebugConsoleServer(const char *description, CVarZone *cvars) : m_sessionId(-1), m_description(description), m_cvars(cvars) {
}

DebugConsoleServer::~DebugConsoleServer() {
	s_ss.Unregister(this);
}

DebugConsoleServer::Ref DebugConsoleServer::Start(const char *description, CVarZone *cvars) {
	DebugConsoleServer::Ref r(new DebugConsoleServer(description, cvars));
	if (!s_ss.Register(r.get()))
		r.reset();
	return r;
}

void DebugConsoleServer::Stop() {
	DisconnectClients();
}

void DebugConsoleServer::BroadcastLogMessage(const char *msg) {
	s_ss.BroadcastLogMessage(msg);
}

void DebugConsoleServer::DisconnectClients() {
	Lock L(m_m);
	m_clients.clear();
}

void DebugConsoleServer::ProcessClients() {
	ProcessClientCmds();
}

void DebugConsoleServer::SetDescription(const char *description) {
	Lock L(m_m);
	m_description = description;
}

void DebugConsoleServer::ProcessClientCmds() {
	Lock L(m_m);

	bool cmds = true;

	while (cmds) {
		cmds = false;
		for (Client::Vec::iterator it = m_clients.begin(); it != m_clients.end();) {
			int z = ProcessClient(*(*it));

			if (z >= 0) {
				cmds = cmds || (z > 0);
				++it;
			} else { // z < 0 (error)
				COut(C_Error) << "DebugConsoleServer: client " << inet_ntoa((*it)->addr) << " disconnected due to error." << std::endl;
				it = m_clients.erase(it);
			}
		}
	}
}

int DebugConsoleServer::ProcessClient(const Client &client) {

	fd_set fd_read;

	int sd = *client.sd;

	FD_ZERO(&fd_read);
	FD_SET(sd, &fd_read);

	timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = 0;

	int z = select(sd+1, &fd_read, 0, 0, &tm);
	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleServer: client " << inet_ntoa(client.addr) << " connection reset." << std::endl;
		return -1;
	} else if (z > 0) { // data waiting
		z = HandleClientCmd(client);
		if (z < 0)
			return z;
		return 1;
	}

	return 0;
}

int DebugConsoleServer::HandleClientCmd(const Client &client) {
	U32 cmds[2];
	if (recv(*client.sd, (char*)cmds, sizeof(cmds), MSG_WAITALL) != sizeof(cmds)) {
		COut(C_Error) << "ERROR: DebugConsoleServer: client " << inet_ntoa(client.addr) << " socket read error." << std::endl;
		return -1;
	}

	if (cmds[1] > kDebugConsoleNetMaxCommandLen) {
		COut(C_Error) << "ERROR: DebugConsoleServer: invalid message size from " << inet_ntoa(client.addr) << "." << std::endl;
		return -1;
	}

	U8 msgBuf[4*kKilo];
	U8 *dmsgBuf = msgBuf;

	if (cmds[1] > 4*kKilo) {
		dmsgBuf = (U8*)safe_zone_malloc(ZTools, cmds[1]);
	}

	if (cmds[1] > 0) {
		if (recv(*client.sd, (char*)dmsgBuf, cmds[1], MSG_WAITALL) != cmds[1]) {
			COut(C_Error) << "ERROR: DebugConsoleServer: client " << inet_ntoa(client.addr) << " socket read error." << std::endl;
			return -1;
		}
	}

	stream::MemInputBuffer ib(dmsgBuf, (stream::SPos)cmds[1]);
	stream::InputStream is(ib);

	int z;
	bool handled = true;

	switch (cmds[0]) {
	case kDebugConsoleNetMessageId_Cmd:
		z = NetMsg_Cmd(client, is);
		break;
	default:
		handled = false;
		break;
	}

	if (dmsgBuf != msgBuf)
		zone_free(dmsgBuf);

	if (!handled) {
		COut(C_Error) << "ERROR: DebugConsoleServer: invalid client command sent from " << inet_ntoa(client.addr) << "." << std::endl;
		z = -1;
	}

	return z;
}

int DebugConsoleServer::NetMsg_Cmd(const Client &client, stream::InputStream &is) {
	String str;
	if (!is.Read(&str)) {
		COut(C_Error) << "ERROR: DebugConsoleServer: client " << inet_ntoa(client.addr) << " bad packet (-1)." << std::endl;
		return -1;
	}

	Console::Exec(str.c_str, m_cvars);

	return 0;
}

int DebugConsoleServer::NetMsg_GetCVarList(const Client &client) {
	stream::DynamicMemOutputBuffer ob(ZTools);
	stream::OutputStream os(ob);

	// how many are there going to be?
	U32 count = (U32)CVarZone::Globals().cvars->size();
	if (m_cvars)
		count += (U32)m_cvars->cvars->size();

	os << count;

	if (m_cvars) {
		for (CVarMap::const_iterator it = m_cvars->cvars->begin(); it != m_cvars->cvars->end(); ++it) {
			os.Write(CStr(it->second->name.get()));
		}
	}

	for (CVarMap::const_iterator it = CVarZone::Globals().cvars->begin(); it != CVarZone::Globals().cvars->end(); ++it) {
		os.Write(CStr(it->second->name.get()));
	}

	U32 msgSize = (U32)os.OutPos();
	int z = client.sd->send((const char*)&msgSize, sizeof(msgSize), 0);
	if (z >= 0)
		z = client.sd->send((const char *)ob.OutputBuffer().Ptr(), (int)msgSize, 0);
	return z;
}

void DebugConsoleServer::Register(const Client::Ref &client) {
	Lock L(m_m);
	m_clients.push_back(client);
}

///////////////////////////////////////////////////////////////////////////////

DebugConsoleServer::SessionServer::SessionServer() : m_nextSessionId(0), m_numActiveServers(0) {
}

DebugConsoleServer::SessionServer::~SessionServer() {
	Join();
}

bool DebugConsoleServer::SessionServer::Register(DebugConsoleServer *sv) {
	Lock L(m_m);
	sv->m_sessionId = m_nextSessionId++;

	bool start = m_servers.empty();
	m_servers.insert(Map::value_type(sv->m_sessionId, sv));

	if (start) {
		if (!StartListening()) {
			m_servers.clear();
			return false;
		}
	}

	++m_numActiveServers;
	DoBroadcast(); // update server list immediately.

	return true;
}

void DebugConsoleServer::SessionServer::Unregister(DebugConsoleServer *sv) {
	Lock L(m_m);
	if (m_servers.empty())
		return;
	RAD_ASSERT(m_numActiveServers > 0);
	--m_numActiveServers;
	m_servers.erase(sv->m_sessionId);
	DoBroadcast(); // update server list immediately.
}

void DebugConsoleServer::SessionServer::BroadcastLogMessage(const char *_msg) {
	const String msg(CStr(_msg));

	Lock L(m_m);
	if (!m_broadcast)
		return;

	U8 buf[kDebugConsoleBroadcastPacketSize];

	const char kNull = 0;
	const char *ofs = _msg;
	int bytesLeft = msg.length;

	while (bytesLeft > 0) {
		stream::FixedMemOutputBuffer ob(buf, sizeof(buf));
		stream::OutputStream os(ob);

		int bytesToStream = bytesLeft + 1; // always null terminated
		if (bytesToStream > (sizeof(buf) - 16))
			bytesToStream = sizeof(buf) - 16;

		RAD_ASSERT(bytesToStream);
		
		os << (U32)kDebugConsoleNetServerId;
		os << (U32)kDebugConsoleNetServerVersion;
		os << (U32)kDebugConsoleNetMessageId_Log;
		os << (U32)bytesToStream;

		os.Write((const void*)ofs, bytesToStream - 1, 0);
		os.Write((const void*)&kNull, 1, 0);

		sockaddr_in addr;
		addr.sin_family = AF_INET;
		addr.sin_addr.s_addr = net::GetLocalIP().s_addr;
		addr.sin_addr.s_addr |= (0xffu<<24U); // broadcast address
		addr.sin_port = htons(kDebugConsoleNetBroadcastPort);

		if (sendto(*m_broadcast, (const char*)buf, (int)os.OutPos(), 0, (const sockaddr*)&addr, sizeof(addr)) != (int)os.OutPos()) {
			m_broadcast.reset(); // otherwise we recurse!
			COut(C_Error) << "ERROR: DebugConsoleServer: broadcast failed." << std::endl;
			break;
		}

		ofs += bytesToStream - 1;
		bytesLeft -= bytesToStream - 1;
	}
}

int DebugConsoleServer::SessionServer::ThreadProc() {

	int k = 0;

	xtime::TimeVal lastBroadcast = xtime::ReadMilliseconds() - kDebugConsoleServerBroadcastFreq;

	for (;;) {

		m_m.lock();
		if (m_servers.empty())
			break;
		
		xtime::TimeVal now = xtime::ReadMilliseconds();

		if (now >= (lastBroadcast+kDebugConsoleServerBroadcastFreq)) {
			lastBroadcast = now;
			DoBroadcast();
		}

		Accept();
		m_m.unlock();
		thread::Sleep(200);
		++k;
	}

	m_listen.reset();
	m_broadcast.reset();
	m_m.unlock();

	return 0;
}

bool DebugConsoleServer::SessionServer::StartListening() {

	COut(C_Info) << "DebugConsoleServer: starting on \"" << net::GetHostName() << "\" (" << inet_ntoa(net::GetLocalIP()) << ":" << kDebugConsoleNetPort << ")..." << std::endl;

	net::Socket sd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (sd == -1) {
		COut(C_Error) << "ERROR: DebugConsoleServer: listen socket failure." << std::endl;
		return false;
	}
	
	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = net::GetLocalIP().s_addr;
	addr.sin_port = htons(kDebugConsoleNetPort);
	
	if (bind(sd, (const sockaddr*)&addr, sizeof(addr))) {
		COut(C_Error) << "ERROR: DebugConsoleServer: listen bind failure." << std::endl;
		return false;
	}

	if (listen(sd, std::min(2, SOMAXCONN))) {
		COut(C_Error) << "ERROR: DebugConsoleServer: listen failure." << std::endl;
		return false;
	}

	m_listen.reset(new net::Socket(sd));

	// make broadcast socket

	sd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == -1) {
		COut(C_Error) << "ERROR: DebugConsoleServer: broadcast socket failure." << std::endl;
		return false;
	}

	int opt = 1;

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))) {
		COut(C_Error) << "ERROR: DebugConsoleServer: broadcast SO_REUSEADDR failure." << std::endl;
		return false;
	}

	opt = 1; // just in case

	if (setsockopt(sd, SOL_SOCKET, SO_BROADCAST, (const char*)&opt, sizeof(opt))) {
		COut(C_Error) << "ERROR: DebugConsoleServer: broadcast SO_BROADCAST failure." << std::endl;
		return false;
	}

	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = net::GetLocalIP().s_addr;
	addr.sin_port = 0;

	if (bind(sd, (const sockaddr*)&addr, sizeof(addr))) {
		COut(C_Error) << "ERROR: DebugConsoleServer: broadcast bind failure." << std::endl;
		return false;
	}

	m_broadcast.reset(new net::Socket(sd));
	sd = -1; // don't close.

	Run();

	return true;
}

void DebugConsoleServer::SessionServer::Accept() {
	if (!m_listen)
		return;

	fd_set fd_read;

	int sd = *m_listen;

	FD_ZERO(&fd_read);
	FD_SET(sd, &fd_read);

	timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = 0;

	int z = select(sd + 1, &fd_read, 0, 0, &tm);

	if (z < 0) {
		m_listen.reset();
		COut(C_Error) << "ERROR: DebugConsoleServer: listen select failure." << std::endl;
	} else if (z > 0) {
		// accept is waiting.
		sockaddr_in addr;
		socklen_t len = sizeof(addr);
		sd = accept(*m_listen, (sockaddr*)&addr, &len);
		if (sd >= 0)
			ConnectClient(sd, addr.sin_addr);
	}
}

void DebugConsoleServer::SessionServer::ConnectClient(int sd, const in_addr &addr) {

	U32 cmds[3];

	int z = recv(sd, (char*)cmds, sizeof(cmds), MSG_WAITALL);
	if (z != sizeof(cmds)) {
		closesocket(sd);
		COut(C_Info) << "DebugConsoleServer: " << inet_ntoa(addr) << " client disconnected (-1)." << std::endl;
		return;
	}

	if (cmds[0] != kDebugConsoleNetServerId ||
		cmds[1] != kDebugConsoleNetServerVersion) {
		closesocket(sd);
		COut(C_Info) << "DebugConsoleServer: " << inet_ntoa(addr) << " client disconnected (-2)." << std::endl;
		return;
	}

	Map::const_iterator it = m_servers.find((int)cmds[2]);
	if (it == m_servers.end()) {
		closesocket(sd);
		COut(C_Info) << "DebugConsoleServer: " << inet_ntoa(addr) << " client disconnected (-3)." << std::endl;
		return;
	}

	DebugConsoleServer *server = it->second;
	net::Socket::Ref _sd(new net::Socket(sd));

	// ack.
	if (_sd->send((const char*)cmds, sizeof(cmds), 0) < 0) {
		closesocket(sd);
		COut(C_Info) << "DebugConsoleServer: " << inet_ntoa(addr) << " client disconnected (-4)." << std::endl;
		return;
	}

	Client::Ref client(new Client(_sd, addr));
	server->Register(client);
}

void DebugConsoleServer::SessionServer::DoBroadcast() {
	if (!m_broadcast)
		return;
		
	// build broadcast packet.
	U8 buf[kDebugConsoleBroadcastPacketSize];
	stream::FixedMemOutputBuffer ob(buf, sizeof(buf));
	stream::OutputStream os(ob);

	os << (U32)kDebugConsoleNetServerId;
	os << (U32)kDebugConsoleNetServerVersion;
	os << (U32)kDebugConsoleNetMessageId_Broadcast;
	os << (U32)m_servers.size();
	os << CStr(net::GetHostName());

	for (Map::const_iterator it = m_servers.begin(); it != m_servers.end(); ++it) {
		os << (U32)(it->second->m_sessionId);
		os.Write(it->second->m_description, 0);
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = net::GetLocalIP().s_addr;
	addr.sin_addr.s_addr |= (0xffu<<24U); // broadcast address
	addr.sin_port = htons(kDebugConsoleNetBroadcastPort);
	
	if (sendto(*m_broadcast, (const char*)buf, (int)os.OutPos(), 0, (const sockaddr*)&addr, sizeof(addr)) != (int)os.OutPos()) {
		COut(C_Error) << "ERROR: DebugConsoleServer: broadcast failed." << std::endl;
	}
}

} // tools
