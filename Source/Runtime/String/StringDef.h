// StringDef.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

namespace string {
namespace details {

class StringBuf;
typedef boost::shared_ptr<StringBuf> StringBufRef;
typedef boost::weak_ptr<StringBuf> StringBufWRef;

} // details

RAD_ZONE_DEC(RADRT_API, ZString);

enum RefType {
	kRefType_Copy,
	kRefType_Ref
};

struct CopyTag_t {};
struct RefTag_t {};

static const CopyTag_t CopyTag;
static const RefTag_t  RefTag;

class String;
typedef boost::shared_ptr<String> StringRef;

template <typename T> class CharBuf;

struct CharTraits {
	typedef char T;
};

struct WCharTraits {
	typedef wchar_t T;
#if defined(RAD_OPT_4BYTE_WCHAR)
	typedef U32 TT;
#else
	typedef U16 TT;
#endif
};

struct UTF16Traits {
	typedef U16 T;
};

struct UTF32Traits {
	typedef U32 T;
};

typedef CharBuf<CharTraits> UTF8Buf;
typedef CharBuf<UTF16Traits> UTF16Buf;
typedef CharBuf<UTF32Traits> UTF32Buf;
typedef CharBuf<WCharTraits> WCharBuf;

static const CharTraits UTF8Tag;
static const UTF16Traits UTF16Tag;
static const UTF32Traits UTF32Tag;
static const WCharTraits WCharTag;

} // string
