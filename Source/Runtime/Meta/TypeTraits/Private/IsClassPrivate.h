// IsClassPrivate.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../IntegralConstant.h"
#include "../TypeSelect.h"
#include "../IsUnion.h"
#include "../../ICE/And.h"
#include "../../ICE/Not.h"
#include "../ConstVolatileTraits.h"


namespace meta {
namespace details {

template <typename T>
struct IsClassHelper
{
private:

	template <typename X> static TrueType Select(void (X::*) (void));
	template <typename X> static FalseType Select(...);

	typedef typename RemoveConstVolatile<T>::Type NonCVType;

	typedef typename TypeSelect<
		And<
			sizeof(Select<NonCVType>(0)) == sizeof(TrueType),
			Not<IsUnion<NonCVType>::VALUE>::VALUE
		>::VALUE,
		TrueType,
		FalseType>::Type SelectedType;

public:

	typedef SelectedType Type;
};

} // details
} // meta

