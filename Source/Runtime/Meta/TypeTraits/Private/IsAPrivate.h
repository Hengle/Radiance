// IsAPrivate.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../IntegralConstant.h"
#include "../TypeSelect.h"
#include "../TypeOf.h"
#include "../IsClass.h"
#include "../ConstVolatileTraits.h"

// This is all more or less pulled from boost, way back when we
// were trying not to use 3rd party stuff

namespace meta {
namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsAHelper<TDerived, TBase>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename X, typename Y>
struct IsAHelper_IsDerived
{
	template <typename Z> static TrueType Select(X const volatile *, Z);
	static FalseType Select(Y const volatile *, int);

	struct ChooseConversion
	{
		operator Y const volatile * () const;
		operator X const volatile * ();
	};

	static const bool VALUE = sizeof(Select(ChooseConversion(), 0)) == sizeof(TrueType);
	// VC BUG?
	//typedef BoolConstant<sizeof(Select(ChooseConversion(), 0)) == sizeof(TrueType)> BoolVal;

	typedef typename TypeSelect
		<
			VALUE,
			TrueType,
			FalseType
		>::Type Type;
};

template <bool DerivedIsClass, bool BaseIsClass, bool Same>
struct IsAHelper_Select
{
	template <typename X, typename Y>
	struct Rebind
	{
		typedef X _x;
		typedef Y _y;
		typedef FalseType Type;
	};
};

template <>
struct IsAHelper_Select<true, true, false>
{
	template <typename X, typename Y>
	struct Rebind
	{
		typedef X _x;
		typedef Y _y;
		typedef typename RemoveConstVolatile<X>::Type NonCVDerivedType;
		typedef typename RemoveConstVolatile<Y>::Type NonCVDBaseType;
		typedef typename IsAHelper_IsDerived<NonCVDerivedType, NonCVDBaseType>::Type Type;
	};
};

template <>
struct IsAHelper_Select<true, true, true>
{
	template <typename X, typename Y>
	struct Rebind
	{
		typedef TrueType Type;
		typedef X _x;
		typedef Y _y;
	};
};

template <typename TDerived, typename TBase>
class IsAHelper
{
public:

	typedef typename IsAHelper_Select<IsClass<TDerived>::VALUE, IsClass<TBase>::VALUE, SameTypes<TDerived, TBase>::VALUE>::template Rebind<TDerived, TBase>::Type Type;
};

} // details
} // meta

