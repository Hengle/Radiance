/*! \file WinKeys.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include "WinKeys.h"
#include <Runtime/Win/WinHeaders.h>

// http://msdn.microsoft.com/en-us/library/windows/desktop/dd375731(v=vs.85).aspx

KeyCode TranslateVKey(int vkey, int lparam) {
	switch (vkey) {
	case VK_BACK:
		return kKeyCode_Backspace;
	case VK_TAB:
		return kKeyCode_Tab;
	case VK_CLEAR:
		return kKeyCode_Clear;
	case VK_RETURN:
		if (lparam & kVK_Extended)
			return kKeyCode_KP_Enter;
		return kKeyCode_Return;
	case VK_SHIFT:
		if (lparam & kVK_Extended)
			return kKeyCode_RShift;
		return kKeyCode_LShift;
	case VK_CONTROL:
		if (lparam & kVK_Extended)
			return kKeyCode_RCtrl;
		return kKeyCode_LCtrl;
	case VK_MENU:
		if (lparam & kVK_Extended)
			return kKeyCode_RAlt;
		return kKeyCode_LAlt;
	case VK_PAUSE:
		return kKeyCode_Pause;
	case VK_CAPITAL:
		return kKeyCode_CapsLock;
	case VK_ESCAPE:
		return kKeyCode_Escape;
	case VK_SPACE:
		return kKeyCode_Space;
	case VK_PRIOR:
		return kKeyCode_PageUp;
	case VK_NEXT:
		return kKeyCode_PageDown;
	case VK_HOME:
		return kKeyCode_Home;
	case VK_LEFT:
		return kKeyCode_Left;
	case VK_UP:
		return kKeyCode_Up;
	case VK_RIGHT:
		return kKeyCode_Right;
	case VK_DOWN:
		return kKeyCode_Down;
	case VK_SNAPSHOT:
		return kKeyCode_PrintScreen;
	case VK_INSERT:
		return kKeyCode_Insert;
	case VK_DELETE:
		return kKeyCode_Delete;
	case VK_HELP:
		return kKeyCode_Help;
	case VK_MULTIPLY:
		return kKeyCode_KP_Multiply;
	case VK_ADD:
		return kKeyCode_KP_Plus;
	case VK_SUBTRACT:
		return kKeyCode_KP_Minus;
	case VK_DECIMAL:
		return kKeyCode_KP_Period;
	case VK_DIVIDE:
		return kKeyCode_KP_Divide;
	case VK_NUMLOCK:
		return kKeyCode_NumLock;
	case VK_SCROLL:
		return kKeyCode_ScrollLock;
	case VK_LWIN:
		return kKeyCode_LCommand;
	case VK_RWIN:
		return kKeyCode_RCommand;
	}

	if (vkey >= kKeyCode_0 && vkey <= kKeyCode_9)
		return (KeyCode)(kKeyCode_0 + (vkey-kKeyCode_0));

	if (vkey >= kKeyCode_A && vkey <= kKeyCode_Z)
		return (KeyCode)(kKeyCode_A + (vkey-kKeyCode_A));

	if (vkey  >= VK_NUMPAD0 && vkey <= VK_NUMPAD9)
		return (KeyCode)(kKeyCode_KP0 + (vkey-VK_NUMPAD0));

	if (vkey >= VK_F1 && vkey <= VK_F15)
		return (KeyCode)(kKeyCode_F1 + (vkey-VK_F1));

	return kKeyCode_Max;
}