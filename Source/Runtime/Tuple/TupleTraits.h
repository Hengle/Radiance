// TupleTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "TupleDef.h"

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<>
//////////////////////////////////////////////////////////////////////////////////////////

template <>
struct TupleTraits<void, void, void, void, void, void, void, void, void, void>
{
	typedef void              TailType;
	typedef void              HeadType;
	typedef Tuple<void, void> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T0>
struct TupleTraits<T0, void, void, void, void, void, void, void, void, void>
{
	typedef void                  TailType;
	typedef T0                    HeadType;
	typedef Tuple<void, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T0, typename T1>
struct TupleTraits<T0, T1, void, void, void, void, void, void, void, void>
{
	typedef T1                             HeadType;
	typedef typename TupleTraits<T0>::Type TailType;
	typedef Tuple<TailType, HeadType>      Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T0, typename T1, typename T2>
struct TupleTraits<T0, T1, T2, void, void, void, void, void, void, void>
{
	typedef T2                                 HeadType;
	typedef typename TupleTraits<T0, T1>::Type TailType;
	typedef Tuple<TailType, HeadType>          Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3
>
struct TupleTraits<T0, T1, T2, T3, void, void, void, void, void, void>
{
	typedef typename TupleTraits<
		T0, T1, T2
	>::Type TailType;

	typedef T3                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3, T4>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4
>
struct TupleTraits<T0, T1, T2, T3, T4, void, void, void, void, void>
{
	typedef typename TupleTraits<
		T0, T1, T2, T3
	>::Type TailType;
	
	typedef T4                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3, T4, T5>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5
>
struct TupleTraits<T0, T1, T2, T3, T4, T5, void, void, void, void>
{	
	typedef typename TupleTraits<
		T0, T1, T2, T3, T4
	>::Type TailType;

	typedef T5                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3, T4, T5, T6>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6
>
struct TupleTraits<T0, T1, T2, T3, T4, T5, T6, void, void, void>
{
	typedef typename TupleTraits<
		T0, T1, T2, T3, T4, T5
	>::Type TailType;

	typedef T6                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7
>
struct TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, void, void>
{
	typedef typename TupleTraits<
		T0, T1, T2, T3, T4, T5, T6
	>::Type TailType;

	typedef T7                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8
>
struct TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8, void>
{
	typedef typename TupleTraits<
		T0, T1, T2, T3, T4, T5, T6, T7
	>::Type TailType;

	typedef T8                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// TupleTraits<T0, T1, T2, T3, T4, T5, T6, T7, T8, T9>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename T0,
	typename T1,
	typename T2,
	typename T3,
	typename T4,
	typename T5,
	typename T6,
	typename T7,
	typename T8,
	typename T9
>
struct TupleTraits
{
	typedef typename TupleTraits<
		T0, T1, T2, T3, T4, T5, T6, T7, T8
	>::Type TailType;

	typedef T9                        HeadType;
	typedef Tuple<TailType, HeadType> Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// TupleElementTraitsExtractor<TTuple, N>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTuple, int N>
struct TupleElementTraitsExtractor;

template <typename TTuple>
struct TupleElementTraitsExtractor<TTuple, 0>
{
	typedef typename TTuple::HeadType Type;

	static Type &Get(TTuple &t)
	{
		return t.Head();
	}

	static const Type &Get(const TTuple &t)
	{
		return t.Head();
	}
};

template <typename TTuple, int N>
struct TupleElementTraitsExtractor
{
	RAD_STATIC_ASSERT(N > 0);

	typedef typename TTuple::TailType                    TailType;
	typedef TupleElementTraitsExtractor<TailType, N - 1> ExtractorType;
	typedef typename ExtractorType::Type                 Type;

	static Type &Get(TTuple &t)
	{
		return ExtractorType::Get(t.Tail());
	}

	static const Type &Get(const TTuple &t)
	{
		return ExtractorType::Get(t.Tail());
	}
};

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace details
//////////////////////////////////////////////////////////////////////////////////////////

} // namespace details

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace TupleElement<TTuple, Index>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TTuple, int Index>
struct TupleElement
{
	RAD_STATIC_ASSERT(Index >= 0);
	RAD_STATIC_ASSERT(Index < TTuple::SIZE);

	typedef ::details::TupleElementTraitsExtractor<
		TTuple,
		TTuple::SIZE - (Index + 1)
	> ExtractorType;

	typedef typename ExtractorType::Type Type;

	static Type &Get(TTuple &t)
	{
		return ExtractorType::Get(t);
	}

	static const Type &Get(const TTuple &t)
	{
		return ExtractorType::Get(t);
	}
};


