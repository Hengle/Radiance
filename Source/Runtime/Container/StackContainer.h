/*! \file StackContainer.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup container
*/

#pragma once

#include "IntContainer.h"
#include <memory>

template <typename T, size_t TNumElms>
class stack_allocator : public std::allocator<T> {
public:
	typedef T value_type;
	typedef value_type *pointer;
	typedef const value_type *const_pointer;
	typedef value_type &reference;
	typedef const value_type &const_reference;
	typedef AddrSize size_type;
	typedef SAddrSize difference_type;

	class buffer {
	public:
		buffer() : used(false) {}
	private:
		friend class stack_allocator<T, TNumElms>;
		typedef aligned_block<sizeof(T[TNumElms]), RAD_ALIGNOF(T)> block_type;
		block_type block;
		bool used;
	};

	template <typename U>
	struct rebind { typedef stack_allocator<U, TNumElms> other; };

	stack_allocator(const stack_allocator &other) : m_storage(other.m_storage) {
	}

	explicit stack_allocator(buffer &storage) : m_storage(&storage) {
	}
	
	// The following is not explicit, mimicking std::allocator [20.4.1]
	template <typename U>
	stack_allocator(const stack_allocator<U, TNumElms> &) : m_storage(0) {
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
			return std::allocator<T>::allocate(n, 0);
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
			std::allocator<T>::deallocate(ptr, n);
		}
	}

private:

	buffer *m_storage;
};

template <typename __unused__>
struct stackify_container;

#define STACKIFY_TYPE(_type, _name, _numElms) typedef stackify_container< _type >::container< _numElms >::Type _name

#define STACKIFY(_type, _var, _numElms) \
	stack_allocator< stackify_container< _type >::ElementType, _numElms >::buffer _var##_storage; \
	stackify_container< _type >::allocator< _numElms >::Type _var##_allocator(_var##_storage); \
	stackify_container< _type >::container< _numElms >::Type _var(_var##_allocator); \
	_var.reserve(_numElms)
