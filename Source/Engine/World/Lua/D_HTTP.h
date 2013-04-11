/*! \file D_HTTP.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "../../Types.h"
#include "../../Lua/LuaRuntime.h"
#include <Runtime/Net/HTTP.h>
#include <Runtime/PushPack.h>

namespace world {

class RADENG_CLASS D_HTTPResponse : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<D_HTTPResponse> Ref;

	static Ref New(const net::HTTPResponse::Ref &r);

	RAD_DECLARE_READONLY_PROPERTY(D_HTTPResponse, httpResponse, const net::HTTPResponse::Ref&);

protected:

	virtual void PushElements(lua_State *L);

private:

	static int lua_ResponseCode(lua_State *L);
	static int lua_ContentType(lua_State *L);
	static int lua_Header(lua_State *L);
	static int lua_Body(lua_State *L);

	D_HTTPResponse(const net::HTTPResponse::Ref& r);

	RAD_DECLARE_GET(httpResponse, const net::HTTPResponse::Ref&) {
		return m_r;
	}

	net::HTTPResponse::Ref m_r;
};

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS D_HTTPGet : public lua::SharedPtr {
public:
	typedef boost::shared_ptr<D_HTTPGet> Ref;

	static Ref New(const net::HTTPGet::Ref &g);

	RAD_DECLARE_READONLY_PROPERTY(D_HTTPGet, httpGet, const net::HTTPGet::Ref&);

protected:

	virtual void PushElements(lua_State *L);

private:

	D_HTTPGet(const net::HTTPGet::Ref& g);

	RAD_DECLARE_GET(httpGet, const net::HTTPGet::Ref &) {
		return m_get;
	}

	static int lua_SendRequest(lua_State *L);
	static int lua_WaitForCompletion(lua_State *L);
	static int lua_Status(lua_State *L);
	static int lua_Response(lua_State *L);

	net::HTTPGet::Ref m_get;
};

} // world

#include <Runtime/PopPack.h>
