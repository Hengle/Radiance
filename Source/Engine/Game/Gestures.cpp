// Gestures.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "../Engine.h"
#include "Game.h"
#include "../App.h"
#include <algorithm>
#undef min
#undef max
#undef emit

namespace {

enum {
	kDoubleClickMs = 150,
	kDoubleTapMs = 200,
	kMinLineClockTime = 50,
	kLineGestureTime = 200
};

struct GConstants {
	enum {
		kBaseMaxTouchMove = 20, // drift more than 20 pixels and we don't emit an IG_Tap
		kBaseMaxDoubleTapMove = kBaseMaxTouchMove+30,
		kBaseMinCircleMove = 50,
		kBaseMinLineMove = 120,
		kBaseMaxCircleEndPointDelta = 64,
		kBaseMaxLineDrift = 50,
		kBaseMinLineEndPointDelta = 128
	};
	
	static int MaxTouchMove(float scale) {
		return FloatToInt(kBaseMaxTouchMove*scale);
	}
	
	static int MaxDoubleTapMove(float scale) {
		return FloatToInt(kBaseMaxDoubleTapMove*scale);
	}
	
	static int MinCircleMove(float scale) {
		return FloatToInt(kBaseMinCircleMove*scale);
	}
	
	static int MinLineMove(float scale) {
		return FloatToInt(kBaseMinLineMove*scale);
	}
	
	static int MaxCircleEndPointDelta(float scale) {
		return FloatToInt(kBaseMaxCircleEndPointDelta*scale);
	}
	
	static int MaxLineDrift(float scale) {
		return FloatToInt(kBaseMaxLineDrift*scale);
	}
	
	static int MinLineEndPointDelta(float scale) {
		return FloatToInt(kBaseMinLineEndPointDelta*scale);
	}
	
};

// Helperes

void IdentifyLineGesture(const InputEvent *e, const TouchState &touch, float n[2], bool &minSize, bool &isLine, bool scale) {
	minSize = false;
	isLine = false;

	if (touch.moves.size() < 2)
		return;
	if (e && ((e->time - touch.startTime) < kMinLineClockTime))
		return;
	
	int dx = touch.maxs[0] - touch.mins[0];
	int dy = touch.maxs[1] - touch.mins[1];
	
	if ((dx*dx+dy*dy) >= GConstants::MaxTouchMove(scale)*GConstants::MaxTouchMove(scale)) { 
		// assume it's a line
		const InputPoint &start = touch.moves.front();
		const InputPoint &end = touch.moves.back();

		dx = end[0] - start[0];
		dy = end[1] - start[1];
		int dd = dx*dx+dy*dy;

		if (dd >= GConstants::MinLineMove(scale)*GConstants::MinLineMove(scale)) {
			minSize = true;

			// figure out if there is a line trend here.
			const InputPoint &start = touch.moves.front();

			// do a little 2D vector math here.
			float m;
			n[0] = (float)dx;
			n[1] = (float)dy;

			m = math::SquareRoot(n[0]*n[0]+n[1]*n[1]);
			n[0] /= m;
			n[1] /= m;

			float x[2];

			m = n[1];
			x[1] = n[0];
			x[0] = -m;

			isLine = true;

			if (touch.gid == IG_Line) { 
				// we already generated a line move with this one, make sure we have reversed direction some
				float d = x[0]*touch.floats[0] + x[1]*touch.floats[1];
				if (d > -0.3f)
					isLine = false;
			} else {
				float d = x[0]*start[0] + x[1]*start[1];

				InputPointVec::const_iterator it;
				for (it = touch.moves.begin(); it != touch.moves.end(); ++it) {
					const InputPoint &p = *it;
					m = (x[0]*p[0] + x[1]*p[1]) - d;
					if (math::Abs(m) > GConstants::MaxLineDrift(scale)) {
						isLine = false;
						break;
					}
				}
			}
		}
	}
}

}

