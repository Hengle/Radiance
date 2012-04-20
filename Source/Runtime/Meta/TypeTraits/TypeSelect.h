// TypeSelect.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::TypeSelect<B, T1, T2>
//////////////////////////////////////////////////////////////////////////////////////////

template <bool B, typename T1, typename T2>
struct TypeSelect;

template <bool B, typename T1, typename T2>
struct TypeSelect
{
	typedef T1 Type;
};

template <typename T1, typename T2>
struct TypeSelect<false, T1, T2>
{
	typedef T2 Type;
};

} // meta

