// Game.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Abstract Game Class
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "App.h"
#include "Engine.h"
#include "SkAnim/SkAnim.h"
#include <Runtime/Tokenizer.h>
#include <Runtime/File.h>
#include <Runtime/Time.h>

#if !defined(RAD_OPT_PC_TOOLS)
#include "Game/Game.h"
#endif

//#define FRAME_SMOOTH
#if defined(FRAME_SMOOTH)
enum { FrameHistorySize = 15 };
#endif

App *App::s_instance = 0;

App *App::Get(int argc, const char **argv) {
	if (!s_instance)
		s_instance = New(argc, argv);
	return s_instance;
}

void App::DestroyInstance() {
	if (s_instance)
		delete s_instance;
	s_instance = 0;
}

void App::DumpMemStats(int level) {
	SizeBuffer buf;
	
	COut(level) << "MemStats: " << std::endl;
	FormatSize(buf, ZRuntime.Get().totalBytes);
	COut(level) << "  Runtime: " << buf << std::endl;
	FormatSize(buf, file::ZFile.Get().totalBytes);
	COut(level) << "    Files (mmap): " << buf << std::endl;
	FormatSize(buf, file::ZPakFile.Get().totalBytes);
	COut(level) << "    PakFiles (mmap): " << buf << std::endl;
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

App::App(int argc, const char **argv) :
NativeApp(argc, argv),
m_ticks(0),
m_exit(false),
m_time(0.f),
m_frameHistoryIdx(0),
m_langId(StringTable::LangId_EN) {
	m_e = Engine::New();
#if defined(FRAME_SMOOTH)
	m_frameHistory.reserve(FrameHistorySize);
#endif
}

App::~App() {
	if (m_e)
		delete m_e;
}

bool App::PreInit() {
	if (!NativeApp::PreInit())
		return false;
	if (!engine->PreInit())
		return false;
	m_langId = LoadLangId(0, systemLangId);
	return true;
}

StringTable::LangId App::LoadLangId(int *enabledLangMask, StringTable::LangId defaultLangId) {

	const StringTable::LangId ErrLang = StringTable::LangId_EN;
	if (enabledLangMask)
		*enabledLangMask = StringTable::LangFlag_EN;

	file::MMFileInputBuffer::Ref ib = engine->sys->files->OpenInputBuffer("@r:/languages.txt", ZEngine);
	if (!ib)	
		return ErrLang;
	
	stream::InputStream is(*ib);
	
	Tokenizer script(is);

	int validLangBits = StringTable::LangFlag_EN;
	StringTable::LangId defaultLang = StringTable::LangId_EN;

	String token;
	while (script.GetToken(token, Tokenizer::kTokenMode_CrossLine)) {
		if (token == "DEFAULT") {
			if (!script.IsNextToken("=", Tokenizer::kTokenMode_SameLine))
				return ErrLang;
			if (!script.GetToken(token, Tokenizer::kTokenMode_SameLine))
				return ErrLang;
			token.Lower();
			int id = StringTable::Map(token.c_str);
			if (id != -1)
				defaultLang = (StringTable::LangId)id;
		} else if (token == "FORCE") {
			if (!script.IsNextToken("=", Tokenizer::kTokenMode_SameLine))
				return ErrLang;
			if (!script.GetToken(token, Tokenizer::kTokenMode_SameLine))
				return ErrLang;
			token.Lower();
			int id = StringTable::Map(token.c_str);
			if (id != -1) {
				if (enabledLangMask) // make sure to set this
					*enabledLangMask = validLangBits;
				return (StringTable::LangId)id;
			}
		} else {
			token.Lower();
			int id = StringTable::Map(token.c_str);
			if (id != -1)
				validLangBits |= (1<<id);
		}
	}

	// If our system language isn't an enabled language then
	// use our default language
	StringTable::LangId langId = defaultLangId;
	if (!((1<<langId) & validLangBits))
		langId = defaultLang;

	if (enabledLangMask)
		*enabledLangMask = validLangBits;

	return langId;
}

bool App::Initialize() {
	return NativeApp::Initialize() && engine->Initialize();
}

void App::Finalize() {
	engine->Finalize();
	NativeApp::Finalize();
}

float App::Tick() {
	if (m_ticks == 0) {
		m_ticks = xtime::ReadMilliseconds();
	}

	xtime::TimeVal ticks = xtime::ReadMilliseconds();
	if (ticks == m_ticks)
		return 0.0f;

	float elapsed = xtime::Constants<float>::MilliToSecond(float(ticks-m_ticks));
	if (elapsed > 0) {
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

#if !defined(RAD_OPT_PC_TOOLS)
void App::MovieFinished() {
	game->MovieFinished();
}

void App::PlainTextDialogResult(bool cancel, const char *text) {
	game->PlainTextDialogResult(cancel, text);
}
#endif

void App::ClearFrameHistory() {
	m_frameHistory.clear();
}

float App::CalcDt(float dt) {
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

void App::Push(const Tickable::Ref &state) {
	m_tickable.Push(state);
}

void App::Pop() {
	m_tickable.Pop();
}

void App::DoTickable(float elapsed) {
	m_tickable.Tick(*this, elapsed, xtime::TimeSlice::Infinite, 0);
}