bool Game::GestureInput(
	const InputEvent &e, 
	InputState &is, 
	InputGesture &g, 
	TouchState &touch,
	int enabledGestures
) {
	g.id = -1;
	g.time = e.time;
	g.mins[0] = g.mins[1] = 0;
	g.maxs[0] = g.maxs[0] = 0;
	g.origin[0] = e.data[0];
	g.origin[1] = e.data[1];

	if (m_pinchTouches.size() == 2) { 
		// during a pinch ignore all unrelated input.
		if (touch.gid != IG_Pinch)
			return false;
	}

	float scale = (m_vp[2]/1024.f) * (m_vp[3]/768.f);
	
	// these must be ordered by precidence
	bool r = false;
	
	if (g.id == -1 && !r && (enabledGestures&IG_Pinch))
		r = G_Pinch(e, is, touch, g, scale);
	if (g.id == -1 && !r && (enabledGestures&IG_LClick))
		r = G_LClick(e, is, touch, g, scale);
	if (g.id == -1 && !r && (enabledGestures&IG_RClick))
		r = G_RClick(e, is, touch, g, scale);
	if (g.id == -1 && !r && (enabledGestures&IG_DoubleTap))
		r = G_DoubleTap(e, is, touch, g, scale);
	if (g.id == -1 && !r && (enabledGestures&IG_Tap))
		r = G_Tap(e, is, touch, g, scale);
	if (g.id == -1 && !r && (enabledGestures&IG_Line))
		r = G_Line(e, is, touch, g, scale);
	if (g.id == -1 && !r && (enabledGestures&IG_Circle))
		r = G_Circle(e, is, touch, g, (enabledGestures&IG_Line) ? true : false, scale);

	if (g.id != -1)
		touch.gid = g.id;
	
	return g.id != -1;
}

bool Game::G_LClick(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, float scale) {
	if (touch.gid != -1)
		return false; // this touch state cannot generate a gesture

	switch (e.type) {
	case InputEvent::T_MouseUp:
		if (e.data[2] & kMouseButton_Left) {
			g.id = IG_LClick;
			g.phase = IGPhase_Begin;
			g.start[0] = e.data[0];
			g.start[1] = e.data[1];
			g.end[0] = e.data[0];
			g.end[1] = e.data[1];
		}
		break;
	default:
		break;
	}
	
	return false;
}

bool Game::G_RClick(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, float scale) {
	if (touch.gid != -1)
		return false; // this touch state cannot generate a gesture

	switch (e.type) {
	case InputEvent::T_MouseUp:
		if(e.data[2] & kMouseButton_Right) {
			g.id = IG_RClick;
			g.phase = IGPhase_Begin;
			g.start[0] = e.data[0];
			g.start[1] = e.data[1];
			g.end[0] = e.data[0];
			g.end[1] = e.data[1];
		}
		break;
	default:
		break;
	}
	
	return false;
}

bool Game::G_DoubleTap(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, float scale) {
	if (touch.gid != -1)
		return false; // this touch state cannot generate a gesture

	switch (e.type) {
	case InputEvent::T_MouseDown:
		if (e.data[2] & kMouseButton_Left) {
			// check double click.
			if ((e.time-is.ms.time) <= kDoubleClickMs) {
				g.id = IG_DoubleTap;
				g.phase = IGPhase_Begin;
			}
		}
		break;
	case InputEvent::T_TouchBegin:
			if (m_doubleTap.type == InputEvent::T_TouchBegin) {
				if ((e.time - m_doubleTap.time) <= kDoubleTapMs) {
					int dx = e.data[0] - m_doubleTap.data[0];
					int dy = e.data[1] - m_doubleTap.data[1];
					int dd = dx*dx + dy*dy;

					if (dd <= GConstants::MaxDoubleTapMove(scale)*GConstants::MaxDoubleTapMove(scale)) {
						g.id = IG_DoubleTap;
						g.phase = IGPhase_Begin;
						g.mins[0] = g.maxs[0] = e.data[0];
						g.mins[1] = g.maxs[1] = e.data[1];
						g.origin[0] = e.data[0];
						g.origin[1] = e.data[1];
						g.start[0] = e.data[0];
						g.start[1] = e.data[1];
						g.end[0] = e.data[0];
						g.end[1] = e.data[1];
					}

					m_doubleTap.type = InputEvent::T_Invalid;
				} else {
					m_doubleTap = e;
				}
			} else {
				m_doubleTap = e;
			}
		break;
	default:
		break;
	}
	
	return false;
}

bool Game::G_Tap(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, float scale) {
	if (touch.gid != -1)
		return false; // this touch state cannot generate a gesture

	switch (e.type) {
	case InputEvent::T_TouchEnd: 
		{
			int dx = touch.maxs[0] - touch.mins[0];
			int dy = touch.maxs[1] - touch.mins[1];
			
			if ((dx < GConstants::MaxTouchMove(scale)) && (dy < GConstants::MaxTouchMove(scale))) {
				g.id = IG_Tap;
				g.phase = IGPhase_Begin;
				g.mins[0] = touch.mins[0];
				g.mins[1] = touch.mins[1];
				g.maxs[0] = touch.maxs[0];
				g.maxs[1] = touch.maxs[1];
				g.origin[0] = touch.e.data[0];
				g.origin[1] = touch.e.data[1];
				g.start[0] = touch.e.data[0];
				g.start[1] = touch.e.data[1];
				g.end[0] = touch.e.data[0];
				g.end[1] = touch.e.data[1];
			}
		}
		break;
	default:
		break;
	}
	
	return false;
}

