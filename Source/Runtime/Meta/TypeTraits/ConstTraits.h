// ConstTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsConst<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsConst :
public FalseType
{
};

template <typename T>
struct IsConst<T const> :
public TrueType
{
};

template <typename T>
struct IsConst<T const &> :
public TrueType
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::RemoveConst<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemoveConst
{
	typedef T Type;
};

template <typename T>
struct RemoveConst<T const>
{
	typedef T Type;
};

template <typename T>
struct RemoveConst<T const &>
{
	typedef T &Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::AddConst<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct AddConst
{
	typedef T const Type;
};

template <typename T>
struct AddConst<T const>
{
	typedef T const Type;
};

template <typename T>
struct AddConst<T const &>
{
	typedef T const &Type;
};

} // meta

