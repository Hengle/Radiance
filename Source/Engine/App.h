// App.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Abstract Game Class
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include "Engine.h"
#include "Tickable.h"
#include <Runtime/Container/ZoneVector.h>
#include <Runtime/TimeDef.h>

class App;
class Game;
struct InputEvent;

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS App
{
public:

	typedef Tickable<App> Tickable;

	App();
	virtual ~App();

	static App *Get();
	static void DestroyInstance();
	static void DumpMemStats(int level);

	virtual bool PreInit();
	virtual bool Initialize();
	virtual void Finalize();
	virtual void Run();
	virtual void NotifyBackground(bool background=true) {}
	virtual void PostInputEvent(const InputEvent &e) {}
	float Tick();

	// Tickable
	void Push(const Tickable::Ref &state);
	void Pop();
	
	void ClearFrameHistory();
	
#if defined(RAD_OPT_PC)
	virtual void CloseWindow() = 0; // occurs if the user tries to close the game window.
	virtual void FocusChange(bool focus) = 0;
#endif

#if defined(RAD_OPT_GOLDEN) || defined(RAD_OPT_IOS)
	RAD_DECLARE_READONLY_PROPERTY(App, game, Game*);
#endif
	RAD_DECLARE_READONLY_PROPERTY(App, engine, Engine*);
	RAD_DECLARE_READONLY_PROPERTY(App, title, const wchar_t*);
	RAD_DECLARE_READONLY_PROPERTY(App, company, const wchar_t*);
	RAD_DECLARE_READONLY_PROPERTY(App, website, const wchar_t*);
	RAD_DECLARE_READONLY_PROPERTY(App, flurryAPIKey, const char*);
	RAD_DECLARE_READONLY_PROPERTY(App, allowMultipleInstances, bool);
	RAD_DECLARE_READONLY_PROPERTY(App, state, Tickable::Ref);
	RAD_DECLARE_READONLY_PROPERTY(App, time, float);
	RAD_DECLARE_PROPERTY(App, exit, bool, bool);

#if defined(RAD_OPT_PC_TOOLS)
	RAD_DECLARE_READONLY_PROPERTY(App, editor, bool);
	RAD_DECLARE_READONLY_PROPERTY(App, wantEditor, bool);
	virtual void SwitchEditorMode(bool editor) = 0;
	virtual void EditorStart() = 0;
#endif

protected:

	virtual void OnTick(float elapsed) {}
	virtual void DoTickable(float elapsed);

#if defined(RAD_OPT_GOLDEN) || defined(RAD_OPT_IOS)
	virtual RAD_DECLARE_GET(game, Game*) = 0;
#endif
	virtual RAD_DECLARE_GET(title, const wchar_t*) = 0;
	virtual RAD_DECLARE_GET(company, const wchar_t*) = 0;
	virtual RAD_DECLARE_GET(website, const wchar_t*) = 0;
	virtual RAD_DECLARE_GET(allowMultipleInstances, bool) = 0;
	virtual RAD_DECLARE_GET(flurryAPIKey, const char *) = 0;
	RAD_DECLARE_GET(exit, bool) { return m_exit; }
	RAD_DECLARE_SET(exit, bool) { m_exit = value; }
	RAD_DECLARE_GET(state, Tickable::Ref) { return m_tickable.state; }
	RAD_DECLARE_GET(time, float) { return m_time; }

#if defined(RAD_OPT_PC_TOOLS)
	virtual RAD_DECLARE_GET(wantEditor, bool) = 0;
	virtual RAD_DECLARE_GET(editor, bool) { return m_editor; }
#endif

private:

	static App *New(); // implemented by game project.

	RAD_DECLARE_GET(engine, Engine*) { return m_e; }

#if defined(RAD_OPT_PC_TOOLS)
	bool m_editor;
	friend void SyncEditorMode(App &game);
#endif
	
	float CalcDt(float dt);
	
	typedef zone_vector<float, ZEngineT>::type FloatVec;
	
	bool m_exit;
	float m_time;
	Engine *m_e;
	TickQueue<App> m_tickable;
	xtime::TimeVal m_ticks;
	int m_frameHistoryIdx;
	FloatVec m_frameHistory;

	static App *s_instance;
};