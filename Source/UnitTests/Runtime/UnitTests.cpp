// UnitTests.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../UTCommon.h"
#include <string>

#if defined(RAD_OPT_WIN)
#if !defined(RAD_TARGET_GOLDEN)
#include <VLD/vld.h> // VLD only in non-golden builds.
#endif
#endif

namespace ut
{
    void StringTest();
	void ReflectTest();
	void FileTest();
	void SIMDTest();
}

int main(int argc, const char **argv)
{
	rt::Initialize();

    INIT();
	std::string testToRun;

	if (argc > 1) { testToRun = argv[1]; }

    RUN("StringTest", ut::StringTest());
	RUN("ReflectTest", ut::ReflectTest());
	RUN("FileTest", ut::FileTest());
	RUN("SIMDTest", ut::SIMDTest());

    rt::Finalize();

    END();
}
