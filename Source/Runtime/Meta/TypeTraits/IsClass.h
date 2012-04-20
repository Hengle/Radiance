// IsClass.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../IntegralConstant.h"
#include "TypeSelect.h"
#include "Private/IsClassPrivate.h"


namespace meta {

//////////////////////////////////////////////////////////////////////////////////////////
// meta::IsClass<T>
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct IsClass : public details::IsClassHelper<T>::Type
{
};

} // meta

