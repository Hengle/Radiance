// Iterator.h
// Iterator types.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// This needs to die.
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntContainer.h"
#include "IteratorDef.h"
#include "../PushPack.h"


namespace iterator {

//////////////////////////////////////////////////////////////////////////////////////////
// Simple Iterators do not support insertion, and are used by lower level
// containers, like the NodeList, and things that do not copy objects that
// are inserted.
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
//
// Base SimpleIterator Template
//
//////////////////////////////////////////////////////////////////////////////////////////
// You cannot derive from this class, you simply need to implement the functions.
//////////////////////////////////////////////////////////////////////////////////////////
//
//class RADRT_CLASS ISimpleBaseIterator
//{
//public:
//	IBaseIterator();
//	virtual ~IBaseIterator();
//
//protected:
//
//	void Next();
//	void Prev();
//
//	void Seek(SReg ofs);
//
//	Type& Value();
//	const Type& Value() const;
//
//	ValueType const& ValueAt(UReg ofs);
//	const ValueType const& ValueAt(UReg ofs) const;
//
//	bool IsEqual(const ITERATOR& iterator) const;
//	bool IsLess(const ITERATOR& iterator) const;
//  bool IsGreater(const ITERATOR& iterator) const;
//	bool IsValid() const;
//
//	IBaseIterator& operator = (const IBaseIterator& it);
//
//// You only need to declare the friend class that you need to use.
//
//friend class RADRT_CLASS   SimpleIterator< IBaseIterator, Type >;
//friend class RADRT_CLASS BDIterator< IBaseIterator, Type >;
//friend class RADRT_CLASS RAIterator< IBaseIterator, Type >;
//
//};
//

//////////////////////////////////////////////////////////////////////////////////////////
//
// Base SimpleIterator Helper Template
//
// Used when a class wants to re-use another iterator type, but only overrides the Value() functions.
// The user of this class must implement the Value() routines.
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename IteratorType >
class BaseSimpleIteratorHelper
{
public:

	typedef BaseSimpleIteratorHelper< IteratorType > Type;

	BaseSimpleIteratorHelper();
	virtual ~BaseSimpleIteratorHelper();

	void Next();
	void Prev();

	bool IsEqual(const Type& iterator) const;
	bool IsGreater(const Type& iterator) const;
	bool IsLess(const Type& iterator) const;
	bool IsValid() const;

protected:

	IteratorType m_it;
};

//////////////////////////////////////////////////////////////////////////////////////////
// iterator::SimpleIterator< BaseIteratorType, ValueType >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Forward Only SimpleIterator
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename BaseIteratorType, typename ValueType >
class SimpleIterator
{
public:

	typedef SimpleIterator< BaseIteratorType, ValueType > Type;

	SimpleIterator();
	SimpleIterator(const Type& it);
	SimpleIterator(const BaseIteratorType& it);

	//
	// Access.
	//
	ValueType const& operator * () const;

	//
	// Forward.
	//
	Type& operator ++ ();     // prefix.
	Type  operator ++ (int);  // postfix.

	//
	// bool
	//
	operator bool   () const;
	bool operator ! () const;

	//
	// equality.
	//
	bool operator == (const Type& it) const;
	bool operator != (const Type& it) const;

	//
	// greater than
	//
	bool operator > (const Type& it) const;

	//
	// less than.
	//
	bool operator < (const Type& it) const;

protected:

	BaseIteratorType m_base;
};

//////////////////////////////////////////////////////////////////////////////////////////
// iterator::FrontInsertIterator< ContainerType >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Front Insert Iterator
//
// Requires ContainerType class to have a ValueType typedef,
// and support InsertFirst()
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename ContainerType >
class FrontInsertIterator
{
public:

	typedef FrontInsertIterator< ContainerType > Type;

	FrontInsertIterator(const Type& it);
	FrontInsertIterator(ContainerType* ct);

	//
	// Forward.
	//
	Type& operator ++ ();     // prefix.
	Type  operator ++ (int);  // postfix.

	//
	// Access.
	//
	Type& operator * ();

	//
	// Assignment
	//
	Type& operator = (const typename ContainerType::ValueType& value);

protected:

	ContainerType* m_container;
};

//////////////////////////////////////////////////////////////////////////////////////////
// iterator::BackInsertIterator< ContainerType >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Back Insert Iterator
// ---------------
//
// Requires ContainerType class to have a ValueType typedef,
// and support InsertLast().
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename ContainerType >
class BackInsertIterator
{
public:

	typedef BackInsertIterator< ContainerType > Type;

	BackInsertIterator(const Type& it);
	BackInsertIterator(ContainerType* ct);

	//
	// Forward.
	//
	Type& operator ++ ();     // prefix.
	Type  operator ++ (int);  // postfix.

	//
	// Access.
	//
	Type& operator * ();

	//
	// Assignment
	//
	Type& operator = (const typename ContainerType::ValueType& value);

protected:

	ContainerType* m_container;
};

//////////////////////////////////////////////////////////////////////////////////////////
// iterator::BDSimpleIterator< BaseIteratorType, ValueType >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Bidirectional SimpleIterator
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename BaseIteratorType, typename ValueType >
class BDSimpleIterator : public SimpleIterator< BaseIteratorType, ValueType >
{
public:

	typedef BDSimpleIterator< BaseIteratorType, ValueType > Type;

	BDSimpleIterator();
	BDSimpleIterator(const Type& it);
	BDSimpleIterator(const BaseIteratorType& it);

	//
	// Backwards.
	//
	Type& operator -- ();     // prefix.
	Type  operator -- (int);  // postfix.
};

//////////////////////////////////////////////////////////////////////////////////////////
// iterator::RASimpleIterator< BaseIteratorType, ValueType >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Random Access SimpleIterator
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename BaseIteratorType, typename ValueType >
class RASimpleIterator : public BDSimpleIterator< BaseIteratorType, ValueType >
{
public:

	typedef RASimpleIterator< BaseIteratorType, ValueType > Type;

	RASimpleIterator();
	RASimpleIterator(const Type& it);
	RASimpleIterator(const BaseIteratorType& it);

	//
	// Random step forwards.
	//
	Type  operator +  (int);
	Type& operator += (int);

	//
	// Random step backwards.
	//
	Type  operator -  (int);
	Type& operator -= (int);

	//
	// Random access.
	//
	ValueType&       operator [] (int);
	const ValueType& operator [] (int) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
//
// Iteration Callback
//
//////////////////////////////////////////////////////////////////////////////////////////
//
// You cannot derrive from this class, you simply need to implement the functions.
//
// Your version of this class can be used for the Iterate functions.
//
//////////////////////////////////////////////////////////////////////////////////////////
//
//class RADRT_CLASS IteratorCallback
//{
//public:
//
//    IteratorCallback();
//	virtual ~IteratorCallback();
//
//	Result Callback(MY_TYPE& val);
//	Result Callback(const MY_TYPE& val);
//};

//////////////////////////////////////////////////////////////////////////////////////////
//
// Iteration Functions
//
//////////////////////////////////////////////////////////////////////////////////////////

enum Result
{
	RESULT_CONTINUE,
	RESULT_STOP
};

template< typename ITERATOR, class ITERATOR_CALLBACK >
void RADRT_CALL IterateForwards(const ITERATOR& iterator, ITERATOR_CALLBACK& iteratorCallback);

template< typename ITERATOR, class ITERATOR_CALLBACK >
void RADRT_CALL IterateBackwards(const ITERATOR& iterator, ITERATOR_CALLBACK& iteratorCallback);

} // iterator


#include "../PopPack.h"
#include "Iterator.inl"
