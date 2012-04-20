// IsUnion.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"
#include "TypeSelect.h"


namespace meta {

template <typename T>
struct IsUnion : public TypeSelect<RAD_TT_IS_UNION(T), TrueType, FalseType>::Type
{
};

} // meta

