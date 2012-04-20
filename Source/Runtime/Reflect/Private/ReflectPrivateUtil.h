// ReflectPrivateUtil.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once


namespace reflect {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::ConstPointerArray<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
class ConstPointerArray
{
public:

	// Constructors

	ConstPointerArray();
	ConstPointerArray(const T **a);
	ConstPointerArray(const ConstPointerArray<T> &a);

	// Destructor

	~ConstPointerArray();

	// Access

	int Length() const;
	const T *operator[](int i) const;

	// Assignment

	ConstPointerArray & operator=(const ConstPointerArray<T> &a);

private:

	const T **m_array;
	int     m_length;
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::Name
//////////////////////////////////////////////////////////////////////////////////////////

class Name
{
public:

	// Constructors

	Name();
	Name(
		const char    *name,
		const wchar_t *wname
	);
	Name(const Name &n);

	// Destructor

	~Name();

	// Access

	template <typename CHAR_TYPE>
	const CHAR_TYPE *As() const;

	// Assignment

	Name &operator=(const Name &n);

private:

	const char    *m_name;
	const wchar_t *m_wname;
};

} // details
} // reflect


#include "ReflectPrivateUtil.inl"
