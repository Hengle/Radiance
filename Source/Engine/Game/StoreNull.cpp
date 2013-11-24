/*! \file StoreNull.cpp
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup world
*/

#include RADPCH
#include "Store.h"

namespace iap {

///////////////////////////////////////////////////////////////////////////////

class TransactionNull : public Transaction {
public:
	TransactionNull(const char *productId) : Transaction(productId) {}

	virtual void Finish() {}

	virtual String GetErrorMessage() const {
		return String();
	}

protected:

	virtual RAD_DECLARE_GET(state, State) {
		return kState_Purchased;
	}
};

///////////////////////////////////////////////////////////////////////////////

class StoreNull;
class PaymentRequestNull : public PaymentRequest {
public:
	PaymentRequestNull(
		StoreNull &store,
		const char *id,
		int quantity
	) : PaymentRequest(id, quantity), m_store(store) {}

	virtual void Submit();

private:

	StoreNull &m_store;
};

///////////////////////////////////////////////////////////////////////////////

class StoreNull : public Store {
public:

	StoreNull(StoreEventQueue *queue) {
		if (queue)
			BindEventQueue(*queue);
	}
	
	virtual void RequestProductInfo(const StringVec &ids) {
		Product::Vec null; // no products
		OnProductInfoResponse.Trigger(null);
	}

	virtual PaymentRequestRef CreatePaymentRequest(const char *id, int quantity) {
		PaymentRequestNull::Ref req(new (ZWorld) PaymentRequestNull(*this, id, quantity));
		return req;
	}

	virtual void RestoreProducts() {
		RestorePurchasesCompleteData data;
		data.error = false;
		OnRestoreProductsComplete.Trigger(data);
	}

	virtual void RequestValidateApplication() {
		OnApplicationValidateResult.Trigger(kResponseCode_Success);
	}

	virtual void RequestValidateProducts(const StringVec &ids) {
		ProductValidationData data;
		for (StringVec::const_iterator it = ids.begin(); it != ids.end(); ++it) {
			data.id = *it;
			data.code = kResponseCode_Success;
			OnProductValidateResult.Trigger(data);
		}
	}

protected:

	virtual RAD_DECLARE_GET(appGUID, const char*) {
		return "0000-0000-0000-0000";
	}

	virtual RAD_DECLARE_GET(enabled, bool) {
		return true;
	}

private:

	friend class PaymentRequestNull;

	void SubmitPayment(const PaymentRequest &req) {
		TransactionNull::Ref t(new (ZWorld) TransactionNull(req.id));
		OnUpdateTransaction.Trigger(t);
	}


	static int s_transId;
};

int StoreNull::s_transId = 0;

void PaymentRequestNull::Submit() {
	m_store.SubmitPayment(*this);
}

Store::Ref Store::Create(StoreEventQueue *queue) {
	return Store::Ref(new (ZWorld) StoreNull(queue));
}

} // iap
