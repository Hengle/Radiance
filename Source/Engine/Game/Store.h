/*! \file Store.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
*/

#pragma once

#include "StoreDef.h"
#include "../World/WorldDef.h"
#include "../Lua/LuaRuntime.h"
#include <Runtime/Event.h>
#include <Runtime/Thread/Locks.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/PushPack.h>

namespace iap {

///////////////////////////////////////////////////////////////////////////////

//! Store object to manage in-app purchases.
class Store : public boost::noncopyable {
	RAD_EVENT_CLASS(EventNoAccess);
public:
	virtual ~Store() {}

	typedef StoreRef Ref;

	static Ref Create(StoreEventQueue *queue);

	virtual void RequestProductInfo(const StringVec &ids) = 0;
	virtual PaymentRequestRef CreatePaymentRequest(const char *id, int quantity) = 0;

	virtual void RestoreProducts() = 0;

	virtual void RequestValidateApplication() = 0;
	virtual void RequestValidateProducts(const StringVec &ids) = 0;

	typedef Event<Product::Vec, EventNoAccess> ProductInfoResponseEvent;
	ProductInfoResponseEvent OnProductInfoResponse;

	typedef Event<ResponseCode, EventNoAccess> ApplicationValidateResultEvent;
	ApplicationValidateResultEvent OnApplicationValidateResult;

	typedef Event<ProductValidationData, EventNoAccess> ProductValidateResultEvent;
	ProductValidateResultEvent OnProductValidateResult;

	typedef Event<TransactionRef, EventNoAccess> UpdateTransactionEvent;
	UpdateTransactionEvent OnUpdateTransaction;

	typedef Event<RestorePurchasesCompleteData, EventNoAccess> RestoreProductsComplete;
	RestoreProductsComplete OnRestoreProductsComplete;

	RAD_DECLARE_READONLY_PROPERTY(Store, appGUID, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Store, enabled, bool);
	
protected:

	void BindEventQueue(StoreEventQueue &queue);

	virtual RAD_DECLARE_GET(appGUID, const char*) = 0;
	virtual RAD_DECLARE_GET(enabled, bool) = 0;

};

///////////////////////////////////////////////////////////////////////////////

class PaymentRequest : public lua::SharedPtr {
public:
	typedef PaymentRequestRef Ref;

	PaymentRequest(const char *id, int quantity) : m_id(id), m_quantity(quantity) {}
	virtual ~PaymentRequest() {}

	RAD_DECLARE_READONLY_PROPERTY(PaymentRequest, id, const char*);
	RAD_DECLARE_READONLY_PROPERTY(PaymentRequest, quantity, int);

	virtual void Submit() = 0;

protected:

	virtual void PushElements(lua_State *L);

private:

	static int lua_Submit(lua_State *L);

	LUART_DECL_GET(Id);
	LUART_DECL_GET(Quantity);

	RAD_DECLARE_GET(id, const char*) {
		return m_id.c_str;
	}

	RAD_DECLARE_GET(quantity, int) {
		return m_quantity;
	}

	String m_id;
	int m_quantity;
};

///////////////////////////////////////////////////////////////////////////////

class Transaction : public lua::SharedPtr {
public:
	typedef TransactionRef Ref;

	Transaction(const char *productId) : m_productId(productId) {}
	virtual ~Transaction() {}

	enum State {
		kState_Purchasing,
		kState_Purchased,
		kState_Failed,
		kState_Restored
	};

	virtual void Finish() = 0;
	virtual String GetErrorMessage() const = 0;

	RAD_DECLARE_READONLY_PROPERTY(Transaction, productId, const char*);
	RAD_DECLARE_READONLY_PROPERTY(Transaction, state, State);

protected:

	virtual void PushElements(lua_State *L);
	virtual RAD_DECLARE_GET(state, State) = 0;

private:

	static int lua_Finish(lua_State *L);
	static int lua_GetErrorMessage(lua_State *L);
	LUART_DECL_GET(ProductId);
	LUART_DECL_GET(State);

	RAD_DECLARE_GET(productId, const char*) {
		return m_productId.c_str;
	}

	String m_productId;
};

///////////////////////////////////////////////////////////////////////////////

class StoreEventQueue : public boost::noncopyable {
public:
	typedef StoreEventQueueRef Ref;

	void Dispatch(world::World &target);

private:

	friend class Store;

	void Bind(Store &store);

	void OnProductInfoResponse(const Product::Vec &products);
	void OnApplicationValidateResult(const ResponseCode &code);
	void OnProductValidateResult(const ProductValidationData &data);
	void OnUpdateTransaction(const TransactionRef &transaction);
	void OnRestoreProductsComplete(const RestorePurchasesCompleteData &data);

	////////////////////////////////////////////////////////

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	class Event {
	public:
		virtual ~Event() {}
		typedef boost::shared_ptr<Event> Ref;
		typedef zone_vector<Ref, ZWorldT>::type Vec;
		virtual void Dispatch(world::World &target) = 0;
	};

	class ProductInfoResponseEvent : public Event {
	public:
		ProductInfoResponseEvent(const Product::Vec &products) : m_products(products) {}
		virtual void Dispatch(world::World &target);
	private:
		Product::Vec m_products;
	};

	class ApplicationValidateResultEvent : public Event {
	public:
		ApplicationValidateResultEvent(ResponseCode code) : m_code(code) {}
		virtual void Dispatch(world::World &target);
	private:
		ResponseCode m_code;
	};

	class ProductValidateResultEvent : public Event {
	public:
		ProductValidateResultEvent(const ProductValidationData &data) : m_data(data) {}
		virtual void Dispatch(world::World &target);
	private:
		ProductValidationData m_data;
	};

	class UpdateTransactionEvent : public Event {
	public:
		UpdateTransactionEvent(const TransactionRef &transaction) : m_transaction(transaction) {}
		virtual void Dispatch(world::World &target);
	private:
		TransactionRef m_transaction;
	};

	class RestoreProductsCompleteEvent : public Event {
	public:
		RestoreProductsCompleteEvent(const RestorePurchasesCompleteData &data) : m_data(data) {}
		virtual void Dispatch(world::World &target);
	private:
		RestorePurchasesCompleteData m_data;
	};

	void PostEvent(Event *e);

	Event::Vec m_events;
	Mutex m_cs;
};

}

#include <Runtime/PopPack.h>