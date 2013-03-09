/*! \file StackVector.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup container
*/

#pragma once
#include <vector>
#include "StackContainer.h"

template <typename T, typename A, size_t TNumElms>
class stackify< std::vector<T, A>, TNumElms > {
public:
	typedef stack_allocator<T, TNumElms, A> allocator_type;
	typedef std::vector<T, allocator_type> container_type;
	typedef stackify< std::vector<T, A>, TNumElms > self_type;
	typedef typename container_type::iterator iterator;
	typedef typename container_type::const_iterator const_iterator;
	typedef typename container_type::pointer pointer;
	typedef typename container_type::reference reference;
	typedef typename container_type::difference_type difference_type;
	typedef typename container_type::const_pointer const_pointer;
	typedef typename container_type::const_reference const_reference;

	stackify() : m_allocator(m_storage), m_container(m_allocator) {
		m_container.reserve(TNumElms);
	}

	stackify(const self_type &other) : m_allocator(m_storage), m_container(m_allocator) {
		m_container.reserve(TNumElms);
		m_container = other.m_container;
	}

	stackify(typename container_type::const_iterator first, typename container_type::const_iterator last) : m_allocator(m_storage), m_container(m_allocator) {
		m_container.reserve(TNumElms);
		while (first < last) {
			m_container.push_back(*first);
			++first;
		}
	}

	container_type * operator -> () {
		return &m_container;
	}

	const container_type * operator -> () const {
		return &m_container;
	}

	container_type &operator * () {
		return m_container;
	}

	const container_type &operator * () const {
		return m_container;
	}

	self_type &operator = (const self_type &other) {
		m_container = other.m_container;
		return *this;
	}

	reference operator [] (int i) {
		return m_container[i];
	}

	const_reference operator [] (int i) const {
		return m_container[i];
	}
	
private:

	typename allocator_type::buffer m_storage;
	allocator_type m_allocator;
	container_type m_container;
};
