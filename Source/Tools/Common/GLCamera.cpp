// GLCamera.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "GLCamera.h"


GLCamera::GLCamera() :
m_near(float(1)),
m_far(float(10000)),
m_fov(float(90)),
m_ratio(float(4.0 / 3.0)),
m_pos(GLVec3::Zero)
{
	LoadIdentity();
}

GLCamera::~GLCamera()
{
}

void GLCamera::LoadIdentity()
{
	m_mat[0][0] = -GLYAxis[0];
	m_mat[1][0] = -GLYAxis[1];
	m_mat[2][0] = -GLYAxis[2];
	m_mat[3][0] = float(0);

	m_mat[0][1] = GLZAxis[0];
	m_mat[1][1] = GLZAxis[1];
	m_mat[2][1] = GLZAxis[2];
	m_mat[3][1] = float(0);

	m_mat[0][2] = -GLXAxis[0];
	m_mat[1][2] = -GLXAxis[1];
	m_mat[2][2] = -GLXAxis[2];
	m_mat[3][2] = float(0);

	m_mat[0][3] = float(0);
	m_mat[1][3] = float(0);
	m_mat[2][3] = float(0);
	m_mat[3][3] = float(1);

	m_pos = GLVec3::Zero;
}

void GLCamera::SetVecs(const GLVec3& up, const GLVec3& left, const GLVec3& forward)
{
	m_mat[0][0] = left[0];
	m_mat[1][0] = left[1];
	m_mat[2][0] = left[2];

	m_mat[0][1] = up[0];
	m_mat[1][1] = up[1];
	m_mat[2][1] = up[2];

	m_mat[0][2] = forward[0];
	m_mat[1][2] = forward[1];
	m_mat[2][2] = forward[2];
}

GLVec3 GLCamera::Up() const
{
	return m_mat.Column(1);
}

GLVec3 GLCamera::Left() const
{
	return m_mat.Column(0);
}

GLVec3 GLCamera::Forward() const
{
	return m_mat.Column(2);
}

GLVec3 GLCamera::Pos() const
{
	return m_pos;
}

void GLCamera::SetPos(const GLVec3& pos)
{
	m_pos = pos;
}

void GLCamera::LookAt(const GLVec3& look)
{
	SetFacing(look - Pos());	
}

void GLCamera::SetFacing(GLVec3 frw)
{
	GLVec3 up = GLZAxis;
	GLVec3 lft;
	frw.Normalize();

	if (math::Abs(frw.Dot(up)) > float(0.98888)) // almost straight up, use another axis
	{
		up = GLYAxis;
	}
	
	lft = frw.Cross(up);

	SetVecs(up, lft, frw);
}

void GLCamera::RotateUp(float rad)
{
	Rotate(GLQuat(Up(), rad));
}


void GLCamera::RotateLeft(float rad)
{
	Rotate(GLQuat(Left(), rad));
}

void GLCamera::RotateForward(float rad)
{
	Rotate(GLQuat(Forward(), rad));
}

void GLCamera::Rotate(const GLQuat& q)
{
	m_mat = GLMat4::Rotation(q) * m_mat;
}

void GLCamera::Rotate(float x, float y, float z, float rad)
{
	Rotate(GLQuat(GLVec3(x, y, z).Normalize(), rad));
}

void GLCamera::SetFOV(float fov, float aspectRatio)
{
	m_fov = fov;
	m_ratio = aspectRatio;
}

void GLCamera::FOV(float* fov, float* aspectRatio) const
{
	RAD_ASSERT(fov&&aspectRatio);
	*fov = m_fov;
	*aspectRatio = m_ratio;
}

void GLCamera::SetClips(float near, float far)
{
	RAD_ASSERT(near > float(0));
	RAD_ASSERT(far  > float(0));

	m_near = near;
	m_far  = far;
}

void GLCamera::Clips(float* near, float* far) const
{
	RAD_ASSERT(near&&far);
	*near = m_near;
	*far  = m_far;
}

GLMat4 GLCamera::Matrix() const
{
	return GLMat4::Translation(m_pos) * m_mat;
}

void GLCamera::Bind(GLState& ctx) const // sets up gl matrices.
{
	GLdouble xmin, xmax, ymin, ymax;
	ymax = m_near * math::Tan(m_fov * double(math::Constants<float>::PI()) / 360.0);
	ymin = -ymax;
	
	xmin = ymin * m_ratio;
	xmax = ymax * m_ratio;
	
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	glFrustum( xmin, xmax, ymin, ymax, double(m_near), double(m_far) );

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	double m[16];

	GLMat4 _m(Matrix());

	m[0] = _m[0][0];
	m[4] = _m[1][0];
	m[8] = _m[2][0];
	m[12] = -_m[3][0];

	m[1] = _m[0][1];
	m[5] = _m[1][1];
	m[9] = _m[2][1];
	m[13] = -_m[3][1];

	m[2] = _m[0][2];
	m[6] = _m[1][2];
	m[10] = _m[2][2];
	m[14] = -_m[3][2];

	m[3] = _m[0][3];
	m[7] = _m[1][3];
	m[11] = _m[2][3];
	m[15] = _m[3][3];

	glMultMatrixd(m);

	CHECK_GL_ERRORS();
}