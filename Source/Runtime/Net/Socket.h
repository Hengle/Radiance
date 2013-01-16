/*! \file Socket.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "../Base.h"

#if defined(RAD_OPT_WIN)
#include "../Win/WinHeaders.h"
#else
#include <sys/socket.h>
#endif

#include "../PushPack.h"

namespace net {

const in_addr &GetLocalIP();
const char *GetHostName();

//! Cleans up a socket descriptor when destructed.

class Socket {
public:
	typedef boost::shared_ptr<Socket> Ref;

	Socket() : m_sd(-1) {
	}

	Socket(const Socket &s) : m_sd(s.m_sd) {
	}

	Socket(int sd) : m_sd(sd) {
		RAD_ASSERT(sd != -1);
	}
	
	~Socket() {
		if (m_sd != -1) {
			closesocket(m_sd);
		}
	}

	operator int () const { 
		return m_sd; 
	}

	Socket &operator = (int sd) {
		m_sd = sd;
		return *this;
	}

	int get() const {
		return m_sd;
	}

	//! Writes all of the data specified by len.
	/*! \returns len on success or a socket send error code (< 0). */ 
	int send(const char *buf, int len, int flags) {
		return sendto(buf, len, flags, 0, 0);
	}

	//! Writes all of the data specified by len.
	/*! \returns len on success or a socket send error code (< 0). */ 
	int sendto(const char *buf, int len, int flags, const struct sockaddr *to, int tolen);

private:

	int m_sd;
};
};

#include "../PopPack.h"
