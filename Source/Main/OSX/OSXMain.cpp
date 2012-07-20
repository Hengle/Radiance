/*! \file OSXMain.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include "../QtAppMain.h"

extern "C" int main(int argc, char *argv[]) {
	return QtAppMain(argc, (const char **)argv);
}
