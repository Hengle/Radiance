// EditorSearchLineWidget.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "EditorSearchLineWidget.h"
#include "EditorUtils.h"
#include <QtGui/QLineEdit>
#include <QtGui/QLabel>
#include <QtGui/QPushButton>
#include <QtGui/QGridLayout>
#include <QtGui/QSpacerItem>

namespace tools {
namespace editor {

SearchLineWidget::SearchLineWidget(QWidget *parent, Qt::WindowFlags f) : QWidget(parent, f)
{
	QGridLayout *g = new (ZEditor) QGridLayout(this);
	
	QLabel *l = new (ZEditor) QLabel(this);
	l->setPixmap(LoadPixmap(L"Editor/search.png"));
	g->addWidget(l, 0, 0);
	
	m_lineEdit = new (ZEditor) QLineEdit(this);
	g->addWidget(m_lineEdit, 0, 1);
	g->setColumnStretch(1, 1);
	RAD_VERIFY(connect(m_lineEdit, SIGNAL(textChanged(const QString&)), SIGNAL(textChanged(const QString&))));
	RAD_VERIFY(connect(m_lineEdit, SIGNAL(returnPressed()), SIGNAL(returnPressed())));

	QPushButton *b = new (ZEditor) QPushButton(
		LoadIcon(L"Editor/x.png"),
		QString(),
		this
	);
	b->setIconSize(QSize(16, 16));
	b->setFlat(true);
	b->setToolTip("Clear Search Filter");
	RAD_VERIFY(connect(b, SIGNAL(clicked()), SLOT(clear())));
	g->addWidget(b, 0, 2);
}

QSize SearchLineWidget::sizeHint() const
{
	return QSize(300, m_lineEdit->sizeHint().height());
}

void SearchLineWidget::clear()
{
	m_lineEdit->clear();
}

QString SearchLineWidget::RAD_IMPLEMENT_GET(text)
{
	return m_lineEdit->text();
}

void SearchLineWidget::RAD_IMPLEMENT_SET(text)(QString s)
{
	m_lineEdit->setText(s);
}

} // editor
} // tools

#include "moc_EditorSearchLineWidget.cc"
