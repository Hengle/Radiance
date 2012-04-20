// Files.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Base.h>

void *LoadFile(const char *filename, int &len);
void *SafeLoadFile(const char *filename, int &len);
void FreeFileData(void *data);
