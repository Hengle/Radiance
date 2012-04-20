// Input.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"
#include "Lua/LuaRuntime.h"
#include <SDL/SDL_keysym.h>
#include <Runtime/Time.h>
#include <Runtime/Container/ZoneMap.h>

#if defined(RAD_OPT_PC)
#include <SDL/SDL_mouse.h>
#include <SDL/SDL_joystick.h>
#else
/* Used as a mask when testing buttons in buttonstate
   Button 1:	Left mouse button
   Button 2:	Middle mouse button
   Button 3:	Right mouse button
   Button 4:	Mouse wheel up	 (may also be a real button)
   Button 5:	Mouse wheel down (may also be a real button)
 */
#define SDL_BUTTON(X)		(1 << ((X)-1))
#define SDL_BUTTON_LEFT		1
#define SDL_BUTTON_MIDDLE	2
#define SDL_BUTTON_RIGHT	3
#define SDL_BUTTON_WHEELUP	4
#define SDL_BUTTON_WHEELDOWN	5
#define SDL_BUTTON_X1		6
#define SDL_BUTTON_X2		7
#define SDL_BUTTON_LMASK	SDL_BUTTON(SDL_BUTTON_LEFT)
#define SDL_BUTTON_MMASK	SDL_BUTTON(SDL_BUTTON_MIDDLE)
#define SDL_BUTTON_RMASK	SDL_BUTTON(SDL_BUTTON_RIGHT)
#define SDL_BUTTON_X1MASK	SDL_BUTTON(SDL_BUTTON_X1)
#define SDL_BUTTON_X2MASK	SDL_BUTTON(SDL_BUTTON_X2)
/*
 * Get the current state of a POV hat on a joystick
 * The return value is one of the following positions:
 */
#define SDL_HAT_CENTERED	0x00
#define SDL_HAT_UP		0x01
#define SDL_HAT_RIGHT		0x02
#define SDL_HAT_DOWN		0x04
#define SDL_HAT_LEFT		0x08
#define SDL_HAT_RIGHTUP		(SDL_HAT_RIGHT|SDL_HAT_UP)
#define SDL_HAT_RIGHTDOWN	(SDL_HAT_RIGHT|SDL_HAT_DOWN)
#define SDL_HAT_LEFTUP		(SDL_HAT_LEFT|SDL_HAT_UP)
#define SDL_HAT_LEFTDOWN	(SDL_HAT_LEFT|SDL_HAT_DOWN)
#endif

#include <algorithm>
#include <Runtime/PushPack.h>

enum { NumMouseButtons = 3 };

// Basic Input Gestures
enum InputGestureId
{
	RAD_FLAG(IG_Null), // TouchMove cannot generate gesture
	RAD_FLAG(IG_LClick),
	RAD_FLAG(IG_RClick),
	RAD_FLAG(IG_DoubleClick),
	RAD_FLAG(IG_Line),
	RAD_FLAG(IG_Circle),
	RAD_FLAG(IG_Pinch),
	
	IG_Tap = IG_LClick,
	IG_DoubleTap = IG_DoubleClick,
	
	IGPhase_Begin = 0,
	IGPhase_Move,
	IGPhase_End
};

typedef boost::array<int, 2> InputPoint;

struct InputGesture
{
	int id;
	int phase;
	InputPoint mins;
	InputPoint maxs;
	InputPoint origin;
	Vec3 args;
	xtime::TimeVal time;
};

struct InputEvent
{
	enum Type
	{
		T_KeyDown,
		T_KeyUp,
		T_MouseDown,
		T_MouseUp,
		T_MouseMove,
		T_MouseWheel,
		T_TouchBegin,
		T_TouchMoved,
		T_TouchStationary,
		T_TouchEnd,
		T_TouchCancelled,
		T_Invalid
	};