bool Game::G_Circle(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, bool lineGestureEnabled, float scale) {
	if (touch.gid != -1)
		return false; // this touch state cannot generate a gesture

	switch (e.type) {
	case InputEvent::T_TouchMoved:
		{
			bool emit = false;
			bool minSize, isLine;

			if (lineGestureEnabled) {
				float n[2];
				IdentifyLineGesture(&e, touch, n, minSize, isLine, scale);
			} else {
				minSize = false;
			}

			if (minSize) {
				emit = !isLine;
			} else {
				int dx = touch.maxs[0] - touch.mins[0];
				int dy = touch.maxs[1] - touch.mins[1];

				if ((dx >= GConstants::MinCircleMove(scale)) && (dy >= GConstants::MinCircleMove(scale))) { 
					// ok we made a big enough move.

					// detect circle when we close the loop.
					const InputPoint &x = touch.moves[0];
					dx = e.data[0] - x[0];
					dy = e.data[1] - x[1];

					if ((dx*dx+dy*dy) <= GConstants::MaxCircleEndPointDelta(scale)*GConstants::MaxCircleEndPointDelta(scale)) { 
						// we met our original point.
						emit = true;
					}
				}
			}

			if (emit) {
				g.id = IG_Circle;
				g.phase = IGPhase_Begin;
				g.mins[0] = touch.mins[0];
				g.mins[1] = touch.mins[1];
				g.maxs[0] = touch.maxs[0];
				g.maxs[1] = touch.maxs[1];
				g.origin[0] = touch.mins[0] + (touch.maxs[0]-touch.mins[0])/2;
				g.origin[1] = touch.mins[1] + (touch.maxs[1]-touch.mins[1])/2;
				g.start[0] = g.origin[0];
				g.start[1] = g.origin[1];
				g.end[0] = g.origin[0];
				g.end[1] = g.origin[1];
			}
		}
		break;
	default:
		break;
	}
	
	return false;
}

bool Game::G_Line(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, float scale) {
	if (touch.gid != -1 && touch.gid != IG_Line)
		return false; // this touch state cannot generate a gesture

	switch (e.type) {
	case InputEvent::T_TouchEnd:
	case InputEvent::T_TouchMoved:
		{
			// if we are "moving" don't start analyzing for some time first
			if (e.type == InputEvent::T_TouchMoved) {
				if (touch.Age() < kLineGestureTime)
					return false;
			}
			
			float n[2];
			bool minSize, isLine;

			IdentifyLineGesture(
				(e.type == InputEvent::T_TouchEnd) ? 0 : &e,
				touch, 
				n, 
				minSize, 
				isLine,
				scale
			);

			if (isLine) {
				g.id = IG_Line;
				g.phase = IGPhase_Begin;
				g.mins[0] = touch.mins[0];
				g.mins[1] = touch.mins[1];
				g.maxs[0] = touch.maxs[0];
				g.maxs[1] = touch.maxs[1];
				g.origin[0] = touch.mins[0] + (touch.maxs[0]-touch.mins[0])/2;
				g.origin[1] = touch.mins[1] + (touch.maxs[1]-touch.mins[1])/2;
				g.args[0] = n[0];
				g.args[1] = n[1];
				g.args[2] = 0.f;
				g.start[0] = touch.moves.front()[0];
				g.start[1] = touch.moves.front()[1];

				InputPoint p = touch.moves.back();
				g.end[0] = p[0];
				g.end[1] = p[1];
				touch.mins[0] = touch.maxs[0] = p[0];
				touch.mins[1] = touch.maxs[1] = p[1];
				touch.moves.clear();
				touch.moves.push_back(p);
				touch.floats[0] = n[0];
				touch.floats[1] = n[1];
				touch.startTime = e.time;
				touch.clockTime = xtime::ReadMilliseconds();

			}
		} break;
	default:
		break;
	}
	
	return false;
}

