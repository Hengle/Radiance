/*! \file GameNetworkDef.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
*/

#pragma once

#include "../Types.h"

namespace gn {

enum NetResult
{
	NR_Success,
	NR_NetworkFailed = -1
};

class GameNetwork;
typedef boost::shared_ptr<GameNetwork> GameNetworkRef;

class Player;
typedef boost::shared_ptr<Player> PlayerRef;

class LocalPlayer;
typedef boost::shared_ptr<LocalPlayer> LocalPlayerRef;

class GameNetworkEventQueue;
typedef boost::shared_ptr<GameNetworkEventQueue> GameNetworkEventQueueRef;

} // gn
