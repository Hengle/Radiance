/*! \file Console.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup engine
*/

#pragma once

#include "Types.h"
#include <Runtime/PushPack.h>

class CVarZone;

//! Console executes console commands, sort of like a bash shell which can get/set cvars
/*! The console and cvars are super dumb, they write out responses to the console using COut. */
class Console : public boost::noncopyable {
public:

	static void Exec(
		const char *cmd, 
		CVarZone *cvars
	);

	static void ListCVars(CVarZone *cvars);

private:

	Console();

};

#include <Runtime/PopPack.h>
