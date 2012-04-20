// WinInterface.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#define PRIVATE_RAD_DECLARE_INTERFACE(_i) class _i
#define PRIVATE_RAD_INTERFACE(_api, _i) class __declspec(novtable) _api _i
