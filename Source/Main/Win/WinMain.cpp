/*! \file WinMain.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#if !defined(RAD_OPT_SHIP)
#include <VLD/vld.h>
#endif

#if defined(RAD_OPT_PC_TOOLS)
#include "../QtAppMain.h"
#include <process.h>

namespace {

std::string ExeName() {
	char name[256];
	int x = ::GetModuleFileNameA(0, name, 255);
	name[x] = 0; // terminate.
	return name;
}

bool IsDotCom() {
	std::string x = ExeName();
	return x.length() > 4 && !::stricmp(&x[x.length()-4], ".com");
}

void SpawnSelf() {
	char str[256];
	::strcpy(str, ExeName().c_str());
	str[::strlen(str)-4] = 0;
	::strcat(str, ".exe");
	::_spawnl(_P_DETACH, str, str, NULL);
}

}

int main(int argc, const char **argv) {
	if (IsDotCom()) {
		SpawnSelf();
		return 0;
	}
	return QtAppMain(argc, argv);
}

#endif
