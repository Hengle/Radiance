/*! \file EditorColorPicker.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#pragma once

#include "../../Types.h"
#include <QtGui/QWidget>
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class RADENG_CLASS ColorPicker : public QWidget {
	Q_OBJECT
public:

	ColorPicker(
		const Vec4 &defaultColor = Vec4(1.f,1.f,1.f,1.f),
		QWidget *parent = 0, 
		Qt::WindowFlags f = 0
	);

	RAD_DECLARE_PROPERTY(ColorPicker, color, const Vec4&, const Vec4&);

signals:

	void OnColorChanged(const Vec4 &rgba);

protected:

	virtual void paintEvent(QPaintEvent *e);
	virtual void mousePressEvent(QMouseEvent *e);

private:

	RAD_DECLARE_GET(color, const Vec4&) {
		return m_rgba;
	}

	RAD_DECLARE_SET(color, const Vec4&);

	void SetBackgroundColor();

	Vec4 m_rgba;
	QColor m_color;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
