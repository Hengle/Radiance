// SCC.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Base.h>
#include "SCC.h"
#include "SCC_null.h"

namespace {

typedef SCC *(*CreateFn)(void);
struct SccReg
{
	const char *name;
	CreateFn create;
};


SccReg s_regs[] =
{
	{ "null", &SCC_null::Create },
	{ 0, 0 }
};

} // namespace

SCC::Ref SCC::Create(const char *name)
{
	RAD_ASSERT(name);
	Ref ref;

	for (SccReg *x = s_regs; x->name && x->create; ++x)
	{
		if (!strcmp(name, x->name))
		{
			ref.reset(x->create());
			break;
		}
	}

	return ref;
}
