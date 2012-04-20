// LuaRuntime.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "../Types.h"
#include <Lua/lua.h>
#include <Lua/lauxlib.h>
#include <Runtime/Interface.h>
#include <Runtime/Reflect.h>
#include <Runtime/Stream.h>
#include "LuaRuntimeDef.h"
#include <Runtime/String.h>
#include <Runtime/Container/ZoneMap.h>
#include <boost/thread/mutex.hpp>
#include <boost/thread/locks.hpp>
#include <string>
#include <map>
#include <vector>

#include <Runtime/PushPack.h>

namespace lua {

RAD_ZONE_DEC(RADENG_API, ZLuaRuntime);

//////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS State
{
public:

	typedef StateRef Ref;

	struct Metrics
	{
		int numAllocs;
		int smallest;
		int biggest;
	};

	State(const char *name);
	~State();

	static void CompactPools();

	RAD_DECLARE_READONLY_PROPERTY(State, L, lua_State*);
	RAD_DECLARE_READONLY_PROPERTY(State, metrics, const Metrics*);
private:

	static void *LuaAlloc(void *ud, void *ptr, size_t, size_t size);

	RAD_DECLARE_GET(L, lua_State*) { return m_s; }
	RAD_DECLARE_GET(metrics, const Metrics*) { return &m_m; }

	char m_sz[64];
	lua_State *m_s;
	Metrics m_m;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS SrcBuffer
{
public:
	typedef boost::shared_ptr<SrcBuffer> Ref;
	virtual ~SrcBuffer() {}
	RAD_DECLARE_READONLY_PROPERTY(SrcBuffer, ptr, const void*);
	RAD_DECLARE_READONLY_PROPERTY(SrcBuffer, size, AddrSize);
	RAD_DECLARE_READONLY_PROPERTY(SrcBuffer, name, const char*);
protected:
	virtual RAD_DECLARE_GET(ptr, const void*) = 0;
	virtual RAD_DECLARE_GET(size, AddrSize) = 0;
	virtual RAD_DECLARE_GET(name, const char*) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////

class RADENG_CLASS ImportLoader
{
public:
	virtual ~ImportLoader() {}
	virtual SrcBuffer::Ref Load(lua_State *L, const char *name) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////

struct StackMemTag {};
struct DynamicMemTag {};

template <int _BufSize, typename T>
class StreamLoader;

template <int _BufSize>
class StreamLoader<_BufSize, StackMemTag>
{
public:
	typedef StreamLoader<_BufSize, StackMemTag> SelfType;
	enum { BufSize = _BufSize };

	StreamLoader(stream::InputStream &stream) 
		: m_stream(&stream)
	{
	}

	static const char *Read(lua_State *L, void *ud, size_t *sz)
	{
		SelfType *self = (SelfType*)ud;
		*sz = (size_t)self->m_stream->Read(self->m_data, BufSize, 0);
		return self->m_data;
	}

private:
	stream::InputStream *m_stream;
	char m_data[BufSize];
};

template <int _BufSize>
class StreamLoader<_BufSize, DynamicMemTag>
{
public:
	typedef StreamLoader<_BufSize, DynamicMemTag> SelfType;
	enum { BufSize = _BufSize };

	StreamLoader(stream::InputStream &stream) 
		: m_stream(&stream)
	{
		m_data = new char[BufSize];
	}

	~StreamLoader()
	{
		delete[] m_data;
	}

	static const char *Read(lua_State *L, void *ud, size_t *sz)
	{
		SelfType *self = (SelfType*)ud;
		*sz = (size_t)self->m_stream->Read(self->m_data, BufSize, 0);
		return self->m_data;
	}

private:
	stream::InputStream *m_stream;
	char *m_data;
};

//////////////////////////////////////////////////////////////////////////////////////////

class Variant
{ // int, bool, string
  // does shallow copy by default, use Clone() to make a copy
public:

	typedef zone_map<String, Variant, ZLuaRuntimeT>::type Map;

	Variant();
	Variant(const Variant &v);
	explicit Variant(const reflect::SharedReflected::Ref &val);

	template <typename T>
	operator const T *() const;

	template <typename T>
	operator T *();

	Variant &operator = (const Variant &v);

	const reflect::Class *Class() const;
	bool Valid() const { return Class() != 0; }

	// this does not support tables
	Variant Clone() const;

private:

	reflect::SharedReflected::Ref m_val;
};

typedef std::map<String, String> SymbolMap;

//////////////////////////////////////////////////////////////////////////////////////////

class SharedPtr : public boost::enable_shared_from_this<SharedPtr>
{
public:
	typedef boost::shared_ptr<SharedPtr> Ref;

	SharedPtr();
	virtual ~SharedPtr();

	void Push(lua_State *L);

	template <typename T>
	static boost::shared_ptr<T> Get(lua_State *L, const char *tname, int index, bool luaError);

	template<typename T>
	static bool IsA(lua_State *L, int index);

protected:

	// table is at -1
	virtual void PushElements(lua_State *L) {}

private:

	enum { PtrId = RAD_FOURCC('l', 's', 'p', 'u') };
};

//////////////////////////////////////////////////////////////////////////////////////////

RADENG_API void RADENG_CALL ExportType(lua_State *L, const ::reflect::Class *type);
RADENG_API void RADENG_CALL ExportTypes(lua_State *L, const ::reflect::Class **types, int num=-1);
RADENG_API void RADENG_CALL EnableModuleImport(lua_State *L, ImportLoader &loader);
RADENG_API bool RADENG_CALL ImportModule(lua_State *L, const char *name);
RADENG_API void RADENG_CALL EnableNativeClassImport(lua_State *L);
RADENG_API void RADENG_CALL RegisterGlobals(lua_State *L, const char *table, luaL_Reg *r);
RADENG_API bool RADENG_CALL GetFieldExt(lua_State *L, int index, const char *k); // supports nested fields a.b.c.d
RADENG_API bool RADENG_CALL ParseVariantTable(lua_State *L, Variant::Map &map, bool luaError);
RADENG_API boost::mutex &RADENG_CALL TypeRegisterLock();

// pushes a full user-data onto the stack.
// the pointer must have been mapped prior to this.
RADENG_API void RADENG_CALL PushUserData(lua_State *L, void *data);
RADENG_API void RADENG_CALL MapUserData(lua_State *L); // user data must be on top of stack
RADENG_API void RADENG_CALL UnmapUserData(lua_State *L, void *data);

//////////////////////////////////////////////////////////////////////////////////////////
// Marshal
//////////////////////////////////////////////////////////////////////////////////////////

template <typename T>
struct Marshal
{
	static void Push(lua_State *L, const T &val);
	static T Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<const char *>
{
	static void Push(lua_State *L, const char *val);
	static const char *Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<const wchar_t *>
{
	static void Push(lua_State *L, const wchar_t *val);
};

template <>
struct Marshal< ::string::string<> >
{
	static void Push(lua_State *L, const ::string::string<> &val);
	static ::string::string<> Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal< ::string::wstring<> >
{
	static void Push(lua_State *L, const ::string::wstring<> &val);
	static ::string::wstring<> Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<std::string>
{
	static void Push(lua_State *L, const std::string &val);
	static std::string Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<std::wstring>
{
	static void Push(lua_State *L, const std::wstring &val);
	static std::wstring Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<bool>
{
	static void Push(lua_State *L, bool val);
	static bool Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<S8>
{
	static void Push(lua_State *L, S8 val);
	static S8 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);

};

template <>
struct Marshal<U8>
{
	static void Push(lua_State *L, U8 val);
	static U8 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<S16>
{
	static void Push(lua_State *L, S16 val);
	static S16 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<U16>
{
	static void Push(lua_State *L, U16 val);
	static U16 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<S32>
{
	static void Push(lua_State *L, S32 val);
	static S32 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<U32>
{
	static void Push(lua_State *L, U32 val);
	static U32 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<S64>
{
	static void Push(lua_State *L, S64 val);
	static S64 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<U64>
{
	static void Push(lua_State *L, U64 val);
	static U64 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<F32>
{
	static void Push(lua_State *L, F32 val);
	static F32 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<F64>
{
	static void Push(lua_State *L, F64 val);
	static F64 Get(lua_State *L, int index, bool forceType);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<Vec2>
{
	static void Push(lua_State *L, const Vec2 &val);
	static Vec2 Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<Vec3>
{
	static void Push(lua_State *L, const Vec3 &val);
	static Vec3 Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index);
};

template <>
struct Marshal<Vec4>
{
	static void Push(lua_State *L, const Vec4 &val);
	static Vec4 Get(lua_State *L, int index, bool luaError);
	static bool IsA(lua_State *L, int index);
};

template <typename T>
struct Marshal<boost::shared_ptr<T> >
{
	static void Push(lua_State *L, const boost::shared_ptr<T> &ptr, int id);
	static boost::shared_ptr<T> Get(lua_State *L, int index, int id, bool luaError);
	static bool IsA(lua_State *L, int index, int id);
private:
	static int gc(lua_State *L);
};

template <>
struct Marshal< ::reflect::Reflected>
{
	static void Push(lua_State *L, const ::reflect::Reflected &val);
	static ::reflect::Reflected Get(lua_State *L, int index);
};

} // lua

#include <Runtime/PopPack.h>

#define LUART_GETFN(_name) luart_get_##_name
#define LUART_SETFN(_name) luart_set_##_name

#define LUART_REGISTER_GETSET(L, _name) \
	LUART_REGISTER_GET(L, _name);\
	LUART_REGISTER_SET(L, _name)

#define LUART_REGISTER_GET(L, _name) \
	lua_pushcfunction(L, LUART_GETFN(_name));\
	lua_setfield(L, -2, #_name)

#define LUART_REGISTER_SET(L, _name) \
	lua_pushcfunction(L, LUART_SETFN(_name));\
	lua_setfield(L, -2, "Set"#_name)

#define LUART_DECL_GETSET(_name) \
	LUART_DECL_GET(_name); \
	LUART_DECL_SET(_name)

#define LUART_DECL_GET(_name) \
	static int LUART_GETFN(_name)(lua_State *L)

#define LUART_DECL_SET(_name) \
	static int LUART_SETFN(_name)(lua_State *L)

#define LUART_GETSET(_class, _name, _type, _member, _self) \
	LUART_GET(_class, _name, _type, _member, _self) \
	LUART_SET(_class, _name, _type, _member, _self)

#define LUART_GET(_class, _name, _type, _member, _self) \
	LUART_GET_CUSTOM(_class, _name, _self, lua::Marshal<_type>::Push(L, self->_member))

#define LUART_SET(_class, _name, _type, _member, _self) \
	LUART_SET_CUSTOM(_class, _name, _self, self->_member = lua::Marshal<_type>::Get(L, 2, true))

#define LUART_GETSET_CUSTOM(_class, _name, _self, _push, _get) \
	LUART_GET_CUSTOM(_class, _name, _self, _push) \
	LUART_SET_CUSTOM(_class, _name, _self, _get)

#define LUART_GET_CUSTOM(_class, _name, _self, _push) \
	int _class::LUART_GETFN(_name)(lua_State *L) \
	{\
		_self;\
		_push;\
		return 1;\
	}

#define LUART_SET_CUSTOM(_class, _name, _self, _get) \
	int _class::LUART_SETFN(_name)(lua_State *L) \
	{\
		_self;\
		_get;\
		return 0;\
	}

#define LUART_DECLARE_API(_api, _name) \
	_api const std::vector<const ::reflect::Class*> &_name##_lua_TypeRegistry()

#define LUART_IMPLEMENT_API(_api, _name) \
	_api std::vector<const ::reflect::Class*> s_##_name##_lua_typeReg; \
	_api void _name##_lua_RegisterType(const ::reflect::Class *type) \
	{ \
		RAD_ASSERT(type); \
		boost::lock_guard<boost::mutex> l(lua::TypeRegisterLock()); \
		s_##_name##_lua_typeReg.push_back(type); \
	} \
	_api const std::vector<const ::reflect::Class*> &_name##_lua_TypeRegistry() \
	{ \
		return s_##_name##_lua_typeReg; \
	}

#define LUART_API(_name) \
	&_name##_lua_TypeRegistry()[0], (int)_name##_lua_TypeRegistry().size()

#define LUART_REGISTER_TYPE(_api, _name, _class) \
	_api void _name##_lua_RegisterType(const ::reflect::Class *type); \
	namespace { \
	struct _name##_class##lua_type_register \
	{ \
		_name##_class##lua_type_register() \
		{ \
			_name##_lua_RegisterType(::reflect::Type<_class>()); \
		} \
	}; \
	_name##_class##lua_type_register _name##_class##lua_type_register_instance; \
	}

#define LUART_REGISTER_TYPE_NAMESPACE(_api, _name, _namespace, _class) \
	_api void _name##_lua_RegisterType(const ::reflect::Class *type); \
	namespace { \
	struct _name##_class##lua_type_register \
	{ \
		_name##_class##lua_type_register() \
		{ \
			_name##_lua_RegisterType(::reflect::Type<_namespace::_class>()); \
		} \
	}; \
	_name##_class##lua_type_register _name##_class##lua_type_register_instance; \
	}

RADREFLECT_DECLARE(RADRT_API, ::lua::Variant::Map);

#include "LuaRuntime.inl"
