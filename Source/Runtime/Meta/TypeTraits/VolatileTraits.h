// VolatileTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsVolatile<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsVolatile :
public FalseType
{
};

template <typename T>
struct IsVolatile<T volatile> :
public TrueType
{
};

template <typename T>
struct IsVolatile<T volatile &> :
public TrueType
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::RemoveVolatile<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemoveVolatile
{
	typedef T Type;
};

template <typename T>
struct RemoveVolatile<T volatile>
{
	typedef T Type;
};

template <typename T>
struct RemoveVolatile<T volatile &>
{
	typedef T &Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::AddVolatile<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct AddVolatile
{
	typedef T volatile Type;
};

template <typename T>
struct AddVolatile<T volatile>
{
	typedef T volatile Type;
};

template <typename T>
struct AddVolatile<T volatile &>
{
	typedef T volatile &Type;
};

} // meta

