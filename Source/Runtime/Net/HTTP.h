/*! \file HTTP.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "Socket.h"
#include "../Thread/Thread.h"
#include <Runtime/PushPack.h>

namespace net {

enum HTTP_OpStatus {
	kHTTP_OpStatus_None,
	kHTTP_OpStatus_Pending,
	kHTTP_OpStatus_Success,
	kHTTP_OpStatus_SocketError = -1,
	kHTTP_OpStatus_ResponseError = -2,
	kHTTP_OpStatus_UnknownHostError = -3,
	kHTTP_OpStatus_BusyError = -4 // already an outstanding request
};

class RADRT_CLASS HTTPResponse {
public:

	typedef boost::shared_ptr<HTTPResponse> Ref;

	HTTPResponse(Zone &zone = ZRuntime);
	~HTTPResponse();

	HTTP_OpStatus Read(Socket &sd);

	RAD_DECLARE_READONLY_PROPERTY(HTTPResponse, responseCode, int);
	RAD_DECLARE_READONLY_PROPERTY(HTTPResponse, contentType, const String&);
	RAD_DECLARE_READONLY_PROPERTY(HTTPResponse, header, const String&);
	RAD_DECLARE_READONLY_PROPERTY(HTTPResponse, bodyData, const void*);
	RAD_DECLARE_READONLY_PROPERTY(HTTPResponse, bodySize, AddrSize);

private:

	int ReadLine(Socket &sd, String &line);

	RAD_DECLARE_GET(responseCode, int) {
		return m_responseCode;
	}

	RAD_DECLARE_GET(contentType, const String&) {
		return m_contentType;
	}

	RAD_DECLARE_GET(header, const String&) {
		return m_header;
	}

	RAD_DECLARE_GET(bodyData, const void*) {
		return m_body;
	}

	RAD_DECLARE_GET(bodySize, AddrSize) {
		return m_size;
	}

	String m_header;
	String m_contentType;
	Zone *m_zone;
	void *m_body;
	AddrSize m_size;
	int m_responseCode;
};

///////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS HTTPGet : private thread::Thread {
public:
	typedef boost::shared_ptr<HTTPGet> Ref;

	HTTPGet(Zone &zone = ZRuntime);
	
	HTTP_OpStatus SendRequest(const char *host, const char *resource, const char *accept);
	HTTP_OpStatus WaitForCompletion();

	RAD_DECLARE_READONLY_PROPERTY(HTTPGet, status, HTTP_OpStatus);
	RAD_DECLARE_READONLY_PROPERTY(HTTPGet, response, HTTPResponse::Ref);

private:

	virtual int ThreadProc();

	RAD_DECLARE_GET(status, HTTP_OpStatus) {
		return m_status;
	}

	RAD_DECLARE_GET(response, HTTPResponse::Ref) {
		return m_response;
	}

	String m_host;
	String m_resource;
	String m_accept;
	sockaddr_in m_hostAddr;
	HTTPResponse::Ref m_response;
	Zone *m_zone;
	HTTP_OpStatus m_status;
};

}

#include <Runtime/PopPack.h>
