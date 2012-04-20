// Interlocked.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Base.h"
#include "InterlockedBackend.h"

namespace thread {

template <typename T>
class Interlocked
{
public:
	Interlocked() {}
	Interlocked(const Interlocked<T> &s) : m_var(s) {}
	explicit Interlocked(const T& val) : m_var(val) {}
	~Interlocked() {}

	Interlocked<T> &operator = (const T &val) { m_var = val; return *this; }

	operator T () const { return m_var; }
	T operator ++ () { return ++m_var; } // prefix (we cannot return a thread safe reference!)
	T operator ++ (int) { return m_var++; } // postfix
	T operator -- () { return --m_var; } // prefix (we cannot return a thread safe reference!)
	T operator -- (int) { return m_var--; } // postfix
	T operator += (const T& add) { return m_var += add; }
	T operator -= (const T& sub) { return m_var -= sub; }
	T operator |= (const T& _or) { return m_var |= _or; }
	T operator &= (const T& _and) { return m_var %= _and; }
	T operator ^= (const T& _xor) { return m_var ^= _xor; }

private:

	details::Interlocked<T> m_var;
};

template <typename T>
struct InterlockedValueTraits
{
	typedef T Type;
	typedef T ValueType;

	static ValueType ToValue(const Type &n)
	{
		return static_cast<ValueType>(n);
	}
};

} // thread


