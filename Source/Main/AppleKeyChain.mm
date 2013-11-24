/*! \file AppleKeyChain.mm
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#include <Security/Security.h>
#import "AppleKeyChain.h"
#include <Engine/App.h>

@implementation AppleKeyChain

+(NSMutableDictionary*)searchDictForKey:(NSString*)key {
	NSMutableDictionary *dict = [[NSMutableDictionary alloc] init];
	
	// password class
	[dict setObject:(id)kSecClassGenericPassword forKey:(id)kSecClass];
	// service name (identifying application)
	String appId = CStr(App::Get()->website.get()) + CStr(".") + CStr(App::Get()->title.get());
	NSString *appName = [[NSString alloc] initWithUTF8String:appId.c_str.get()];
	[dict setObject:appName forKey:(id)kSecAttrService];
	// unique key name
	NSData *encodedKey = [key dataUsingEncoding:NSUTF8StringEncoding];
	[dict setObject:encodedKey forKey:(id)kSecAttrGeneric];
	[dict setObject:encodedKey forKey:(id)kSecAttrAccount];
	
	return dict;
}

+(NSData*)dataForKey:(NSString*)key {
	NSMutableDictionary *dict = [self searchDictForKey:key];
	[dict setObject:(id)kSecMatchLimitOne forKey:(id)kSecMatchLimit];
	[dict setObject:(id)kCFBooleanTrue forKey:(id)kSecReturnData];
	
	// search and copy
	NSData *data = nil;
	CFTypeRef cfData = NULL;
	
	if (SecItemCopyMatching((CFDictionaryRef)dict, &cfData) == noErr) {
		data = (NSData*)cfData;
	}
	
	return data;
}

+(NSString*)stringForKey:(NSString*)key {
	NSData *data = [self dataForKey:key];
	NSString *str = nil;
	if (data) {
		str = [[NSString alloc] initWithData:data encoding:NSUTF8StringEncoding];
		[data release];
	}
	
	return str;
}

+(BOOL)setKeyValue:(NSString*)key asString:(NSString*)string {
	NSData *data = [string dataUsingEncoding:NSUTF8StringEncoding];
	BOOL r = [self setKeyValue:key asData:data];
	return r;
}

+(BOOL)setKeyValue:(NSString*)key asData:(NSData*)data {
	return [self createKey:key withValue:data];
}

+(BOOL)deleteKey:(NSString*)key {
	NSMutableDictionary *dict = [self searchDictForKey:key];
	BOOL r = SecItemDelete((CFDictionaryRef)dict) == errSecSuccess;
	[dict release];
	return r;
}

+(BOOL)createKey:(NSString*)key withValue:(NSData*)value {
	NSMutableDictionary *dict = [self searchDictForKey:key];
	[dict setObject:value forKey:(id)kSecValueData];
	[dict setObject:(id)kSecAttrAccessibleWhenUnlocked forKey:(id)kSecAttrAccessible];
	
	OSStatus result = SecItemAdd((CFDictionaryRef)dict, NULL);
		
	if (result == errSecDuplicateItem)
		return [self updateKey:key withValue:value];
	
	return (result == errSecSuccess) ? YES : NO;
}

+(BOOL)updateKey:(NSString*)key withValue:(NSData*)value {
	NSMutableDictionary *dict = [self searchDictForKey:key];
	NSMutableDictionary *valueDict = [[NSMutableDictionary alloc] init];
	
	[valueDict setObject:value forKey:(id)kSecValueData];
	
	OSStatus result = SecItemUpdate((CFDictionaryRef)dict, (CFDictionaryRef)valueDict);
	
	return (result == errSecSuccess) ? YES : NO;
}

@end
