// PredicateDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once


namespace predicate {

template< typename, typename > struct ImplicitCastAdapter;
template< typename > struct NullAdapter;
template< typename, typename > struct ExplicitCastAdapter;

template
<
	class,
	class
>
struct Compare;

template
<
	class,
	class
>
struct ReverseCompare;

} // predicate

