// WorldLua.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include "../Input.h"
#include "../Lua/LuaRuntime.h"
#include "../Packages/PackagesDef.h"
#include "../Game/GameNetworkDef.h"
#include "WorldCinematics.h"
#include "Entity.h"
#include "Lua/LuaTask.h"
#include <Runtime/PushPack.h>

namespace world {

class World;

class RADENG_CLASS WorldLua
{
public:

	typedef boost::shared_ptr<WorldLua> Ref;

	~WorldLua();

	void Tick(float dt);

	void PushKeysTable(const Keys &keys);
	bool ParseKeysTable(Keys &keys, int index, bool luaError);
	void PushEntityFrame(Entity &ent);
	bool PushEntityCall(Entity &ent, const char *name);
	bool CoSpawn(Entity &ent, const Keys &keys);
	bool CoPostSpawn(Entity &ent);
	bool CoThink(Entity &ent);
	bool RunCo(Entity &ent, bool &complete);
	bool PushGlobalCall(const char *name);
	Entity *EntFramePtr(int index, bool luaError);
	void DeleteEntId(Entity &ent);
	bool Call(const char *context, int nargs, int nresults, int errfunc);
	bool CreateEntity(Entity &ent, int id, const char *classname);

	bool HandleInputEvent(const InputEvent &e, const TouchState *touch, const InputState &is);
	bool HandleInputGesture(const InputGesture &g, const TouchState &touch, const InputState &is);

	void NotifyBackground();
	void NotifyResume();
	void SaveApplicationState();
	void RestoreApplicationState();

	void PostSpawn();

	void OnLocalPlayerAuthenticated(gn::NetResult r);
	void OnShowLeaderboard(bool show);
	void OnShowAchievements(bool show);

	static void PushKeysTable(lua_State *L, const Keys &keys);
	static bool ParseKeysTable(lua_State *L, Keys &keys, int index, bool luaError);
	static Entity *EntFramePtr(lua_State *L, int index, bool luaError);
	static void PushEntityFrame(lua_State *L, Entity &ent);
	static bool PushEntityCall(lua_State *L, Entity &ent, const char *name);
	static bool Call(lua_State *L, const char *context, int nargs, int nresults, int errfunc);

	// --------- Script Callbacks, some ents don't have 'em but these are safe to call even if they don't
	bool PostSpawn(Entity &ent);

	RAD_DECLARE_READONLY_PROPERTY(WorldLua, L, lua_State*);

private:

	friend class World;

	WorldLua(World *w);

	bool Init();
	void GarbageCollect();
	Entity::Ref CreateEntity(const Keys &keys);

	RAD_DECLARE_GET(L, lua_State*) { return m_L->L; }

	class ImportLoader : public lua::ImportLoader
	{
	public:
		ImportLoader() {}
		virtual lua::SrcBuffer::Ref Load(lua_State *L, const char *name);
	};

	class CinematicsNotify : public WorldCinematics::Notify, public lua::SharedPtr
	{
	public:
		typedef boost::shared_ptr<CinematicsNotify> Ref;

		CinematicsNotify(World *world, Entity &entity, int callbackId);
		virtual ~CinematicsNotify();

		virtual void OnTag(const char *str);
		virtual void OnComplete();
		virtual void OnSkip();

	protected:

		virtual void PushElements(lua_State *L);

	private:

		static int lua_SetMasked(lua_State *L);

		bool m_masked;
		World *m_world;
		EntityWRef m_entity;
		int m_callbackId;
	};

