/*! \file OSXMain.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include "../QtAppMain.h"
#include <string.h>
#include <unistd.h>

namespace {
void up(const char *exe, int num)
{
	char path[1024];
	strcpy(path, exe);
	size_t l = strlen(path)-1;
		
	while (l-- > 0 && num > 0) {
		if (path[l] == '/') {
			path[l] = 0;
			--num;
		}
	}
	
	chdir(path);
}
}

extern "C" int main(int argc, char *argv[]) {
	
	if (argc >= 2 && !strncmp(argv[1], "-psn", 4)) {
		// launched from finder, set our cwd to be the folder
		// that contains our bundle
		up(argv[0], 2);
	}
											
											
	return QtAppMain(argc, (const char **)argv);
}
