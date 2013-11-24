/*! \file WorldLuaStore.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "World.h"
#include "WorldLuaCommon.h"
#include "../Game/Game.h"

namespace world {

void WorldLua::OnProductInfoResponse(const iap::Product::Vec &products) {
	if (products.empty())
		return;
	if (!PushGlobalCall("Store.OnProductInfoResponse"))
		return;

	lua_State *L = m_L->L;
	lua_createtable(L, products.size(), 0);

	for (int i = 0; i < (int)products.size(); ++i) {
		const iap::Product &p = products[i];
		lua_pushinteger(L, i+1);
		lua_createtable(L, 0, 3);
		lua_pushstring(L, p.id.c_str);
		lua_setfield(L, -2, "id");
		lua_pushstring(L, p.price.c_str);
		lua_setfield(L, -2, "price");
		lua_settable(L, -3);
	}

	Call("Store.OnProductInfoResponse", 1, 0, 0);
}

void WorldLua::OnApplicationValidateResult(iap::ResponseCode code) {
	if (!PushGlobalCall("Store.OnApplicationValidateResult"))
		return;
	lua_pushinteger(m_L->L, (int)code);
	Call("Store.OnApplicationValidateResult", 1, 0, 0);
}

void WorldLua::OnProductValidateResult(const iap::ProductValidationData &data) {
	if (!PushGlobalCall("Store.OnProductValidateResult"))
		return;
	lua_pushstring(m_L->L, data.id.c_str);
	lua_pushinteger(m_L->L, (int)data.code);
	Call("Store.OnProductValidateResult", 2, 0, 0);
}

void WorldLua::OnUpdateTransaction(const iap::TransactionRef &transaction) {
	if (!PushGlobalCall("Store.OnUpdateTransaction"))
		return;
	transaction->Push(m_L->L);
	Call("Store.OnUpdateTransaction", 1, 0, 0);
}

void WorldLua::OnRestoreProductsComplete(const iap::RestorePurchasesCompleteData &data) {
	if (!PushGlobalCall("Store.OnRestoreProductsComplete"))
		return;
	int numArgs = 1;
	lua_pushboolean(L, data.error ? 1 : 0);
	if (data.error) {
		lua_pushstring(L, data.msg.c_str);
		++numArgs;
	}

	Call("Store.OnRestoreProductsComplete", numArgs, 0, 0);
}

int WorldLua::lua_StoreCreate(lua_State *L) {
	LOAD_SELF
	lua_pushboolean(L, self->m_world->game->CreateStore() ? 1 : 0);
	return 1;
}

int WorldLua::lua_StoreEnabled(lua_State *L) {
	LOAD_SELF
	iap::Store *store = self->m_world->game->store;
	lua_pushboolean(L, (store&&store->enabled) ? 1 : 0);
	return 1;
}

int WorldLua::lua_StoreAppGUID(lua_State *L) {
	LOAD_SELF
	iap::Store *store = self->m_world->game->store;
	if (store) {
		lua_pushstring(L, store->appGUID);
		return 1;
	}
	return 0;
}

int WorldLua::lua_StoreRestoreProducts(lua_State *L) {
	LOAD_SELF
	iap::Store *store = self->m_world->game->store;
	if (store)
		store->RestoreProducts();
	return 0;
}

int WorldLua::lua_StoreRequestProductInfo(lua_State *L) {
	LOAD_SELF
	iap::Store *store = self->m_world->game->store;
	if (store) {
		StringVec ids;
		ids.reserve(8);

		lua_pushnil(L);
		while (lua_next(L, 1) != 0) {
			ids.push_back(String(luaL_checkstring(L, -1)));
			lua_pop(L, 1);
		}

		if (!ids.empty()) {
			store->RequestProductInfo(ids);
		}
	}
	return 0;
}

int WorldLua::lua_StoreCreatePaymentRequest(lua_State *L) {
	LOAD_SELF
	iap::Store *store = self->m_world->game->store;

	if (store) {
		iap::PaymentRequest::Ref req = store->CreatePaymentRequest(
			luaL_checkstring(L, 1),
			luaL_checkinteger(L, 2)
		);
		if (req) {
			req->Push(L);
			return 1;
		}
	}

	return 0;
}

int WorldLua::lua_StoreRequestValidateApplication(lua_State *L) {
	LOAD_SELF
	iap::Store *store = self->m_world->game->store;

	if (store)
		store->RequestValidateApplication();

	return 0;
}

int WorldLua::lua_StoreRequestValidateProducts(lua_State *L) {
	LOAD_SELF

	iap::Store *store = self->m_world->game->store;

	if (store) {
		StringVec ids;
		ids.reserve(8);

		lua_pushnil(L);
		while (lua_next(L, 1) != 0) {
			ids.push_back(String(luaL_checkstring(L, -1)));
			lua_pop(L, 1);
		}

		if (!ids.empty()) {
			store->RequestValidateProducts(ids);
		}
	}

	return 0;
}

} // world