	// ----------- Native Calls -----------
	static int lua_System_Platform(lua_State *L);
	static int lua_SysCalls_CreatePrecacheTask(lua_State *L);
	static int lua_CreateSpawnTask(lua_State *L);
	static int lua_CreateTempSpawnTask(lua_State *L);
	static int lua_SysCalls_COut(lua_State *L);
	static int lua_FindEntityId(lua_State *L);
	static int lua_FindEntityClass(lua_State *L);
	static int lua_FindEntityTargets(lua_State *L);
	static int lua_BBoxTouching(lua_State *L);
	static int lua_CreateScreenOverlay(lua_State *L);
	static int lua_PostEvent(lua_State *L);
	static int lua_DispatchEvent(lua_State *L);
	static int lua_Project(lua_State *L);
	static int lua_Unproject(lua_State *L);
	static int lua_SetUIViewport(lua_State *L);
	static int lua_SetRootWidget(lua_State *L);
	static int lua_CreateWidget(lua_State *L);
	static int lua_AddWidgetTickMaterial(lua_State *L);
	static int lua_CreatePostProcessEffect(lua_State *L);
	static int lua_FadePostProcessEffect(lua_State *L);
	static int lua_EnablePostProcessEffect(lua_State *L);
	static int lua_PlayCinematic(lua_State *L);
	static int lua_StopCinematic(lua_State *L);
	static int lua_SkipCinematics(lua_State *L);
	static int lua_CinematicTime(lua_State *L);
	static int lua_SetCinematicTime(lua_State *L);
	static int lua_SoundFadeMasterVolume(lua_State *L);
	static int lua_SoundFadeChannelVolume(lua_State *L);
	static int lua_SoundChannelVolume(lua_State *L);
	static int lua_SoundPauseChannel(lua_State *L);
	static int lua_SoundChannelIsPaused(lua_State *L);
	static int lua_SoundPauseAll(lua_State *L);
	static int lua_SoundStopAll(lua_State *L);
	static int lua_SoundSetDoppler(lua_State *L);
	static int lua_RequestLoad(lua_State *L);
	static int lua_RequestReturn(lua_State *L);
	static int lua_RequestSwitch(lua_State *L);
	static int lua_RequestUnloadSlot(lua_State *L);
	static int lua_RequestSwitchLoad(lua_State *L);
	static int lua_SaveSession(lua_State *L);
	static int lua_LoadSession(lua_State *L);
	static int lua_SaveGlobals(lua_State *L);
	static int lua_LoadGlobals(lua_State *L);
	static int lua_CreateSaveGame(lua_State *L);
	static int lua_LoadSavedGame(lua_State *L);
	static int lua_LoadSaveTable(lua_State *L);
	static int lua_SaveGame(lua_State *L);
	static int lua_NumSavedGameConflicts(lua_State *L);
	static int lua_LoadSavedGameConflict(lua_State *L);
	static int lua_ResolveSavedGameConflict(lua_State *L);
	static int lua_EnableCloudStorage(lua_State *L);
	static int lua_CloudFileStatus(lua_State *L);
	static int lua_StartDownloadingLatestSaveVersion(lua_State *L);
	static int lua_CloudStorageAvailable(lua_State *L);
	static int lua_SetGameSpeed(lua_State *L);
	static int lua_SetPauseState(lua_State *L);
	static int lua_SwapMaterial(lua_State *L);
	static int lua_PlayerPawn(lua_State *L);
	static int lua_SetPlayerPawn(lua_State *L);
	static int lua_Worldspawn(lua_State *L);
	static int lua_SetWorldspawn(lua_State *L);
	static int lua_ViewController(lua_State *L);
	static int lua_SetViewController(lua_State *L);
	static int lua_Time(lua_State *L);
	static int lua_SysTime(lua_State *L);
	static int lua_DeltaTime(lua_State *L);
	static int lua_Viewport(lua_State *L);
	static int lua_CameraPos(lua_State *L);
	static int lua_CameraFarClip(lua_State *L);
	static int lua_SetCameraFarClip(lua_State *L);
	static int lua_CameraAngles(lua_State *L);
	static int lua_CameraFOV(lua_State *L);
	static int lua_CameraFwd(lua_State *L);
	static int lua_CameraLeft(lua_State *L);
	static int lua_CameraUp(lua_State *L);
	static int lua_SetEnabledGestures(lua_State *L);
	static int lua_FlushInput(lua_State *L);
	static int lua_DrawCounters(lua_State *L);
	static int lua_EnableWireframe(lua_State *L);
	static int lua_EnableColorBufferClear(lua_State *L);
	static int lua_AttachViewModel(lua_State *L);
	static int lua_RemoveViewModel(lua_State *L);
	static int lua_SetViewModelAngles(lua_State *L);
	static int lua_SetViewModelScale(lua_State *L);
	static int lua_SetViewModelVisible(lua_State *L);
	static int lua_ViewModelVisible(lua_State *L);
	static int lua_FadeViewModel(lua_State *L);
	static int lua_ViewModelBonePos(lua_State *L);
	static int lua_ViewModelFindBone(lua_State *L);
	static int lua_CurrentDateAndTime(lua_State *L);
	static int lua_gnCreate(lua_State *L);
	static int lua_gnAuthenticateLocalPlayer(lua_State *L);
	static int lua_gnLocalPlayerId(lua_State *L);
	static int lua_gnSendScore(lua_State *L);
	static int lua_gnSendAchievement(lua_State *L);
	static int lua_gnShowLeaderboard(lua_State *L);
	static int lua_gnShowAchievements(lua_State *L);
	static int lua_gnLogEvent(lua_State *L);
	static int lua_gnEndTimedEvent(lua_State *L);
	static int lua_gnLogError(lua_State *L);
	static int lua_gnSessionReportOnAppClose(lua_State *L);
	static int lua_gnSetSessionReportOnAppClose(lua_State *L);
	static int lua_gnSessionReportOnAppPause(lua_State *L);
	static int lua_gnSetSessionReportOnAppPause(lua_State *L);
	
	friend class ImportLoader;

	ImportLoader m_impLoader;
	lua::State::Ref m_L;
	World *m_world;
};

} // world

#include <Runtime/PopPack.h>
