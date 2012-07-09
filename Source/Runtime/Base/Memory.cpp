// Memory.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Base.h"

#if !defined(RAD_TARGET_GOLDEN)
	#define RAD_OPT_MEMGUARD
#endif

namespace details {

#if defined(RAD_OPT_MEMGUARD)
void CheckMemGuards(void *pp)
{
	int  *px = reinterpret_cast<int*>(reinterpret_cast<U8*>(pp)-sizeof(void*)*2-2);
	for (size_t i = 0; i < sizeof(void*)/sizeof(int); ++i)
	{
		// if you assert here, you have memory corruption.
		RAD_VERIFY_MSG(*(px++) == RAD_MEM_GUARD, "Memory Corruption Detected");
	}

	px = reinterpret_cast<int*>(reinterpret_cast<U8*>(pp)-sizeof(void*));

	for (size_t i = 0; i < sizeof(void*)/sizeof(int); ++i)
	{
		// if you assert here, you have memory corruption.
		RAD_VERIFY_MSG(*(px++) == RAD_MEM_GUARD, "Memory Corruption Detected");
	}
}

void WriteMemGuards(void *pp)
{
	int  *px = reinterpret_cast<int*>(((U8*)pp)-sizeof(void*)*2-2);
	for (size_t i = 0; i < sizeof(void*)/sizeof(int); ++i)
	{
		*(px++) = RAD_MEM_GUARD;
	}

	px = reinterpret_cast<int*>(((U8*)pp)-sizeof(void*));
	for (size_t i = 0; i < sizeof(void*)/sizeof(int); ++i)
	{
		*(px++) = RAD_MEM_GUARD;
	}
}
#endif

inline AddrSize MemBaseOfs(void *pp)
{
#if defined(RAD_OPT_MEMGUARD)
	return (AddrSize)*reinterpret_cast<U16*>(reinterpret_cast<U8*>(pp)-sizeof(void*)-2);
#else
	return (AddrSize)*reinterpret_cast<U16*>(reinterpret_cast<U8*>(pp)-2);
#endif
}

inline void *MemBasePtr(void *pp)
{
	return reinterpret_cast<U8*>(pp) - MemBaseOfs(pp);
}
} // details

// returnvalue + headerSize = aligned
void *aligned_realloc(void *ptr, AddrSize size, AddrSize headerSize, AddrSize align)
{
	if (size == 0)
	{
		if (ptr)
		{
			aligned_free(ptr);
		}

		return 0;
	}

	RAD_ASSERT(size > 0);
	RAD_ASSERT(align < (std::numeric_limits<U16>::max()-sizeof(void*)*2+2));

	align = std::max<AddrSize>(std::max<AddrSize>(align, MinAlignment), sizeof(void*));

	void *pptr = ptr;
	AddrSize oldOfs = 0;
	AddrSize oldSize = 0;

	if (ptr)
	{
#if defined(RAD_OPT_MEMGUARD)
		details::CheckMemGuards(ptr);
#endif
		oldOfs = details::MemBaseOfs(ptr);
		pptr = details::MemBasePtr(ptr);
		oldSize = malloc_size(pptr);
		RAD_ASSERT(oldSize > oldOfs+headerSize);
		oldSize -= oldOfs+headerSize;
	}

#if defined(RAD_OPT_MEMGUARD)
	U8 *p = (U8*)::realloc(pptr, size+sizeof(void*)*2+2+headerSize+align-1);
#else
	U8 *p = (U8*)::realloc(pptr, size+headerSize+2+align-1);
#endif

	if (p)
	{
#if defined(RAD_OPT_MEMGUARD) // basic over/under guards
		U8 *pp = Align(p, align, sizeof(void*)*2+2+headerSize) - headerSize;
#else
		U8 *pp = Align(p, align, headerSize+2) - headerSize;
#endif

		RAD_ASSERT(pp > p);
		AddrSize ofs = pp-p;

		if (ptr && oldOfs != ofs) // realign?
		{
			RAD_ASSERT((void*)pp != ptr);
			if (ofs > oldOfs)
			{
				memmove(pp, pp-(ofs-oldOfs), std::min(oldSize, size)+headerSize);
			}
			else
			{
				memmove(pp, pp+(oldOfs-ofs), std::min(oldSize, size)+headerSize);
			}
		}

#if defined(RAD_OPT_MEMGUARD)
		details::WriteMemGuards(pp);
		*reinterpret_cast<U16*>(pp-sizeof(void*)-2) = (U16)(ofs);
#else
		*reinterpret_cast<U16*>(pp-2) = (U16)(ofs);
#endif

		p = pp;
	}

	RAD_ASSERT(!p || ((((AddrSize)p)+headerSize) & (align-1)) == 0);

	return p;
}

void aligned_free(void *pp)
{
	if (pp)
	{
#if defined(RAD_OPT_MEMGUARD) // check over/under guards
		details::CheckMemGuards(pp);
#endif
		void *p = details::MemBasePtr(pp);
		free(p);
	}
}

AddrSize aligned_malloc_size(const void *ptr)
{
	if (!ptr) return 0;
#if defined(RAD_OPT_MEMGUARD) // check over/under guards
	details::CheckMemGuards(const_cast<void*>(ptr));
#endif
	AddrSize ofs = details::MemBaseOfs(const_cast<void*>(ptr));
	void *pp = details::MemBasePtr(const_cast<void*>(ptr));
	AddrSize size = malloc_size(pp);
	RAD_ASSERT(size > ofs);
	return size - ofs;
}
