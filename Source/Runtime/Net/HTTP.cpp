/*! \file HTTP.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#include RADPCH
#include "HTTP.h"

namespace net {

HTTPResponse::HTTPResponse(Zone &zone)
: m_zone(&zone), m_body(0), m_size(0), m_responseCode(-1) {
}

HTTPResponse::~HTTPResponse() {
	if (m_body)
		zone_free(m_body);
}

int HTTPResponse::ReadLine(Socket &sd, String &line) {
	line.Clear();

	int n = 0;
	char c;

	do {
		int r = recv(sd, &c, 1, 0);
		if (r == 0)
			return n; // no more data.
		if (r < 0)
			return r;

		++n;
		line += c;
	} while (c != '\n');

	return n;
}

HTTP_OpStatus HTTPResponse::Read(Socket &sd) {
	
	if (m_body)
		zone_free(m_body);

	m_header.Clear();
	m_size = 0;
	m_contentType.Clear();
	m_responseCode = -1;

	String line;
	bool eoh = false;
	bool first = true;
	bool gotSize = false;

	for (;;) {
		int r = ReadLine(sd, line);
		if (r < 0)
			return kHTTP_OpStatus_SocketError;
		if (r == 0)
			break;

		RAD_ASSERT(!line.empty);
		m_header += line;

		if (line == "\r\n") {
			eoh = true;
			break;
		}

		if (first) {
			r = line.StrStr("HTTP/1.1");
			if (r != 0) {
				return kHTTP_OpStatus_ResponseError;
			}

			sscanf(line.c_str, "HTTP/1.1 %d ", &m_responseCode);
			first = false;

		} else {
			// parse fields we are interested in.

			if (m_contentType.empty) {
				r = line.StrStr("Content-Type:");
				if (r == 0) {
					char sz[kKilo];
					sscanf(line.c_str, "Content-Type: %s\r\n", sz);
					m_contentType = sz;
				}
			}

			if (m_size == 0) {
				r = line.StrStr("Content-Length:");
				if (r == 0) {
					int size;
					sscanf(line.c_str, "Content-Length: %d\r\n", &size);
					m_size = (AddrSize)size;
					gotSize = true;
				}
			}
		}
	}

	if ((m_responseCode == -1) ||
		(m_contentType.empty) || 
		(!gotSize)) {
			return kHTTP_OpStatus_ResponseError;
	}

	if (m_size > 0) {
		m_body = safe_zone_malloc(*m_zone, m_size);
		if ((int)m_size != recv(sd, (char*)m_body, (int)m_size, 0)) {
			return kHTTP_OpStatus_SocketError;
		}
	}

	return kHTTP_OpStatus_Success;
}

///////////////////////////////////////////////////////////////////////////////

HTTPGet::HTTPGet(Zone &zone) 
: m_zone(&zone), m_status(kHTTP_OpStatus_None) {
}

HTTPGet::~HTTPGet() {
	Join();
}

HTTP_OpStatus HTTPGet::SendRequest(const char *host, const char *resource, const char *accept) {
	RAD_ASSERT(host);
	RAD_ASSERT(resource);
	RAD_ASSERT(m_status != kHTTP_OpStatus_Pending);

	if (m_status == kHTTP_OpStatus_Pending)
		return kHTTP_OpStatus_BusyError;

	m_response.reset(new (*m_zone) HTTPResponse(*m_zone));

	m_host = host;
	m_resource = resource;

	if (accept) {
		m_accept = accept;
	} else {
		m_accept.Clear();
	}

	struct hostent *z = gethostbyname(host);
	if (!z || (z->h_addrtype != AF_INET)) {
		m_status = kHTTP_OpStatus_UnknownHostError;
		return m_status;
	}

	m_hostAddr.sin_family = AF_INET;
	m_hostAddr.sin_port = htons(80);
	m_hostAddr.sin_addr.s_addr = *((ulong*)z->h_addr_list[0]);

	m_status = kHTTP_OpStatus_Pending;
	Run();

	return kHTTP_OpStatus_Pending;
}

HTTP_OpStatus HTTPGet::WaitForCompletion() {
	Join();
	return m_status;
}

int HTTPGet::ThreadProc() {

	Socket sd = (int)socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sd == -1) {
		m_status = kHTTP_OpStatus_SocketError;
		return 0;
	}

	int opt = 1;

	if (connect(sd, (const sockaddr*)&m_hostAddr, sizeof(m_hostAddr))) {
		m_status = kHTTP_OpStatus_SocketError;
		return 0;
	}

	String req("GET ");
	req += m_resource;
	req += " HTTP/1.1\r\n";
	req += "Host: ";
	req += m_host + "\r\n";
	if (!m_accept.empty) {
		req += "Accept: ";
		req += m_accept;
		req += "\r\n";
	}
	req += "\r\n";

	if (!sd.send(req.c_str.get(), req.length, 0)) {
		m_status = kHTTP_OpStatus_SocketError;
		return 0;
	}

	m_status = m_response->Read(sd);
	return 0;
}

}
