/*! \file StoreDef.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
*/

#pragma once

#include "../Types.h"
#include <Runtime/Container/ZoneVector.h>

namespace iap {

class Store;
typedef boost::shared_ptr<Store> StoreRef;

class Transaction;
typedef boost::shared_ptr<Transaction> TransactionRef;

class PaymentRequest;
typedef boost::shared_ptr<PaymentRequest> PaymentRequestRef;

class StoreEventQueue;
typedef boost::shared_ptr<StoreEventQueue> StoreEventQueueRef;

enum ResponseCode {
	kResponseCode_Success,
	kResponseCode_InvalidReceipt,
	kResponseCode_Unsupported,
	kResponseCode_ErrorUnavailable,
	kResponseCode_ErrorUknown
};

struct Product {
	typedef zone_vector<Product, ZWorldT>::type Vec;

	String id;
	String price;
};

struct ProductValidationData {
	String id;
	ResponseCode code;
};

struct RestorePurchasesCompleteData {
	bool error;
	String msg;
};

} // iap
