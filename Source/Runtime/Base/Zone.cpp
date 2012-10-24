// Zone.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Hierarchical memory allocation tracking for memory profiling.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Base.h"
#include <algorithm>
#include <boost/thread/mutex.hpp>
#include <boost/thread/recursive_mutex.hpp>
#include <boost/thread/locks.hpp>

#if defined(RAD_OVERLOAD_STD_NEW)
void * RAD_ANSICALL operator new(size_t s, const std::nothrow_t&) throw()
{
	return operator new(s, ZUnknown);
}

void * RAD_ANSICALL operator new(size_t s)
{
	return operator new(s, std::nothrow);
}

void * RAD_ANSICALL operator new[](size_t s, const std::nothrow_t&) throw()
{
	return operator new(s, std::nothrow);
}

void * RAD_ANSICALL operator new[](size_t s)
{
	return operator new(s, std::nothrow);
}

void  RAD_ANSICALL operator delete(void *p, const std::nothrow_t&) throw()
{
	operator delete(p, ZUnknown);
}

void  RAD_ANSICALL operator delete(void *p)
{
	operator delete(p, std::nothrow);
}

void  RAD_ANSICALL operator delete[](void *p, const std::nothrow_t&) throw()
{
	operator delete(p, std::nothrow);
}

void  RAD_ANSICALL operator delete[](void *p)
{
	operator delete(p, std::nothrow);
}
#endif

#if defined(RAD_OPT_PC_TOOLS) && defined(RAD_OVERLOAD_STD_NEW)

// the linker load on all platforms will bind to the first
// instance of an imported symbol.
//
// this causes any shared libraries (like qt) to potentially
// link against the crt or another .so defintion of the
// new/delete symbol
//
// in tools builds where we know that 3rd party libraries
// do this (qt), we track all zone pointers to avoid confusion
// where these libraries may allocate/free memory inline in headers
// (which links to us) and the dll will allocate/free against the
// crt which it was compiled against, potentially causing mixing
// of the new/delete calls and a crash.

#define RAD_OPT_ZONE_PTR_HASH

#include <set>
typedef boost::mutex ZonePtrMutex;
typedef boost::lock_guard<ZonePtrMutex> ZonePtrLock;
typedef std::set<void*, std::less<void*>, malloc_allocator<void*> > ZonePtrHash;
namespace {

static U8 hashMem[sizeof(ZonePtrHash)];
static U8 mutexMem[sizeof(ZonePtrMutex)];

struct StaticHash
{
	StaticHash() : init(true), destructed(false)
	{
		hash = new (hashMem) ZonePtrHash();
		mutex = new (mutexMem) ZonePtrMutex();
	};

	~StaticHash()
	{
		destructed = true;
		CheckFree();
	}

	void CheckFree()
	{
		if (destructed && hash->empty())
		{
			hash->~set();
			mutex->~mutex();
			init = false;
		}
	}

	bool init;
	bool destructed;
	ZonePtrHash *hash;
	ZonePtrMutex *mutex;
};

StaticHash &PtrHash()
{
	static StaticHash s_hash;
	return s_hash;
}

}

#endif

void * RAD_ANSICALL operator new(size_t s, Zone &zone) throw()
{

#if defined(RAD_OVERLOAD_STD_NEW)
	void *p = safe_zone_malloc(zone, s, 0, DefaultZoneAlignment);
#if defined(RAD_OPT_ZONE_PTR_HASH)
	if (p)
	{
		ZonePtrLock L(*PtrHash().mutex);
		PtrHash().hash->insert(p);
	}
#endif
#else
	void *p = operator new(s, std::nothrow);
#endif
	return p;
}

void * RAD_ANSICALL operator new[](size_t s, Zone &zone) throw()
{
#if defined(RAD_OVERLOAD_STD_NEW)
	return operator new (s, zone);
#else
	return operator new[](s, std::nothrow);
#endif
}

