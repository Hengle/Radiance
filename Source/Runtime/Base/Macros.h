// Macros.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(__PRIVATE_RAD_OPT_INTBASE__)
	#error "Do not include this file directly, please include Base.h instead"
#endif

#define RAD_NOT_USED(x) x
#define RAD_COUNTER __COUNTER__
#define RADNULL_API

#define RAD_PRIVATE_STRINGIZE(x)      #x
#define RAD_PRIVATE_JOIN_HELPER(x, y) x##y
#define RAD_PRIVATE_JOIN(x, y)        RAD_PRIVATE_JOIN_HELPER(x, y)
#define RAD_PRIVATE_WIDE(_string) L ## _string

#define RAD_STRINGIZE(x) RAD_PRIVATE_STRINGIZE(x)
#define RAD_JOIN(x, y)   RAD_PRIVATE_JOIN(x, y)
#define RAD_WIDE(_string) RAD_PRIVATE_WIDE(_string)

#if defined(_DOXYGEN)

#define RAD_FLAG_BIT(name, bit) name
#define RAD_FLAG(name) name

#else

#define RAD_FLAG_ARRAY_INDEX(_bit_, _array_type_) ((_bit_) / (sizeof(_array_type_) << 3))
#define RAD_FLAG_ARRAY_VALUE(_bit_, _array_type_) (1 << ((_bit_) - ((_bit_) / (sizeof(_array_type_) << 3) * (sizeof(_array_type_) << 3))))

#define RAD_STL_FOREACH(_type, _var, _container) for (_type _var = (_container).begin(); (_var) != (_container).end(); ++(_var))

#define RAD_LINE_SIG_FILE       rad_sigFile
#define RAD_LINE_SIG_FUNCTION   rad_sigFunction
#define RAD_LINE_SIG_LINE       rad_sigLine

#define RAD_LINE_SIG_PARMS      const char* RAD_LINE_SIG_FILE, const char* RAD_LINE_SIG_FUNCTION, const UReg RAD_LINE_SIG_LINE,
#define RAD_LINE_SIG_ARGS       RAD_LINE_SIG_FILE, RAD_LINE_SIG_FUNCTION, RAD_LINE_SIG_LINE,

#define RAD_VOID_LINE_SIG_PARMS const char* RAD_LINE_SIG_FILE, const char* RAD_LINE_SIG_FUNCTION, const UReg RAD_LINE_SIG_LINE
#define RAD_VOID_LINE_SIG_ARGS  RAD_LINE_SIG_FILE, RAD_LINE_SIG_FUNCTION, RAD_LINE_SIG_LINE

#define RAD_LINE_SIG            __FILE__, __FUNCTION__, __LINE__,
#define RAD_VOID_LINE_SIG       __FILE__, __FUNCTION__, __LINE__

#define RAD_FOURCC(a, b, c, d) ((U32)(((U32)(a)) + (((U32)(b))<<8) + (((U32)(c))<<16)+ (((U32)(d))<<24)))

#if defined(RAD_OPT_LITTLE_ENDIAN)
	#define RAD_FOURCC_LE(a, b, c, d) RAD_FOURCC(a, b, c, d)
	#define RAD_FOURCC_BE(a, b, c, d) RAD_FOURCC(d, c, b, a)
#else
	#define RAD_FOURCC_LE(a, b, c, d) RAD_FOURCC(d, c, b, a)
	#define RAD_FOURCC_BE(a, b, c, d) RAD_FOURCC(a, b, c, d)
#endif

#define RAD_MEMBER_OFFSET(_class, _member)            (reinterpret_cast<const unsigned char *const>(&(reinterpret_cast<const _class *const>(1)->_member)) - reinterpret_cast<const unsigned char *const>(1))
#define RAD_SUPER_OFFSET(_class, _super)              (reinterpret_cast<const unsigned char *const>(static_cast<const _super *const>(reinterpret_cast<const _class *const>(1))) - reinterpret_cast<const unsigned char *const>(1))
#define RAD_CLASS_FROM_MEMBER(_class, _member, _this) const_cast<_class *>(reinterpret_cast<const _class *const>(reinterpret_cast<const unsigned char *const>(_this) - RAD_MEMBER_OFFSET(_class, _member)))

#define RAD_SS(_x) static_cast<std::stringstream&>((std::stringstream() << _x)).str()
#define RAD_WSS(_x) static_cast<std::wstringstream&>((std::wstringstream() << _x)).str()

#if defined(RAD_OPT_DEBUG)
	#define RAD_DEBUG_ONLY(x) x
#else
	#define RAD_DEBUG_ONLY(x)
#endif

#if !defined(NULL)
	#define NULL 0
#endif



#endif
