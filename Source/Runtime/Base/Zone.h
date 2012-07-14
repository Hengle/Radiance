// Zone.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Hierarchical memory allocation tracking for memory profiling.
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <limits>
#include <boost/shared_ptr.hpp>

#include "../PushPack.h"

#if !defined(__PRIVATE_RAD_OPT_INTBASE__)
	#error "Do not include this file directly, please include Base.h instead"
#endif

#if !defined(RAD_TARGET_GOLDEN)
	#define RAD_OPT_ZONE_MEMGUARD
#endif

enum
{
	DefaultZoneAlignment = sizeof(double)
};

void aligned_free(void*);

class Zone
{
public:
	Zone(void*, const char *_name) :
		 m_parent(0),
		 m_next(0),
		 m_head(0),
		 m_name(_name),
		 m_numBytes(0),
		 m_overhead(0),
		 m_numAllocs(0),
		 m_small(std::numeric_limits<AddrSize>::max()),
		 m_large(0),
		 m_high(0)
	 {
		 Init();
	 }

	Zone(Zone &_parent, const char *_name) :
		 m_parent(&_parent),
		 m_next(0),
		 m_head(0),
		 m_name(_name),
		 m_numBytes(0),
		 m_overhead(0),
		 m_numAllocs(0),
		 m_small(std::numeric_limits<AddrSize>::max()),
		 m_large(0),
		 m_high(0)
	 {
		 Init();
	 }

	RAD_DECLARE_READONLY_PROPERTY(Zone, parent, Zone*);
	RAD_DECLARE_READONLY_PROPERTY(Zone, next, Zone*);
	RAD_DECLARE_READONLY_PROPERTY(Zone, head, Zone*);
	RAD_DECLARE_READONLY_PROPERTY(Zone, name, const char *);

	RAD_DECLARE_READONLY_PROPERTY(Zone, numBytes, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, overhead, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, count, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, small, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, large, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, high, AddrSize);
	// totals include children.
	RAD_DECLARE_READONLY_PROPERTY(Zone, totalBytes, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, totalOverhead, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, totalCount, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, totalSmall, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, totalLarge, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(Zone, totalHigh, AddrSize);

	// returnvalue + headerSize = aligned
	void *Realloc(void *p, size_t size, AddrSize headerSize, AddrSize alignment);

	void Inc(AddrSize size, AddrSize overhead);
	void Dec(AddrSize size, AddrSize overhead);

	static void Delete(void *p)
	{
		if (!p) return;
		U8 *pp = (U8*)p;
		pp -= EHeaderSize;
		Zone *zone = FromPtr(p);
		RAD_ASSERT(zone);
		zone->_Delete(pp);
	}

	static AddrSize AllocSize(void *p)
	{
		if (!p) return 0;
		U8 *pp = (U8*)p;
		pp -= EHeaderSize-sizeof(Zone*);
		RAD_ASSERT(FromPtr(p)); // check guards
		return *reinterpret_cast<AddrSize*>(pp);
	}

	static AddrSize HeaderSize(void *p)
	{
		if (!p) return 0;
		U8 *pp = (U8*)p;
		pp -= EHeaderSize-sizeof(Zone*)-sizeof(AddrSize);
		RAD_ASSERT(FromPtr(p)); // check guards
		return (AddrSize)*reinterpret_cast<U16*>(pp);
	}

	static Zone *FromPtr(void *p)
	{
		if (!p) return 0;
#if defined(RAD_OPT_ZONE_MEMGUARD)
		CheckMemGuards(p);
#endif
		U8 *pp = (U8*)p;
		pp -= EHeaderSize;
		Zone *zone = *reinterpret_cast<Zone**>(pp);
		RAD_ASSERT(zone);
#if defined(RAD_OPT_ZONE_MEMGUARD)
		// make sure we didn't just deref bullshit.
		zone->CheckGuards();
#endif
		return zone;
	}


private:

	void Init();

	RAD_DECLARE_GET(parent, Zone*) { return m_parent; }
	RAD_DECLARE_GET(next, Zone*) { return m_next; }
	RAD_DECLARE_GET(head, Zone*) { return m_head; }
	RAD_DECLARE_GET(name, const char*) { return m_name; }
	RAD_DECLARE_GET(numBytes, AddrSize) { return m_numBytes; }
	RAD_DECLARE_GET(overhead, AddrSize) { return m_overhead; }
	RAD_DECLARE_GET(count, AddrSize) { return m_numAllocs; }
	RAD_DECLARE_GET(small, AddrSize) { return m_small; }
	RAD_DECLARE_GET(large, AddrSize) { return m_large; }
	RAD_DECLARE_GET(high, AddrSize) { return m_high; }
	RAD_DECLARE_GET(totalBytes, AddrSize);
	RAD_DECLARE_GET(totalOverhead, AddrSize);
	RAD_DECLARE_GET(totalCount, AddrSize);
	RAD_DECLARE_GET(totalSmall, AddrSize);
	RAD_DECLARE_GET(totalLarge, AddrSize);
	RAD_DECLARE_GET(totalHigh, AddrSize);

	enum
	{
#if defined (RAD_OPT_ZONE_MEMGUARD)
		EHeaderSize = sizeof(Zone*) + sizeof(AddrSize) + sizeof(U16) + sizeof(void*)
#else
		EHeaderSize = sizeof(Zone*) + sizeof(AddrSize) + sizeof(U16)
#endif
	};

	void _Delete(U8 *p)
	{
		--m_numAllocs;
		m_numBytes -= *reinterpret_cast<AddrSize*>(p+sizeof(Zone*));
		m_overhead -= EHeaderSize + *reinterpret_cast<U16*>(p+sizeof(Zone*)+sizeof(AddrSize));
		aligned_free(p);
	}

#if defined (RAD_OPT_ZONE_MEMGUARD)
	static void CheckMemGuards(void *ptr);
	static void *WriteMemGuards(void *ptr);
	void CheckGuards();
#endif

#if defined(RAD_OPT_ZONE_MEMGUARD)
	unsigned int m_frontGuard[2];
#endif
	AddrSize m_numBytes;
	AddrSize m_overhead;
	AddrSize m_numAllocs;
	AddrSize m_small;
	AddrSize m_large;
	AddrSize m_high;
	const char *m_name;
	Zone *m_parent;
	Zone *m_next;
	Zone *m_head;
#if defined(RAD_OPT_ZONE_MEMGUARD)
	unsigned int m_backGuard[2];
#endif
};

AddrSize zone_malloc_size(void *p);

void *zone_realloc(
	Zone &zone,
	void *p,
	AddrSize size,
	AddrSize headerSize=0,
	AddrSize alignment=DefaultZoneAlignment
);

void *zone_malloc(
	Zone &zone,
	AddrSize size,
	AddrSize headerSize=0,
	AddrSize alignment=DefaultZoneAlignment
);

void *zone_calloc(
	Zone &zone,
	AddrSize numElms,
	AddrSize elmSize,
	AddrSize headerSize=0,
	AddrSize alignment=DefaultZoneAlignment
);

void *safe_zone_realloc(
	Zone &zone,
	void *p,
	AddrSize size,
	AddrSize headerSize=0,
	AddrSize alignment=DefaultZoneAlignment
);

void *safe_zone_malloc(
	Zone &zone,
	AddrSize size,
	AddrSize headerSize=0,
	AddrSize alignment=DefaultZoneAlignment
);

void *safe_zone_calloc(
	Zone &zone,
	AddrSize numElms,
	AddrSize elmSize,
	AddrSize headerSize=0,
	AddrSize alignment=DefaultZoneAlignment
);

void zone_free(void *p);

template <typename T, typename Z>
class zone_allocator
{
public:
	typedef Z zone_type;
	typedef T value_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef AddrSize size_type;
	typedef SAddrSize difference_type;

