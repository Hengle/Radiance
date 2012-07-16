/*! \file WinKeys.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include <Engine/KeyCodes.h>

enum {
	kVK_Extended = 0x1000000
};

KeyCode TranslateVKey(int vkey, int lparam);
