/*! \file GameNetworkNull.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
*/

#include RADPCH
#include "GameNetwork.h"
#include "../World/World.h"

namespace gn {

void GameNetwork::BindEventQueue(GameNetworkEventQueue &queue) {
	queue.Bind(*this);
}

void GameNetworkEventQueue::Bind(GameNetwork &network) {
	network.OnAuthenticated.Bind<GameNetworkEventQueue>(
		this, 
		&GameNetworkEventQueue::OnAuthenticated, 
		ManualReleaseEventTag
	);

	network.OnShowLeaderboard.Bind<GameNetworkEventQueue>(
		this,
		&GameNetworkEventQueue::OnShowLeaderboard,
		ManualReleaseEventTag
	);

	network.OnShowAchievements.Bind<GameNetworkEventQueue>(
		this,
		&GameNetworkEventQueue::OnShowAchievements,
		ManualReleaseEventTag
	);
}

void GameNetworkEventQueue::Dispatch(world::World &target) {
	Event::Vec pending;
	{
		Lock L(m_cs);
		pending.swap(m_events);
	}

	for (Event::Vec::iterator it = pending.begin(); it != pending.end(); ++it) {
		const Event::Ref &e = *it;
		e->Dispatch(target);
	}
}

void GameNetworkEventQueue::OnAuthenticated(const NetResult &r) {
	PostEvent(new AuthenticatedEvent(r));
}

void GameNetworkEventQueue::OnShowLeaderboard(const bool &show) {
	PostEvent(new ShowLeaderboardEvent(show));
}

void GameNetworkEventQueue::OnShowAchievements(const bool &show) {
	PostEvent(new ShowAchievementsEvent(show));
}

void GameNetworkEventQueue::PostEvent(Event *e) {
	Lock L(m_cs);
	m_events.push_back(Event::Ref(e));
}

void GameNetworkEventQueue::AuthenticatedEvent::Dispatch(world::World &target) {
	target.OnLocalPlayerAuthenticated(m_result);
}

void GameNetworkEventQueue::ShowLeaderboardEvent::Dispatch(world::World &target) {
	target.OnShowLeaderboard(m_result);
}

void GameNetworkEventQueue::ShowAchievementsEvent::Dispatch(world::World &target) {
	target.OnShowAchievements(m_result);
}

}