void  RAD_ANSICALL operator delete(void *p, Zone &) throw()
{
#if defined(RAD_OVERLOAD_STD_NEW)
	if (p)
	{
#if defined(RAD_OPT_ZONE_PTR_HASH)
		PtrHash().mutex->lock();
		ZonePtrHash::iterator it = PtrHash().hash->find(p);
		if (it == PtrHash().hash->end())
		{
			PtrHash().mutex->unlock();
			// this assumes all crt free-store allocations use malloc()
			// is this bad?
			free(p);
			return;
		}
		PtrHash().hash->erase(it);
		PtrHash().mutex->unlock();
		PtrHash().CheckFree();
#endif
		zone_free(p);
	}
#else
	operator delete(p);
#endif
}

void RAD_ANSICALL operator delete[](void *p, Zone &z) throw()
{
#if defined(RAD_OVERLOAD_STD_NEW)
	operator delete(p, z);
#else
	operator delete[](p);
#endif
}

//#define NEED_LOCKS

#if defined(NEED_LOCKS)
namespace {
typedef boost::recursive_mutex Mutex;
typedef boost::lock_guard<Mutex> Lock;
Mutex s_m;
} // namespace
#endif

RAD_ZONE_DEF(RADRT_API, ZUnknown, "Unknown", 0);
RAD_ZONE_DEF(RADRT_API, ZRuntime, "Runtime", 0);

void Zone::Init()
{
#if defined(RAD_OPT_ZONE_MEMGUARD)
	m_frontGuard[0]=RAD_MEM_GUARD;
	m_frontGuard[1]=RAD_MEM_GUARD;
	m_backGuard[0]=RAD_MEM_GUARD;
	m_backGuard[1]=RAD_MEM_GUARD;
#endif

	if (m_parent)
	{
#if defined(NEED_LOCKS)
		Lock l(s_m);
#endif
		m_next = m_parent->m_head;
		m_parent->m_head = this;
	}
}

void Zone::Inc(AddrSize size, AddrSize overhead)
{
	m_numBytes += size;
	m_overhead += overhead;
	m_high = (AddrSize)std::max(m_numBytes, m_high);
	m_small = (AddrSize)std::min(m_small, (AddrSize)size);
	m_large = (AddrSize)std::max(m_large, (AddrSize)size);
}

void Zone::Dec(AddrSize size, AddrSize overhead)
{
	m_numBytes -= size;
	m_overhead -= overhead;
}

void *Zone::Realloc(void *ptr, size_t size, AddrSize headerSize, AddrSize alignment)
{
	RAD_ASSERT(headerSize <= std::numeric_limits<U16>::max());
#if defined(RAD_OPT_ZONE_MEMGUARD)
	if (ptr)
	{
		RAD_VERIFY(FromPtr(ptr) == this);
	}
#endif

	AddrSize oldSize = AllocSize(ptr);
	AddrSize oldHeaderSize = ptr ? HeaderSize(ptr) : headerSize;
	RAD_ASSERT(headerSize == oldHeaderSize); // you can't change this with Realloc.

	U8 *pptr = (U8*)ptr - EHeaderSize;
	U8 *p = (U8*)aligned_realloc(ptr ? pptr : 0, size, EHeaderSize+headerSize, alignment);

	if (ptr && (p||!size))
	{
		if (!p)
			--m_numAllocs;

		RAD_ASSERT(m_numBytes >= oldSize);
		RAD_ASSERT(m_overhead >= (oldHeaderSize+EHeaderSize));

		m_numBytes -= oldSize;
		m_overhead -= oldHeaderSize+EHeaderSize;
	}

	if (p)
	{
		size += EHeaderSize + headerSize; // count this as well.

		if (!ptr) // make another alloc.
			++m_numAllocs;

		m_numBytes += size;
		m_overhead += EHeaderSize + headerSize;
		m_high = (AddrSize)std::max(m_numBytes, m_high);
		m_small = (AddrSize)std::min(m_small, (AddrSize)size);
		m_large = (AddrSize)std::max(m_large, (AddrSize)size);

		*reinterpret_cast<U16*>(p) = (U16)headerSize;
		p += sizeof(U16);
		*reinterpret_cast<Zone**>(p) = this;
		p += sizeof(Zone*);
		*reinterpret_cast<AddrSize*>(p) = size;
		p += sizeof(AddrSize);
#if defined(RAD_OPT_ZONE_MEMGUARD)
		p = (U8*)WriteMemGuards(p);
#endif
	}

	RAD_ASSERT((((AddrSize)p) & (alignment-1)) == 0);

	return p;
}

