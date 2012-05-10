// PredicatedAdd.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PredicatedAdd<true, Value, Add>
//////////////////////////////////////////////////////////////////////////////////////////
//
// Result is an IntConstant
//
//////////////////////////////////////////////////////////////////////////////////////////

template <bool Predicate, int Value, int Add>
struct PredicatedAdd;

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PredicatedAdd<true, Value, Add>
//////////////////////////////////////////////////////////////////////////////////////////

template <bool Predicate, int Value, int Add>
struct PredicatedAdd :
public IntConstant<Value + Add>
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::PredicatedAdd<false, Value, Add>
//////////////////////////////////////////////////////////////////////////////////////////

template <int Value, int Add>
struct PredicatedAdd<false, Value, Add> :
public IntConstant<Value>
{
};

} // meta
