// IntegerTraits.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IsInteger.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsInteger<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IntegerTraits
{
	RAD_STATIC_ASSERT(IsInteger<T>::VALUE);
	typedef void SignedType;
	typedef void UnsignedType;
	static const int NUM_BYTES = 0;
	static const int NUM_BITS = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsInteger<>
//////////////////////////////////////////////////////////////////////////////////////////

#define RADMETA_INTEGERTRAITS_SPECIALIZE(_type, _signed, _unsigned) \
	template <> \
	struct IntegerTraits<_type> \
	{ \
		typedef _signed SignedType; \
		typedef _unsigned UnsignedType; \
		static const int NUM_BYTES = sizeof(_type); \
		static const int NUM_BITS = sizeof(_type) * 8; \
	};

#define RADMETA_INTEGERTRAITS_SPECIALIZE_PAIR(_signed, _unsigned) \
	RADMETA_INTEGERTRAITS_SPECIALIZE(_signed, _signed, _unsigned) \
	RADMETA_INTEGERTRAITS_SPECIALIZE(_unsigned, _signed, _unsigned)

RADMETA_INTEGERTRAITS_SPECIALIZE_PAIR(S8, U8)
RADMETA_INTEGERTRAITS_SPECIALIZE_PAIR(S16, U16)
RADMETA_INTEGERTRAITS_SPECIALIZE_PAIR(S32, U32)
RADMETA_INTEGERTRAITS_SPECIALIZE_PAIR(S64, U64)

#undef RADMETA_INTEGERTRAITS_SPECIALIZE_PAIR
#undef RADMETA_INTEGERTRAITS_SPECIALIZE

} // meta

