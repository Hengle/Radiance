// GLTable.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include <Runtime/Math/Math.h>

namespace r {

inline GLMatrixStack::GLMatrixStack() {
	m_s.push_back(Mat4::Identity);
}

inline void GLMatrixStack::Push() {
	Mat4 m(m_s.back());
	m_s.push_back(m);
}

inline void GLMatrixStack::Pop() {
	if (m_s.size() > 1)
		m_s.pop_back();
}

inline Mat4 &GLMatrixStack::Top() {
	return m_s.back();
}

#define MATRIXP ((mm == GL_PROJECTION) ? &prj : &mv)

inline void GLTable::MatrixMode(GLenum mode) {
	RAD_ASSERT(mode == GL_PROJECTION || mode == GL_MODELVIEW);
	mm = mode;
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glMatrixMode(mode);
	CHECK_GL_ERRORS();
#endif
}

inline void GLTable::LoadIdentity() {
	GLMatrixStack *ms = MATRIXP;
	ms->Top() = Mat4::Identity;
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glLoadIdentity();
	CHECK_GL_ERRORS();
#endif
	++matrixOps;
}

inline void GLTable::PushMatrix() {
	GLMatrixStack *ms = MATRIXP;
	ms->Push();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glPushMatrix();
	CHECK_GL_ERRORS();
#endif
	++matrixOps;
}

inline void GLTable::PopMatrix() {
	GLMatrixStack *ms = MATRIXP;
	ms->Pop();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glPopMatrix();
	CHECK_GL_ERRORS();
#endif
	++matrixOps;
}

inline void GLTable::Rotatef(GLfloat angle, GLfloat x, GLfloat y, GLfloat z) {
	Quat q(Vec3(x, y, z), math::DegToRad(angle));
	GLMatrixStack *ms = MATRIXP;
	ms->Top() = Mat4::Rotation(q) * ms->Top();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glRotatef(angle, x, y, z);
	CHECK_GL_ERRORS();
#endif
	++matrixOps;
}

inline void GLTable::Scalef(GLfloat x, GLfloat y, GLfloat z) {
	GLMatrixStack *ms = MATRIXP;
	ms->Top() = Mat4::Scaling(Scale3(x, y, z)) * ms->Top();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glScalef(x, y, z);
	CHECK_GL_ERRORS();
#endif
	++matrixOps;
}

inline void GLTable::Translatef(GLfloat x, GLfloat y, GLfloat z) {
	GLMatrixStack *ms = MATRIXP;
	ms->Top() = Mat4::Translation(Vec3(x, y, z)) * ms->Top();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glTranslatef(x, y, z);
	CHECK_GL_ERRORS();
#endif
	++matrixOps;
}

inline void GLTable::Ortho(GLdouble left, GLdouble right, GLdouble bottom, GLdouble top, GLdouble zNear, GLdouble zFar) {
	GLMatrixStack *ms = MATRIXP;
	ms->Top() = Mat4::Ortho(
		(float)left, 
		(float)right, 
		(float)bottom,
		(float)top,
		(float)zNear,
		(float)zFar
	) * ms->Top();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glOrtho(left, right, bottom, top, zNear, zFar);
	CHECK_GL_ERRORS();
#endif

	++matrixOps;
}

inline void GLTable::Perspective(GLdouble fovy, GLdouble aspect, GLdouble zNear, GLdouble zFar) {
	GLdouble xmin, xmax, ymin, ymax;
	ymax = zNear * math::Tan(math::DegToRad(fovy*0.5f));
	ymin = -ymax;
	xmin = ymin * aspect;
	xmax = ymax * aspect;

	GLMatrixStack *ms = MATRIXP;
	ms->Top() = Mat4::PerspectiveOffCenterRH(
		(float)xmin, 
		(float)xmax, 
		(float)ymin,
		(float)ymax,
		(float)zNear,
		(float)zFar
	) * ms->Top();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	glFrustum(xmin, xmax, ymin, ymax, zNear, zFar);
	CHECK_GL_ERRORS();
#endif

	++matrixOps;
}

inline void GLTable::MultMatrix(const Mat4 &m) {
	GLMatrixStack *ms = MATRIXP;
	Mat4 temp = m.Transpose();
	ms->Top() = temp * ms->Top();
#if defined(RAD_OPT_OGLES1_AND_2)
	if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
	{
		glMultMatrixf((const float*)&temp);
		CHECK_GL_ERRORS();
	}
#endif
	++matrixOps;
}

inline Mat4 GLTable::GetModelViewMatrix() {
	return mv.Top();
}

inline Mat4 GLTable::GetProjectionMatrix() {
	return prj.Top();
}

inline Mat4 GLTable::GetModelViewProjectionMatrix() {
	return mv.Top() * prj.Top();
}

#undef MATRIXP

inline void GLTable::Color4f(float r, float g, float b, float a, bool force) {
	if (force ||
		r != color[0] ||
		g != color[1] ||
		b != color[2] ||
		a != color[3])
	{
		color[0] = r;
		color[1] = g;
		color[2] = b;
		color[3] = a;
#if defined(RAD_OPT_OGLES1_AND_2)
		if (!ogles2)
#endif
#if !defined(RAD_OPT_OGLES) || defined(RAD_OPT_OGLES1)
		glColor4f(r, g, b, a);
#endif
		++colorOps;
	}
}

inline void GLTable::GetColor4fv(float *v) {
	v[0] = color[0];
	v[1] = color[1];
	v[2] = color[2];
	v[3] = color[3];
}

inline void GLTable::SetEye(const float *_eye) {
	eye[0] = _eye[0];
	eye[1] = _eye[1];
	eye[2] = _eye[2];
	++eyeOps;
}

inline void GLTable::GetEye(float *_eye) {
	_eye[0] = eye[0];
	_eye[1] = eye[1];
	_eye[2] = eye[2];
}

} // r
