/*! \file StackContainer.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup container
*/

#pragma once

#include "IntContainer.h"
#include <memory>

template <typename T, size_t TNumElms, typename TFallbackAllocator >
class stack_allocator : public TFallbackAllocator {
public:
	typedef T value_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef AddrSize size_type;
	typedef SAddrSize difference_type;
	typedef TFallbackAllocator fallback_allocator;

	class buffer {
	public:
		buffer() : used(false) {}
	private:
		friend class stack_allocator<T, TNumElms, fallback_allocator >;
		typedef aligned_block<sizeof(T[TNumElms]), RAD_ALIGNOF(T)> block_type;
		block_type block;
		bool used;
	};

	template <typename U>
	struct rebind { typedef stack_allocator<U, TNumElms, typename fallback_allocator::rebind<U>::other > other; };

	stack_allocator(const stack_allocator &other) : m_storage(other.m_storage) {
	}

	explicit stack_allocator(buffer &storage) : m_storage(&storage) {
	}
	
	// The following is not explicit, mimicking std::allocator [20.4.1]
	template <typename U>
	stack_allocator(const stack_allocator<U, TNumElms, typename fallback_allocator::rebind<U>::other> &) : m_storage(0) {
	}

	static pointer address(reference r) { return &r; }
	static const_pointer address(const_reference s) { return &s; }
	static size_type max_size()	{ return (std::numeric_limits<size_type>::max)(); }
	static void construct(const pointer ptr, const value_type & t) { new (ptr) T(t); }
	static void destroy(const pointer ptr) {
		ptr->~T();
		(void) ptr;
	}

	bool operator==(const stack_allocator &) const { return true; }
	bool operator!=(const stack_allocator &) const { return false; }

	pointer allocate(const size_type n, const void * const) { 
		if (m_storage && !m_storage->used && (n <= TNumElms)) {
			m_storage->used = true;
			return (pointer)m_storage->block.data;
		} else {
			return TFallbackAllocator::allocate(n, 0);
		}
	}

	pointer allocate(const size_type n) {
		return allocate(n, 0);
	}

	void deallocate(const pointer ptr, const size_type n) {
#ifdef BOOST_NO_PROPER_STL_DEALLOCATE
		if (ptr == 0 || n == 0)
			return;
#endif
		if (m_storage && (((const pointer)m_storage->block.data) == ptr)) {
			m_storage->used = false;
		} else {
			TFallbackAllocator::deallocate(ptr, n);
		}
	}

private:

	buffer *m_storage;
};

//! Creates a version of the specified container which allocates memory from a buffer on the stack.
/*! STL containers with a short lifetime or who only address a fixed number of elements may benefit
    from performing allocations from a fixed size buffer located on the stack.

	This specialization is limited to random access containers. If a containers size exceeds the
	static limit specified (by TNumElms) then the container allocation moves to the heap.

	Currently only std::vector is supported.

	USAGE: \code{.cpp}

	stackify< std::vector<int>, 1024 > StackInts;
	StackInts ints;

	for (int i = 0; i < 1024; ++i)
		ints->push_back(1024);

	\endcode

	\sa StackVector.h
*/
template <typename TContainer, size_t TNumElms>
class stackify;
