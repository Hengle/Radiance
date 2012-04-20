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

void AssertAliasOK(const wchar_t* alias);
void UncheckedSetAlias(UReg aliasNumber, const wchar_t* string);
void AssertCreationType( CreationType ct );
void AssertFilePath(const wchar_t *string, bool requireAlias);
void AssertExtension(const wchar_t *ext);

#endif

bool ExtractAlias(const wchar_t* path, UReg* aliasNumber, UReg* strOfs);

} // details
} // file


#include "../PopPack.h"
