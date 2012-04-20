// Tuple.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "TupleTraits.h"



//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<TTail, THead>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTail, typename THead>
class Tuple
{
public:

	typedef TTail               TailType;
	typedef THead               HeadType;	
	typedef Tuple<TTail, THead> SelfType;

	static const int SIZE = TailType::SIZE + 1;

	template <typename T>
	struct AddTraits
	{
		typedef Tuple<SelfType, T> Type;
	};

	template <int Index>
	struct ElementTraits
	{
		typedef typename TupleElement<SelfType, Index>::Type Type;
		typedef const typename TupleElement<SelfType, Index>::Type ConstType;
	};

	Tuple(const TTail &tail, const THead &head);
	Tuple(const SelfType &t);

	template <typename T>
	typename AddTraits<T>::Type operator +(const T &head) const;

	HeadType &Head();
	const HeadType &Head() const;

	TailType &Tail();
	const TailType &Tail() const;

	template <int Index>
	typename ElementTraits<Index>::Type &Element();

	template <int Index>
	typename ElementTraits<Index>::ConstType &Element() const;

private:

	TailType m_tail;
	HeadType m_head;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, THead>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename THead>
class Tuple<void, THead>
{
public:

	typedef Tuple<void, void>  TailType;
	typedef THead              HeadType;	
	typedef Tuple<void, THead> SelfType;

	static const int SIZE = 1;

	template <typename T>
	struct AddTraits
	{
		typedef Tuple<SelfType, T> Type;
	};

	template <int Index>
	struct ElementTraits
	{
		typedef typename TupleElement<SelfType, Index>::Type Type;
		typedef const typename TupleElement<SelfType, Index>::Type ConstType;
		static const Type Element(const SelfType &t);
	};

	Tuple(const THead &head);
	Tuple(const SelfType &v);

	template <typename T>
	typename AddTraits<T>::Type operator +(const T &head) const;

	TailType &Tail();
	const TailType &Tail() const;

	HeadType &Head();
	const HeadType &Head() const;

	template <int Index>
	typename ElementTraits<Index>::Type &Element();

	template <int Index>
	typename ElementTraits<Index>::ConstType &Element() const;

private:

	HeadType m_head;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Tuple<void, void>
//////////////////////////////////////////////////////////////////////////////////////////

template <>
class Tuple<void, void>
{
public:

	typedef void              HeadType;
	typedef void              TailType;
	typedef Tuple<void, void> SelfType;

	template <typename T>
	struct AddTraits
	{
		typedef Tuple<void, T> Type;
	};

	static const int SIZE = 0;

	Tuple();
	Tuple(const SelfType &v);

	template <typename T>
	typename AddTraits<T>::Type operator +(const T &head) const;
};



#include "Tuple.inl"
