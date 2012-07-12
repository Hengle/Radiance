// Memory.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#if !defined(__PRIVATE_RAD_OPT_INTBASE__)
	#error "Do not include this file directly, please include Base.h instead"
#endif

#if defined(RAD_OPT_WINX)
	#define stack_alloc(_x) (_alloca(_x))
#elif defined(RAD_OPT_GCC)
	#define stack_alloc(_x) ::alloca(_x)
	#if defined(RAD_OPT_LINUX)
		#include <stdlib.h>
		#include <malloc.h>
	#else
		#include <malloc/malloc.h>
	#endif
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif

#include <algorithm>
#include <stdlib.h>

template <typename T>
inline bool IsAligned(T src, AddrSize align)
{
	RAD_ASSERT((align & (align-1)) == 0);
	AddrSize z = (AddrSize)src;
	return (z & (align-1)) == 0;
}

template <typename T>
inline T Align(T src, AddrSize align, AddrSize headerSize=0)
{
	AddrSize z = (AddrSize)src + headerSize;
	return (T)((z+(align-1)) & ~(align-1));
}

inline void *safe_malloc(size_t size)
{
	void *p = malloc(size);
	RAD_OUT_OF_MEM(p);
	return p;
}

inline void *safe_calloc(size_t numElms, size_t elmSize)
{
	void *p = calloc(numElms, elmSize);
	RAD_OUT_OF_MEM(p);
	return p;
}

inline void *safe_realloc(void *p, size_t size)
{
	void *r = realloc(p, size);
	RAD_OUT_OF_MEM(r);
	return r;
}

#if !defined(RAD_OPT_APPLE) // mac implements malloc_size
inline AddrSize malloc_size(const void *p)
{
	if (!p) 
		return 0;
#if defined(RAD_OPT_WINX)
	return (AddrSize)_msize(const_cast<void*>(p));
#elif defined(RAD_OPT_LINUX)
	return (AddrSize)malloc_usable_size(const_cast<void*>(p));
#else
	#error RAD_ERROR_UNSUP_PLAT
#endif
}
#endif

#define RAD_MEM_GUARD ((((int)'m') << 24) + (((int)'E') << 16) + (((int)'m') << 8) + (((int)'G')))

// returnvalue + headerSize = aligned
void *aligned_realloc(void *p, AddrSize size, AddrSize headerSize, AddrSize align);
void aligned_free(void *p);
AddrSize aligned_malloc_size(const void *p);

inline void *aligned_malloc(AddrSize size, AddrSize headerSize, AddrSize align)
{
	return aligned_realloc(0, size, headerSize, align);
}

inline void *aligned_calloc(AddrSize numElms, AddrSize elmSize, AddrSize headerSize, AddrSize align)
{
	void *p = aligned_malloc(numElms*elmSize, headerSize, align);
	if (p) { memset(p, 0, numElms*elmSize+headerSize); }
	return p;
}

inline void *safe_aligned_malloc(AddrSize size, AddrSize headerSize, AddrSize align)
{
	void *p = aligned_malloc(size, headerSize, align);
	RAD_OUT_OF_MEM(p||!(size+headerSize));
	return p;
}

inline void *safe_aligned_calloc(AddrSize numElms, AddrSize elmSize, AddrSize headerSize, AddrSize align)
{
	void *p = aligned_calloc(numElms, elmSize, headerSize, align);
	RAD_OUT_OF_MEM(p||!(numElms*elmSize+headerSize));
	return p;
}

inline void *safe_aligned_realloc(void *p, AddrSize size, AddrSize headerSize, AddrSize align)
{
	p = aligned_realloc(p, size, headerSize, align);
	RAD_OUT_OF_MEM(p||!(size+headerSize));
	return p;
}

template <typename T>
class malloc_allocator
{
public:
	typedef T value_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef AddrSize size_type;
	typedef SAddrSize difference_type;

	template <typename U>
	struct rebind { typedef malloc_allocator<U> other; };

	malloc_allocator() {}
	// The following is not explicit, mimicking std::allocator [20.4.1]
	template <typename U>
	malloc_allocator(const malloc_allocator<U> &) {}

	static pointer address(reference r) { return &r; }
	static const_pointer address(const_reference s) { return &s; }
	static size_type max_size()	{ return (std::numeric_limits<size_type>::max)(); }
	static void construct(const pointer ptr, const value_type & t) { new (ptr) T(t); }
	static void destroy(const pointer ptr)
	{
		ptr->~T();
		(void) ptr;
	}

	bool operator==(const malloc_allocator &) const { return true; }
	bool operator!=(const malloc_allocator &) const { return false; }

	static pointer allocate(const size_type n)
	{
		return (pointer)malloc(n*sizeof(T));
	}

	static pointer allocate(const size_type n, const void * const) { return allocate(n); }

	static void deallocate(const pointer ptr, const size_type n)
	{
#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
		if (ptr == 0 || n == 0)
			return;
#endif
		free(ptr);
	}
};
