// IsA.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"
#include "TypeSelect.h"
#include "Private/IsAPrivate.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsClass<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename TDerived, typename TBase>
struct IsA : public details::IsAHelper<TDerived, TBase>::Type
{
};

} // meta

