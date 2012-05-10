// Attributes.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "ReflectDef.h"
#include "../PushPack.h"


namespace reflect {

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::Enum
//////////////////////////////////////////////////////////////////////////////////////////

class Enum
{
	RADREFLECT_EXPOSE_PRIVATES(Enum)
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::EnumFlags
//////////////////////////////////////////////////////////////////////////////////////////

class EnumFlags
{
	RADREFLECT_EXPOSE_PRIVATES(EnumFlags)
};

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::EnumValue
//////////////////////////////////////////////////////////////////////////////////////////

class EnumValue
{
	RADREFLECT_EXPOSE_PRIVATES(EnumValue)

public:

	EnumValue(int value) { m_value = value; }

	RAD_DECLARE_READONLY_PROPERTY(EnumValue, value, int);

private:

	RAD_DECLARE_GET(value, int) { return m_value; }

	int m_value;
};

} // namespace reflect


#include "../PopPack.h"

RADREFLECT_DECLARE(RADRT_API, reflect::Enum)
RADREFLECT_DECLARE(RADRT_API, reflect::EnumFlags)
RADREFLECT_DECLARE(RADRT_API, reflect::EnumValue)