	InputEvent() : touch(0), delay(0), type(T_Invalid), gesture(false) {}
	InputEvent(const InputEvent &e) : 
	touch(e.touch),
	type(e.type),
	time(e.time),
	delay(e.delay),
	gesture(e.gesture)
	{
		data[0] = e.data[0];
		data[1] = e.data[1];
		data[2] = e.data[2];
	}

	void *touch;
	bool gesture;
	Type type;
	xtime::TimeVal time;
	xtime::TimeVal delay;
	
	// MOUSE(mouse) 0 = xpos, 1 == ypos, 2 == buttons
	// WHEEL(mouse) 0 = xpos, 1 == ypos, 2 == delta
	// KEYB(keycode) scancode
	boost::array<int, 3> data;

	bool IsMouse() const
	{
		return type == T_MouseDown || type == T_MouseUp || type == T_MouseMove;
	}

	bool IsMouseButton() const
	{
		return type == T_MouseDown || type == T_MouseUp;
	}

	bool IsTouch() const
	{
		return type >= T_TouchBegin && type <= T_TouchCancelled;
	}

	bool IsKeyboard() const
	{
		return type == T_KeyDown || type == T_KeyUp;
	}

	bool IsTouchBegin() const
	{
		return (type == T_TouchBegin);
	}

	bool IsTouchEnd(void *touch) const
	{
		return (!touch || this->touch == touch) && 
			((type == T_TouchEnd) || (type == T_TouchCancelled));
	}

	bool IsTouchMove(void *touch) const
	{
		return  (!touch || this->touch == touch) && (type == T_TouchMoved);
	}
};

typedef zone_vector<InputPoint, ZEngineT>::type InputPointVec;

struct TouchState
{
	TouchState(): startTime(0), clockTime(0)
	{
		mins[0] = mins[1] = 99999;
		maxs[0] = maxs[1] = -99999;
		gid = -1;
		begin = false;
		gesture = false;
		floats[0] = floats[1] = floats[2] = 0.f;
	}

	void SwapCopy(TouchState &other)
	{
		gid = other.gid;
		begin = other.begin;
		gesture = other.gesture;
		e = other.e;
		mins = other.mins;
		maxs = other.maxs;
		moves.swap(other.moves);
		startTime = other.startTime;
		clockTime = other.clockTime;
		floats = other.floats;
	}
	
	xtime::TimeVal Age() const
	{
		return xtime::ReadMilliseconds() - clockTime;
	}

	int gid;
	bool begin;
	bool gesture;
	InputEvent e;
	InputPoint mins;
	InputPoint maxs;
	InputPointVec moves;
	boost::array<float, 3> floats;
	xtime::TimeVal startTime;
	xtime::TimeVal clockTime;
};

typedef zone_map<void*, TouchState, ZEngineT>::type InputTouchMap;

struct KeyState
{
	KeyState() : state(false), impulse(false), time(0) {}
	bool state; // true = down
	bool impulse;
	xtime::TimeVal time;
};

struct KBState
{
	boost::array<KeyState, SDLK_LAST> keys;
};

struct MouseState
{
	MouseState() : buttons(0), wheel(0) , dwheel(0)
	{ 
		wpos[0] = wpos[1] = 0;
		dpos[0] = dpos[1] = 0;
		delta[0] = delta[1] = 0;
		time = 0;
	}
	boost::array<int, 2> wpos; // window position
	boost::array<int, 2> dpos; // delta calculated position
	boost::array<int, 2> delta; // delta this frame.
	int buttons;
	int wheel;
	int dwheel; // delta wheel
	xtime::TimeVal time;
};

struct InputState
{
	KBState kb;
	MouseState ms;
	InputTouchMap touches;
};


namespace lua {

template <>
struct Marshal<InputEvent>
{
	static void Push(lua_State *L, const InputEvent &e, const TouchState *touch);
};

template <>
struct Marshal<InputGesture>
{
	static void Push(lua_State *L, const InputGesture &g, const TouchState &touch);
};

} // lua


#include <Runtime/PopPack.h>
