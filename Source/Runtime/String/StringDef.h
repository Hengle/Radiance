// StringDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntString.h"

namespace string {
namespace details {

class StringBuf;
typedef boost::shared_ptr<StringBuf> StringBufRef;
typedef boost::weak_ptr<StringBuf> StringBufWRef;

} // details

RAD_ZONE_DEC(RADRT_API, ZString);

enum RefType {
	RT_Copy,
	RT_Ref
};

struct CopyTag {};
struct RefTag {};

class String;
typedef boost::shared_ptr<String> StringRef;

class WCharBuf;
typedef boost::shared_ptr<WCharBuf> WCharBufRef;

} // string
