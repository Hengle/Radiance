// EditorUtils.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "EditorUtils.h"
#include "EditorMainWindow.h"
#include <Runtime/Thread/Thread.h>
#include <Runtime/StringBase.h>
#include <Runtime/File.h>
#include <Runtime/Time.h>
#include "../../Engine.h"
#include "../../Input.h"
#include "../../Game/Game.h"
#include <QtGui/QWheelEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QKeyEvent>

#undef LoadIcon

namespace tools {
namespace editor {

void PostInputEvent(QWheelEvent *event, Game &game)
{
	InputEvent i;
	i.type = InputEvent::T_MouseWheel;
	i.time = xtime::ReadMilliseconds();
	i.data[0] = event->x();
	i.data[1] = event->y();
	i.data[2] = event->delta();
	game.PostInputEvent(i);
}

void PostInputEvent(QMouseEvent *event, Game &game, bool press, bool move)
{
	InputEvent i;
	i.type = move ? InputEvent::T_MouseMove : press ? InputEvent::T_MouseDown : InputEvent::T_MouseUp;
	i.time = xtime::ReadMilliseconds();
	i.data[0] = event->x();
	i.data[1] = event->y();
	int b = 0;
	if ((event->buttons() & Qt::LeftButton) || event->button() == Qt::LeftButton)
		b |= kMouseButton_Left;
	if ((event->buttons() & Qt::RightButton) || event->button() == Qt::RightButton)
		b |= kMouseButton_Right;
	if ((event->buttons() & Qt::MidButton) || event->button() == Qt::MidButton)
		b |= kMouseButton_Middle;
	i.data[2] = b;
	game.PostInputEvent(i);

	if (event->type() == QEvent::MouseButtonDblClick)
	{ // force mouse up as well.
		i.type = InputEvent::T_MouseUp;
		game.PostInputEvent(i);
	}
}

void PostInputEvent(QKeyEvent *event, Game &game, bool press)
{
	if (event->count() > 1)
		return;

	int key = 0;

	switch (event->key())
	{
	case Qt::Key_Escape:
		key = kKeyCode_Escape;
		break;
	case Qt::Key_Tab:
		key = kKeyCode_Tab;
		break;
	case Qt::Key_Backspace:
		key = kKeyCode_Backspace;
		break;
	case Qt::Key_Return:
		key = kKeyCode_Return;
		break;
	case Qt::Key_Enter:
		key = kKeyCode_KP_Enter;
		break;
	case Qt::Key_Insert:
		key = kKeyCode_Insert;
		break;
	case Qt::Key_Delete:
		key = kKeyCode_Delete;
		break;
	case Qt::Key_Pause:
		key = kKeyCode_Pause;
		break;
	case Qt::Key_SysReq:
		key = kKeyCode_SysReq;
		break;
	case Qt::Key_Clear:
		key = kKeyCode_Clear;
		break;
	case Qt::Key_Home:
		key = kKeyCode_Home;
		break;
	case Qt::Key_End:
		key = kKeyCode_End;
		break;
	case Qt::Key_Left:
		key = kKeyCode_Left;
		break;
	case Qt::Key_Up:
		key = kKeyCode_Up;
		break;
	case Qt::Key_Right:
		key = kKeyCode_Right;
		break;
	case Qt::Key_Down:
		key = kKeyCode_Down;
		break;
	case Qt::Key_PageUp:
		key = kKeyCode_PageUp;
		break;
	case Qt::Key_PageDown:
		key = kKeyCode_PageDown;
		break;
	case Qt::Key_Shift:
		key = kKeyCode_LShift;
		break;
	case Qt::Key_Control:
		key = kKeyCode_LCtrl;
		break;
	case Qt::Key_Alt:
		key = kKeyCode_LAlt;
		break;
	case Qt::Key_CapsLock:
		key = kKeyCode_CapsLock;
		break;
	case Qt::Key_NumLock:
		key = kKeyCode_NumLock;
		break;
	case Qt::Key_ScrollLock:
		key = kKeyCode_ScrollLock;
		break;
	case Qt::Key_F1:
		key = kKeyCode_F1;
		break;
	case Qt::Key_F2:
		key = kKeyCode_F2;
		break;
	case Qt::Key_F3:
		key = kKeyCode_F3;
		break;
	case Qt::Key_F4:
		key = kKeyCode_F4;
		break;
	case Qt::Key_F5:
		key = kKeyCode_F5;
		break;
	case Qt::Key_F6:
		key = kKeyCode_F6;
		break;
	case Qt::Key_F7:
		key = kKeyCode_F7;
		break;
	case Qt::Key_F8:
		key = kKeyCode_F8;
		break;
	case Qt::Key_F9:
		key = kKeyCode_F9;
		break;
	case Qt::Key_F10:
		key = kKeyCode_F10;
		break;
	case Qt::Key_F11:
		key = kKeyCode_F11;
		break;
	case Qt::Key_F12:
		key = kKeyCode_F12;
		break;
	case Qt::Key_Space:
		key = kKeyCode_Space;
		break;
	case Qt::Key_Exclam:
		key = kKeyCode_Exclaim;
		break;
	case Qt::Key_QuoteDbl:
		key = kKeyCode_QuoteDbl;
		break;
	case Qt::Key_NumberSign:
		key = kKeyCode_Hash;
		break;
	case Qt::Key_Dollar:
		key = kKeyCode_DollarSign;
		break;
	case Qt::Key_Percent:
		key = kKeyCode_Percent;
		break;
	case Qt::Key_Ampersand:
		key = kKeyCode_Ampersand;
		break;
	case Qt::Key_Apostrophe:
		key = kKeyCode_SingleQuote;
		break;
	case Qt::Key_ParenLeft:
		key = kKeyCode_ParenLeft;
		break;
	case Qt::Key_ParenRight:
		key = kKeyCode_ParenRight;
		break;
	case Qt::Key_Asterisk:
		key = kKeyCode_Asterisk;
		break;
	case Qt::Key_Plus:
		key = kKeyCode_Plus;
		break;
	case Qt::Key_Comma:
		key = kKeyCode_Comma;
		break;
	case Qt::Key_Minus:
		key = kKeyCode_Minus;
		break;
	case Qt::Key_Period:
		key = kKeyCode_Period;
		break;
	case Qt::Key_Slash:
		key = kKeyCode_Slash;
		break;
	case Qt::Key_0:
		key = kKeyCode_0;
		break;
	case Qt::Key_1:
		key = kKeyCode_1;
		break;
	case Qt::Key_2:
		key = kKeyCode_2;
		break;
	case Qt::Key_3:
		key = kKeyCode_3;
		break;
	case Qt::Key_4:
		key = kKeyCode_4;
		break;
	case Qt::Key_5:
		key = kKeyCode_5;
		break;
	case Qt::Key_6:
		key = kKeyCode_6;
		break;
	case Qt::Key_7:
		key = kKeyCode_7;
		break;
	case Qt::Key_8:
		key = kKeyCode_8;
		break;
	case Qt::Key_9:
		key = kKeyCode_9;
		break;
	case Qt::Key_Colon:
		key = kKeyCode_Colon;
		break;
	case Qt::Key_Semicolon:
		key = kKeyCode_Semicolon;
		break;
	case Qt::Key_Less:
		key = kKeyCode_Less;
		break;
	case Qt::Key_Equal:
		key = kKeyCode_Equals;
		break;
	case Qt::Key_Greater:
		key = kKeyCode_Greater;
		break;
	case Qt::Key_Question:
		key = kKeyCode_Question;
		break;
	case Qt::Key_At:
		key = kKeyCode_At;
		break;
	case Qt::Key_A:
		key = kKeyCode_A;
		break;
	case Qt::Key_B:
		key = kKeyCode_B;
		break;
	case Qt::Key_C:
		key = kKeyCode_C;
		break;
	case Qt::Key_D:
		key = kKeyCode_D;
		break;
	case Qt::Key_E:
		key = kKeyCode_E;
		break;
	case Qt::Key_F:
		key = kKeyCode_F;
		break;
	case Qt::Key_G:
		key = kKeyCode_G;
		break;
	case Qt::Key_H:
		key = kKeyCode_H;
		break;
	case Qt::Key_I:
		key = kKeyCode_I;
		break;
	case Qt::Key_J:
		key = kKeyCode_J;
		break;
	case Qt::Key_K:
		key = kKeyCode_K;
		break;
	case Qt::Key_L:
		key = kKeyCode_L;
		break;
	case Qt::Key_M:
		key = kKeyCode_M;
		break;
	case Qt::Key_N:
		key = kKeyCode_N;
		break;
	case Qt::Key_O:
		key = kKeyCode_O;
		break;
	case Qt::Key_P:
		key = kKeyCode_P;
		break;
	case Qt::Key_Q:
		key = kKeyCode_Q;
		break;
	case Qt::Key_R:
		key = kKeyCode_R;
		break;
	case Qt::Key_S:
		key = kKeyCode_S;
		break;
	case Qt::Key_T:
		key = kKeyCode_T;
		break;
	case Qt::Key_U:
		key = kKeyCode_U;
		break;
	case Qt::Key_V:
		key = kKeyCode_V;
		break;
	case Qt::Key_W:
		key = kKeyCode_W;
		break;
	case Qt::Key_X:
		key = kKeyCode_X;
		break;
	case Qt::Key_Y:
		key = kKeyCode_Y;
		break;
	case Qt::Key_Z:
		key = kKeyCode_Z;
		break;
	case Qt::Key_BracketLeft:
		key = kKeyCode_BracketLeft;
		break;
	case Qt::Key_Backslash:
		key = kKeyCode_BackSlash;
		break;
	case Qt::Key_BracketRight:
		key = kKeyCode_BracketRight;
		break;
	case Qt::Key_AsciiCircum:
		key = kKeyCode_Caret;
		break;
	case Qt::Key_Underscore:
		key = kKeyCode_Underscore;
		break;
	case Qt::Key_QuoteLeft:
		key = kKeyCode_BackQuote;
		break;
	case Qt::Key_BraceLeft:
		key = kKeyCode_BraceLeft;
		break;
	case Qt::Key_Bar:
		key = kKeyCode_Bar;
		break;
	case Qt::Key_BraceRight:
		key = kKeyCode_BraceRight;
		break;
	case Qt::Key_AsciiTilde:
		key = kKeyCode_Tilde;
		break;
	default:
		return;
	}

	InputEvent i;
	i.repeat = event->isAutoRepeat();
	i.type = press ? InputEvent::T_KeyDown : InputEvent::T_KeyUp;
	i.time = xtime::ReadMilliseconds();
	i.data[0] = key;

	QString unicodeString = event->text();
	i.unicode = unicodeString.toUtf8().data();
	
	game.PostInputEvent(i);
}

const pkg::PackageManRef &Packages()
{
	return App::Get()->engine->sys->packages;
}

const file::FileSystem::Ref &Files()
{
	return App::Get()->engine->sys->files;
}

r::HRenderer Renderer()
{
	return App::Get()->engine->sys->r;
}

String ExpandBaseDir(const char *append)
{
	String native;
	if (Files()->ExpandToNativePath("", native, file::kFileMask_Base))
	{
		if (append)
			native += append;
		return native;
	}

	return String();
}

bool IsGuiThread()
{
	return App::Get()->mainThreadId == thread::ThreadId();
}

bool LoadPixmap(const char *filename, QPixmap &pixmap)
{
	file::MMapping::Ref mm = Files()->MapFile(filename, ZTools);
	if (!mm)
		return false;

	return pixmap.loadFromData(
		(const uchar*)mm->data.get(),
		(uint)mm->size.get()
	);
}

bool LoadIcon(const char *filename, QIcon &icon)
{
	QPixmap pixmap;
	if (!LoadPixmap(filename, pixmap)) 
	{
		return false;
	}
	icon = QIcon(pixmap);
	return true;
}

} // editor
} // tools
