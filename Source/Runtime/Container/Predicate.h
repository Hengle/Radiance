// Predicate.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// This needs to die.
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../PushPack.h"


namespace predicate {

enum { LESS = -1, EQUAL = 0, GREATER = 1 };


#define RADCONTAINER_DECLARE_ADAPTER_TYPEDEFS(_ret, _type)\
	typedef _ret  RetType;\
	typedef _type ArgType

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::ImplicitCastAdapter< typename _ARG_RET_TYPE, typename _ARG_TYPE >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Transform operator, takes a source type and implicitly casts it.
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename _ARG_RET_TYPE, typename _ARG_TYPE >
struct ImplicitCastAdapter
{
	RADCONTAINER_DECLARE_ADAPTER_TYPEDEFS(_ARG_RET_TYPE, _ARG_TYPE);
	_ARG_RET_TYPE operator () (_ARG_TYPE arg);
};

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::NullAdapter< typename _ARG_RET_TYPE, typename _ARG_TYPE >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Another name for an implicit cast, however, the return type is the same as the argument type.
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename _ARG_TYPE >
struct NullAdapter : public ImplicitCastAdapter< _ARG_TYPE, _ARG_TYPE > {};

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::ExplicitCastAdapter< typename _ARG_RET_TYPE, typename _ARG_TYPE >
//////////////////////////////////////////////////////////////////////////////////////////
//
// Transform operator, takes a source type and explicitly casts it.
//
//////////////////////////////////////////////////////////////////////////////////////////

template< typename _ARG_RET_TYPE, typename _ARG_TYPE >
struct ExplicitCastAdapter
{
	RADCONTAINER_DECLARE_ADAPTER_TYPEDEFS(_ARG_RET_TYPE, _ARG_TYPE);
	_ARG_RET_TYPE operator () (_ARG_TYPE arg);
};

//////////////////////////////////////////////////////////////////////////////////////////
// predicate::Compare< typename _ARG0_TYPE, typename _ARG1_TYPE, typename _ARG0_ADAPTER, typename _ARG1_ADAPTER >
//////////////////////////////////////////////////////////////////////////////////////////

#define RADCONTAINER_DECLARE_COMPARE_TYPENAME_TYPEDEFS(_arg0adapter, _arg1adapter)\
	typedef typename _arg0adapter::RetType Adapter0RetType;\
	typedef typename _arg0adapter::ArgType Adapter0ArgType;\
	typedef typename _arg1adapter::RetType Adapter1RetType;\
	typedef typename _arg1adapter::ArgType Adapter1ArgType;\
	_arg0adapter arg0Adapter;\
	_arg1adapter arg1Adapter

#define RADCONTAINER_DECLARE_COMPARE_TYPEDEFS(_arg0adapter, _arg1adapter)\
	typedef _arg0adapter::RetType Adapter0RetType;\
	typedef _arg0adapter::ArgType Adapter0ArgType;\
	typedef _arg1adapter::RetType Adapter1RetType;\
	typedef _arg1adapter::ArgType Adapter1ArgType;\
	_arg0adapter arg0Adapter;\
	_arg1adapter arg1Adapter

template
<
	class    _ARG0_ADAPTER,
	class    _ARG1_ADAPTER
>
struct Compare
{
	RADCONTAINER_DECLARE_COMPARE_TYPENAME_TYPEDEFS(_ARG0_ADAPTER, _ARG1_ADAPTER);
	SReg operator () (const Adapter0RetType& x, const Adapter1RetType& y);
};

template
<
	class    _ARG0_ADAPTER,
	class    _ARG1_ADAPTER
>
struct ReverseCompare
{
	RADCONTAINER_DECLARE_COMPARE_TYPENAME_TYPEDEFS(_ARG0_ADAPTER, _ARG1_ADAPTER);
	SReg operator () (const Adapter0RetType& x, const Adapter1RetType& y);
};

} // predicate


#include "../PopPack.h"
#include "Predicate.inl"
