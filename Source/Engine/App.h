// App.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Abstract Game Class
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include "Engine.h"
#include "Tickable.h"
#include "StringTable.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/TimeDef.h>
#include <Main/NativeApp.h>

class App;
class Game;
struct InputEvent;

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS App : public NativeApp
{
public:

	typedef Tickable<App> Tickable;
	
	App(int argc, const char **argv);

	virtual ~App();

	virtual bool PreInit();
	virtual int DoLauncher() = 0;
	virtual bool InitWindow() = 0;
	virtual bool Initialize();
	virtual void Finalize();
	virtual float Tick();

#if !defined(RAD_OPT_PC_TOOLS)
	virtual void MovieFinished();
	virtual void PlainTextDialogResult(bool cancel, const char *text);
#endif

	static App *Get(int argc = 0, const char **argv = 0);
	static void DestroyInstance();
	static void DumpMemStats(int level);

	StringTable::LangId LoadLangId(int *enabledLangMask, StringTable::LangId defaultLangId);

	// Tickable
	void Push(const Tickable::Ref &state);
	void Pop();
	
	void ClearFrameHistory();

#if !defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_READONLY_PROPERTY(App, game, Game*);
#endif
	RAD_DECLARE_READONLY_PROPERTY(App, engine, Engine*);
	RAD_DECLARE_READONLY_PROPERTY(App, title, const char*);
	RAD_DECLARE_READONLY_PROPERTY(App, company, const char*);
	RAD_DECLARE_READONLY_PROPERTY(App, website, const char*);
	RAD_DECLARE_READONLY_PROPERTY(App, flurryAPIKey, const char*);
	RAD_DECLARE_READONLY_PROPERTY(App, state, Tickable::Ref);
	RAD_DECLARE_READONLY_PROPERTY(App, time, float);
	RAD_DECLARE_READONLY_PROPERTY(App, langId, StringTable::LangId);
	RAD_DECLARE_PROPERTY(App, exit, bool, bool);

protected:

	virtual void OnTick(float elapsed) {}
	virtual void DoTickable(float elapsed);

#if !defined(RAD_OPT_PC_TOOLS)
	virtual RAD_DECLARE_GET(game, Game*) = 0;
#endif
	virtual RAD_DECLARE_GET(title, const char*) = 0;
	virtual RAD_DECLARE_GET(company, const char*) = 0;
	virtual RAD_DECLARE_GET(website, const char*) = 0;
	virtual RAD_DECLARE_GET(flurryAPIKey, const char *) = 0;
	
	RAD_DECLARE_GET(exit, bool) { 
		return m_exit; 
	}

	RAD_DECLARE_SET(exit, bool) { 
		m_exit = value; 
	}

	RAD_DECLARE_GET(state, Tickable::Ref) { 
		return m_tickable.state; 
	}

	RAD_DECLARE_GET(time, float) { 
		return m_time; 
	}

	RAD_DECLARE_GET(langId, StringTable::LangId) {
		return m_langId;
	}

private:

	static App *New(int argc, const char **argv); // implemented by game project.

	RAD_DECLARE_GET(engine, Engine*) { 
		return m_e; 
	}
	
	float CalcDt(float dt);
	
	typedef zone_vector<float, ZEngineT>::type FloatVec;
	
	bool m_exit;
	float m_time;
	Engine *m_e;
	TickQueue<App> m_tickable;
	xtime::TimeVal m_ticks;
	int m_frameHistoryIdx;
	StringTable::LangId m_langId;
	FloatVec m_frameHistory;

	static App *s_instance;
};
