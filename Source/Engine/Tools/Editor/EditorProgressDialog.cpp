// EditorProgressDialog.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorProgressDialog.h"
#include <QtGui/QApplication>
#include <QtGui/QDesktopWidget>

namespace tools {
namespace editor {

ProgressDialog::ProgressDialog(
	QWidget *parent,
	Qt::WFlags f
) : QProgressDialog(parent, f),
m_inFlight(0) {
	setAutoClose(false);
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumDuration(0);
	setModal(true);
	setWindowFlags(
		Qt::Dialog|
		Qt::CustomizeWindowHint|
		Qt::WindowTitleHint
	);
}

ProgressDialog::ProgressDialog(
	const QString &title,
	const QString &labelText, 
	const QString &cancelButtonText, 
	int minimum, 
	int maximum, 
	QWidget *parent, 
	Qt::WFlags f
) : QProgressDialog(labelText, cancelButtonText, minimum, maximum, parent, f),
m_inFlight(0) {
	setWindowTitle(title);
	setAutoClose(false);
	setAttribute(Qt::WA_DeleteOnClose);
	setMinimumDuration(0);
	setModal(true);
}

ProgressDialog::~ProgressDialog() {
}

void ProgressDialog::Step() { 
	m_total[1] = std::min(m_total[1]+1, m_total[0]);
}

void ProgressDialog::SubStep() { 
	m_sub[1] = std::min(m_sub[1]+1, m_sub[0]);
}

void ProgressDialog::Refresh() {
	// avoid setValue() recursion with modal progress dialog.

	bool post = false;
	m_m.lock();
	
	if (!m_inFlight) {
		m_inFlight = new (ZEditor) RefreshEvent();
		post = true;
	}

	m_inFlight->s.total[0] = m_total[0];
	m_inFlight->s.total[1] = m_total[1];
	m_inFlight->s.title = QString(m_strings[0].c_str.get());

	m_m.unlock();

	if (post)
		QCoreApplication::postEvent(this, m_inFlight);
}

QSize ProgressDialog::sizeHint() const {
	QSize s = QProgressDialog::sizeHint();
	QRect r = QApplication::desktop()->screenGeometry();
	int w = (int)(r.width() * 0.21f);
	return QSize(std::max(s.width(), w), s.height());
}

int ProgressDialog::RAD_IMPLEMENT_GET(total) {
	return m_total[0];
}

void ProgressDialog::RAD_IMPLEMENT_SET(total) (int x) {
	m_total[0] = x;
}

int ProgressDialog::RAD_IMPLEMENT_GET(totalProgress) {
	return m_total[1];
}

void ProgressDialog::RAD_IMPLEMENT_SET(totalProgress) (int x) {
	m_total[1] = x;
}

const char *ProgressDialog::RAD_IMPLEMENT_GET(title) {
	return m_strings[0].c_str;
}

void ProgressDialog::RAD_IMPLEMENT_SET(title) (const char *x) {
	if (!x) {
		m_strings[0].Clear();
		return;
	}

	m_strings[0] = x;
}

const char *ProgressDialog::RAD_IMPLEMENT_GET(subTitle) {
	return m_strings[1].c_str;
}

void ProgressDialog::RAD_IMPLEMENT_SET(subTitle) (const char *x) {
	if (!x) {
		m_strings[1].Clear();
		return;
	}

	m_strings[1] = x;
}

int ProgressDialog::RAD_IMPLEMENT_GET(subTotal) {
	return m_sub[0];
}

void ProgressDialog::RAD_IMPLEMENT_SET(subTotal) (int x) {
	m_sub[0] = x;
}

int ProgressDialog::RAD_IMPLEMENT_GET(subProgress) {
	return m_sub[1];
}

void ProgressDialog::RAD_IMPLEMENT_SET(subProgress) (int x) {
	m_sub[1] = x;
}

void ProgressDialog::EmitString(int, const std::string &) {
}

void ProgressDialog::customEvent(QEvent *e) {
	switch (e->type()) {
	case EV_ProgressDialog:
		OnRefreshEvent(*static_cast<RefreshEvent*>(e));
		break;
	}
}

void ProgressDialog::OnRefreshEvent(const RefreshEvent &e) {
	m_m.lock();
	setMinimum(0);
	setMaximum(e.s.total[0]);
	setValue(e.s.total[1]);
	if (e.s.title != labelText()) {
		setLabelText(e.s.title);
	}
	m_inFlight = 0;
	m_m.unlock();
}

} // editor
} // tools

#include "moc_EditorProgressDialog.cc"
