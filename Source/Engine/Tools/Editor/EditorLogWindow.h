// EditorLogWindow.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "EditorTypes.h"
#include "EditorEventRegistry.h"
#include <QtGui/QDialog>
#include <QtGui/QResizeEvent>
#include <QtCore/QMutex>
#include <Runtime/PushPack.h>

class QPlainTextEdit;

namespace tools {
namespace editor {

class MainWindow;

class RADENG_CLASS LogWindow : public QDialog
{
	Q_OBJECT
public:

	static bool SPrint(int level, const char *msg);

	LogWindow(QWidget *parent=0);
	virtual ~LogWindow();
	void Print(int level, const char *msg);
	void EnableCloseButton();
	void ExitAfterDialog();

	virtual void setVisible(bool visible);

protected:

	struct PrintMsgEvent : public QEvent
	{
		PrintMsgEvent(int _level, const char *_msg) : QEvent((QEvent::Type)EV_LogWindowPrint)
		{
			RAD_ASSERT(_msg);
			level = _level;
			msg   = _msg;
		};

		int level;
		String msg;
	};

	virtual void resizeEvent(QResizeEvent *e);
	virtual void customEvent(QEvent *e);

private:
	friend class MainWindow;

	void OnPrintMsg(const PrintMsgEvent &msg);

	bool m_exitAfterDialog;
	QPlainTextEdit *m_textArea;

	static QMutex s_m;
	static LogWindow *s_instance;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
