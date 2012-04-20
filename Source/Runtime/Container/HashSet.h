// HashSet.h
// Pyramind STL HashSet
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"

#if defined(RAD_OPT_GCC)
	#include <ext/hash_set>
#else
	#include <hash_set>
#endif

namespace container {

template
<
	typename Key,
	typename Alloc  = ::std::allocator<Key>
>
struct hash_set
{
#if defined(RAD_OPT_GCC)
	typedef RAD_STDEXT::hash_set<Key, RAD_STDEXT::hash<Key>, std::equal_to<Key>, Alloc> type;
#else
	typedef RAD_STDEXT::hash_set<Key, RAD_STDEXT::hash_compare<Key, ::std::less<Key> >, Alloc> type;
#endif
};

template
<
	typename Key,
	typename Alloc  = ::std::allocator<Key>
>
struct hash_multiset
{
#if defined(RAD_OPT_GCC)
	typedef RAD_STDEXT::hash_multiset<Key, RAD_STDEXT::hash<Key>, std::equal_to<Key>, Alloc> type;
#else
	typedef RAD_STDEXT::hash_multiset<Key, RAD_STDEXT::hash_compare<Key, ::std::less<Key> >, Alloc> type;
#endif
};

} // container

#include "StdExtHash.inl"
