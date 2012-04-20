// PrivateFunctionTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<RAD_FASTCALL Function, Arg>
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_FASTCALL_TRAITS) && !defined(RAD_OPT_CLR) // __fastcall not compatible with CLR
	#define RADMETA_CALLDECL RAD_FASTCALL
	#include "PrivatePushArgumentSpecializations.h"
	#undef RADMETA_CALLDECL
#endif // defined(RAD_FASTCALL)

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<RAD_STDCALL Function, Arg>
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_STDCALL_TRAITS)
	#define RADMETA_CALLDECL RAD_STDCALL
	#include "PrivatePushArgumentSpecializations.h"
	#undef RADMETA_CALLDECL
#endif // defined(RAD_STDCALL)

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PushArgument<RAD_ANSICALL Function, Arg>
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_ANSICALL_TRAITS)
	#define RADMETA_CALLDECL RAD_ANSICALL
	#include "PrivatePushArgumentSpecializations.h"
	#undef RADMETA_CALLDECL
#endif // defined(RAD_ANSICALL)

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::details::FunctionBaseTraits<Function, R, NumArgs>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename SubFunction, typename R, typename LastArg, int NumArgs>
struct FunctionBaseTraits
{
	RAD_STATIC_ASSERT(NumArgs >= 0);

	typedef SubFunction SubFunctionType;
	typedef typename PushArgument<
		SubFunctionType, 
		LastArg
	>::Type FunctionType;

	typedef FunctionTraits<FunctionType>            SelfType;
	typedef FunctionTraits<SubFunctionType>         SubFunctionTraitsType;
	typedef R                                       ReturnType;
	typedef LastArg                                 ArgType;
	typedef typename AddPointer<FunctionType>::Type PtrType;
	
	static const int NUM_ARGS = NumArgs;
};

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

} // namespace details

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<RAD_FASTCALL Function>
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_FASTCALL_TRAITS) && !defined(RAD_OPT_CLR) // __fastcall not compatible with CLR
	#define RADMETA_CALLDECL RAD_FASTCALL
	#include "PrivateFunctionTraitsSpecializations.h"
	#undef RADMETA_CALLDECL
#endif // defined(RAD_FASTCALL)

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<RAD_STDCALL Function>
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_STDCALL_TRAITS)
	#define RADMETA_CALLDECL RAD_STDCALL
	#include "PrivateFunctionTraitsSpecializations.h"
	#undef RADMETA_CALLDECL
#endif // defined(RAD_STDCALL)

//////////////////////////////////////////////////////////////////////////////////////////
// meta::FunctionTraits<RAD_ANSICALL Function>
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_ANSICALL_TRAITS)
	#define RADMETA_CALLDECL RAD_ANSICALL
	#include "PrivateFunctionTraitsSpecializations.h"
	#undef RADMETA_CALLDECL
#endif // defined(RAD_ANSICALL)

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PopArgument<Function>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename Function>
struct PopArgument
{
	typedef typename FunctionTraits<Function>::SubFunctionType Type;
	typedef typename FunctionTraits<Function>::ArgType         ArgType;
};

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

namespace details {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::details::ArgumentTraitsExtractor<FuncTraits, N>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename FuncTraits, int N>
struct ArgumentTraitsExtractor;

template <typename FuncTraits>
struct ArgumentTraitsExtractor<FuncTraits, 0>
{
	typedef typename FuncTraits::ArgType Type;
};

template <typename FuncTraits, int N>
struct ArgumentTraitsExtractor
{
	RAD_STATIC_ASSERT(N > 0);

	typedef typename ArgumentTraitsExtractor<
		typename FuncTraits::SubFunctionTraitsType,
		N - 1
	>::Type Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace meta::details
//////////////////////////////////////////////////////////////////////////////////////////

} // namespace details

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace meta::Argument<Function, Index>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename Function, int Index>
struct Argument
{
	typedef FunctionTraits<Function> FuncTraits;

	RAD_STATIC_ASSERT(Index >= 0);
	RAD_STATIC_ASSERT(Index < FuncTraits::NUM_ARGS);

	typedef typename meta::details::ArgumentTraitsExtractor<
		FuncTraits,
		FuncTraits::NUM_ARGS - (Index + 1)
	>::Type Type;
};

} // meta

