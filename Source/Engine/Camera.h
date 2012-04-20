// Camera.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include <Runtime/PushPack.h>

class RADENG_CLASS Camera
{
public:

	typedef boost::shared_ptr<Camera> Ref;

	Camera();
	Camera(const Camera &c);
	Camera(const Vec3 &pos, const Mat4 &rot, float fov, float farClip);
	Camera(const Vec3 &pos, const Quat &rot, float fov, float farClip);
	Camera(const Vec3 &pos, const Vec3 &angles, float fov, float farClip);

	void MakeIdentity(); // set at origin, up = +Z, left = +Y, fwd = +X
	void LookAt(const Vec3 &target) { SetFacing(target-m_pos); }
	void SetFacing(const Vec3 &fwd);

	RAD_DECLARE_PROPERTY(Camera, pos, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(Camera, rot, const Quat&, const Quat&);
	RAD_DECLARE_PROPERTY(Camera, angles, const Vec3&, const Vec3&);
	RAD_DECLARE_PROPERTY(Camera, fov, float, float);
	RAD_DECLARE_PROPERTY(Camera, farClip, float, float);
	RAD_DECLARE_PROPERTY(Camera, quatMode, bool, bool);
	RAD_DECLARE_READONLY_PROPERTY(Camera, fwd, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(Camera, up, const Vec3&);
	RAD_DECLARE_READONLY_PROPERTY(Camera, left, const Vec3&);

private:

	RAD_DECLARE_GET(pos, const Vec3&) { return m_pos; }
	RAD_DECLARE_SET(pos, const Vec3&) { m_pos = value; }
	RAD_DECLARE_GET(rot, const Quat&) { return m_rot; }
	RAD_DECLARE_SET(rot, const Quat&);
	RAD_DECLARE_GET(angles, const Vec3&) { return m_angles; }
	RAD_DECLARE_SET(angles, const Vec3&);
	RAD_DECLARE_GET(fwd, const Vec3&) { return m_fwd; }
	RAD_DECLARE_GET(up, const Vec3&) { return m_up; }
	RAD_DECLARE_GET(left, const Vec3&) { return m_left; }
	RAD_DECLARE_GET(fov, float) { return m_fov; }
	RAD_DECLARE_SET(fov, float) { m_fov = value; }
	RAD_DECLARE_GET(farClip, float) { return m_far; }
	RAD_DECLARE_SET(farClip, float) { m_far = value; }
	RAD_DECLARE_GET(quatMode, bool) { return m_quatMode; }
	RAD_DECLARE_SET(quatMode, bool) { m_quatMode = value; }

	Vec3 m_fwd;
	Vec3 m_left;
	Vec3 m_up;
	Vec3 m_pos;
	Vec3 m_angles;
	Quat m_rot;
	float m_fov;
	float m_far;
	bool m_quatMode;
};

#include <Runtime/PopPack.h>