bool Game::G_Pinch(const InputEvent &e, InputState &is, TouchState &touch, InputGesture &g, float scale) {
	if (touch.gid != -1 && touch.gid != IG_Pinch)
		return false; // not our gestures.
	
	switch (e.type) {
	case InputEvent::T_TouchBegin:
		{
			// NOTE: T_TouchBegin will have a null TouchState object!
			
			bool valid = false;
			
			if (m_pinch) {
				if (m_pinchTouches.size() < 2) {
					m_pinchTouches.insert(e.touch);
					valid = m_pinchTouches.size() == 2;
				}
			} else {
				m_pinch = e.time;
				m_pinchTouches.insert(e.touch);
			}
			
			if (valid) {
				// starting a pinch gesture
				TouchSet::const_iterator it = m_pinchTouches.begin();
				void *pOtherTouch = *it;
				
				if (pOtherTouch == e.touch) {
					++it;
					pOtherTouch = *it;
				}
				
				InputTouchMap::iterator tmIt = is.touches.find(pOtherTouch);
				if (tmIt == is.touches.end()) {
					m_pinch = 0;
					m_pinchTouches.clear();
					break;
				}
				
				TouchState &otherTouch = tmIt->second;
				
				// figure out base size.
				Vec2 x((float)otherTouch.moves[0][0], (float)otherTouch.moves[0][1]);
				Vec2 y((float)e.data[0], (float)e.data[1]);
				
				float baseLen = (x-y).Magnitude();
				
				if (baseLen > 1.f) {
					g.id = IG_Pinch;
					g.phase = IGPhase_Begin;
					g.args[0] = baseLen;
					g.args[1] = baseLen;
					g.args[2] = 1.f;
					g.mins[0] = std::min(e.data[0], otherTouch.moves[0][0]);
					g.mins[1] = std::min(e.data[1], otherTouch.moves[0][1]);
					g.maxs[0] = std::max(e.data[0], otherTouch.moves[0][0]);
					g.maxs[1] = std::max(e.data[1], otherTouch.moves[0][1]);
					g.origin[0] = g.mins[0] + (g.maxs[0]-g.mins[0])/2;
					g.origin[1] = g.mins[1] + (g.maxs[1]-g.mins[1])/2;
					g.start[0] = g.origin[0];
					g.start[1] = g.origin[1];
					g.end[0] = g.origin[0];
					g.end[1] = g.origin[1];

					otherTouch.gid = IG_Pinch;
				} else {
					m_pinch = 0;
					m_pinchTouches.clear();
				}
			}
		}
		break;
	case InputEvent::T_TouchEnd:
		{
			bool valid = m_pinchTouches.size() == 2;
			
			m_pinchTouches.erase(e.touch);
			if (m_pinchTouches.size() < 2 && valid) {
				m_pinchTouches.clear();
				
				if (m_pinch) {
					m_pinch = 0;
					g.id = IG_Pinch;
					g.phase = IGPhase_End;
					g.mins[0] = 0;
					g.mins[1] = 0;
					g.maxs[0] = 0;
					g.maxs[1] = 0;
					g.origin[0] = 0;
					g.origin[1] = 0;
				}
			}
		}
		break;
			
	case InputEvent::T_TouchMoved:
			if (m_pinch && (m_pinchTouches.size()==2) && (m_pinchTouches.find(e.touch) != m_pinchTouches.end())) {
				TouchSet::const_iterator it = m_pinchTouches.begin();
				void *pOtherTouch = *it;
				
				if (pOtherTouch == e.touch) {
					++it;
					pOtherTouch = *it;
				}
				
				InputTouchMap::iterator tmIt = is.touches.find(pOtherTouch);
				if (tmIt == is.touches.end()) {
					m_pinch = 0;
					m_pinchTouches.clear();
					g.id = IG_Pinch;
					g.phase = IGPhase_End;
					g.mins[0] = 0;
					g.mins[1] = 0;
					g.maxs[0] = 0;
					g.maxs[1] = 0;
					g.origin[0] = 0;
					g.origin[1] = 0;
					break;
				}
				
				const TouchState &otherTouch = tmIt->second;
				
				if (touch.moves.empty() || (otherTouch.moves.size() < 2))
					break;
				
				// base size
				Vec2 x((float)otherTouch.moves[0][0], (float)otherTouch.moves[0][1]);
				Vec2 y((float)touch.moves[0][0], (float)touch.moves[0][1]);
				
				float baseLen = (x-y).Magnitude();
				
				x = Vec2((float)otherTouch.moves.back()[0], (float)otherTouch.moves.back()[1]);
				y = Vec2((float)e.data[0], (float)e.data[1]);
				
				float len = (x-y).Magnitude();
				
				g.id = IG_Pinch;
				g.phase = IGPhase_Move;
				g.args[0] = baseLen;
				g.args[1] = len;
				g.args[2] = len / baseLen;
				g.mins[0] = (int)std::min(x[0], y[0]);
				g.mins[1] = (int)std::min(x[1], y[1]);
				g.maxs[0] = (int)std::max(x[0], y[0]);
				g.maxs[1] = (int)std::max(x[1], y[1]);
				g.origin[0] = g.mins[0] + (g.maxs[0]-g.mins[0])/2;
				g.origin[1] = g.mins[1] + (g.maxs[1]-g.mins[1])/2;
			}
		break;
	default:
		break;
	}
	
	return g.id != -1;
}
