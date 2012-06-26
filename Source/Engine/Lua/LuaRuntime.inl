// LuaRuntime.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace lua {

//////////////////////////////////////////////////////////////////////////////////////////

inline Variant::Variant()
{
}

inline Variant::Variant(const Variant &val) : m_val(val.m_val)
{
}

inline Variant::Variant(const reflect::SharedReflected::Ref &val) : m_val(val)
{
}

template <typename T>
inline Variant::operator const T *() const
{
	RAD_ASSERT(reflect::Type<T>());
	if (!m_val || !m_val->IsValid() || reflect::Type<T>()->ConstType() != m_val->Type()->ConstType())
	{
		return 0;
	}

	return static_cast<const T*>(*m_val);
}

template <typename T>
inline Variant::operator T *()
{
	RAD_ASSERT(reflect::Type<T>());
	if (!m_val || !m_val->IsValid() || reflect::Type<T>()->ConstType() != m_val->Type()->ConstType())
	{
		return 0;
	}

	return static_cast<T*>(*m_val);
}

inline const reflect::Class *Variant::Class() const
{
	if (!m_val) 
		return 0;
	return m_val->Type();
}

inline Variant &Variant::operator = (const Variant &v)
{
	m_val = v.m_val;
	return *this;
}

inline Variant Variant::Clone() const
{
	if (m_val)
	{
		return Variant(
			reflect::SharedReflected::Ref(new reflect::SharedReflected(
				reflect::Reflected::Clone(ZLuaRuntime, *m_val)
			))
		);
	}

	return Variant();
}

//////////////////////////////////////////////////////////////////////////////////////////

inline SharedPtr::SharedPtr()
{
}

inline SharedPtr::~SharedPtr()
{
}

inline void SharedPtr::Push(lua_State *L)
{
	lua_createtable(L, 0, 1);
	Marshal<Ref>::Push(L, shared_from_this(), PtrId);
	lua_setfield(L, -2, "@sp");
	PushElements(L);
}

template <typename T>
inline boost::shared_ptr<T> SharedPtr::Get(lua_State *L, const char *tname, int index, bool luaError)
{
	if (lua_type(L, index) != LUA_TTABLE)
	{
		if (luaError)
			luaL_typerror(L, index, tname);
		return boost::shared_ptr<T>();
	}

	lua_getfield(L, index, "@sp");
	Ref r = lua::Marshal<Ref>::Get(L, -1, PtrId, luaError);
	lua_pop(L, 1);
	if (!r && luaError)
		luaL_typerror(L, index, tname);

	boost::shared_ptr<T> x = boost::dynamic_pointer_cast<T>(r);
	if (!x && luaError)
		luaL_typerror(L, index, tname);

	return x;
}

template <typename T>
inline bool SharedPtr::IsA(lua_State *L, int index)
{
	return Get<T>(L, index, false);
}

//////////////////////////////////////////////////////////////////////////////////////////

inline void Marshal<const char *>::Push(lua_State *L, const char *val)
{
	RAD_ASSERT(L&&val);
	lua_pushstring(L, val);
}

inline const char *Marshal<const char *>::Get(lua_State *L, int index, bool forceType)
{
	RAD_ASSERT(L);
	if (forceType && !lua_isstring(L, index))
	{
		luaL_argerror(L, index, "expected string");
	}
	return lua_tostring(L, index);
}

inline bool Marshal<const char *>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isstring(L, index) != 0;
}

inline void Marshal<const wchar_t*>::Push(lua_State *L, const wchar_t *val)
{
	std::wstring x(val);
	Marshal<std::wstring>::Push(L, x);
}

inline void Marshal<String>::Push(lua_State *L, const String &val)
{
	RAD_ASSERT(L);
	lua_pushlstring(L, val.c_str, (int)val.length);
}

inline String Marshal<String>::Get(lua_State *L, int index, bool forceType)
{
	const char *p = Marshal<const char*>::Get(L, index, forceType);
	String x;
	if (p)
		x = p;
	return x;
}

inline bool Marshal<String>::IsA(lua_State *L, int index)
{
	return Marshal<const char*>::IsA(L, index);
}

inline void Marshal<std::string>::Push(lua_State *L, const std::string &val)
{
	RAD_ASSERT(L);
	lua_pushlstring(L, val.c_str(), (int)val.length());
}

inline std::string Marshal<std::string>::Get(lua_State *L, int index, bool forceType)
{
	const char *p = Marshal<const char*>::Get(L, index, forceType);
	std::string x;
	if (p) { x = p; }
	return x;
}

inline bool Marshal<std::string>::IsA(lua_State *L, int index)
{
	return Marshal<const char*>::IsA(L, index);
}

inline void Marshal<std::wstring>::Push(lua_State *L, const std::wstring &val)
{
	String x(val); // convert to UTF8
	Marshal<String>::Push(L, x);
}

inline std::wstring Marshal<std::wstring>::Get(lua_State *L, int index, bool forceType)
{
	String x(Marshal<String>::Get(L, index, forceType));
	std::wstring z(x.toWChar().c_str.get());
	return z;
}

