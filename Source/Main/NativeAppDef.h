/*! \file NativeAppDef.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#pragma once

#include <Runtime/Base.h>
#include <Runtime/Container/ZoneVector.h>

class DisplayDevice;
typedef boost::shared_ptr<DisplayDevice> DisplayDeviceRef;
typedef zone_vector<DisplayDeviceRef, ZEngineT>::type DisplayDeviceVec;

class NativeApp;
