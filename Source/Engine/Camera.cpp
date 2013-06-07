// Camera.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Camera.h"
#include "MathUtils.h"

Camera::Camera() : m_fov(90.f), m_far(16384.f), m_pos(Vec3::Zero), m_quatMode(false)
{
	MakeIdentity();
}

Camera::Camera(const Camera &c) :
m_pos(c.m_pos), 
m_rot(c.m_rot), 
m_angles(c.m_angles),
m_fwd(c.m_fwd), 
m_left(c.m_left), 
m_up(c.m_up), 
m_fov(c.m_fov),
m_far(c.m_far),
m_quatMode(false)
{
}

Camera::Camera(const Vec3 &pos, const Mat4 &_rot, float fov, float farClip) :
m_pos(pos), m_fov(fov), m_far(farClip), m_quatMode(false)
{
	rot = _rot.Rotation();
}

Camera::Camera(const Vec3 &org, const Quat &_rot, float fov, float farClip) :
m_pos(pos), m_fov(fov), m_far(farClip), m_quatMode(false)
{
	rot = _rot;
}

Camera::Camera(const Vec3 &org, const Vec3 &_angles, float fov, float farClip) :
m_pos(pos), m_fov(fov), m_far(farClip), m_quatMode(false)
{
	angles = _angles;
}

void Camera::MakeIdentity()
{
	rot = Quat::Identity;
}

void Camera::SetFacing(const Vec3 &_fwd)
{
	Vec3 fwd = _fwd.Unit();
	Vec3 left;
	Vec3 up;

	fwd.FrameVecs(up, left);

	m_fwd = fwd;
	m_left = left;
	m_up = up;

	m_angles = LookAngles(fwd);
	m_rot = QuatFromAngles(m_angles);
}

void Camera::RAD_IMPLEMENT_SET(angles)(const Vec3 &angles)
{
	m_angles = angles;

	if( m_angles[0] > 360.0f )
		m_angles[0] -= 360.0f;
	if( m_angles[0] < 0.0f )
		m_angles[0] += 360.0f;
	
	if( m_angles[1] > 360.0f )
		m_angles[1] -= 360.0f;
	if( m_angles[1] < 0.0f )
		m_angles[1] += 360.0f;

	if( m_angles[2] > 360.0f )
		m_angles[2] -= 360.0f;
	if( m_angles[2] < 0.0f )
		m_angles[2] += 360.0f;

	m_rot = QuatFromAngles(m_angles);

	Mat4 m = Mat4::Rotation(m_rot);
	m_fwd  = Vec3(1, 0, 0) * m;
	m_left = Vec3(0, 1, 0) * m;
	m_up = Vec3(0, 0, 1) * m;
}

void Camera::RAD_IMPLEMENT_SET(rot)(const Quat &r)
{
	m_rot = r;
	Mat4 m = Mat4::Rotation(r);
	m_fwd  = Vec3(1, 0, 0) * m;
	m_left = Vec3(0, 1, 0) * m;
	m_up = Vec3(0, 0, 1) * m;
	m_angles = LookAngles(m_fwd);
}