inline bool Marshal<std::wstring>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isstring(L, index) != 0;
}

inline void Marshal<bool>::Push(lua_State *L, bool val)
{
	RAD_ASSERT(L);
	lua_pushboolean(L, val ? 1 : 0);
}

inline bool Marshal<bool>::Get(lua_State *L, int index, bool forceType)
{
	RAD_ASSERT(L);
	if (forceType && !lua_isboolean(L, index))
	{
		luaL_argerror(L, index, "expected bool");
	}
	return lua_toboolean(L, index)==1 ? true : false;
}

inline bool Marshal<bool>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isboolean(L, index) != 0;
}

inline void Marshal<S8>::Push(lua_State *L, S8 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline S8 Marshal<S8>::Get(lua_State *L, int index, bool forceType)
{
	return (S8)Marshal<S32>::Get(L, index, forceType);
}

inline bool Marshal<S8>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<U8>::Push(lua_State *L, U8 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline U8 Marshal<U8>::Get(lua_State *L, int index, bool forceType)
{
	return (U8)Marshal<U32>::Get(L, index, forceType);
}

inline bool Marshal<U8>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<S16>::Push(lua_State *L, S16 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline S16 Marshal<S16>::Get(lua_State *L, int index, bool forceType)
{
	return (S16)Marshal<S32>::Get(L, index, forceType);
}

inline bool Marshal<S16>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<U16>::Push(lua_State *L, U16 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline U16 Marshal<U16>::Get(lua_State *L, int index, bool forceType)
{
	return (U16)Marshal<U32>::Get(L, index, forceType);
}

inline bool Marshal<U16>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<S32>::Push(lua_State *L, S32 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline S32 Marshal<S32>::Get(lua_State *L, int index, bool forceType)
{
	return (S32)FloatToInt(Marshal<F32>::Get(L, index, forceType));
}

inline bool Marshal<S32>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<U32>::Push(lua_State *L, U32 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline U32 Marshal<U32>::Get(lua_State *L, int index, bool forceType)
{
	return (U32)FloatToInt(Marshal<F32>::Get(L, index, forceType));
}

inline bool Marshal<U32>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<S64>::Push(lua_State *L, S64 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline S64 Marshal<S64>::Get(lua_State *L, int index, bool forceType)
{
	return (S64)Marshal<F64>::Get(L, index, forceType);
}

inline bool Marshal<S64>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<U64>::Push(lua_State *L, U64 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline U64 Marshal<U64>::Get(lua_State *L, int index, bool forceType)
{
	return (U64)Marshal<F64>::Get(L, index, forceType);
}

inline bool Marshal<U64>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<F32>::Push(lua_State *L, F32 val)
{
	Marshal<F64>::Push(L, (F64)val);
}

inline F32 Marshal<F32>::Get(lua_State *L, int index, bool forceType)
{
	return (F32)Marshal<F64>::Get(L, index, forceType);
}

inline bool Marshal<F32>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline void Marshal<F64>::Push(lua_State *L, F64 val)
{
	RAD_ASSERT(L);
	lua_pushnumber(L, val);
}

inline F64 Marshal<F64>::Get(lua_State *L, int index, bool forceType)
{
	RAD_ASSERT(L);
	if (forceType && !lua_isnumber(L, index))
	{
		luaL_argerror(L, index, "expected number");
	}
	return lua_tonumber(L, index);
}

inline bool Marshal<F64>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	return lua_isnumber(L, index) != 0;
}

inline bool Marshal<Vec2>::IsA(lua_State *L, int index)
{
	return lua_type(L, index) == LUA_TTABLE;
}

inline bool Marshal<Vec3>::IsA(lua_State *L, int index)
{
	return lua_type(L, index) == LUA_TTABLE;
}

inline bool Marshal<Vec4>::IsA(lua_State *L, int index)
{
	return lua_type(L, index) == LUA_TTABLE;
}

template <typename T>
inline void Marshal<boost::shared_ptr<T> >::Push(lua_State *L, const boost::shared_ptr<T> &ptr, int id)
{
	RAD_ASSERT(L);
	lua_createtable(L, 2, 0);
	new (lua_newuserdata(L, sizeof(boost::shared_ptr<T>))) boost::shared_ptr<T>(ptr);
	lua_createtable(L, 1, 0);
	lua_pushcfunction(L, gc);
	lua_setfield(L, -2, "__gc");
	lua_setmetatable(L, -2);
	lua_setfield(L, -2, "@sp");
	lua_pushinteger(L, id);
	lua_setfield(L, -2, "@id");
}

template <typename T>
inline boost::shared_ptr<T> Marshal<boost::shared_ptr<T> >::Get(lua_State *L, int index, int id, bool luaError)
{
	if (lua_type(L, index) != LUA_TTABLE)
	{
		if (luaError)
			luaL_typerror(L, index, "table");
		
		return boost::shared_ptr<T>();
	}

	if (id != -1)
	{
		lua_getfield(L, index, "@id");
		if (lua_type(L, -1) != LUA_TNUMBER)
		{
			lua_pop(L, 1);
			if (luaError)
				luaL_typerror(L, index, "<boost::shared_ptr>");
			return boost::shared_ptr<T>();
		}
		if (lua_tointeger(L, -1) != id)
		{
			lua_pop(L, 1);
			if (luaError)
				luaL_typerror(L, index, "<boost::shared_ptr>");
			return boost::shared_ptr<T>();
		}
		lua_pop(L, 1);
	}

	lua_getfield(L, index, "@sp");
	if (lua_type(L, -1) != LUA_TUSERDATA)
	{
		lua_pop(L, 1);
		if (luaError)
			luaL_typerror(L, index, "<boost::shared_ptr>");
		return boost::shared_ptr<T>();
	}

	boost::shared_ptr<T>* p = reinterpret_cast<boost::shared_ptr<T>*>(lua_touserdata(L, index));
	lua_pop(L, 1);
	return *p;
}

template <typename T>
inline bool Marshal<boost::shared_ptr<T> >::IsA(lua_State *L, int index, int id)
{
	return lua_type(L, index) == LUA_TUSERDATA;
}

template <typename T>
inline int Marshal<boost::shared_ptr<T> >::gc(lua_State *L)
{
	if (lua_type(L, -1) == LUA_TUSERDATA)
	{
		boost::shared_ptr<T> *x = reinterpret_cast<boost::shared_ptr<T>*>(lua_touserdata(L, -1));
		RAD_ASSERT(x);
		x->boost::shared_ptr<T>::~shared_ptr();
	}
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Marshaling support for Generics
//////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS ReflectedRef
{
public:

	static ReflectedRef *PushNew(lua_State *L, const ::reflect::Reflected &r);
	static ReflectedRef *Get(lua_State *L, int index);

	~ReflectedRef()
	{
		if (m_r)
		{
			m_r.Delete();
		}
	}

	const ::reflect::Reflected &Reflected() { return m_r; }
private:

	ReflectedRef(const ::reflect::Reflected &r) : m_r(r) {}
	::reflect::Reflected m_r;
};

template <typename T>
void Marshal<T>::Push(lua_State *L, const T &val)
{
	RAD_ASSERT(L);

	const ::reflect::Class *type = ::reflect::Type<T>();

	if (!type)
	{
		luaL_error(L, "Missing type reflection map. (Function %s, File %s, Line %d).",
			__FUNCTION__, __FILE__, __LINE__
		);
	}

	T *x = reinterpret_cast<T*>(::reflect::Allocate(ZLuaRuntime, type));
	RAD_OUT_OF_MEM(x);
	*x = val;
	ReflectedRef::PushNew(L, ::reflect::Reflect(x, type));
}

template <typename T>
T Marshal<T>::Get(lua_State *L, int index, bool forceType)
{
	RAD_ASSERT(L);
	ReflectedRef *ref = ReflectedRef::Get(L, index);
	RAD_ASSERT(ref);

	const ::reflect::Class *type = ::reflect::Type<T>();
	if (!type)
	{
		luaL_error(L, "Missing type reflection map for argument %d. (Function %s, File %s, Line %d)",
			index,
			__FUNCTION__, __FILE__, __LINE__
		);
	}

	if (forceType && (ref->Reflected().Type() != type))
	{
		luaL_error(L, "Argument %d is not the expected type. Expected (%s), got (%s). (Function %s, File %s, Line %d).",
			index,
			ref->Reflected().Type()->Name<char>(),
			type->Name<char>(),
			__FUNCTION__, __FILE__, __LINE__
		);
	}

	const T *x = 0;

	try
	{
		x = static_cast<const T*>(ref->Reflected());
	}
	catch (::reflect::InvalidCastException)
	{
		luaL_error(L, "Argument %d InvalidCastException. Tried to cast a (%s) to a (%s). (Function %s, File %s, Line %d).",
			index,
			ref->Reflected().Type()->Name<char>(),
			type->Name<char>(),
			__FUNCTION__, __FILE__, __LINE__);
	}

	RAD_ASSERT(x);

	return *x;
}

template <typename T>
inline bool Marshal<T>::IsA(lua_State *L, int index)
{
	RAD_ASSERT(L);
	ReflectedRef *ref = ReflectedRef::Get(L, index);
	RAD_ASSERT(ref);

	const ::reflect::Class *type = ::reflect::Type<T>();
	if (!type)
	{
		luaL_error(L, "Type of argument %d missing reflection map. (Function %s, File %s, Line %d)",
			index,
			__FUNCTION__, __FILE__, __LINE__
		);
	}

	return ref->Reflected().Type()->IsA(type);
}

inline void Marshal< ::reflect::Reflected>::Push(lua_State *L, const ::reflect::Reflected &val)
{
	ReflectedRef::PushNew(L, val);
}

inline ::reflect::Reflected Marshal< ::reflect::Reflected>::Get(lua_State *L, int index)
{
	ReflectedRef *ref = ReflectedRef::Get(L, index);
	RAD_ASSERT(ref);
	return ref->Reflected();
}

} // lua
