/*! \file D_HTTP.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#include RADPCH
#include "D_HTTP.h"

namespace world {

D_HTTPResponse::Ref D_HTTPResponse::New(const net::HTTPResponse::Ref &r) {
	return Ref(new (ZWorld) D_HTTPResponse(r));
}

D_HTTPResponse::D_HTTPResponse(const net::HTTPResponse::Ref &r) : m_r(r) {
}

void D_HTTPResponse::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_ResponseCode);
	lua_setfield(L, -2, "ResponseCode");
	lua_pushcfunction(L, lua_ContentType);
	lua_setfield(L, -2, "ContentType");
	lua_pushcfunction(L, lua_Header);
	lua_setfield(L, -2, "Header");
	lua_pushcfunction(L, lua_Body);
	lua_setfield(L, -2, "Body");
}

int D_HTTPResponse::lua_ResponseCode(lua_State *L) {
	D_HTTPResponse::Ref r = lua::SharedPtr::Get<D_HTTPResponse>(L, "HTTPResponse", 1, true);
	lua_pushinteger(L, r->m_r->responseCode.get());
	return 1;
}

int D_HTTPResponse::lua_ContentType(lua_State *L) {
	D_HTTPResponse::Ref r = lua::SharedPtr::Get<D_HTTPResponse>(L, "HTTPResponse", 1, true);
	lua_pushstring(L, r->m_r->contentType.get().c_str);
	return 1;
}

int D_HTTPResponse::lua_Header(lua_State *L) {
	D_HTTPResponse::Ref r = lua::SharedPtr::Get<D_HTTPResponse>(L, "HTTPResponse", 1, true);
	lua_pushstring(L, r->m_r->header.get().c_str);
	return 1;
}

int D_HTTPResponse::lua_Body(lua_State *L) {
	D_HTTPResponse::Ref r = lua::SharedPtr::Get<D_HTTPResponse>(L, "HTTPResponse", 1, true);

	AddrSize len = r->m_r->bodySize;
	if (len < 1)
		return 0;

	const void *data = r->m_r->bodyData;
	lua_pushlstring(L, (const char*)data, (size_t)len);
	return 1;
}

///////////////////////////////////////////////////////////////////////////////

D_HTTPGet::Ref D_HTTPGet::New(const net::HTTPGet::Ref &g) {
	return Ref(new (ZWorld) D_HTTPGet(g));
}

D_HTTPGet::D_HTTPGet(const net::HTTPGet::Ref& g) : m_get(g) {
}

void D_HTTPGet::PushElements(lua_State *L) {
	lua_pushcfunction(L, lua_SendRequest);
	lua_setfield(L, -2, "SendRequest");
	lua_pushcfunction(L, lua_WaitForCompletion);
	lua_setfield(L, -2, "WaitForCompletion");
	lua_pushcfunction(L, lua_Status);
	lua_setfield(L, -2, "Status");
	lua_pushcfunction(L, lua_Response);
	lua_setfield(L, -2, "Response");
}

int D_HTTPGet::lua_SendRequest(lua_State *L) {
	D_HTTPGet::Ref r = lua::SharedPtr::Get<D_HTTPGet>(L, "HTTPGet", 1, true);
	int z = r->m_get->SendRequest(
		luaL_checkstring(L, 2),
		luaL_checkstring(L, 3),
		lua_tostring(L, 4)
	);
	lua_pushinteger(L, z);
	return 1;
}

int D_HTTPGet::lua_WaitForCompletion(lua_State *L) {
	D_HTTPGet::Ref r = lua::SharedPtr::Get<D_HTTPGet>(L, "HTTPGet", 1, true);
	r->m_get->WaitForCompletion();
	return 0;
}

int D_HTTPGet::lua_Status(lua_State *L) {
	D_HTTPGet::Ref r = lua::SharedPtr::Get<D_HTTPGet>(L, "HTTPGet", 1, true);
	lua_pushinteger(L, (int)r->m_get->status.get());
	return 1;
}

int D_HTTPGet::lua_Response(lua_State *L) {
	D_HTTPGet::Ref r = lua::SharedPtr::Get<D_HTTPGet>(L, "HTTPGet", 1, true);
	net::HTTPResponse::Ref rr = r->m_get->response;

	if (rr) {
		D_HTTPResponse::Ref x(D_HTTPResponse::New(rr));
		x->Push(L);
		return 1;
	}

	return 0;
}

} // world