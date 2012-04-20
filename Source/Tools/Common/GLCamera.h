// GLCamera.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "RGL.h"

class GLCamera
{
public:
	GLCamera();
	~GLCamera();

	void LoadIdentity();
	void SetVecs(const GLVec3& up, const GLVec3& left, const GLVec3& forward);
	GLVec3 Up() const;
	GLVec3 Left() const;
	GLVec3 Forward() const;
	GLVec3 Pos() const;
	void SetPos(const GLVec3& pos);
	void LookAt(const GLVec3& pos);
	void SetFacing(GLVec3 forward);
	void RotateUp(float rad);
	void RotateLeft(float rad);
	void RotateForward(float rad);
	void Rotate(const GLQuat& q);
	void Rotate(float x, float y, float z, float rad);
	void SetFOV(float fov, float aspectRatio); // aspectRatio = y/x

#if defined(near)
#undef near
#endif

#if defined(far)
#undef far
#endif

	void SetClips(float near, float far);

	GLMat4 Matrix() const;
	void FOV(float* fov, float* aspectRatio) const;
	void Clips(float* near, float* far) const;
	void Bind(GLState& ctx) const; // sets up gl matrices.

private:

	GLMat4 m_mat;
	GLVec3 m_pos;
	float m_fov, m_ratio, m_near, m_far;
};
