// EditorProgressDialog.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <QtGui/QProgressDialog>
#include <QtCore/QMutex>
#include <QtCore/QEvent>
#include "EditorEventRegistry.h"
#include "../Progress.h"
#include <Runtime/PushPack.h>

namespace tools {
namespace editor {

class RADENG_CLASS ProgressDialog : public QProgressDialog, public UIProgress
{
	Q_OBJECT
public:
	ProgressDialog(
		QWidget *parent = 0,
		Qt::WFlags f = 0
	);
	ProgressDialog(
		const QString &title,
		const QString &labelText,
		const QString &cancelButtonText,
		int minimum,
		int maximum,
		QWidget * parent = 0,
		Qt::WFlags f = 0
	);

	virtual ~ProgressDialog();

	virtual void Step();
	virtual void SubStep();

	virtual Ref Child(const char *title, bool progressBar) {
		return Ref(new NullUIProgress_t());
	}

	virtual void Refresh();

protected:

	virtual QSize sizeHint() const;

	virtual RAD_DECLARE_GET(total, int);
	virtual RAD_DECLARE_SET(total, int);
	virtual RAD_DECLARE_GET(totalProgress, int);
	virtual RAD_DECLARE_SET(totalProgress, int);
	virtual RAD_DECLARE_GET(title, const char *);
	virtual RAD_DECLARE_SET(title, const char *);
	virtual RAD_DECLARE_GET(subTitle, const char *);
	virtual RAD_DECLARE_SET(subTitle, const char *);
	virtual RAD_DECLARE_GET(subTotal, int);
	virtual RAD_DECLARE_SET(subTotal, int);
	virtual RAD_DECLARE_GET(subProgress, int);
	virtual RAD_DECLARE_SET(subProgress, int);
	virtual RAD_DECLARE_GET(canceled, bool) {
		return wasCanceled();
	}

	virtual void EmitString(int level, const std::string &str);

	virtual void customEvent(QEvent *e);
private:

	struct State {
		int total[2];
		QString title;
	};

	struct RefreshEvent : public QEvent {
		RefreshEvent() : QEvent((QEvent::Type)EV_ProgressDialog) {}
		State s;
	};

	void OnRefreshEvent(const RefreshEvent &e);

	int m_total[2];
	int m_sub[2];
	String m_strings[2];
	RefreshEvent *m_inFlight;
	QMutex m_m;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
