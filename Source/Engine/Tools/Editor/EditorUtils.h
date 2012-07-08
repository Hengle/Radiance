// EditorUtils.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <QtCore/QEvent>
#include <QtGui/QWidget>
#include <QtGui/QCursor>
#include <QtGui/QPixmap>
#include <QtGui/QIcon>
#include <QtGui/QApplication>
#include "../../App.h"
#include <Runtime/PushPack.h>

#undef LoadIcon

class App;
class Game;
class QWheelEvent;
class QMouseEvent;
class QKeyEvent;

namespace tools {
namespace editor {

pkg::PackageManRef Packages();
file::HFileSystem Files();
r::HRenderer Renderer();

void PostInputEvent(QWheelEvent *event, Game &game);
void PostInputEvent(QMouseEvent *event, Game &game, bool press, bool move);
void PostInputEvent(QKeyEvent *event, Game &game, bool press);

String ExpandBaseDir(const char *append=0);

bool IsGuiThread();

bool LoadPixmap(const char *filename, QPixmap &pixmap);
bool LoadIcon(const char *filename, QIcon &icon);

inline QPixmap LoadPixmap(const char *filename)
{
	QPixmap pixmap;
	LoadPixmap(filename, pixmap);
	return pixmap;
}

inline QIcon LoadIcon(const char *filename)
{
	QIcon icon;
	LoadIcon(filename, icon);
	return icon;
}

inline void PercentSize(QWidget &x, const QRect &rect, float w, float h)
{
	x.resize((int)(rect.width()*w), (int)(rect.height()*h));
}

inline void PercentSize(QWidget &x, const QWidget &outer, float w, float h)
{
	PercentSize(x, outer.geometry(), w, h);
}

inline void CenterWidget(QWidget &x, const QRect &rect)
{
	x.move(rect.x()+((rect.width()-x.width())/2), rect.y()+((rect.height()-x.height())/2));
}

inline void CenterWidget(QWidget &inner, const QWidget &outer)
{
	CenterWidget(inner, outer.geometry());
}

inline void QDispatch()
{
	QApplication::processEvents();
	QApplication::flush();
}

class SetCursor
{
public:
	SetCursor(Qt::CursorShape shape, QWidget *win = 0) : m_win(win)
	{
		if (win)
		{
			win->setCursor(QCursor(shape));
		}
		else
		{
			QApplication::setOverrideCursor(QCursor(shape));
		}
	}

	SetCursor(const QCursor &cursor, QWidget *win = 0) : m_win(win)
	{
		if (win)
		{
			win->setCursor(cursor);
		}
		else
		{
			QApplication::setOverrideCursor(cursor);
		}
	}

	~SetCursor()
	{
		if (m_win)
		{
			m_win->unsetCursor();
		}
		else
		{
			QApplication::restoreOverrideCursor();
		}
	}

private:
	QWidget *m_win;
};

} // editor
} // tools

#include <Runtime/PopPack.h>
