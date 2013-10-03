/*! \file DebugCommon.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once
#include "../Types.h"
#include <Runtime/Stream/MemoryStream.h>

namespace tools {

enum {
	kDebugConsoleNetServerId = RAD_FOURCC_LE('r', 'r', 'd', 's'),
	kDebugConsoleNetServerVersion = 1,
	kDebugConsoleNetPort = 33331,
	kDebugConsoleNetBroadcastPort = kDebugConsoleNetPort + 1,
	kDebugConsoleNetMaxCommandLen = 4*kMeg,
	kDebugConsoleBroadcastPacketSize = 512,
	kDebugConsoleServerBroadcastFreq = 250,
	kDebugConsoleServerExpiry = kDebugConsoleServerBroadcastFreq*8
};

enum DebugConsoleNetMessageId {
	kDebugConsoleNetMessageId_Broadcast,
	kDebugConsoleNetMessageId_Log,
	kDebugConsoleNetMessageId_Cmd,
	kDebugConsoleNetMessageId_GetCVarList
};

}