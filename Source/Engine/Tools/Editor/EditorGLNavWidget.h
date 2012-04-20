// EditorGLNavWidget.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorGLWidget.h"
#include "../../Camera.h"
#include <QtCore/QPoint>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class RADENG_CLASS GLNavWidget : public GLWidget
{
	Q_OBJECT
public:

	GLNavWidget(QWidget *parent = 0, Qt::WindowFlags f = 0);
	virtual ~GLNavWidget();

	enum OrbitMode
	{
		OM_LeftButton,
		OM_MiddleButton
	};

	void SetOrbitMode(const Vec3 &pos, OrbitMode mode);
	void SetRotateMode();
	virtual void CameraMoved();

	RAD_DECLARE_READONLY_PROPERTY(GLNavWidget, camera, Camera*);
	RAD_DECLARE_PROPERTY(GLNavWidget, orbitSpeed, float, float);
	RAD_DECLARE_PROPERTY(GLNavWidget, lookSpeed, float, float);
	RAD_DECLARE_PROPERTY(GLNavWidget, mouseSpeed, float, float);
	RAD_DECLARE_PROPERTY(GLNavWidget, wheelSpeed, float, float);
	RAD_DECLARE_PROPERTY(GLNavWidget, kbSpeed, float, float);

signals:

	void OnCameraMoved();

protected:

	virtual void wheelEvent(QWheelEvent *e);
	virtual void mouseMoveEvent(QMouseEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);
	virtual void mouseReleaseEvent(QMouseEvent *e);
	virtual void keyPressEvent(QKeyEvent *e);

private:

	RAD_DECLARE_GET(camera, Camera*) { return &const_cast<GLNavWidget*>(this)->m_c; }
	RAD_DECLARE_GET(orbitSpeed, float) { return m_orbit; }
	RAD_DECLARE_SET(orbitSpeed, float) { m_orbit = value; }
	RAD_DECLARE_GET(lookSpeed, float) { return m_look; }
	RAD_DECLARE_SET(lookSpeed, float) { m_look = value; }
	RAD_DECLARE_GET(mouseSpeed, float) { return m_mouse; }
	RAD_DECLARE_SET(mouseSpeed, float) { m_mouse = value; }
	RAD_DECLARE_GET(wheelSpeed, float) { return m_wheel; }
	RAD_DECLARE_SET(wheelSpeed, float) { m_wheel = value; }
	RAD_DECLARE_GET(kbSpeed, float) { return m_kb; }
	RAD_DECLARE_SET(kbSpeed, float) { m_kb = value; }

	enum Mode
	{
		M_Orbit,
		M_Rotate
	};

	Camera m_c;
	float m_orbit;
	float m_look;
	float m_mouse;
	float m_wheel;
	float m_kb;
	Mode m_mode;
	QPoint m_p;
	Vec3 m_orbitPos;
	OrbitMode m_orbitMode;
};

} // editor
} // tools


#include <Runtime/PopPack.h>
