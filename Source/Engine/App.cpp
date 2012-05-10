// Game.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Abstract Game Class
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "App.h"
#include "SkAnim/SkAnim.h"
#include <Runtime/Time.h>

//#define FRAME_SMOOTH
#if defined(FRAME_SMOOTH)
enum { FrameHistorySize = 15 };
#endif

App *App::s_instance = 0;

App *App::Get()
{
	if (!s_instance)
		s_instance = New();
	return s_instance;
}

void App::DestroyInstance()
{
	if (s_instance)
		delete s_instance;
	s_instance = 0;
}

void App::DumpMemStats(int level)
{
	SizeBuffer buf;
	
	COut(level) << "MemStats: " << std::endl;
	FormatSize(buf, ZRuntime.Get().totalBytes);
	COut(level) << "  Runtime: " << buf << std::endl;
	FormatSize(buf, ZEngine.Get().totalBytes);
	COut(level) << "    Engine: " << buf << std::endl;
	FormatSize(buf, r::ZRender.Get().totalBytes);
	COut(level) << "      Renderer: " << buf << std::endl;
	FormatSize(buf, r::ZTextures.Get().totalBytes);
	COut(level) << "        Textures: " << buf << std::endl;
	FormatSize(buf, r::ZVertexBuffers.Get().totalBytes);
	COut(level) << "        VertexBuffers: " << buf << std::endl;
	FormatSize(buf, r::ZSkm.Get().totalBytes);
	COut(level) << "        SkMesh: " << buf << std::endl;
	FormatSize(buf, r::ZFonts.Get().totalBytes);
	COut(level) << "        Fonts: " << buf << std::endl;
	FormatSize(buf, ska::ZSka.Get().totalBytes);
	COut(level) << "      SkAnim: " << buf << std::endl;
	FormatSize(buf, ZSound.Get().totalBytes);
	COut(level) << "      Sound: " << buf << std::endl;
	FormatSize(buf, ZMusic.Get().totalBytes);
	COut(level) << "      Music: " << buf << std::endl;
	FormatSize(buf, ZWorld.Get().totalBytes);
	COut(level) << "      World: " << buf << std::endl;
	FormatSize(buf, lua::ZLuaRuntime.Get().totalBytes);
	COut(level) << "      Lua: " << buf << std::endl;
	FormatSize(buf, pkg::ZPackages.Get().totalBytes);
	COut(level) << "      Packages: " << buf << std::endl;
	FormatSize(buf, ZUnknown.Get().totalBytes);
	COut(level) << "  Uncategorized: " << buf << std::endl;
}

App::App() :
m_ticks(0),
m_exit(false),
m_time(0.f),
m_frameHistoryIdx(0)
#if defined(RAD_OPT_PC_TOOLS)
,m_editor(false)
#endif
{
	m_e = Engine::New();
#if defined(FRAME_SMOOTH)
	m_frameHistory.reserve(FrameHistorySize);
#endif
}

App::~App()
{
	if (m_e)
		delete m_e;
}

bool App::PreInit()
{
	return engine->PreInit();
}

bool App::Initialize()
{
	return engine->Initialize();
}

void App::Finalize()
{
	engine->Finalize();
}

void App::Run()
{ // called to startup game
}

float App::Tick()
{
	if (m_ticks == 0)
	{
		m_ticks = xtime::ReadMilliseconds();
	}

	xtime::TimeVal ticks = xtime::ReadMilliseconds();
	if (ticks == m_ticks)
		return 0.0f;

	float elapsed = xtime::Constants<float>::MilliToSecond(float(ticks-m_ticks));
	if (elapsed > 0)
	{
		if (elapsed > 1.f)
			elapsed = 0.01f; // assume a hickup (load pause etc).
		elapsed = CalcDt(elapsed);

		DoTickable(elapsed);
		OnTick(elapsed);
		engine->Tick(elapsed);
		m_ticks = ticks;
		m_time += elapsed;
	}

	return elapsed;
}

void App::ClearFrameHistory()
{
	m_frameHistory.clear();
}

float App::CalcDt(float dt)
{
#if defined(FRAME_SMOOTH)
	if (m_frameHistoryIdx >= (int)m_frameHistory.size())
		m_frameHistory.push_back(dt);
	else 
		m_frameHistory[m_frameHistoryIdx] = dt;

	m_frameHistoryIdx = (m_frameHistoryIdx+1)%FrameHistorySize;
	
	float avg = 0.f;
	for (size_t i = 0; i < m_frameHistory.size(); ++i)
	{
		avg += m_frameHistory[i];
	}
	
	return avg / (float)m_frameHistory.size();
#else
	return dt;
#endif
}

void App::Push(const Tickable::Ref &state)
{
	m_tickable.Push(state);
}

void App::Pop()
{
	m_tickable.Pop();
}

void App::DoTickable(float elapsed)
{
	m_tickable.Tick(*this, elapsed, xtime::TimeSlice::Infinite, 0);
}
