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

namespace plat {

enum OSType {
	kOSType_Windows,
	kOSType_OSX,
	kOSType_iOS
};

enum DeviceFamily {
	kDeviceFamily_PC,
	kDeviceFamily_iPhone,
	kDeviceFamily_iPod,
	kDeviceFamily_iPad,
	kDeviceFamily_Uknown
};

enum DeviceType {
	kDeviceType_PC,
	kDeviceType_iPhone1,
	kDeviceType_iPhone3,
	kDeviceType_iPhone3GS,
	kDeviceType_iPhone4,
	kDeviceType_iPhone4S,
	kDeviceType_iPhone5,
	kDeviceType_iPod1,
	kDeviceType_iPod2,
	kDeviceType_iPod3,
	kDeviceType_iPod4,
	kDeviceType_iPad1,
	kDeviceType_iPad2,
	kDeviceType_iPad3,
	kDeviceType_Unknown = 0xff
};

#define RAD_MAKE_OS_VERSION(_major, _minor, _revision) \
	((_major<<16) | (_minor<<8) | _revision)
	
} // plat