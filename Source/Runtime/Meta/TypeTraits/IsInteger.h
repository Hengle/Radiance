// IsInteger.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsInteger<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsInteger :
public FalseType
{
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsInteger<>
//////////////////////////////////////////////////////////////////////////////////////////

#define RADMETA_ISINTEGER_SPECIALIZE(_type) \
	template <> \
	struct IsInteger<_type> : \
	public TrueType \
	{ \
	};

RADMETA_ISINTEGER_SPECIALIZE(U8)
RADMETA_ISINTEGER_SPECIALIZE(U16)
RADMETA_ISINTEGER_SPECIALIZE(U32)
RADMETA_ISINTEGER_SPECIALIZE(U64)
RADMETA_ISINTEGER_SPECIALIZE(S8)
RADMETA_ISINTEGER_SPECIALIZE(S16)
RADMETA_ISINTEGER_SPECIALIZE(S32)
RADMETA_ISINTEGER_SPECIALIZE(S64)

#undef RADMETA_ISINTEGER_SPECIALIZE

} // meta

