/*! \file AppleKeyChain.h
	\copyright Copyright (c) 2013 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup main
*/

#include <Foundation/Foundation.h>

@interface AppleKeyChain : NSObject

// Key a key value
+(NSData*)dataForKey:(NSString*)key;
+(NSString*)stringForKey:(NSString*)key;
+(BOOL)setKeyValue:(NSString*)key asString:(NSString*)string;
+(BOOL)setKeyValue:(NSString*)key asData:(NSData*)data;
+(BOOL)deleteKey:(NSString*)key;

@end
