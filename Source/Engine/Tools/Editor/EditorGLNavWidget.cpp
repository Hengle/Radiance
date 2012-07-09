// EditorGLNavWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorGLNavWidget.h"
#include <QtGui/QMouseEvent>
#include <QtGui/QWheelEvent>
#include <QtGui/QKeyEvent>
#undef min

namespace tools {
namespace editor {

GLNavWidget::GLNavWidget(QWidget *parent, Qt::WindowFlags f) : GLWidget(parent, f),
m_orbit(0.f),
m_look(0.f),
m_mouse(0.f),
m_wheel(0.f),
m_kb(0.f),
m_mode(M_Rotate),
m_orbitMode(OM_LeftButton)
{
}

GLNavWidget::~GLNavWidget()
{
}

void GLNavWidget::SetOrbitMode(const Vec3 &pos, OrbitMode mode)
{
	m_mode = M_Orbit;
	m_orbitPos = pos;
	m_orbitMode = mode;
}

void GLNavWidget::SetRotateMode()
{
	m_mode = M_Rotate;
}

void GLNavWidget::CameraMoved()
{
	emit OnCameraMoved();
}

void GLNavWidget::wheelEvent(QWheelEvent *e)
{
	if (e->orientation() == Qt::Vertical)
	{
		float move = m_wheel * e->delta() / 8.f;
		if (move != 0.f)
		{
			if (m_mode == M_Orbit)
			{ // don't move through orbit center
				Vec3 v = m_orbitPos - m_c.pos.get();
				move = std::min(move, v.Magnitude()-10.f);
			}
			m_c.pos = m_c.pos.get() + (move*m_c.fwd.get());
			CameraMoved();
		}
	}
}

void GLNavWidget::mouseMoveEvent(QMouseEvent *e)
{
	GLWidget::mouseMoveEvent(e); // do signal

	bool move = false;
	bool moveLook = false;

	if (e->buttons()&(Qt::LeftButton|Qt::RightButton))
	{
		if (m_mode == M_Rotate || (m_mode == M_Orbit && m_orbitMode == OM_LeftButton))
		{
			move = true;
			moveLook = e->buttons() == Qt::LeftButton;
		}
	}
	else if(e->buttons()&Qt::MidButton)
	{
		if (m_mode == M_Orbit && m_orbitMode == OM_MiddleButton)
			move = true;
	}

	if (!move)
		return;

	QPoint d = e->pos()-m_p;
	if (d.x() == 0 && d.y() == 0)
		return;

	if (m_mode != M_Orbit && m_mode != M_Rotate)
		return;

	if (m_mode == M_Orbit && m_orbit == 0.f)
		return;
	if (m_mode == M_Rotate && m_mouse == 0.f)
		return;

	float dx = d.x() * ((m_mode==M_Orbit) ? moveLook ? m_look : m_orbit : m_mouse);
	float dy = d.y() * ((m_mode==M_Orbit) ? moveLook ? m_look : m_orbit : m_mouse);

	if (m_mode==M_Orbit && moveLook)
	{
		Quat q = Quat(Vec3(0.f, 0.f, 1.f), math::DegToRad(-dx)) *
			Quat(m_c.left.get(), math::DegToRad(dy));

		m_c.rot = q * m_c.rot.get();
	}
	else
	{
		Vec3 fwd = m_c.fwd;

		Quat q = Quat(Vec3(0.f, 0.f, 1.f), math::DegToRad(-dx)) *
			Quat(m_c.left.get(), math::DegToRad(dy));

		Mat4 m = Mat4::Rotation(q);

		Vec3 v;
		if (m_mode == M_Orbit)
			v = ((m_c.pos.get() - m_orbitPos)*m)+m_orbitPos;
		else
			v = m_c.pos;

		fwd = fwd*m;

		m_c.rot = q * m_c.rot.get();
		m_c.pos = v;
	}

	m_p = e->pos();

	CameraMoved();
}

void GLNavWidget::mousePressEvent(QMouseEvent *e)
{
	GLWidget::mousePressEvent(e); // do signal

	bool capture=false;

	if (e->buttons()&(Qt::LeftButton|Qt::RightButton))
	{
		if (m_mode == M_Rotate || m_orbitMode == OM_LeftButton)
			capture = true;
	}
	else if(e->buttons()&Qt::MidButton)
	{
		if (m_mode == M_Orbit && m_orbitMode == OM_MiddleButton)
			capture = true;
	}

	if (capture)
		m_p = e->pos();
}

void GLNavWidget::mouseReleaseEvent(QMouseEvent *e)
{
}

void GLNavWidget::keyPressEvent(QKeyEvent *e)
{
}

} // editor
} // tools

#include "moc_EditorGLNavWidget.cc"
