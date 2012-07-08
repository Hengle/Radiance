// EditorUtils.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

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

thread::Id __QMainThreadId();

namespace tools {
namespace editor {

void PostInputEvent(QWheelEvent *event, Game &game)
{
	InputEvent i;
	i.touch = 0;
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
	i.touch = 0;
	i.type = move ? InputEvent::T_MouseMove : press ? InputEvent::T_MouseDown : InputEvent::T_MouseUp;
	i.time = xtime::ReadMilliseconds();
	i.data[0] = event->x();
	i.data[1] = event->y();
	int b = 0;
	if ((event->buttons() & Qt::LeftButton) || event->button() == Qt::LeftButton)
		b |= SDL_BUTTON_LMASK;
	if ((event->buttons() & Qt::RightButton) || event->button() == Qt::RightButton)
		b |= SDL_BUTTON_RMASK;
	if ((event->buttons() & Qt::MidButton) || event->button() == Qt::MidButton)
		b |= SDL_BUTTON_MMASK;
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
	if (event->count() != 1 || event->isAutoRepeat())
		return;

	int key = 0;

	switch (event->key())
	{
	case Qt::Key_Escape:
		key = SDLK_ESCAPE;
		break;
	case Qt::Key_Tab:
		key = SDLK_TAB;
		break;
	case Qt::Key_Backspace:
		key = SDLK_BACKSPACE;
		break;
	case Qt::Key_Return:
		key = SDLK_RETURN;
		break;
	case Qt::Key_Enter:
		key = SDLK_KP_ENTER;
		break;
	case Qt::Key_Insert:
		key = SDLK_INSERT;
		break;
	case Qt::Key_Delete:
		key = SDLK_DELETE;
		break;
	case Qt::Key_Pause:
		key = SDLK_PAUSE;
		break;
	case Qt::Key_Print:
		key = SDLK_PRINT;
		break;
	case Qt::Key_SysReq:
		key = SDLK_PRINT;
		break;
	case Qt::Key_Clear:
		key = SDLK_CLEAR;
		break;
	case Qt::Key_Home:
		key = SDLK_HOME;
		break;
	case Qt::Key_End:
		key = SDLK_END;
		break;
	case Qt::Key_Left:
		key = SDLK_LEFT;
		break;
	case Qt::Key_Up:
		key = SDLK_UP;
		break;
	case Qt::Key_Right:
		key = SDLK_RIGHT;
		break;
	case Qt::Key_Down:
		key = SDLK_DOWN;
		break;
	case Qt::Key_PageUp:
		key = SDLK_PAGEUP;
		break;
	case Qt::Key_PageDown:
		key = SDLK_PAGEDOWN;
		break;
	case Qt::Key_Shift:
		key = SDLK_LSHIFT;
		break;
	case Qt::Key_Control:
		key = SDLK_LCTRL;
		break;
	case Qt::Key_Meta:
		key = SDLK_LMETA;
		break;
	case Qt::Key_Alt:
		key = SDLK_LALT;
		break;
	case Qt::Key_CapsLock:
		key = SDLK_CAPSLOCK;
		break;
	case Qt::Key_NumLock:
		key = SDLK_NUMLOCK;
		break;
	case Qt::Key_ScrollLock:
		key = SDLK_SCROLLOCK;
		break;
	case Qt::Key_F1:
		key = SDLK_F1;
		break;
	case Qt::Key_F2:
		key = SDLK_F2;
		break;
	case Qt::Key_F3:
		key = SDLK_F3;
		break;
	case Qt::Key_F4:
		key = SDLK_F4;
		break;
	case Qt::Key_F5:
		key = SDLK_F5;
		break;
	case Qt::Key_F6:
		key = SDLK_F6;
		break;
	case Qt::Key_F7:
		key = SDLK_F7;
		break;
	case Qt::Key_F8:
		key = SDLK_F8;
		break;
	case Qt::Key_F9:
		key = SDLK_F9;
		break;
	case Qt::Key_F10:
		key = SDLK_F10;
		break;
	case Qt::Key_F11:
		key = SDLK_F11;
		break;
	case Qt::Key_F12:
		key = SDLK_F12;
		break;
	case Qt::Key_Space:
		key = SDLK_SPACE;
		break;
	case Qt::Key_Exclam:
		key = SDLK_EXCLAIM;
		break;
	case Qt::Key_QuoteDbl:
		key = SDLK_QUOTEDBL;
		break;
	case Qt::Key_NumberSign:
		key = SDLK_HASH;
		break;
	case Qt::Key_Dollar:
		key = SDLK_DOLLAR;
		break;
	case Qt::Key_Percent:
		key = SDLK_5; // ? no SDLK_PERCENT
		break;
	case Qt::Key_Ampersand:
		key = SDLK_AMPERSAND;
		break;
	case Qt::Key_Apostrophe:
		key = SDLK_QUOTE;
		break;
	case Qt::Key_ParenLeft:
		key = SDLK_LEFTPAREN;
		break;
	case Qt::Key_ParenRight:
		key = SDLK_RIGHTPAREN;
		break;
	case Qt::Key_Asterisk:
		key = SDLK_ASTERISK;
		break;
	case Qt::Key_Plus:
		key = SDLK_PLUS;
		break;
	case Qt::Key_Comma:
		key = SDLK_COMMA;
		break;
	case Qt::Key_Minus:
		key = SDLK_MINUS;
		break;
	case Qt::Key_Period:
		key = SDLK_PERIOD;
		break;
	case Qt::Key_Slash:
		key = SDLK_SLASH;
		break;
	case Qt::Key_0:
		key = SDLK_0;
		break;
	case Qt::Key_1:
		key = SDLK_1;
		break;
	case Qt::Key_2:
		key = SDLK_2;
		break;
	case Qt::Key_3:
		key = SDLK_3;
		break;
	case Qt::Key_4:
		key = SDLK_4;
		break;
	case Qt::Key_5:
		key = SDLK_5;
		break;
	case Qt::Key_6:
		key = SDLK_6;
		break;
	case Qt::Key_7:
		key = SDLK_7;
		break;
	case Qt::Key_8:
		key = SDLK_8;
		break;
	case Qt::Key_9:
		key = SDLK_9;
		break;
	case Qt::Key_Colon:
		key = SDLK_COLON;
		break;
	case Qt::Key_Semicolon:
		key = SDLK_SEMICOLON;
		break;
	case Qt::Key_Less:
		key = SDLK_LESS;
		break;
	case Qt::Key_Equal:
		key = SDLK_EQUALS;
		break;
	case Qt::Key_Greater:
		key = SDLK_GREATER;
		break;
	case Qt::Key_Question:
		key = SDLK_QUESTION;
		break;
	case Qt::Key_At:
		key = SDLK_AT;
		break;
	case Qt::Key_A:
		key = SDLK_a;
		break;
	case Qt::Key_B:
		key = SDLK_b;
		break;
	case Qt::Key_C:
		key = SDLK_c;
		break;
	case Qt::Key_D:
		key = SDLK_d;
		break;
	case Qt::Key_E:
		key = SDLK_e;
		break;
	case Qt::Key_F:
		key = SDLK_f;
		break;
	case Qt::Key_G:
		key = SDLK_g;
		break;
	case Qt::Key_H:
		key = SDLK_h;
		break;
	case Qt::Key_I:
		key = SDLK_i;
		break;
	case Qt::Key_J:
		key = SDLK_j;
		break;
	case Qt::Key_K:
		key = SDLK_k;
		break;
	case Qt::Key_L:
		key = SDLK_l;
		break;
	case Qt::Key_M:
		key = SDLK_m;
		break;
	case Qt::Key_N:
		key = SDLK_n;
		break;
	case Qt::Key_O:
		key = SDLK_o;
		break;
	case Qt::Key_P:
		key = SDLK_p;
		break;
	case Qt::Key_Q:
		key = SDLK_q;
		break;
	case Qt::Key_R:
		key = SDLK_r;
		break;
	case Qt::Key_S:
		key = SDLK_s;
		break;
	case Qt::Key_T:
		key = SDLK_t;
		break;
	case Qt::Key_U:
		key = SDLK_u;
		break;
	case Qt::Key_V:
		key = SDLK_v;
		break;
	case Qt::Key_W:
		key = SDLK_w;
		break;
	case Qt::Key_X:
		key = SDLK_x;
		break;
	case Qt::Key_Y:
		key = SDLK_y;
		break;
	case Qt::Key_Z:
		key = SDLK_z;
		break;
	case Qt::Key_BracketLeft:
		key = SDLK_LEFTBRACKET;
		break;
	case Qt::Key_Backslash:
		key = SDLK_BACKSLASH;
		break;
	case Qt::Key_BracketRight:
		key = SDLK_RIGHTBRACKET;
		break;
	case Qt::Key_AsciiCircum:
		key = SDLK_CARET;
		break;
	case Qt::Key_Underscore:
		key = SDLK_UNDERSCORE;
		break;
	case Qt::Key_QuoteLeft:
		key = SDLK_BACKQUOTE;
		break;
	case Qt::Key_BraceLeft:
		key = SDLK_LEFTBRACKET;
		break;
	case Qt::Key_Bar:
		key = SDLK_BACKSLASH;
		break;
	case Qt::Key_BraceRight:
		key = SDLK_RIGHTBRACKET;
		break;
	case Qt::Key_AsciiTilde:
		key = SDLK_BACKQUOTE;
		break;
	default:
		return;
	}

	InputEvent i;
	i.touch = 0;
	i.type = press ? InputEvent::T_KeyDown : InputEvent::T_KeyUp;
	i.time = xtime::ReadMilliseconds();
	i.data[0] = key;
	game.PostInputEvent(i);
}

pkg::PackageManRef Packages()
{
	return App::Get()->engine->sys->packages;
}

file::HFileSystem Files()
{
	return App::Get()->engine->sys->files;
}

r::HRenderer Renderer()
{
	return App::Get()->engine->sys->r;
}

String ExpandBaseDir(const char *append)
{
	String dir(CStr("9:/"));
	dir += Files()->hddRoot.get();
	char native[file::MaxFilePathLen+1];
	if (file::ExpandToNativePath(dir.c_str, native, file::MaxFilePathLen+1))
	{
		if (append)
			string::cat(native, append);

		return String(native);
	}

	return String();
}

bool IsGuiThread()
{
	return __QMainThreadId() == thread::ThreadId();
}

bool LoadPixmap(const char *filename, QPixmap &pixmap)
{
	int media = file::AllMedia;
	file::HBufferedAsyncIO buf;

	int r = Files()->LoadFile(
		filename,
		media,
		buf,
		file::HIONotify()
	);

	if (r == file::Pending)
	{
		buf->WaitForCompletion();
		r = buf->result;
	}

	if (r != file::Success) return false;

	return pixmap.loadFromData(
		(const uchar*)buf->data->ptr.get(),
		(uint)buf->data->size.get()
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
