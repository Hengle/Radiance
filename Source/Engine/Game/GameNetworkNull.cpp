/*! \file GameNetworkNull.cpp
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
*/

#include RADPCH
#include "GameNetwork.h"

namespace gn {

namespace {

class NullLocalPlayer : public LocalPlayer
{
public:

	NullLocalPlayer() : m_authenticated(false)
	{
	}

	bool m_authenticated;

protected:

	virtual RAD_DECLARE_GET(authenticated, bool) { return m_authenticated; }
	virtual RAD_DECLARE_GET(id, const char *) { return "null"; }

};

class NullGameNetwork : public GameNetwork
{
public:

	NullGameNetwork(GameNetworkEventQueue *queue) : m_sessionReportOnClose(true), m_sessionReportOnPause(false)
	{
		m_localPlayer.reset(new NullLocalPlayer());

		if (queue)
			BindEventQueue(*queue);
	}

	virtual void AuthenticateLocalPlayer()
	{
		static_cast<NullLocalPlayer&>(*m_localPlayer).m_authenticated = true;
		OnAuthenticated.Trigger(NR_Success);
	}

	virtual void SendScore(const char *leaderboardId, int score)
	{
	}

	virtual void SendAchievement(const char *achievementId, float percent)
	{
	}

	virtual void ShowLeaderboard(const char *leaderboardId)
	{
	}

	virtual void ShowAchievements()
	{
	}

	virtual void LogEvent(const char *eventName, const world::Keys *optionalKey, bool timed)
	{
	}

	virtual void EndTimedEvent(const char *eventName, const world::Keys *optionalKey)
	{
	}

	virtual void LogError(const char *error, const char *message)
	{
	}

protected:

	virtual RAD_DECLARE_GET(localPlayer, const LocalPlayerRef&) { return m_localPlayer; }
	virtual RAD_DECLARE_GET(sessionReportOnAppClose, bool) { return m_sessionReportOnClose; }
	virtual RAD_DECLARE_SET(sessionReportOnAppClose, bool) { m_sessionReportOnClose = value; }
	virtual RAD_DECLARE_GET(sessionReportOnAppPause, bool) { return m_sessionReportOnPause; }
	virtual RAD_DECLARE_SET(sessionReportOnAppPause, bool) { m_sessionReportOnPause = value; }

private:

	bool m_sessionReportOnClose;
	bool m_sessionReportOnPause;

	LocalPlayer::Ref m_localPlayer;
};

}

GameNetwork::Ref GameNetwork::Create(GameNetworkEventQueue *queue)
{
	return GameNetwork::Ref(new NullGameNetwork(queue));
}

}
