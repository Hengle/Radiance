/*! \file E_ViewController.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup world
*/

#pragma once

#include "../Entity.h"
#include "../BSPFile.h"
#include <Runtime/Container/ZoneList.h>
#include <Runtime/PushPack.h>

namespace world {

//! Controls the view used to render the game world.
/*! The ViewController entity is a special singleton entity (only one will ever exist) whose
	job is to control the camera FOV and world space position and orientation. When ticked
	the view controller entity computes the desired camera orientation and writes it to
	the worlds camera object.

	This base view controller class works by "following" its target entity. There are
	two animated points relative to the target object that the view controller calculates.

	The distance point (E_ViewController::TargetMode::TM_Distance) places the camera in space
	by declaring the position of the camera relative to the target (i.e. its distance/angle
	to the target).

	The orientation point of the camera is defined by another point offset and animated in
	an identical manner from the target entity as the distance. The camera will look at this 
	point as it animates around.

	Secondary FOV animations can then be attached to the camera, either to track the FOV
	as a function of distance from the target object or as a smooth lerp between two values.
*/
class RADENG_CLASS E_ViewController : public Entity {
	E_DECL_BASE(Entity);
public:

	E_ViewController();
	virtual ~E_ViewController();

	virtual void Tick(
		int frame,
		float dt, 
		const xtime::TimeSlice &time
	);

	virtual int Spawn(
		const Keys &keys,
		const xtime::TimeSlice &time,
		int flags
	);

	RAD_DECLARE_PROPERTY(E_ViewController, target, Entity::Ref, const Entity::Ref&);

	//! Target mode enumeration.
	/*! The Target mode defines what part of the view controllers targeting system is being
	    modified.
	*/
	enum TargetMode {
		kTargetMode_Distance,
		kTargetMode_Look,
		kNumTargetModes
	};

	//! Sets the parameters for a camera that tracks the target by a specified distance
	/*! The camera position and orientation are defined by two points offset from the
		view controllers target. Each point is offset via an animating distance and angle 
		from the target.

		The distance oscillates between a min/max over time. Additionally as the "target"
		object moves through space lag can be introduced to the distance. This means that 
		the camera will vary its distance to the target normally, but the camera may
		see a "delay" in the motion of the target. This has the effect of loosly coupling
		the position of the target to where the camera thinks it is, or something akin to
		a poor mans spring.

		Likewise the euler angles which define the direction that the point is offset
		from the target is controlled via a min/max. Similiar to how distance is coupled
		to the targets "position", the angles are also coupled to the targets orientation.
		This coupling can also have a "lag" value which serves to the camera see changes
		to the targets orientation in a delayed or laggy way.
	*/
	void SetTargetMode(
		TargetMode mode,
		float in, // blend in time
		float out, // blend out time
		float hold, // hold time, 0 for infinite
		float minDistance, // minimum distance
		float maxDistance, // maximum distance
		float distanceSpeed, // speed to lerp between the min/max distance
		float distanceLag, // <= 0 means no lag, numbers closer to zero are very laggy, with larger numbers being less laggy.
		const Vec3 &minAngles, // Zero angles specifies looking down +X
		const Vec3 &maxAngles,
		float angleSpeed,      // speed to lerp between min/max angles
		float angleLagTime,    // <= 0 means no lag, numbers closer to zero are very laggy, with larger numbers being less laggy.
		bool useTargetPitch
	);

	void SetFixedCamera(
		const Vec3 &pos,
		const Vec3 &angles
	);

	void SetRailMode(
		const char *cinematicName,
		float distance,    // ideal distance to target.
		bool  strict,      // distance to target is strict.
		float trackingLag, // how quickly the camera adjusts its position in response to target movement. 
		                   // <= 0 means no lag, numbers close to zero are very laggy, larger numbers are less laggy.
						   // This is a seperate lag control than the target position lag, set by kTargetMode_Look.
						   // kTargetMode_Look may lag the "look at" point
		float lookAtLag,   // how quickly the camera turns to look at the target object.
		float stayBehind,  // >= 0 if we want to stay behind the player and this tells us how often to check.
		bool useCinematicFOV, // true = uses embedded fov from cinematic
		const Vec3 &angleClamp // how much camera can "turn" from its rail-track to look at target.
		                       // NOTE: X angles are ignored, camera always controls roll angle.
	);
	
	//! Generates a rocking or swaying motion in the camera
	/*! Camera sway is done by shifting the cameras position in view-space (i.e. left/right
		up/down). This is done by rotating a vector around in a circle while varying its
		length. The camera is attached to the end of this vector.
	*/
	void SetCameraSway(
		float in, // blend in time
		float out, // blend out time
		float hold, // hold time, 0 for infinite
		float minDistance,
		float maxDistance,
		float frequency,
		float angleSpeed, // how fast we rotate in a 360
		const Vec2 &scale // scaling factor
	);

	void SetCameraFOV(
		float in, // blend in time
		float out, // blend out time
		float hold, // hold time, 0 for infinite
		float fov,
		float plus,
		float minus,
		float frequency,
		float minFov,
		float maxFov,
		float minDistance,
		float maxDistance
	);
	
	void LerpCameraFOVShift(float fov, float time);

	void BlendToTarget(float time);

