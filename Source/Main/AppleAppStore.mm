/*! \file AppleAppStore.mm
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#include <StoreKit/StoreKit.h>
#include <Engine/Game/Store.h>
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/Thread/Locks.h>

#if defined(RAD_OPT_IOS)
#import "AppleKeyChain.h"
#endif

namespace details {
class AppStore;
}

//! Provides calls into our C++ store system from StoreKit delegates
@interface AppStoreHooks : NSObject<SKPaymentTransactionObserver, SKProductsRequestDelegate> {}

- (id)initWithHooks:(details::AppStore*)hooks;
- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response;
- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions;
- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue;
- (void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error;
- (void)paymentQueue:(SKPaymentQueue*)queue updatedDownloads:(NSArray*)downloads;

- (void)removeFromQueue;
- (SKProduct*)productForId:(const char*)productId;

@end

@interface AppStoreHooks () {
details::AppStore *m_hooks;
bool m_inQueue;
NSArray *m_products;
}
@end

namespace details {

namespace {
inline NSString *Unhash(unsigned int *src) __attribute__((always_inline));
inline NSString *Unhash(unsigned int *src) {
	char dst[256];
	int i;
	for (i = 0; src[i] && (i<255); ++i) {
		dst[i] = (char)(((~src[i])&0x000fe000)>>13);
	}
	dst[i] = 0;
	return [[NSString alloc] initWithUTF8String:dst];
}
	
// original string: 'applicationUUID'
	unsigned int _hashedStr_ApplicationUUID[] = {
		0x9f33dbcc, 0x5931e174, 0x5911e7c6, 0x86b26efb, 0x8082c8a1, 0xe4639977,
		0xbf03d2a3, 0x84317da7, 0x1d52dd6a, 0x73a203f5, 0x50222c65, 0xd2c55184,
		0xe1e55748, 0x9136da95, 0x9f576eaf, 0
	};
}

///////////////////////////////////////////////////////////////////////////////

class AppStoreTransaction;
typedef boost::shared_ptr<AppStoreTransaction> AppStoreTransactionRef;

class AppStore : public iap::Store {
public:
	AppStore(iap::StoreEventQueue *queue);
	virtual ~AppStore();
	
	virtual void RequestProductInfo(const StringVec &ids);
	virtual iap::PaymentRequestRef CreatePaymentRequest(const char *id, int quantity);
	virtual void RestoreProducts();
	virtual void RequestValidateApplication();
	virtual void RequestValidateProducts(const StringVec &ids);
	
	iap::TransactionRef GetTransaction(SKPaymentTransaction *transaction);
	
protected:

	virtual RAD_DECLARE_GET(appGUID, const char*);
	virtual RAD_DECLARE_GET(enabled, bool);
	
private:

	friend class AppStoreTransaction;
	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;
	
	void RemoveTransaction(iap::Transaction *transaction);
	
	typedef zone_vector<AppStoreTransactionRef, ZWorldT>::type TransactionVec;

	TransactionVec m_transactions;
	mutable String m_guid;
	AppStoreHooks *m_hooks;
	Mutex m_m;
};

///////////////////////////////////////////////////////////////////////////////

class AppStorePaymentRequest : public iap::PaymentRequest {
public:
	AppStorePaymentRequest(
		SKPayment *payment,
		const char *productId,
		int quantity
	);
	virtual ~AppStorePaymentRequest();
	
	virtual void Submit();
	
private:

	SKPayment *m_payment;
};

///////////////////////////////////////////////////////////////////////////////

class AppStoreTransaction : public iap::Transaction {
public:
	AppStoreTransaction(AppStore *store, SKPaymentTransaction *transaction);
	
	virtual ~AppStoreTransaction();
	
	virtual void Finish();
	virtual String GetErrorMessage() const;
	
	RAD_DECLARE_READONLY_PROPERTY(AppStoreTransaction, skTransaction, SKPaymentTransaction*);
	
protected:

	virtual RAD_DECLARE_GET(state, iap::Transaction::State);
	
private:

	RAD_DECLARE_GET(skTransaction, SKPaymentTransaction*) {
		return m_transaction;
	}

	AppStore *m_store;
	SKPaymentTransaction *m_transaction;
};

///////////////////////////////////////////////////////////////////////////////

AppStore::AppStore(::iap::StoreEventQueue *queue)  {
	m_hooks = [[AppStoreHooks alloc] initWithHooks:this];
	
	if (queue)
		BindEventQueue(*queue);
}

AppStore::~AppStore() {
	if (m_hooks) {
		[m_hooks removeFromQueue];
		[m_hooks release];
	}
}

void AppStore::RequestProductInfo(const StringVec &ids) {
	NSMutableSet *set = [[NSMutableSet alloc] initWithCapacity:ids.size()];
	
	for (StringVec::const_iterator it = ids.begin(); it != ids.end(); ++it) {
		const String &str = *it;
		
		NSString *nsstr = [[NSString alloc] initWithUTF8String:str.c_str.get()];
		[set addObject: nsstr];
	}
	
	SKProductsRequest *req = [[SKProductsRequest alloc] initWithProductIdentifiers:set];
	req.delegate = m_hooks;
	[req start];
}

iap::PaymentRequestRef AppStore::CreatePaymentRequest(const char *productId, int quantity) {
	RAD_ASSERT(quantity > 0);
	RAD_ASSERT(quantity <= 10);
	SKProduct *product = [m_hooks productForId:productId];
	if (!product)
		return iap::PaymentRequestRef();
		
	SKMutablePayment *payment = [SKMutablePayment paymentWithProduct:product];
	payment.quantity = quantity;
	
	return iap::PaymentRequestRef(new (ZWorld) AppStorePaymentRequest(payment, productId, quantity));
}

void AppStore::RestoreProducts() {
	[[SKPaymentQueue defaultQueue] restoreCompletedTransactions];
}

void AppStore::RequestValidateApplication() {
	OnApplicationValidateResult.Trigger(iap::kResponseCode_Success);
}

void AppStore::RequestValidateProducts(const StringVec &ids) {
	iap::ProductValidationData data;
	
	for (StringVec::const_iterator it = ids.begin(); it != ids.end(); ++it) {
		data.id = *it;
		data.code = iap::kResponseCode_Success;
		OnProductValidateResult.Trigger(data);
	}
}

iap::TransactionRef AppStore::GetTransaction(SKPaymentTransaction *transaction) {
	Lock L(m_m);
	
	for (TransactionVec::const_iterator it = m_transactions.begin(); it != m_transactions.end(); ++it) {
		const AppStoreTransactionRef &r = *it;
		if (r->skTransaction.get() == transaction)
			return boost::static_pointer_cast<iap::Transaction>(r);
	}
	
	AppStoreTransactionRef r(new (ZWorld) AppStoreTransaction(this, transaction));
	m_transactions.push_back(r);
	return boost::static_pointer_cast<iap::Transaction>(r);
}

void AppStore::RemoveTransaction(iap::Transaction *t) {
	Lock L(m_m);
	
	for (TransactionVec::iterator it = m_transactions.begin(); it != m_transactions.end(); ++it) {
		const AppStoreTransactionRef &r = *it;
		if ((iap::Transaction*)r.get() == t) {
			m_transactions.erase(it);
			return;
		}
	}
}

const char* AppStore::RAD_IMPLEMENT_GET(appGUID) {
	if (m_guid.empty) {
		
#if defined(RAD_OPT_IOS)
	NSString *key = Unhash(_hashedStr_ApplicationUUID);
	NSString *guid = [AppleKeyChain stringForKey:key];
		
	if (guid) {
		m_guid = [guid UTF8String];
		[guid release];
	} else {
		guid = [[NSUUID UUID] UUIDString];
		[AppleKeyChain setKeyValue:key asString:guid];
		m_guid = [guid UTF8String];
	}
#else
	m_guid = CStr("0000-0000-0000-0000");
#endif
	}
	
	return m_guid.c_str;
}

bool AppStore::RAD_IMPLEMENT_GET(enabled) {
	return [SKPaymentQueue canMakePayments] == YES;
}

///////////////////////////////////////////////////////////////////////////////

AppStorePaymentRequest::AppStorePaymentRequest(
	SKPayment *payment,
	const char *productId,
	int quantity
) : iap::PaymentRequest(productId, quantity), m_payment(payment) {
	[payment retain];
}

AppStorePaymentRequest::~AppStorePaymentRequest() {
	[m_payment release];
}

void AppStorePaymentRequest::Submit() {
	[[SKPaymentQueue defaultQueue] addPayment:m_payment];
}

///////////////////////////////////////////////////////////////////////////////

AppStoreTransaction::AppStoreTransaction(AppStore *store, SKPaymentTransaction *transaction) :
iap::Transaction([transaction.payment.productIdentifier UTF8String]), m_store(store), m_transaction(transaction) {
	[transaction retain];
}

AppStoreTransaction::~AppStoreTransaction() {
	[m_transaction release];
}

void AppStoreTransaction::Finish() {
	[[SKPaymentQueue defaultQueue] finishTransaction: m_transaction];
	m_store->RemoveTransaction(this);
}

String AppStoreTransaction::GetErrorMessage() const {
	String err;
	if (m_transaction.transactionState == SKPaymentTransactionStateFailed) {
		err = [[m_transaction.error localizedFailureReason] UTF8String];
	}
	return err;
}

iap::Transaction::State AppStoreTransaction::RAD_IMPLEMENT_GET(state) {
	SKPaymentTransactionState x = m_transaction.transactionState;
	switch (x) {
		case SKPaymentTransactionStatePurchasing:
			return kState_Purchasing;
		case SKPaymentTransactionStatePurchased:
			return kState_Purchased;
		case SKPaymentTransactionStateRestored:
			return kState_Restored;
		default:
			break;
	}
	
	return kState_Failed;
}
	
} // details

@implementation AppStoreHooks

- (void) dealloc {
	if (m_products)
		[m_products release];
	[super dealloc];
}

- (id)initWithHooks:(details::AppStore*)hooks {
	self = [super init];
	self->m_hooks = hooks;
	self->m_products = nil;
	return self;
}

- (void)productsRequest:(SKProductsRequest *)request didReceiveResponse:(SKProductsResponse *)response {

	// keep the product list around so we can look for them later
	if (m_products) {
		[m_products release];
	}
	
	m_products = response.products;
	[m_products retain];
	
	iap::Product::Vec productList;
	productList.reserve([response.products count]);
	
	NSNumberFormatter *fm = [[NSNumberFormatter alloc] init];
	[fm setFormatterBehavior:NSNumberFormatterBehavior10_4];
	[fm setNumberStyle:NSNumberFormatterCurrencyStyle];
	
	for (SKProduct *skp in response.products) {
		iap::Product p;
		p.id = [skp.productIdentifier UTF8String];
		
		[fm setLocale:[skp priceLocale]];
		p.price = [[fm stringFromNumber:skp.price] UTF8String];
		
		productList.push_back(p);
	}
	
	[fm release];
	m_hooks->OnProductInfoResponse.Trigger(productList);
	
	[request autorelease];
	
	m_inQueue = true;
	[[SKPaymentQueue defaultQueue] addTransactionObserver:self];
}

- (void)paymentQueue:(SKPaymentQueue*)queue updatedTransactions:(NSArray*)transactions {

	for (SKPaymentTransaction *skt in transactions) {
		iap::TransactionRef t = m_hooks->GetTransaction(skt);
		m_hooks->OnUpdateTransaction.Trigger(t);
	}

}

- (void)paymentQueueRestoreCompletedTransactionsFinished:(SKPaymentQueue*)queue {
	iap::RestorePurchasesCompleteData data;
	data.error = false;
	m_hooks->OnRestoreProductsComplete.Trigger(data);
}

- (void)paymentQueue:(SKPaymentQueue*)queue restoreCompletedTransactionsFailedWithError:(NSError*)error {
	iap::RestorePurchasesCompleteData data;
	data.error = true;
	data.msg = [[error localizedFailureReason] UTF8String];
}

- (void)paymentQueue:(SKPaymentQueue*)queue updatedDownloads:(NSArray*)downloads {
}

- (void)removeFromQueue {
	if (m_inQueue) {
		[[SKPaymentQueue defaultQueue] removeTransactionObserver: self];
		m_inQueue = false;
	}
}

- (SKProduct*)productForId:(const char*)productId {
	NSString *str = [[NSString alloc] initWithUTF8String:productId];
	SKProduct *product = nil;
	
	if (m_products) {
		for (SKProduct *p in m_products) {
			if ([str compare:p.productIdentifier] == NSOrderedSame) {
				product = p;
				break;
			}
		}
	}
	
	[str release];
	return product;
}

@end

namespace iap {
Store::Ref Store::Create(StoreEventQueue *queue) {
	return Store::Ref(new (ZWorld) details::AppStore(queue));
}
}


