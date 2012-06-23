// SharedLibrary.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Base.h"
#include "../PushPack.h"

class RADRT_CLASS SharedLibrary
{
public:
	SharedLibrary() : m_h(0) {}
	~SharedLibrary() { Unload(); }

	// you must supply the platform native filename. you can use the pml file system to
	// translate this, or provide your own way.

	bool Load(const char *nativeFilename, bool reportErrors);
	bool Loaded() { return m_h != 0; }

	void Unload();
	void *ProcAddress(const char* procName);

private:

	void *m_h;
};

#include "../PopPack.h"