	void BlendToLookTarget(
		const Vec3 &target,
		float in,
		float out,
		float hold,
		float weight,
		float inSmooth, // The smooth factor, 0 = no smoothing, numbers very close to zero are very "laggy". Larger numbers are less smooth.
		float outSmooth 
	);

protected:

	virtual void PushCallTable(lua_State *L);

private:

	struct Blend {
		enum Step {
			kStep_In,
			kStep_Out,
			kStep_Hold,
			kStep_Done
		};

		float in[2];
		float out[2];
		float hold[2];
		float frac;
		Step step;

		void Init(float in, float out, float hold);
		void Tick(float dt);
	};

	class VecAnim {
	public:
		typedef zone_list<VecAnim, ZWorldT>::type List;

		void Start(
			float in, // blend in time
			float out, // blend out time
			float hold, // hold time, 0 for infinite
			float minDistance, // minimum distance
			float maxDistance, // maximum distance
			float distanceSpeed, // speed to lerp between the min/max distance
			float distanceLag, // fraction time to blend from current position to target distance
			const Vec3 &minAngles, // Zero angles specifies looking down +X
			const Vec3 &maxAngles,
			float angleSpeed,      // speed to lerp between min/max angles
			float angleLag,    // fraction time to blend between current angles and desired angles
			bool useTargetPitch
		);

		Vec3 curPos;
		Vec3 curAngles;

		static Vec3 Tick(
			List &list,
			float dt,
			const Vec3 &targetPos,
			const Vec3 &targetAngles,
			float *roll,
			bool sync
		);

	private:

		Vec3 Tick(
			float dt,
			const Vec3 &targetPos,
			const Vec3 &targetAngles,
			float *roll,
			bool sync
		);

		Blend m_blend;

		float m_time;
		float m_distance[2]; // min/max
		float m_dspeed[2]; // speed/lag
		Vec3  m_angles[2]; // min/max
		float m_aspeed[2]; // speed/lag
		bool m_pitch;
		bool m_sync;
	};

	enum Mode {
		kMode_Distance,
		kMode_Rail,
		kMode_Fixed
	};

	struct Sway {
		typedef zone_list<Sway, ZWorldT>::type List;

		Blend blend;

		float time;
		float angle;
		float dist[2];
		float freq;
		float aspeed;
		Vec2 scale;

		static Vec3 Tick(List &list, float dt, const Vec3 &fwd);

	private:

		Vec3 Tick(float dt, const Vec3 &fwd);
	};

	struct FOV {
		typedef zone_list<FOV, ZWorldT>::type List;

		Blend blend;

		float fov[3];
		float dist[2];
		float dfov[2];
		float freq;
		float time;

		static float Tick(List &list, float dt, float distance);

	private:

		float Tick(float dt, float distance);
	};

	struct LookTarget {
		typedef zone_list<LookTarget, ZWorldT>::type List;

		Blend blend;
		Vec3 target;
		float weight;
		float frac;
		float smooth[2];

		static Vec3 Tick(List &list, const Vec3 &pos, const Vec3 &fwd, float dt);
	};

	void TickFixedMode(int frame, float dt, const Entity::Ref &target);
	void TickDistanceMode(int frame, float dt, const Entity::Ref &target);
	void TickRailMode(int frame, float dt, const Entity::Ref &target);

	float TickFOV(int frame, float dt, float distance);

	void UpdateRailTarget(const Vec3 &target, const Vec3 &targetFwd);

	static int lua_SetTargetMode(lua_State *L);
	static int lua_SetRailMode(lua_State *L);
	static int lua_SetFixedCamera(lua_State *L);
	static int lua_SetCameraSway(lua_State *L);
	static int lua_SetCameraFOV(lua_State *L);
	static int lua_LerpCameraFOVShift(lua_State *L);
	static int lua_BlendToTarget(lua_State *L);
	static int lua_BlendToLookTarget(lua_State *L);
	static int lua_Sync(lua_State *L);
	
	ENT_DECL_GETSET(Target);

	RAD_DECLARE_GET(target, Entity::Ref) { 
		return m_target.lock(); 
	}

	RAD_DECLARE_SET(target, const Entity::Ref&) { 
		m_target = value; 
	}

	struct RailCameraState {
		String name;
		const bsp_file::BSPCameraTrack *track;
		const bsp_file::BSPCameraTM *tm[3];
		Vec3 pos;
		Vec3 fwd;
		Vec3 lookFwd;
		Vec3 clamp;
		Vec3 targetFwd;
		Quat rot;
		float distance;
		float distanceSq;
		float trackLag;
		float turnLag;
		float fov;
		float stayBehind;
		float lastBehindTime;
		bool cinematicFOV;
		bool strict;
	};

	Mode m_mode;
	Vec3 m_targetPos;
	Vec3 m_targetLook;
	Vec3 m_pos;
	Vec3 m_angles;
	Sway::List m_sways;
	FOV::List m_fovs;
	LookTarget::List m_looks;
	float m_fovShift[4];
	float m_blendTime[2];
	VecAnim::List m_anims[kNumTargetModes];
	RailCameraState m_rail;
	Entity::WRef m_target;
	bool m_sync;
};

} // world

#include <Runtime/PopPack.h>
