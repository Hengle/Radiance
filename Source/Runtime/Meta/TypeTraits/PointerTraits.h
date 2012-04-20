// PointerTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsPointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsPointer :
public FalseType
{
};

template <typename T>
struct IsPointer<T *> :
public TrueType
{
};

template <typename T>
struct IsPointer<T *&> :
public TrueType
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::RemovePointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct RemovePointer
{
	typedef T Type;
};

template <typename T>
struct RemovePointer<T *>
{
	typedef T Type;
};

template <typename T>
struct RemovePointer<T *&>
{
	typedef T &Type;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::AddPointer<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct AddPointer
{
	typedef T *Type;
};

template <typename T>
struct AddPointer<T *>
{
	typedef T *Type;
};

template <typename T>
struct AddPointer<T *&>
{
	typedef T *&Type;
};

} // meta

