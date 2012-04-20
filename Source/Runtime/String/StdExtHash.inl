// StdExtHash.inl
// Adds hashing functions for pml string types for use with stdext::hash_*
// Copyright (c) 2005-2009 Pyramind Labs LLC, All Rights Reserved

#include "../String/String.h"

namespace PML_STDEXT {

//////////////////////////////////////////////////////////////////////////////////////////
// stdext::hash_value
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(PML_OPT_GCC)

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

template <typename A>
struct hash<pml::string::string<A> >
{
	size_t operator () (const pml::string::string<A> &str) const
	{
		hash<const char*> x;
		return x(str.c_str());
	}
};

template <typename A>
struct hash<pml::string::mbstring<A> >
{
	size_t operator () (const pml::string::mbstring<A> &str) const
	{
		hash<const char*> x;
		return x(str.c_str());
	}
};

template <typename A>
struct hash<pml::string::wstring<A> >
{
	size_t operator () (const pml::string::wstring<A> &str) const
	{
		hash<const wchar_t*> x;
		return x(str.c_str());
	}
};

#else

template <typename A>
inline size_t hash_value(const ::pml::string::string<A> &str)
{
	return hash_value(str.c_str());
}

template <typename A>
inline size_t hash_value(const ::pml::string::mbstring<A> &str)
{
	return hash_value(str.c_str());
}

template <typename A>
inline size_t hash_value(const ::pml::string::wstring<A> &str)
{
	return hash_value(str.c_str());
}

#endif

} // PML_STDEXT
