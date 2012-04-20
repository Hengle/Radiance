// COut.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Console Print
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include <iostream>

enum COutLevel
{
	C_Debug,
	C_Info,
	C_Warn,
	C_Error,
	C_ErrMsgBox, // No Msg box on non-pc targets, becomes same as C_Error
	C_Max
};

RADENG_API std::ostream &RADENG_CALL COut(int level);
