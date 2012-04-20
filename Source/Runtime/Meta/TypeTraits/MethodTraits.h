// MethodTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "FunctionTraits.h"
#include "MemberPointerTraits.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<Function, C>
//////////////////////////////////////////////////////////////////////////////////////////
//
// Type parameters:
//   Function - function type (signature)
//   C        - object type
//
// Result types:
//   ReturnType - return type of method
//   PtrType    - pointer type of method
//   ObjectType - object type (same as C parameter)
//
// Result values:
//   NUM_ARGS   - number of arguments to method
//
//////////////////////////////////////////////////////////////////////////////////////////

template <typename Function, typename C>
struct MethodTraits;

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodBaseTraits<Function, C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename Function, typename C>
struct MethodBaseTraits :
public FunctionTraits<Function>
{
	typedef MethodTraits<Function, C> SelfType;
	typedef FunctionTraits<Function>  FunctionTraitsType;
	typedef C                         ObjectType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

} // namespace details

#define PRIVATE_RADMETA_EXPAND_METHOD_TRAITS\
	typedef typename SuperType::SelfType SelfType;\
	typedef typename SuperType::FunctionTraitsType FunctionTraitsType;\
	typedef typename SuperType::ObjectType ObjectType;\
	typedef typename FunctionTraitsType::FunctionType FunctionType;\
	typedef typename FunctionTraitsType::SubFunctionTraitsType SubFunctionTraitsType;\
	typedef typename FunctionTraitsType::ReturnType ReturnType;\
	typedef typename FunctionTraitsType::ArgType ArgType;\
	static const int NUM_ARGS = FunctionTraitsType::NUM_ARGS;

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename C>
struct MethodTraits<R (), C>/*:
public details::MethodBaseTraits<R (), C>*/
{
	typedef details::MethodBaseTraits<R (), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType) ();
};

template <typename R, typename C>
struct MethodTraits<R (), C const>
{
	typedef details::MethodBaseTraits<R (), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType) () const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0, typename C>
struct MethodTraits<R (A0), C>
{
	typedef details::MethodBaseTraits<R (A0), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0);
};

template <typename R, typename A0, typename C>
struct MethodTraits<R (A0), C const>
{
	typedef details::MethodBaseTraits<R (A0), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename R, typename A0, typename A1, typename C>
struct MethodTraits<R (A0, A1), C>
{
	typedef details::MethodBaseTraits<R (A0, A1), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1);
};

template <typename R, typename A0, typename A1, typename C>
struct MethodTraits<R (A0, A1), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename C
>
struct MethodTraits<R (A0, A1, A2), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename C
>
struct MethodTraits<R (A0, A1, A2), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename A19,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename A19,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20), C>
//////////////////////////////////////////////////////////////////////////////////////////

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename A19,
	typename A20,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20), C>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20), C> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20);
};

template <
	typename R,
	typename A0,
	typename A1,
	typename A2,
	typename A3,
	typename A4,
	typename A5,
	typename A6,
	typename A7,
	typename A8,
	typename A9,
	typename A10,
	typename A11,
	typename A12,
	typename A13,
	typename A14,
	typename A15,
	typename A16,
	typename A17,
	typename A18,
	typename A19,
	typename A20,
	typename C
>
struct MethodTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20), C const>
{
	typedef details::MethodBaseTraits<R (A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20), C const> SuperType;
	PRIVATE_RADMETA_EXPAND_METHOD_TRAITS

	typedef R (C::*PtrType)(A0, A1, A2, A3, A4, A5, A6, A7, A8, A9, A10, A11, A12, A13, A14, A15, A16, A17, A18, A19, A20) const;
};

} // meta

