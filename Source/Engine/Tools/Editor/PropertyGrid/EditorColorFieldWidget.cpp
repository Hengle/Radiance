// EditorColorFieldWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorColorFieldWidget.h"
#include "../EditorMainWindow.h"
#include "../EditorUtils.h"
#include <Runtime/File.h>
#include <QtGui/QHBoxLayout>
#include <QtGui/QLineEdit>
#include <QtGui/QPushButton>
#include <QtGui/QColorDialog>
#include <QtGui/QPainter>
#include <QtGui/QSizePolicy>
#include <string.h>

namespace tools {
namespace editor {

ColorFieldWidget::ColorFieldWidget(QWidget *parent)
: QWidget(parent)
{
	QHBoxLayout *l = new QHBoxLayout(this);
	l->setContentsMargins(0, 0, 0, 0);
	m_edit = new QLineEdit();
	l->addWidget(m_edit, 1);
	QHBoxLayout *l2 = new QHBoxLayout();
	QPushButton *b = new QPushButton("...");
	b->setMinimumWidth(30);
	b->setMaximumWidth(30);
	b->setSizePolicy(QSizePolicy::Minimum, QSizePolicy::Maximum);
	l2->addWidget(b, 0);
	l->addLayout(l2, 0);
	l->setSpacing(0);
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(BrowseClicked())));
	RAD_VERIFY(connect(m_edit, SIGNAL(editingFinished()), SLOT(EditLineFinished())));
}

void ColorFieldWidget::SetColor(const QString &color)
{
	m_color = color;
	m_edit->setText(color);
	m_edit->selectAll();
}

void ColorFieldWidget::BrowseClicked()
{
	char sz[64];
	int r, g, b, a;
	sscanf(m_color.toAscii().constData(), "%d %d %d %d",
		&r, &g, &b, &a
	);
	QColorDialog d(QColor(r, g, b, a), m_edit);

	if (d.exec())
	{
		QColor c = d.selectedColor();
		sprintf(sz, "%d %d %d %d", 
			c.red(), c.green(), c.blue(), c.alpha()
		);

		SetColor(sz);
		emit CloseEditor();
	}
}

void ColorFieldWidget::EditLineFinished()
{
	m_color = m_edit->text();
}

void ColorFieldWidget::focusInEvent(QFocusEvent*)
{
	m_edit->setFocus();
}

} // editor
} // tools

#include "moc_EditorColorFieldWidget.cc"
