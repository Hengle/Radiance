// PrivateFile.h
// Platform Agnostic File System Private Functions
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntFile.h"
#include "../PushPack.h"


namespace file {
namespace details {

#if defined(RAD_OPT_DEBUG)

void AssertAliasOK(const char* alias);
void UncheckedSetAlias(UReg aliasNumber, const char* string);
void AssertCreationType( CreationType ct );
void AssertFilePath(const char* string, bool requireAlias);
void AssertExtension(const char* ext);

#endif

bool ExtractAlias(const char* path, UReg* aliasNumber, UReg* strOfs);

} // details
} // file


#include "../PopPack.h"
