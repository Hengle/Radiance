// LuaRuntimeDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

struct lua_State;

namespace lua {

class State;
class Srcbuffer;
class Loader;
class TypeRegistrar;
class Variant;
class State;
template <typename T> struct Marshal;
typedef boost::shared_ptr<State> StateRef;

enum { InvalidIndex = -100 };

} // lua
