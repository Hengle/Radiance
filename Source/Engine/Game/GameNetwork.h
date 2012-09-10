/*! \file GameNetwork.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
*/

#pragma once
#include "GameNetworkDef.h"
#include "../World/WorldDef.h"
#include "../World/Keys.h"
#include <Runtime/Event.h>
#include <Runtime/Container/ZoneList.h>
#include <Runtime/Thread/Locks.h>

namespace gn {

//! Interface class for a game network (GameCenter etc).
class GameNetwork
{
	RAD_EVENT_CLASS(EventNoAccess);
public:
	typedef GameNetworkRef Ref;

	//! Creates a network management object. May return NULL if network is unavailable.
	/*! \param queue Optional event queue. */
	static Ref Create(GameNetworkEventQueue *queue);

	//! Authenticates the local player. Response via OnAuthenticated
	virtual void AuthenticateLocalPlayer() = 0;

	typedef Event<NetResult, EventNoAccess> AuthenticatedEvent;
	AuthenticatedEvent OnAuthenticated;

	//! Reports a score update.
	virtual void SendScore(const char *leaderboardId, int score) = 0;

	//! Reports an achievement update.
	virtual void SendAchievement(const char *achievementId, float percent) = 0;

	//! Display leader board
	virtual void ShowLeaderboard(const char *leaderboardId) = 0;

	typedef Event<bool, EventNoAccess> ShowLeaderboardEvent;
	ShowLeaderboardEvent OnShowLeaderboard;

	//! Display achievements.
	virtual void ShowAchievements() = 0;

	typedef Event<bool, EventNoAccess> ShowAchievementsEvent;
	ShowAchievementsEvent OnShowAchievements;

	// Flurry Integration
	virtual void LogEvent(const char *eventName, const world::Keys *optionalKeys = 0, bool timed = false) = 0;
	virtual void EndTimedEvent(const char *eventName, const world::Keys *optionalKeys = 0) = 0;
	virtual void LogError(const char *error, const char *message) = 0;

	RAD_DECLARE_PROPERTY(GameNetwork, sessionReportOnAppClose, bool, bool);
	RAD_DECLARE_PROPERTY(GameNetwork, sessionReportOnAppPause, bool, bool);

	//! localPlayer is *always* a valid object even if not-authenticated
	RAD_DECLARE_READONLY_PROPERTY(GameNetwork, localPlayer, const LocalPlayerRef&);

protected:

	void BindEventQueue(GameNetworkEventQueue &queue);

	virtual RAD_DECLARE_GET(localPlayer, const LocalPlayerRef&) = 0;
	virtual RAD_DECLARE_GET(sessionReportOnAppClose, bool) = 0;
	virtual RAD_DECLARE_SET(sessionReportOnAppClose, bool) = 0;
	virtual RAD_DECLARE_GET(sessionReportOnAppPause, bool) = 0;
	virtual RAD_DECLARE_SET(sessionReportOnAppPause, bool) = 0;

};

//! Player object
class Player
{
public:
	typedef PlayerRef Ref;

	RAD_DECLARE_READONLY_PROPERTY(Player, id, const char *);

protected:

	virtual RAD_DECLARE_GET(id, const char *) = 0;
};

//! Local player
class LocalPlayer : public Player
{
public:
	typedef LocalPlayerRef Ref;

	RAD_DECLARE_READONLY_PROPERTY(LocalPlayer, authenticated, bool);

protected:

	virtual RAD_DECLARE_GET(authenticated, bool) = 0;
};

//! Network dispatch queue
class GameNetworkEventQueue
{
public:
	typedef GameNetworkEventQueueRef Ref;

	void Dispatch(world::World &target);

private:

	friend class GameNetwork;

	void Bind(GameNetwork &network);
	void OnAuthenticated(const NetResult &r);
	void OnShowLeaderboard(const bool &show);
	void OnShowAchievements(const bool &show);

	////////////////////////////////////////////////////////

	typedef boost::mutex Mutex;
	typedef boost::lock_guard<Mutex> Lock;

	class Event
	{
	public:
		virtual ~Event() {}
		typedef boost::shared_ptr<Event> Ref;
		typedef zone_list<Ref, ZWorldT>::type List;
		virtual void Dispatch(world::World &target) = 0;
	};

	class AuthenticatedEvent : public Event
	{
	public:
		AuthenticatedEvent(NetResult r) : m_result(r) {}
		virtual void Dispatch(world::World &target);

	private:

		NetResult m_result;
	};

	class ShowLeaderboardEvent : public Event
	{
	public:
		ShowLeaderboardEvent(bool r) : m_result(r) {}
		virtual void Dispatch(world::World &target);

	private:

		bool m_result;
	};
	
	class ShowAchievementsEvent : public Event
	{
	public:
		ShowAchievementsEvent(bool r) : m_result(r) {}
		virtual void Dispatch(world::World &target);

	private:

		bool m_result;
	};

	void PostEvent(Event *e);

	Event::List m_events;
	Mutex m_cs;
};

} // gn
