/*! \file DebugConsoleClient.inl
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

namespace tools {

template <typename T>
inline DebugConsoleClient::Ref DebugConsoleClient::Connect(const DebugConsoleServerId &id) {
	Ref r;
	int sd = ConnectClient(id);
	if (sd < 0)
		return r;

	net::Socket::Ref _sd(new net::Socket(sd));
	r.reset(new T(_sd, id));
	return r;
}

inline const DebugConsoleServerId::Vec &DebugConsoleClient::ServerList() {
	return s_servers;
}

} // tools
