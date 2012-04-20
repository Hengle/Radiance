// QualifierTraits.h
// Qualifier (const, volatile, reference, pointer) type traits.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ConstTraits.h"
#include "ReferenceTraits.h"
#include "PointerTraits.h"
#include "../ICE/And.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsConstReference<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsConstReference :
public BoolConstant<
	And<
		IsConst<T>::VALUE,
		IsReference<T>::VALUE
	>::VALUE
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RemoveConstReference<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemoveConstReference
{
	typedef typename RemoveConst<
		typename RemoveReference<T>::Type
	>::Type Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsConstPointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsConstPointer :
public BoolConstant<
	And<
		IsConst<T>::VALUE,
		IsPointer<T>::VALUE
	>::VALUE
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RemoveConstPointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemoveConstPointer
{
	typedef typename RemoveConst<
		typename RemovePointer<T>::Type
	>::Type Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsConstPointerReference<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsConstPointerReference :
public BoolConstant<
	And<
		IsConst<T>::VALUE,
		IsPointer<T>::VALUE,
		IsReference<T>::VALUE
	>::VALUE
>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::details::RemoveConstPointerReference<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemoveConstPointerReference
{
	typedef typename RemoveConst<
		typename RemovePointer<
			typename RemoveReference<T>::Type
		>::Type
	>::Type Type;
};

} // meta

