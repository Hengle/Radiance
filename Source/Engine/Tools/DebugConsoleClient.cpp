/*! \file DebugConsoleClient.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "DebugConsoleClient.h"
#include <Runtime/Time.h>

namespace tools {

///////////////////////////////////////////////////////////////////////////////

bool DebugConsoleServerId::VecEquals(const Vec &v1, const Vec &v2) {
	if (v1.size() != v2.size())
		return false;
	for (Vec::const_iterator it = v1.begin(); it != v1.end(); ++it) {
		if (!Contains(*it, v2))
			return false;
	}

	return true;
}

bool DebugConsoleServerId::Contains(const DebugConsoleServerId &id, const Vec &v) {
	for (Vec::const_iterator it = v.begin(); it != v.end(); ++it) {
		if (*it == id)
			return true;
	}
	return false;
}

DebugConsoleServerId *DebugConsoleServerId::Find(const DebugConsoleServerId &id, Vec &v) {
	for (Vec::iterator it = v.begin(); it != v.end(); ++it) {
		if (*it == id)
			return &(*it);
	}

	return 0;
}

DebugConsoleServerId::DebugConsoleServerId(const DebugConsoleServerId &id) {
	*this = id;
}

DebugConsoleServerId &DebugConsoleServerId::operator = (const DebugConsoleServerId &id) {
	m_id = id.m_id;
	m_ip = id.m_ip;
	m_name = id.m_name;
	m_description = id.m_description;
	m_expiry = id.m_expiry;
	return *this;
}

bool DebugConsoleServerId::operator == (const DebugConsoleServerId &id) const {
	return (m_id == id.m_id) && (m_ip.s_addr == id.m_ip.s_addr);
}

///////////////////////////////////////////////////////////////////////////////

net::Socket::Ref DebugConsoleClient::s_broadcast;
DebugConsoleClient::ClientSet DebugConsoleClient::s_clients;
DebugConsoleServerId::Vec DebugConsoleClient::s_servers;

DebugConsoleClient::DebugConsoleClient(const net::Socket::Ref &socket, const DebugConsoleServerId &id)
: m_sd(socket), m_id(id) {
	s_clients.insert(this);
}

DebugConsoleClient::~DebugConsoleClient() {
	s_clients.erase(this);
}

void DebugConsoleClient::ProcessMessages() {

	if (s_broadcast) {
		int len;
		U8 buf[kDebugConsoleBroadcastPacketSize];
		sockaddr_in addr;

		while((len=ReadBroadcastPacket(buf, sizeof(buf), addr)) > 0) {
			U32 cmds[3];
			if (len < sizeof(cmds))
				continue; // invalid broadcast packet.

			stream::MemInputBuffer ib(buf, len);
			stream::InputStream is(ib);

			is >> cmds[0];
			is >> cmds[1];
			is >> cmds[2];
		
			if (cmds[0] != kDebugConsoleNetServerId)
				continue;
			if (cmds[1] != kDebugConsoleNetServerVersion)
				continue;

			if ((cmds[2] != kDebugConsoleNetMessageId_Broadcast) &&
				(cmds[2] != kDebugConsoleNetMessageId_Log)) {
				continue;
			}

			switch (cmds[2]) {
			case kDebugConsoleNetMessageId_Broadcast:
				HandleBroadcast(is, addr);
				break;
			case kDebugConsoleNetMessageId_Log:
				HandleLogMessage(is, addr);
				break;
			default:
				break;
			}
		}
	}

	ExpireServers();
}

void DebugConsoleClient::ExpireServers() {
	xtime::TimeVal now = xtime::ReadMilliseconds();

	for (DebugConsoleServerId::Vec::iterator it = s_servers.begin(); it != s_servers.end();) {
		DebugConsoleServerId &id = *it;
		if (now >= id.m_expiry) {
			it = s_servers.erase(it);
		} else {
			++it;
		}
	}
}

bool DebugConsoleClient::Exec(const char *cmd) {
	RAD_ASSERT(cmd);

	if (!m_sd)
		return false;

	const String kCmd(CStr(cmd));
	char buf[kDebugConsoleBroadcastPacketSize];
	stream::FixedMemOutputBuffer ob(buf, sizeof(buf));
	stream::OutputStream os(ob);
	os.Write(kCmd);

	U32 cmds[2];
	cmds[0] = kDebugConsoleNetMessageId_Cmd;
	cmds[1] = (U32)os.OutPos();

	int z = m_sd->send((const char*)cmds, sizeof(cmds), 0);
	if (z >= 0) {
		z = m_sd->send(buf, (int)os.OutPos(), 0);
	}

	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleClient::Exec() socket send failure -> " << inet_ntoa(m_id.m_ip) << std::endl;
		m_sd.reset();
	}

	return z >= 0;
}

DebugConsoleClient::StringVec DebugConsoleClient::GetCVarList() {
	StringVec v;

	if (!m_sd)
		return v;

	U32 cmds[2];
	cmds[0] = kDebugConsoleNetMessageId_GetCVarList;
	cmds[1] = 0;

	int z = m_sd->send((const char*)cmds, sizeof(cmds), 0);
	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleClient::Exec() socket send failure -> " << inet_ntoa(m_id.m_ip) << std::endl;
		return v;
	}

	z = recv(*m_sd, (char*)&cmds[0], sizeof(U32), MSG_WAITALL);
	if (z >= 0) {
		// sanity check
		if (cmds[0] < Meg) {
			void *data = safe_zone_malloc(ZTools, cmds[0]);
			z = recv(*m_sd, (char*)data, (int)cmds[0], MSG_WAITALL);
			if (z >= 0) {
				stream::MemInputBuffer ib(data, (stream::SPos)cmds[0]);
				stream::InputStream is(ib);

				U32 count;
				is >> count;

				String s;
				while (count-- > 0) {
					if (is.Read(&s))
						v.push_back(s);
				}
			}
			zone_free(data);
		} else {
			COut(C_Error) << "ERROR: message size too big for cvarlist." << std::endl;
			return v;
		}
	}

	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleClient::Exec() socket read failure -> " << inet_ntoa(m_id.m_ip) << std::endl;
	}

	return v;
}

int DebugConsoleClient::ConnectClient(const DebugConsoleServerId &id) {
	Ref r;
	net::Socket sd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sd == -1) {
		COut(C_Error) << "ERROR: DebugConsoleClient: socket create failure." << std::endl;
		return -1;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = id.m_ip.s_addr;
	addr.sin_port = htons(kDebugConsoleNetPort);

	if (connect(sd, (const sockaddr*)&addr, sizeof(addr))) {
		COut(C_Error) << "ERROR: DebugConsoleClient: unable to connect to " << inet_ntoa(id.m_ip) << "." << std::endl;
		return -1;
	}

	U32 cmds[3];
	cmds[0] = kDebugConsoleNetServerId;
	cmds[1] = kDebugConsoleNetServerVersion;
	cmds[2] = id.m_id;

	int z = sd.send((const char*)cmds, sizeof(cmds), 0);
	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleClient: send failure." << std::endl;
		return -1;
	}

	// read ack.
	z = recv(sd, (char*)cmds, sizeof(cmds), MSG_WAITALL);
	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleClient: " << inet_ntoa(id.m_ip) << " terminated the connection." << std::endl;
		return -1;
	}

	int _sd = sd;
	sd = -1; // don't close
	return _sd;
}

void DebugConsoleClient::StaticStart() {
	net::Socket sd = (int)socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sd == -1) {
		COut(C_Error) << "ERROR: DebugConsoleClient: broadcast socket failure." << std::endl;
		return;
	}

	int opt = 1;

	if (setsockopt(sd, SOL_SOCKET, SO_REUSEADDR, (const char *)&opt, sizeof(opt))) {
		COut(C_Error) << "ERROR: DebugConsoleClient: broadcast SO_REUSEADDR failure." << std::endl;
		return;
	}

	sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = net::GetLocalIP().s_addr;
#if defined(RAD_OPT_OSX) // mac needs to bind to the broadcast address
	addr.sin_addr.s_addr |= (0xffu << 24U); // broadcast
#endif
	addr.sin_port = htons(kDebugConsoleNetBroadcastPort);

	if (bind(sd, (const sockaddr*)&addr, sizeof(addr))) {
		COut(C_Error) << "ERROR: DebugConsoleClient: broadcast socket failure." << std::endl;
		return;
	}

	s_broadcast.reset(new net::Socket(sd));
	sd = -1; // don't close.
}

void DebugConsoleClient::StaticStop() {
	s_broadcast.reset();
}

int DebugConsoleClient::ReadBroadcastPacket(void *buf, int maxLen, sockaddr_in &addr) {
	fd_set fd_read;

	int sd = *s_broadcast;

	FD_ZERO(&fd_read);
	FD_SET(sd, &fd_read);

	timeval tm;
	tm.tv_sec = 0;
	tm.tv_usec = 0;

	int z = select(sd+1, &fd_read, 0, 0, &tm);
	if (z < 0) {
		COut(C_Error) << "ERROR: DebugConsoleServer: select failed on broadcast socket." << std::endl;
		return 0;
	} else if (z > 0) { // data is waiting
		socklen_t addrsize = sizeof(addr);
		z = recvfrom(sd, (char*)buf, maxLen, 0, (sockaddr*)&addr, &addrsize);
		return z;
	}

	return 0;
}

void DebugConsoleClient::HandleBroadcast(stream::InputStream &is, const sockaddr_in &addr) {
	U32 numServers;
	String serverName;

	if (!is.Read(&numServers))
		return;
	if (!is.Read(&serverName))
		return;

	int id;
	String str;

	xtime::TimeVal expiry = xtime::ReadMilliseconds() + kDebugConsoleServerExpiry;

	for (U32 i = 0; i < numServers; ++i) {
		if (!is.Read(&id))
			break;
		if (!is.Read(&str))
			break;

		DebugConsoleServerId sid;
		sid.m_ip = addr.sin_addr;
		sid.m_id = id;
		sid.m_expiry = expiry;
		sid.m_name = serverName;
		sid.m_description = str;

		DebugConsoleServerId *_sid = DebugConsoleServerId::Find(sid, s_servers);
		if (!_sid) {
			s_servers.push_back(sid);
		} else {
			if (_sid->m_description != str)
				_sid->m_description = str; // avoid lots of new/delete here.
			_sid->m_expiry = expiry;
		}
	}
}

void DebugConsoleClient::HandleLogMessage(stream::InputStream &is, const sockaddr_in &addr) {

	if (s_clients.empty())
		return;

	U32 len;
	
	if (!is.Read(&len))
		return; // bad packet.

	char buf[kDebugConsoleBroadcastPacketSize];
	if (!is.Read(buf, (stream::SPos)len, 0))
		return; // bad packet.

	// See DebugConsoleServer.cpp DebugConsoleServer::SessionServer::BroadcastLogMessage()
	// len is chars + null byte.
	// RefTag string constructor wants null byte at len + 1
	const String kMsg(buf, len - 1, string::RefTag);

	// Find any active clients with matching IPs.
	for (ClientSet::const_iterator it = s_clients.begin(); it != s_clients.end(); ++it) {
		DebugConsoleClient *client = *it;
		if (client->m_id.m_ip.s_addr == addr.sin_addr.s_addr) {
			client->HandleLogMessage(kMsg);
		}
	}
}

} // tools
