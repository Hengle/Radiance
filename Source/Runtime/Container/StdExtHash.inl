// StdExtHash.inl
// Adds hashing functions for string types for use with stdext::hash_*
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../String.h"

namespace RAD_STDEXT {

//////////////////////////////////////////////////////////////////////////////////////////
// RAD_STDEXT::hash_value
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_GCC)

inline size_t __stl_hash_string(const wchar_t* __s)
{
	unsigned long __h = 0;
	for ( ; *__s; ++__s)
	  __h = 5 * __h + *__s;
	return size_t(__h);
}

template<>
struct hash<wchar_t*>
{
	size_t operator() (const wchar_t *str) const
	{
		return __stl_hash_string(str);
	}
};

template<>
struct hash<const wchar_t*>
{
	size_t operator() (const wchar_t *str) const
	{
		return __stl_hash_string(str);
	}
};

template <typename A, typename B, typename C>
struct hash<std::basic_string<A, B, C> >
{
	size_t operator () (const std::basic_string<A, B, C>  &str) const
	{
		hash<const A*> x;
		return x(str.c_str());
	}
};

template <>
struct hash< ::string::String >
{
	size_t operator () (const ::string::String &str) const
	{
		hash<const char*> x;
		return x(str.c_str.get());
	}
};
template <>
struct hash<void*>
{
	size_t operator() (const void* x) const
	{
		return reinterpret_cast<size_t>(x);
	}
};

#define RAD_HASH_INT_TYPE(_type) \
namespace RAD_STDEXT { \
template <> \
struct hash<_type> \
{ \
	size_t operator() (const _type &x) const \
	{ RAD_STDEXT::hash<int> y; return y(x); } \
}; \
}

#define RAD_HASH_INT_FIELD(_type, _int_accessor) \
namespace RAD_STDEXT { \
template <> \
struct hash<_type> \
{ \
	size_t operator () (const _type &x) const \
	{ RAD_STDEXT::hash<int> y; return y(x._int_accessor); } \
}; \
}

#define RAD_HASH_CUSTOM_FN(_type, _fn) \
namespace RAD_STDEXT { \
template <> \
struct hash<_type> \
{ \
	size_t operator () (const _type &x) const \
	{ return _fn(x); } \
}; \
}

#else

inline size_t hash_value(const ::string::String &str)
{
	return hash_value(str.c_str.get());
}

#define RAD_HASH_INT_TYPE(_type)

#define RAD_HASH_INT_FIELD(_type, _int_accessor) \
namespace RAD_STDEXT { \
inline size_t hash_value(const _type &x) \
{ return RAD_STDEXT::hash_value(x._int_accessor); } \
}

#define RAD_HASH_CUSTOM_FN(_type, _fn) \
namespace RAD_STDEXT { \
inline size_t hash_value(const _type &x) { return _fn(x); } \
}

#endif

} // RAD_STDEXT