	template <typename U>
	struct rebind { typedef zone_allocator<U, Z> other; };

	zone_allocator() {}
	// The following is not explicit, mimicking std::allocator [20.4.1]
	template <typename U>
	zone_allocator(const zone_allocator<U, Z> &) {}

	static pointer address(reference r) { return &r; }
	static const_pointer address(const_reference s) { return &s; }
	static size_type max_size()	{ return (std::numeric_limits<size_type>::max)(); }
	static void construct(const pointer ptr, const value_type & t) { new (ptr) T(t); }
	static void destroy(const pointer ptr)
	{
		ptr->~T();
		(void) ptr;
	}

	bool operator==(const zone_allocator &) const { return true; }
	bool operator!=(const zone_allocator &) const { return false; }

	static pointer allocate(const size_type n)
	{
		return (pointer)safe_zone_malloc(zone_type::Get(), n*sizeof(T));
	}

	static pointer allocate(const size_type n, const void * const) { return allocate(n); }

	static void deallocate(const pointer ptr, const size_type n)
	{
#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
		if (ptr == 0 || n == 0)
			return;
#endif
		zone_free(ptr);
	}
};

template <typename Z>
class zone_allocator<void, Z>
{
public:
	typedef Z zone_type;
	typedef void value_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;

	template <typename U>
	struct rebind { typedef zone_allocator<U, Z> other; };
};

// Declare a zone
// _type: the zone identifier, used with new: Class *p = new (_type) Class();
#define RAD_ZONE_DEC(_api, _type) \
	struct _api _type##_zone { \
		typedef _type##_zone T; \
		operator ::Zone &() const { return Get(); } \
		static ::Zone &Get(); \
	}; \
	typedef _type##_zone _type##T; \
	extern _api _type##T _type

// Defines a zone
// _type: the zone identifier
// _name: the zone name
// _parent: the zone parent
#define RAD_ZONE_DEF(_api, _type, _name, _parent) \
	_api _type##_zone _type; \
	::Zone &_type##_zone::Get() { static ::Zone z(_parent, _name); return z; }

RAD_ZONE_DEC(RADRT_API, ZUnknown);
RAD_ZONE_DEC(RADRT_API, ZRuntime);

void *operator new(size_t s, Zone &zone) throw();
void *operator new[](size_t s, Zone &zone) throw();
void operator delete(void *p, Zone &) throw();
void operator delete[](void *p, Zone &) throw();

#include "../PopPack.h"
#include "Zone.inl"


