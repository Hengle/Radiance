/*! \file EditorMapEditorTypes.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup map_editor
*/

#pragma once

#include "../EditorTypes.h"
#include "../../../Packages/Packages.h"
#include "../../../Renderer/GL/GLState.h"
#include "../../../Camera.h"
#include <QtCore/QObject>
#include <QtCore/QSize>
#include <QtCore/QPoint>

namespace tools {
namespace editor {
namespace map_editor {

///////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS XForm {
public:

	XForm();

	XForm(const XForm &xf);

	XForm(
		const Vec3D &pos,
		const QuatD &rot
	);
	
	XForm(
		const Vec3 &pos,
		const Mat4 &rot
	);
	
	XForm(
		const Vec3 &pos,
		const Vec3 &fwd,
		const Vec3 &left,
		const Vec3 &up
	);

	RAD_DECLARE_READONLY_PROPERTY(XForm, pos, const Vec3*);
	RAD_DECLARE_READONLY_PROPERTY(XForm, mat, const Mat4*);
	RAD_DECLARE_READONLY_PROPERTY(XForm, quat, const Quat*);
	RAD_DECLARE_READONLY_PROPERTY(XForm, fwd, const Vec3*);
	RAD_DECLARE_READONLY_PROPERTY(XForm, left, const Vec3*);
	RAD_DECLARE_READONLY_PROPERTY(XForm, up, const Vec3*);

private:

	RAD_DECLARE_GET(pos, const Vec3*) {
		return &m_pos;
	}

	RAD_DECLARE_GET(mat, const Mat4*) {
		return &m_mat;
	}

	RAD_DECLARE_GET(quat, const Quat*) {
		return &m_quat;
	}

	RAD_DECLARE_GET(fwd, const Vec3*) {
		return &m_fwd;
	}

	RAD_DECLARE_GET(left, const Vec3*) {
		return &m_left;
	}

	RAD_DECLARE_GET(up, const Vec3*) {
		return &m_up;
	}

	Mat4 m_mat;
	Quat m_quat;
	Vec3 m_pos;
	Vec3 m_fwd;
	Vec3 m_left;
	Vec3 m_up;
};

} // map_editor
} // editor
} // tools