AddrSize Zone::RAD_IMPLEMENT_GET(totalBytes)
{
#if defined(NEED_LOCKS)
	Lock l(s_m);
#endif
	AddrSize total = m_numBytes;
	for (Zone *z = m_head; z; z = z->next)
	{
		total += z->totalBytes;
	}
	return total;
}

AddrSize Zone::RAD_IMPLEMENT_GET(totalOverhead)
{
#if defined(NEED_LOCKS)
	Lock l(s_m);
#endif
	AddrSize total = m_overhead;
	for (Zone *z = m_head; z; z = z->next)
	{
		total += z->m_overhead;
	}
	return total;
}

AddrSize Zone::RAD_IMPLEMENT_GET(totalCount)
{
#if defined(NEED_LOCKS)
	Lock l(s_m);
#endif
	AddrSize total = m_numAllocs;
	for (Zone *z = m_head; z; z = z->next)
	{
		total += z->totalCount;
	}
	return total;
}

AddrSize Zone::RAD_IMPLEMENT_GET(totalSmall)
{
#if defined(NEED_LOCKS)
	Lock l(s_m);
#endif
	AddrSize total = m_small;
	for (Zone *z = m_head; z; z = z->next)
	{
		total = std::min<AddrSize>(z->totalSmall, total);
	}
	return total;
}

AddrSize Zone::RAD_IMPLEMENT_GET(totalLarge)
{
#if defined(NEED_LOCKS)
	Lock l(s_m);
#endif
	AddrSize total = m_large;
	for (Zone *z = m_head; z; z = z->next)
	{
		total = std::max<AddrSize>(z->totalLarge, total);
	}
	return total;
}

AddrSize Zone::RAD_IMPLEMENT_GET(totalHigh)
{
#if defined(NEED_LOCKS)
	Lock l(s_m);
#endif
	AddrSize total = m_high;
	for (Zone *z = m_head; z; z = z->next)
	{
		total = std::max<AddrSize>(z->totalHigh, total);
	}
	return total;
}

#if defined (RAD_OPT_ZONE_MEMGUARD)

void Zone::CheckMemGuards(void *ptr)
{
	U8 *pp = (U8*)ptr;
	int *px = reinterpret_cast<int*>(pp-sizeof(void*));
	for (size_t i = 0; i < sizeof(void*)/sizeof(int); ++i)
	{
		// if you assert here, you have memory corruption.
		RAD_VERIFY_MSG(*(px++) == RAD_MEM_GUARD, "Memory Corruption Detected");
	}
}

void *Zone::WriteMemGuards(void *ptr)
{
	int *px = reinterpret_cast<int*>(ptr);
	for (size_t i = 0; i < sizeof(void*)/sizeof(int); ++i)
	{
		*(px++) = RAD_MEM_GUARD;
	}
	return ((U8*)ptr) + sizeof(void*);
}

void Zone::CheckGuards()
{
	RAD_VERIFY_MSG(m_frontGuard[0]==RAD_MEM_GUARD, "Memory Corruption Detected");
	RAD_VERIFY_MSG(m_frontGuard[1]==RAD_MEM_GUARD, "Memory Corruption Detected");
	RAD_VERIFY_MSG(m_backGuard[0]==RAD_MEM_GUARD, "Memory Corruption Detected");
	RAD_VERIFY_MSG(m_backGuard[1]==RAD_MEM_GUARD, "Memory Corruption Detected");
}

#endif

