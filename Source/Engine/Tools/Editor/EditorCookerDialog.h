// EditorCookerDialog.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../../Packages/Packages.h"
#include "EditorTypes.h"
#include "EditorEventRegistry.h"
#include <Runtime/Stream/STLStream.h>
#include <QtCore/QThread>
#include <QtGui/QDialog>
#include <QtCore/QMutex>
#include <Runtime/PushPack.h>

class QPlainTextEdit;
class QThread;
class QSignalMapper;
class QPushButton;
class QCloseEvent;
class QSlider;
class QCheckBox;

namespace tools {
namespace editor {

class GLWidget;

class CookThread : public QThread
{
	Q_OBJECT
public:
	CookThread(
		GLWidget *glw,
		const pkg::PackageMan::StringVec &roots,
		int plats, 
		int compression,
		std::ostream &cout
	);

	void Cancel();

signals:

	void Finished();

protected:

	virtual void run();

private:

	pkg::PackageMan::StringVec m_roots;
	std::ostream *m_cout;
	GLWidget *m_glw;
	int m_plats;
	int m_compression;
};

class RADENG_CLASS CookerDialog : public QDialog
{
	Q_OBJECT
public:

	CookerDialog(QWidget *parent=0);
	virtual ~CookerDialog();

	void Print(const char *msg);
	static void SPrint(const char *msg);

protected:

	virtual void customEvent(QEvent *e);

private slots:

	virtual void closeEvent(QCloseEvent *e);
	void OnCheckBoxChanged(int flags);
	void CookClicked();
	void CookFinished();
	void CompressionChanged(int value);
	void CleanChecked(int value);
	void ScriptsOnly(int value);

private:

	struct PrintMsgEvent : public QEvent
	{
		PrintMsgEvent(const char *_msg) : QEvent((QEvent::Type)EV_CookWindowPrint)
		{
			RAD_ASSERT(_msg);
			msg   = _msg;
		};

		String msg;
	};

	typedef boost::shared_ptr<std::ostream> OStreamRef;

	class CookerStringBuf : public stream::basic_stringbuf<char>
	{
	public:
		typedef stream::basic_stringbuf<char> Super;
		typedef Super::StringType StringType;
		typedef boost::shared_ptr<CookerStringBuf> Ref;

		CookerStringBuf(CookerDialog *cooker) : m_cooker(cooker)
		{
		}

	protected:

		virtual int Flush(const StringType &str)
		{
			m_cooker->Print(str.c_str());
			return 0;
		}

	private:
		CookerDialog *m_cooker;
	};
	
	void OnPrintMsg(const PrintMsgEvent &msg);

	static QMutex s_m;
	static CookerDialog *s_instance;

	OStreamRef m_oStream;
	CookerStringBuf::Ref m_stringBuf;
	int m_platforms;
	QSignalMapper *m_sigMap;
	QPlainTextEdit *m_textArea;
	QPushButton *m_cook;
	CookThread *m_thread;
	QSlider *m_compression;
	QCheckBox *m_clean;
	QCheckBox *m_scriptsOnly;
	GLWidget *m_glw;
	bool m_closeWhenFinished;
};

} // editor
} // tools

#include <Runtime/PopPack.h>

