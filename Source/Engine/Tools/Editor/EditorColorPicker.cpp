/*! \file EditorColorPicker.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup tools
*/

#include RADPCH
#include "EditorColorPicker.h"
#include <QtGui/QColorDialog>
#include <QtGui/QMouseEvent>

namespace tools {
namespace editor {

ColorPicker::ColorPicker(
	const Vec4 &defaultColor,
	QWidget *parent, 
	Qt::WindowFlags f
) : m_rgba(defaultColor), QWidget(parent, f) {
	setAutoFillBackground(true);
	SetBackgroundColor();
}

void ColorPicker::paintEvent(QPaintEvent *e) {
	QWidget::paintEvent(e);
}

void ColorPicker::mousePressEvent(QMouseEvent *e) {
	QColorDialog d(m_color, this);
	if (d.exec()) {
		QColor c = d.selectedColor();
		Vec4 z(
			c.red() / 255.f,
			c.green() / 255.f,
			c.blue() / 255.f,
			c.alpha() / 255.f
		);
		this->color = z;
		emit OnColorChanged(z);
	}

	e->setAccepted(true);
}

void ColorPicker::RAD_IMPLEMENT_SET(color)(const Vec4 &value) {
	m_rgba = value;
	SetBackgroundColor();
	update();
}

void ColorPicker::SetBackgroundColor() {
	m_color = QColor(
		(int)(m_rgba[0] * 255.f),
		(int)(m_rgba[1] * 255.f),
		(int)(m_rgba[2] * 255.f),
		(int)(m_rgba[3] * 255.f)
	);

	QPalette p(palette());
	p.setColor(QPalette::Background, m_color);
	setPalette(p);
}

} // editor
} // tools

#include "moc_EditorColorPicker.cc"
