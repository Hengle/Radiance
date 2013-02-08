// LuaRuntime.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "LuaRuntime.h"
#include <Runtime/Reflect.h>
#include <Runtime/Reflect/Attributes.h>
#include <Runtime/Reflect/RTLInterop.h>
#include <Runtime/String.h>
#include <Runtime/Thread.h>
#include <Runtime/Container/HashMap.h>
#include <Runtime/Base/MemoryPool.h>
#include <Runtime/File.h>
#include "../Zones.h"
#include "../COut.h"
#include <set>
#include <vector>
#include <limits>
#include <algorithm>
#include <new>

extern "C" {
#include <Lua/lualib.h>
}

#undef min
#undef max

//#define LOG_ALLOCS

#if LUA_VERSION_NUM >= 502
// Why did they remove this?
RADENG_API int RADENG_CALL luaL_typerror (lua_State *L, int narg, const char *tname) {
  const char *msg = lua_pushfstring(L, "%s expected, got %s",
                                    tname, luaL_typename(L, narg));
  return luaL_argerror(L, narg, msg);
}
#endif

namespace lua {

struct LuaPools {
	enum {
		kNumPools = 10,
		kBasePoolSize = 8,
		kMaxPoolSize = kBasePoolSize<<(kNumPools-1)
	};

	static inline int PoolIndex(size_t size) {
		if (size > kMaxPoolSize)
			return -1;

		BOOST_STATIC_ASSERT(kNumPools==10);
		BOOST_STATIC_ASSERT(kBasePoolSize==8);

		if (size <= 8)
			return 0;
		if (size <= 16)
			return 1;
		if (size <= 32)
			return 2;
		if (size <= 64)
			return 3;
		if (size <= 128)
			return 4;
		if (size <= 256)
			return 5;
		if (size <= 512)
			return 6;
		if (size <= 1024)
			return 7;
		if (size <= 2048)
			return 8;
		RAD_ASSERT(size<=4096);
		return 9;
	}

	static void *Alloc(void *ptr, size_t osize, size_t nsize) {
		if (!nsize) {
			if (ptr&&osize) {
				--s_numAllocs;

				int opool = PoolIndex(osize);
				if (opool < 0)
					zone_free(ptr);
				else
					s_pools[opool].ReturnChunk(ptr);
			}
			return 0;
		}

		if (nsize < s_min)
			s_min = nsize;
		if (nsize > s_max)
			s_max = nsize;

		int npool = PoolIndex(nsize);
		
		if (ptr&&osize) { 
			// existing allocation
			int opool = PoolIndex(osize);
			if (npool < 0) { 
				// heap
				if (opool < 0) { 
					// heap
					return safe_zone_realloc(ZLuaRuntime, ptr, nsize);
				}

				void *nptr = safe_zone_malloc(ZLuaRuntime, nsize);
				memcpy(nptr, ptr, std::min(osize, nsize));
				s_pools[opool].ReturnChunk(ptr);
				return nptr;
			} else { 
				// pool
				if (opool < 0) { 
					// heap
					void *nptr = s_pools[npool].SafeGetChunk();
					memcpy(nptr, ptr, nsize);
					zone_free(ptr);
					return nptr;
				} else if (npool != opool) { 
					// moving pools
					void *nptr = s_pools[npool].SafeGetChunk();
					memcpy(nptr, ptr, std::min(osize, nsize));
					s_pools[opool].ReturnChunk(ptr);
					return nptr;
				}

				return ptr;
			}
		} else { 
			// new allocation
			++s_numAllocs;
			if (npool < 0)
				return safe_zone_malloc(ZLuaRuntime, nsize);
			return s_pools[npool].SafeGetChunk();
		}
	}

	static void Open() {
		if (++s_refs == 1) {
			for (int i = 0; i < kNumPools; ++i) {
				AddrSize chunkSize = kBasePoolSize<<i;

				char sz[64];
				sprintf(sz, "lua_pool_%d", (int)chunkSize);

				s_pools[i].Create(
					ZLuaRuntime,
					sz,
					chunkSize,
					std::max<UReg>((UReg)((32*Kilo)/chunkSize), 4)
				);
			}
		}
	}

	static void Close() {
		RAD_ASSERT(s_refs>0);
		if (--s_refs == 0) {
			for (int i = 0; i < kNumPools; ++i)
				s_pools[i].Destroy();
		} else {
			Compact();
		}
	}

	static void Compact() {
		for (int i = 0; i < kNumPools; ++i)
			s_pools[i].Compact();
	}
	
#if defined(RAD_OPT_IOS)
	typedef boost::array<MemoryPool, kNumPools> PoolsArray;
#else
	typedef boost::array<ThreadSafeMemoryPool, kNumPools> PoolsArray;
#endif
	
	static int s_refs;
	static int s_numAllocs;
	static size_t s_min;
	static size_t s_max;
	static PoolsArray s_pools;
};

int LuaPools::s_refs = 0;
int LuaPools::s_numAllocs = 0;
size_t LuaPools::s_max = std::numeric_limits<size_t>::min();
size_t LuaPools::s_min = std::numeric_limits<size_t>::max();
LuaPools::PoolsArray LuaPools::s_pools;

State::State(const char *name) {
	string::ncpy(m_sz, name, 64);

	m_m.numAllocs = 0;
	m_m.smallest = std::numeric_limits<int>::max();
	m_m.biggest = std::numeric_limits<int>::min();
	LuaPools::Open();

	m_s = ::lua_newstate(LuaAlloc, this);
	RAD_ASSERT(m_s);
}

State::~State() {
	if (m_s)
		::lua_close(m_s);
	LuaPools::Close();
}

void State::CompactPools() {
	LuaPools::Compact();
}

void *State::LuaAlloc(void *ud, void *ptr, size_t osize, size_t nsize) {
	State *s = reinterpret_cast<State*>(ud);
	if (!nsize) {
		if (ptr&&osize)
			--s->m_m.numAllocs;
	} else {
		if ((int)nsize < s->m_m.smallest)
			s->m_m.smallest = (int)nsize;
		if ((int)nsize > s->m_m.biggest)
			s->m_m.biggest = (int)nsize;

		if (!osize)
			++s->m_m.numAllocs;
	}

#if defined(LOG_ALLOCS)
#if defined(RAD_OPT_PC)
	DebugString("Lua(%s): (%d/%d) - (%d/%d) - (%d)\n", 
		s->m_sz, 
		nsize, 
		osize, 
		s->m_m.smallest,
		s->m_m.biggest,
		s->m_m.numAllocs
	);
#else
	COut(C_Debug) << "Lua(" << s->m_sz << "): (" << nsize << "/" << osize << ") - (" <<
		s->m_m.smallest << "/" << s->m_m.biggest << ") - (" << s->m_m.numAllocs << ")" << std::endl;
#endif
#endif

	return LuaPools::Alloc(ptr, osize, nsize);
}

namespace {

#define INTEROP_GC_KEY "@igc"
#define RCALL_KEY "@rmc"
#define LOADER_KEY "@ldr"
#define MODULES_KEY "@imp"
#define NATIVE_CLASS_KEY "@ntv"

typedef ::reflect::Class Class;
typedef ::reflect::ATTRIBUTE Attribute;
typedef ::reflect::rtl_interop::Visible VisibleAttr;
typedef ::reflect::rtl_interop::RTLInterfaceHandle RTLInterfaceHandle;
typedef ::reflect::rtl_interop::RTLInterface RTLInterface;
typedef ::reflect::Enum EnumAttr;
typedef ::reflect::EnumValue EnumValue;
typedef ::reflect::ArgumentList ArgumentList;
typedef ::reflect::IFunction IFunction;

//////////////////////////////////////////////////////////////////////////////////////////
// Type Helpers
//////////////////////////////////////////////////////////////////////////////////////////

bool IsVisible(const Class *type) {
	VisibleAttr vis(false);
	bool isVis = type->AttributeValue(vis) && vis;

	if (!isVis) { 
		// make handles inherit their interfaces visiblity (they are hidden by default)
		RTLInterfaceHandle h(0);
		if (type->AttributeValue(h) && h.InterfaceType()) {
			isVis = h.InterfaceType()->AttributeValue(vis) && vis;
		}
	}

	return isVis;
}

bool IsEnum(const Class *type) {
	return type->FindAttribute(::reflect::Type<EnumAttr>()) != 0;
}

int NumEnumVals(const Class *type) {
	int c = 0;
	const int NumAttrs = type->NumAttributes();
	for (int i = 0; i < NumAttrs; ++i) {
		const ::reflect::ATTRIBUTE *attr = type->Attribute(i);
		if (attr->Type() == ::reflect::Type<EnumValue>()) {
			++c;
		}
	}

	return c;
}

bool IsInterface(const Class *type) {
	return type->FindAttribute(::reflect::Type<RTLInterface>()) != 0;
}

bool IsInterfaceHandle(const Class *type) {
	return type->FindAttribute(::reflect::Type<RTLInterfaceHandle>()) != 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// import
//////////////////////////////////////////////////////////////////////////////////////////

bool FileIsLoaded(lua_State *L, const char *name) {
	RAD_ASSERT(name);
	lua_getfield(L, LUA_REGISTRYINDEX, MODULES_KEY);
	lua_getfield(L, -1, name);
	bool loaded = lua_toboolean(L, -1) ? true : false;
	lua_pop(L, 2);
	return loaded;
}

void MarkFileLoaded(lua_State *L, const char *name) {
	RAD_ASSERT(name);
	lua_getfield(L, LUA_REGISTRYINDEX, MODULES_KEY);
	lua_pushboolean(L, 1);
	lua_setfield(L, -2, name);
	lua_pop(L, 1);
}

int lua_Import(lua_State *L) {
	const char *name = Marshal<const char*>::Get(L, -1, true);
	if (FileIsLoaded(L, name)) 
		return 0;

	lua_getfield(L, LUA_REGISTRYINDEX, LOADER_KEY);
	ImportLoader *l = reinterpret_cast<ImportLoader*>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	RAD_ASSERT(l);

	SrcBuffer::Ref code = l->Load(L, name);
	if (!code) { // load failed
		luaL_error(L, "Import: could not find file '%s'.", name);
	}

	MarkFileLoaded(L, name);

	if (luaL_loadbuffer(L, (const char *)((const void*)code->ptr), code->size, code->name)) {
		luaL_error(L, "Error importing '%s'(%s):\n\t%s", name, (const char*)code->name, lua_tostring(L, -1));
	}

	lua_call(L, 0, 0); // run loaded code.

	return 0;
}

int lua_NativeClass(lua_State *L) {
	const char *name = Marshal<const char*>::Get(L, -1, true);
	const Class *type = reflect::Class::Find<char>(name);

	if (!type) {
		luaL_error(L, "NativeClass: unable to find '%s' for import.", name);
	}

	ExportType(L, type);
	return 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// Reflection Interop Support
//////////////////////////////////////////////////////////////////////////////////////////

std::string FormatNamespace(const char *ns) {
	std::string x = "@"; // anything that starts with this means native.
	for (;*ns;++ns) {
		if (*ns == ':') {
			x += '_';
			++ns; // skip extra char.
		} else {
			x += *ns;
		}
	}

	return x;
}

struct RCMethod {
	enum MethodType {
		Static,
		Const,
		Mutable
	};

	MethodType methodType;
	const Class *type;
	const Class::MEMBER *iface;

	union {
		const Class::MUTABLEMETHOD *m;
		const Class::CONSTMETHOD   *c;
		const Class::STATICMETHOD  *s;
	} x;

	const Class::METHOD *M() const {
		switch (methodType) {
		case Static: return x.s;
		case Const: return x.c;
		case Mutable: return x.m;
		}
		return 0;
	}
};

void ExportClassFn(lua_State *L, const Class *type, const RCMethod *rcm) {
	luaL_getmetatable(L, RCALL_KEY);
	lua_setmetatable(L, -2); // assign reflected method call metatable.
	lua_setfield(L, -2, rcm->M()->Name<char>()); // pops udata
}

bool HasMutableMethodNamed(const Class *type, const char *name) {
	const int NumMethods = type->NumMethods();
	for (int i = 0; i < NumMethods; ++i) {
		VisibleAttr flag;
		const Class::MUTABLEMETHOD *m = type->Method(i);
		if (!strcmp(name, m->Name<char>()) && (!m->AttributeValue(flag) || flag)) {
			return true;
		}
	}

	return false;
}

void ExportClassHelper(lua_State *L, const Class *type, const Class::MEMBER *iface) {
	std::string name = FormatNamespace(type->Name<char>());

	// NOTE: this wastes a bit of space if there are a lot of const methods
	// that have duplicate mutable methods.
	// May need to reinvestigate this later.
	lua_getglobal(L, name.c_str());
	RAD_VERIFY(lua_isnil(L, -1) || lua_istable(L, -1));

	bool create = lua_isnil(L, -1);

	if (create) {
		lua_pop(L, 1);
		lua_createtable(L, 0,
			type->NumStaticMethods() +
			type->NumConstMethods() +
			type->NumMethods()
		);
	}

	// Static
	{
		const int NumMethods = type->NumStaticMethods();
		for (int i = 0; i < NumMethods; ++i) {
			VisibleAttr flag;
			const Class::STATICMETHOD *m = type->StaticMethod(i);
			if (!m->AttributeValue(flag) || flag) {
				RCMethod *rcm = new (lua_newuserdata(L, sizeof(RCMethod))) RCMethod;
				if (rcm) {
					rcm->methodType = RCMethod::Static;
					rcm->x.s = m;
					rcm->type = type;
					rcm->iface = 0;
					ExportClassFn(L, type, rcm);
				}
			}
		}
	}

	// Const (only if there is no mutable with the same name)
	{
		const int NumMethods = type->NumConstMethods();
		for (int i = 0; i < NumMethods; ++i) {
			VisibleAttr flag;
			const Class::CONSTMETHOD *m = type->ConstMethod(i);
			if (!HasMutableMethodNamed(type, m->Name<char>()) && (!m->AttributeValue(flag) || flag)) {
				RCMethod *rcm = new (lua_newuserdata(L, sizeof(RCMethod))) RCMethod;
				if (rcm) {
					rcm->methodType = RCMethod::Const;
					rcm->x.c = m;
					rcm->type = type;
					rcm->iface = iface;
					ExportClassFn(L, type, rcm);
				}
			}
		}
	}

	// Mutable
	{
		const int NumMethods = type->NumMethods();
		for (int i = 0; i < NumMethods; ++i) {
			VisibleAttr flag;
			const Class::MUTABLEMETHOD *m = type->Method(i);
			if (!m->AttributeValue(flag) || flag) {
				RCMethod *rcm = new (lua_newuserdata(L, sizeof(RCMethod))) RCMethod;
				if (rcm) {
					rcm->methodType = RCMethod::Mutable;
					rcm->x.m = m;
					rcm->type = type;
					rcm->iface = iface;
					ExportClassFn(L, type, rcm);
				}
			}
		}
	}

	if (create) {
		lua_setglobal(L, name.c_str());
	} else {
		lua_pop(L, 1);
	}
}

int lua_gcReflected(lua_State *L) {
	ReflectedRef *ref = reinterpret_cast<ReflectedRef*>(luaL_checkudata(L, 1, INTEROP_GC_KEY));
	RAD_ASSERT(ref);
	ref->~ReflectedRef();
	return 0; // lua will free memory.
}

int lua_gcReflectedCall(lua_State *L) {
	RCMethod *m = reinterpret_cast<RCMethod*>(luaL_checkudata(L, 1, RCALL_KEY));
	RAD_ASSERT(m);
	m->~RCMethod();
	return 0; // lua will free memory.
}

struct ArgRef {
	virtual ~ArgRef() {}
};

typedef boost::shared_ptr<ArgRef> ArgRefRef;

template<typename T>
struct TArgRef : public ArgRef {
	T storage;
};

typedef std::vector<ArgRefRef> ArgRefs;

template <typename T>
inline bool MarshalGetBasicHelper(lua_State *L, int index, const IFunction::ARGUMENT *arg, ArgumentList &list, void *basic) {
	if (arg->Type() == ::reflect::Type<T>()) {
		*reinterpret_cast<T*>(basic) = Marshal<T>::Get(L, index, true);
		list.PushBack(::reflect::Reflect(basic, ::reflect::Type<T>()));
		return true;
	}
	return false;
}

template <typename T>
inline bool MarshalPushBasicHelper(lua_State *L, const ::reflect::Reflected &reflected) {
	if (reflected.Type() == ::reflect::Type<T>()) {
		T val = *static_cast<const T*>(reflected);
		Marshal<T>::Push(L, val);
		return true;
	}
	return false;
}

bool MarshalGetBasic(lua_State *L, int index, const IFunction::ARGUMENT *arg, ArgumentList &list, void *basic) {
	return MarshalGetBasicHelper<S8>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<U8>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<S16>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<U16>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<S32>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<U32>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<S64>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<U64>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<F32>(L, index, arg, list, basic) ||
		MarshalGetBasicHelper<F64>(L, index, arg, list, basic);
}

bool MarshalPushBasicOrString(lua_State *L, const ::reflect::Reflected &reflected) {
	// we can do the strings here because we don't have generate temporary storage for the arguments
	return MarshalPushBasicHelper<S8>(L, reflected) ||
		MarshalPushBasicHelper<U8>(L, reflected) ||
		MarshalPushBasicHelper<S16>(L, reflected) ||
		MarshalPushBasicHelper<U16>(L, reflected) ||
		MarshalPushBasicHelper<S32>(L, reflected) ||
		MarshalPushBasicHelper<U32>(L, reflected) ||
		MarshalPushBasicHelper<S64>(L, reflected) ||
		MarshalPushBasicHelper<U64>(L, reflected) ||
		MarshalPushBasicHelper<F32>(L, reflected) ||
		MarshalPushBasicHelper<F64>(L, reflected) ||
		MarshalPushBasicHelper<const char*>(L, reflected) ||
		MarshalPushBasicHelper<const wchar_t*>(L, reflected) ||
		MarshalPushBasicHelper<std::string>(L, reflected) ||
		MarshalPushBasicHelper<std::wstring>(L, reflected) ||
		MarshalPushBasicHelper< String >(L, reflected);
}

bool MarshalGetString(lua_State *L, int index, const IFunction::ARGUMENT *arg, ArgumentList &list, ArgRefs &refs, void *basic) {
	if (arg->Type() == ::reflect::Type<const char*>()) {
		*reinterpret_cast<const char**>(basic) = Marshal<const char*>::Get(L, index, true);
		list.PushBack(Reflect(basic, ::reflect::Type<const char*>()));
		return true;
	}

	if (arg->Type() == ::reflect::Type<const wchar_t*>()) {
		// marshal as std::wstring
		TArgRef<std::wstring> *sref = new (ZLuaRuntime) TArgRef<std::wstring>();
		refs.push_back(ArgRefRef(sref)); // avoid type exceptions causing leak.
		sref->storage = Marshal<std::wstring>::Get(L, index, true);
		*reinterpret_cast<const wchar_t**>(basic) = sref->storage.c_str();
		list.PushBack(::reflect::Reflect(basic, ::reflect::Type<const wchar_t*>()));
		return true;
	}

	if (arg->Type() == ::reflect::Type<std::string>()) {
		TArgRef<std::string> *sref = new (ZLuaRuntime) TArgRef<std::string>();
		refs.push_back(ArgRefRef(sref)); // avoid type exceptions causing leak.
		sref->storage = Marshal<std::string>::Get(L, index, true);
		list.PushBack(::reflect::Reflect(&sref->storage, ::reflect::Type<std::string>()));
		return true;
	}

	if (arg->Type() == ::reflect::Type<std::wstring>()) {
		TArgRef<std::wstring> *sref = new (ZLuaRuntime) TArgRef<std::wstring>();
		refs.push_back(ArgRefRef(sref)); // avoid type exceptions causing leak.
		sref->storage = Marshal<std::wstring>::Get(L, index, true);
		list.PushBack(::reflect::Reflect(&sref->storage, ::reflect::Type<std::wstring>()));
		return true;
	}

	if (arg->Type() == ::reflect::Type<String>()) {
		TArgRef<String> *sref = new (ZLuaRuntime) TArgRef<String>();
		refs.push_back(ArgRefRef(sref)); // avoid type exceptions causing leak.
		sref->storage = Marshal<String>::Get(L, index, true);
		list.PushBack(::reflect::Reflect(&sref->storage, ::reflect::Type<String>()));
		return true;
	}

	return false;
}

bool MarshalGetEnum(lua_State *L, int index, const IFunction::ARGUMENT *arg, ArgumentList &list, void *basic) {
	if (IsEnum(arg->Type())) {
		*reinterpret_cast<S32*>(basic) = Marshal<S32>::Get(L, index, true);
		list.PushBack(::reflect::Reflect(basic, arg->Type()));
	}
	return false;
}

bool MarshalPushEnum(lua_State *L, const ::reflect::Reflected &reflected) {
	if (IsEnum(reflected.Type())) {
		// we can't cast to an S32, since that will cause a cast exception, since
		// the reflection system doesn't know an enum is an integral type.
		S32 x = *reinterpret_cast<S32*>(reflected.Data());
		Marshal<S32>::Push(L, x);
		return true;
	}
	return false;
}

void MarshalGetArgument(lua_State *L, int index, const Class::METHOD *m, ArgumentList &list, ArgRefs &refs, void *basic) {
	const IFunction::ARGUMENT *arg = m->Argument(index);
	RAD_ASSERT(arg);

	bool generic = !MarshalGetBasic(L, index, arg, list, basic) &&
		!MarshalGetEnum(L, index, arg, list, basic) &&
		!MarshalGetString(L, index, arg, list, refs, basic);

	if (generic) {
		// just do reflected marshal
		list.PushBack(Marshal< ::reflect::Reflected>::Get(L, index));
	}
}

bool MarshalPushArgument(lua_State *L, const ::reflect::Reflected &reflected) {
	bool free = true;
	bool generic = !MarshalPushBasicOrString(L, reflected) && !MarshalPushEnum(L, reflected);

	if (generic) {
		free = false;
		// just do reflected marshal (it's a C++ object or whatever, it's opaque to lua).
		Marshal< ::reflect::Reflected>::Push(L, reflected);
	}

	return free;
}

// stack: metatable, thisptr (if not static), args...
int lua_ReflectedCall(lua_State *L) {
	const int numArgs = lua_gettop(L);
	RCMethod *m = reinterpret_cast<RCMethod*>(luaL_checkudata(L, 1, RCALL_KEY));
	RAD_ASSERT(m);
	const Class::METHOD *method = m->M();
	::reflect::Reflected self;
	int argOfs = 2;

	if (m->methodType != RCMethod::Static) {
		self = Marshal< ::reflect::Reflected>::Get(L, argOfs++);
		RAD_ASSERT(self.IsValid());

		if (m->iface) {	
			// marshal an InterfaceHandle into an IInterface derived class.
			// type check
			RTLInterfaceHandle attr(0);

			if (!self.Type()->AttributeValue(attr) || !attr.InterfaceType()->IsA(m->type)) {
				luaL_error(L, "Type Error: expected an HInterface for %s got %s. Class %s, Function %s, (Function %s, File %s, Line %d)",
					m->type->Name<char>(),
					self.Type()->Name<char>(),
					m->type->Name<char>(),
					method->Name<char>(),
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			}

			::reflect::Reflected thisPtr = self.Member(m->iface, false); // no cast necessary, IInterface always parent.
			RAD_ASSERT(thisPtr.IsValid());
			self = ::reflect::Reflect(*static_cast<void**>(thisPtr), m->type);
			RAD_ASSERT(self.IsValid());
		}
	}

	ArgRefs refs; // <-- this holds shared_ptrs to complex arguments.
	ArgumentList args;

	for (;argOfs < numArgs; ++argOfs) {
#define CAWARN_DISABLE 6263
#include <Runtime/PushCAWarnings.h>
		MarshalGetArgument(L, argOfs, method, args, refs, stack_alloc(8));
#include <Runtime/PopCAWarnings.h>
	}

	::reflect::Reflected result;

	if (method->ReturnType()) { 
		// non-void return
		// TODO: we need to pool this for built-in types.
		result = ::reflect::Reflected::New(ZLuaRuntime, method->ReturnType(), ::reflect::NullArgs());
		RAD_ASSERT(result.IsValid());
	}

	try {
		switch(m->methodType) {
		case RCMethod::Static:
			m->x.s->Call(result, args);
			break;
		case RCMethod::Mutable:
			self.CallMethod(result, m->x.m, args);
			break;
		case RCMethod::Const:
			self.CallConstMethod(result, m->x.c, args);
			break;
		}
	} catch (::reflect::IFunction::InvalidArgumentExceptionType &e) {
		if (method->ReturnType()) {
			result.Delete();
		}

		luaL_error(L, "InvalidArgumentException: not enough arguments for function call. First missing argument (%d) %s, Class %s, Function %s, (Function %s, File %s, Line %d)",
			e.ArgumentIndex(),
			e.Argument()->Name<char>(),
			m->type->Name<char>(),
			method->Name<char>(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	} catch (::reflect::IFunction::MissingArgumentExceptionType &e) {
		if (method->ReturnType()) {
			result.Delete();
		}

		luaL_error(L, "MissingArgumentException: not enough arguments for function call. First missing argument (%d) %s, Class %s, Function %s, (Function %s, File %s, Line %d)",
			e.ArgumentIndex(),
			e.Argument()->Name<char>(),
			m->type->Name<char>(),
			method->Name<char>(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	} catch (::reflect::InvalidCastException &) {
		if (method->ReturnType()) {
			result.Delete();
		}

		luaL_error(L, "InvalidCastException (invalid argument passed to function), Class %s, Function %s, (Function %s, File %s, Line %d)",
			m->type->Name<char>(),
			method->Name<char>(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	} catch (exception &e) {
		if (method->ReturnType()) {
			result.Delete();
		}

		const char *eType = e.type() ? e.type() : "unknown";
		const char *eMsg  = e.what() ? e.what() : "N/A";

		luaL_error(L, "Unhandled exception '%s' occurred, Msg '%s', Class %s, Function %s, (Function %s, File %s, Line %d)",
			eType,
			eMsg,
			m->type->Name<char>(),
			method->Name<char>(),
			__FUNCTION__,
			__FILE__,
			__LINE__
		);
	}

	if (method->ReturnType() && MarshalPushArgument(L, result)) {
		// since the Reflected object is a reference to the object,
		// it's a pointer to the type which is allocated on the heap.
		// These pointers are marshaled by value to lua and can be freed.
		result.Delete();
	}

	return method->ReturnType() ? 1 : 0;
}

} // namespace

//////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////

RADENG_API boost::mutex &RADENG_CALL TypeRegisterLock() {
	static boost::mutex m;
	return m;
}

ReflectedRef *ReflectedRef::PushNew(lua_State *L, const ::reflect::Reflected &r) {
	RAD_ASSERT(L);
	ReflectedRef *ref = new (lua_newuserdata(L, sizeof(ReflectedRef))) ReflectedRef(r);
	luaL_getmetatable(L, INTEROP_GC_KEY);
	lua_setmetatable(L, -2);
	return ref;
}

ReflectedRef *ReflectedRef::Get(lua_State *L, int index) {
	RAD_ASSERT(L);
	ReflectedRef *ref = reinterpret_cast<ReflectedRef*>(luaL_checkudata(L, index, INTEROP_GC_KEY));
	if (!ref) {
		luaL_error(L, "Argument %d is not a C++ object. (Function %s, File %s, Line %d).",
			index,
			__FUNCTION__, __FILE__, __LINE__
		);
	}
	return ref;
}

RADENG_API void RADENG_CALL PushUserData(lua_State *L, void *data) {
	lua_pushlightuserdata(L, data);
	lua_gettable(L, LUA_REGISTRYINDEX);
}

RADENG_API void RADENG_CALL MapUserData(lua_State *L) {
	lua_pushvalue(L, -1); // copy user data. s+2
	lua_pushvalue(L, -1); // s+3, 4
	void *x = lua_touserdata(L, -1);
	RAD_ASSERT(x);
	lua_pushlightuserdata(L, x); // s+4, k
	lua_replace(L, -3); // s+3
	lua_settable(L, LUA_REGISTRYINDEX); // t[k] = v
}

RADENG_API void RADENG_CALL UnmapUserData(lua_State *L, void *data) {
	lua_pushlightuserdata(L, data);
	lua_pushnil(L);
	lua_settable(L, LUA_REGISTRYINDEX);
}

namespace {

void ExportEnum(lua_State *L, const Class *type) {
	// create global table with enum name.
	// for each value in enum
	//   make a table field, assign value.
	std::string name = FormatNamespace(type->Name<char>());
	lua_createtable(L, 0, NumEnumVals(type));
	const int NumAttrs = type->NumAttributes();
	for (int i = 0; i < NumAttrs; ++i) {
		const Attribute *attr = type->Attribute(i);
		if (attr->Type() == ::reflect::Type<EnumValue>()) {
			const EnumValue *val = (const EnumValue*)attr->Value();
			lua_pushnumber(L, (lua_Number)val->value);
			lua_setfield(L, -2, attr->Name<char>());
		}
	}
	lua_setglobal(L, name.c_str());
}

void ExportClass(lua_State *L, const Class *type) {
	ExportClassHelper(L, type, 0);
}

void ExportInterface(lua_State *L, const Class *type) {
	static const Class::MEMBER *ifacePtr = 0;
	if (!ifacePtr) { 
		// cache iinterface data pointer.
		boost::lock_guard<boost::mutex> l(TypeRegisterLock());
		if (!ifacePtr) {
			const Class *iinterface = ::reflect::Type<HInterface>();
			RAD_ASSERT(iinterface);
			ifacePtr = iinterface->FindMember<char>("Data");
			RAD_ASSERT(ifacePtr);
		}
	}
	ExportClassHelper(L, type, ifacePtr);
}

} // namespace

const void *FileSrcBuffer::RAD_IMPLEMENT_GET(ptr) { 
	return m_mm->data; 
}

AddrSize FileSrcBuffer::RAD_IMPLEMENT_GET(size) { 
	return m_mm->size; 
}

const char *FileSrcBuffer::RAD_IMPLEMENT_GET(name) { 
	return m_name.c_str; 
}

RADENG_API void RADENG_CALL ExportType(lua_State *L, const Class *type) {
	RAD_ASSERT(L && type);

	lua_getfield(L, LUA_REGISTRYINDEX, NATIVE_CLASS_KEY);
	RAD_ASSERT(lua_isnil(L, -1) || lua_istable(L, -1));

	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		lua_newtable(L);
		lua_pushvalue(L, -1);
		lua_setfield(L, LUA_REGISTRYINDEX, NATIVE_CLASS_KEY);
	}

	lua_getfield(L, -1, type->Name<char>());
	if (!lua_isnil(L, -1)) {
		lua_pop(L, 2);
		return; // already been done.
	}

	lua_pushboolean(L, 1);
	lua_setfield(L, -3, type->Name<char>());
	lua_pop(L, 2);

	if (IsEnum(type)) {
		ExportEnum(L, type);
	} else if (IsInterface(type)) {
		ExportInterface(L, type);
	} else if (!IsInterfaceHandle(type)) {
		ExportClass(L, type);
	}
}

RADENG_API void RADENG_CALL ExportTypes(lua_State *L, const ::reflect::Class **types, int num) {
	if (num == 0) 
		return;

	int ofs = 0;
	while (num > 0 || (num==-1 && types[ofs])) {
		ExportType(L, types[ofs]);

		++ofs;
		if (num > 0) {
			--num;
		}
	}
}

namespace {

void AddNativeGCHandlers(lua_State *L) {
	// interop type collector for gc
	if (luaL_newmetatable(L, INTEROP_GC_KEY)) {
		lua_pushcfunction(L, lua_gcReflected);
		lua_setfield(L, -2, "__gc");
	}

	// interop reflected methodcall
	if (luaL_newmetatable(L, RCALL_KEY)) {
		lua_pushcfunction(L, lua_gcReflectedCall);
		lua_setfield(L, -2, "__gc");
		lua_pushcfunction(L, lua_ReflectedCall);
		lua_setfield(L, -2, "__call");
	}

	lua_pop(L, 2);
}

}

RADENG_API void RADENG_CALL EnableModuleImport(lua_State *L, ImportLoader &loader) {
	lua_pushlightuserdata(L, &loader);
	lua_setfield(L, LUA_REGISTRYINDEX, LOADER_KEY);

	// 'import' function
	lua_createtable(L, 0, 0); // make table to hold imported modules.
	lua_setfield(L, LUA_REGISTRYINDEX, MODULES_KEY);

	luaL_Reg regs [] = {
		{ "Import", lua_Import },
		{ 0, 0 }
	};

	RegisterGlobals(L, 0, regs);
	AddNativeGCHandlers(L);
}

RADENG_API bool RADENG_CALL GetFieldExt(lua_State *L, int index, const char *k) {
	RAD_ASSERT(k);
	char path[256];
	char *t = path;
	*t = 0;
	bool first = true;

	for (;;) {
		if (!*k || *k == '.') {
			if (*path) {
				*t++ = 0;

				if (lua_type(L, index) != LUA_TTABLE)
					return false;
				lua_getfield(L, index, path);
				if (!first)
					lua_remove(L, -2);
				first = false;
				index = -1;

				t = path;
				*t = 0;
			}

			if (!*k)
				break;
		} else {
			*t++ = *k;
		}

		++k;
	}

	return true;
}

RADENG_API bool RADENG_CALL ImportModule(lua_State *L, const char *name) {
	if (FileIsLoaded(L, name)) 
		return true;

	lua_getfield(L, LUA_REGISTRYINDEX, LOADER_KEY);
	ImportLoader *l = reinterpret_cast<ImportLoader*>(lua_touserdata(L, -1));
	lua_pop(L, 1);
	RAD_ASSERT(l);

	SrcBuffer::Ref code = l->Load(L, name);
	if (!code) { 
		// load failed
		COut(C_Error) << "Import: could not find file " << name << std::endl;
		return false;
	}

	MarkFileLoaded(L, name);

	if (luaL_loadbuffer(L, (const char *)((const void*)code->ptr), code->size, code->name)) {
		COut(C_Error) << "Error importing " << name << "(" << (const char*)code->name.get() << ")" << std::endl << lua_tostring(L, -1) << std::endl;
		return false;
	}

	if (lua_pcall(L, 0, 0, 0)) {
		COut(C_Error) << "ScriptError(" << name << "): " << lua_tostring(L, -1) << std::endl;
		return false;
	}

	return true;
}

RADENG_API void RADENG_CALL EnableNativeClassImport(lua_State *L) {
	luaL_Reg regs [] = {
		{ "NativeClass", lua_NativeClass },
		{ 0, 0 }
	};

	RegisterGlobals(L, 0, regs);
	AddNativeGCHandlers(L);
}

RADENG_API void RADENG_CALL RegisterGlobals(lua_State *L, const char *table, luaL_Reg *r) {
	RAD_ASSERT(L);
#if LUA_VERSION_NUM >= 502
	int index = -1000;
#else
	int index = LUA_GLOBALSINDEX;
#endif

	if (table != 0) {
		int size = 0;
		for (luaL_Reg *x = r; x->name && x->func; ++x, ++size) {}
		lua_createtable(L, 0, size);
		index = -2;
	}

	while (r->name && r->func) {
		lua_pushcfunction(L, r->func);
#if LUA_VERSION_NUM >= 502
		if (index == -1000) {
			lua_setglobal(L, r->name);
		} else {
			lua_setfield(L, index, r->name);
		}
#else
		lua_setfield(L, index, r->name);
#endif
		++r;
	}

	if (table != 0) {
		lua_setglobal(L, table);
	}
}

RADENG_API bool RADENG_CALL ParseVariantTable(lua_State *L, Variant::Map &map, bool luaError) {
	if (lua_type(L, -1) != LUA_TTABLE) {
		if (luaError) {
			luaL_error(L, "Expected table, (Function %s, File %s, Line %d).",
				__FUNCTION__,
				__FILE__,
				__LINE__
			);
		} else {
			COut(C_Error) << "ParseVariantTable: error expected table!" << std::endl;
			return false;
		}
	}

	lua_checkstack(L, 3);
	lua_pushnil(L);
	while (lua_next(L, -2) != 0) {
		String key;

		if (lua_type(L, -2) == LUA_TNUMBER) {
			lua_Number n = lua_tonumber(L, -2);
			char x[256];
			string::sprintf(x, "%i", (int)n);
			key = x;
		} else if (lua_type(L, -2) == LUA_TSTRING) {
			key = lua_tolstring(L, -2, 0);
		} else {
			if (luaError) {
				luaL_error(L, "Invalid key type for variant table, (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			} else {
				lua_pop(L, 2);
				COut(C_Error) << "ParseVariantTable: Invalid key type for variant table!" << std::endl;
				return false;
			}
		}

		switch (lua_type(L, -1)) {
		case LUA_TTABLE:
			{
				reflect::SharedReflected::Ref table(
					new (ZLuaRuntime) reflect::SharedReflected(
						reflect::Reflected::New<void ()>(
							ZLuaRuntime,
							reflect::Type<lua::Variant::Map>(),
							reflect::NullArgs()
						)
					)
				);
				RAD_ASSERT(table);
				if(map.insert(Variant::Map::value_type(key, Variant(table))).second) {
					if (!ParseVariantTable(L, *static_cast<Variant::Map*>(*table), luaError)) {
						return false;
					}
				}
			} break;
		case LUA_TSTRING:
			{
				reflect::SharedReflected::Ref string(
					new (ZLuaRuntime) reflect::SharedReflected(
						reflect::Reflected::New<void ()>(
							ZLuaRuntime,
							reflect::Type<String>(),
							reflect::NullArgs()
						)
					)
				);
				RAD_ASSERT(string);
				*static_cast<String*>(*string) = String(lua_tolstring(L, -1, 0));
				map.insert(Variant::Map::value_type(key, Variant(string)));
			} break;
		case LUA_TNUMBER:
			{
				lua_Number *x = (lua_Number*)::reflect::SafeAllocate(ZLuaRuntime, ::reflect::Type<lua_Number>());
				reflect::SharedReflected::Ref number(
					new (ZLuaRuntime) reflect::SharedReflected(
						reflect::Reflect(*x)
					)
				);
				*x = lua_tonumber(L, -1);
				RAD_ASSERT(*x == *static_cast<lua_Number*>(*number));
				map.insert(Variant::Map::value_type(key, Variant(number)));
			} break;
		case LUA_TBOOLEAN:
			{
				bool *x = (bool*)::reflect::SafeAllocate(ZLuaRuntime, ::reflect::Type<bool>());
				reflect::SharedReflected::Ref boolean(
					new (ZLuaRuntime) reflect::SharedReflected(
						reflect::Reflect(*x)
					)
				);
				*x = lua_toboolean(L, -1) ? true : false;
				RAD_ASSERT(*x == *static_cast<bool*>(*boolean));
				map.insert(Variant::Map::value_type(key, Variant(boolean)));
			} break;
		case LUA_TLIGHTUSERDATA:
		case LUA_TUSERDATA:
			{
				void **x = (void**)::reflect::SafeAllocate(ZLuaRuntime, ::reflect::Type<void*>());
				reflect::SharedReflected::Ref ptr(
					new (ZLuaRuntime) reflect::SharedReflected(
						reflect::Reflect(*x)
					)
				);
				*x = lua_touserdata(L, -1);
				RAD_ASSERT(*x == *static_cast<void**>(*ptr));
				map.insert(Variant::Map::value_type(key, Variant(ptr)));
			} break;
		default:
			if (luaError) {
				luaL_error(L, "Invalid value type for variant, (Function %s, File %s, Line %d).",
					__FUNCTION__,
					__FILE__,
					__LINE__
				);
			} else {
				COut(C_Error) << "ParseVariantTable: Invalid value type for variant!" << std::endl;
				return false;
			}
			break;
		}

		lua_pop(L, 1);
	}

	return true;
}

void Marshal<Vec2>::Push(lua_State *L, const Vec2 &val) {
	RAD_ASSERT(L);
	lua_createtable(L, 2, 0);
	
	for (int i = 0; i < 2; ++i) {
		lua_pushinteger(L, i+1);
		lua_pushnumber(L, val[i]);
		lua_settable(L, -3);
	}
}

Vec2 Marshal<Vec2>::Get(lua_State *L, int index, bool luaError) {
	if (lua_type(L, index) != LUA_TTABLE) {
		if (luaError)
			luaL_typerror(L, index, "Vec2");
		return Vec2::Zero;
	}

	Vec2 v;
	for (int i = 0; i < 2; ++i) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, index < 0 ? index-1 : index);
		if (luaError && lua_type(L, -1) != LUA_TNUMBER)
			luaL_typerror(L, index, "Vec2");
		v[i] = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	return v;
}

void Marshal<Vec3>::Push(lua_State *L, const Vec3 &val) {
	RAD_ASSERT(L);
	lua_createtable(L, 3, 0);
	
	for (int i = 0; i < 3; ++i) {
		lua_pushinteger(L, i+1);
		lua_pushnumber(L, val[i]);
		lua_settable(L, -3);
	}
}

Vec3 Marshal<Vec3>::Get(lua_State *L, int index, bool luaError) {
	if (lua_type(L, index) != LUA_TTABLE) {
		if (luaError)
			luaL_typerror(L, index, "Vec3");
		return Vec3::Zero;
	}

	Vec3 v;
	for (int i = 0; i < 3; ++i) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, index < 0 ? index-1 : index);
		if (luaError && lua_type(L, -1) != LUA_TNUMBER)
			luaL_typerror(L, index, "Vec3");
		v[i] = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	return v;
}

void Marshal<Vec4>::Push(lua_State *L, const Vec4 &val) {
	RAD_ASSERT(L);
	lua_createtable(L, 4, 0);
	
	for (int i = 0; i < 4; ++i) {
		lua_pushinteger(L, i+1);
		lua_pushnumber(L, val[i]);
		lua_settable(L, -3);
	}
}

Vec4 Marshal<Vec4>::Get(lua_State *L, int index, bool luaError) {
	if (lua_type(L, index) != LUA_TTABLE) {
		if (luaError)
			luaL_typerror(L, index, "Vec4");
		return Vec4::Zero;
	}

	Vec4 v;
	for (int i = 0; i < 4; ++i) {
		lua_pushinteger(L, i+1);
		lua_gettable(L, index < 0 ? index-1 : index);
		if (luaError && lua_type(L, -1) != LUA_TNUMBER)
			luaL_typerror(L, index, "Vec4");
		v[i] = (float)lua_tonumber(L, -1);
		lua_pop(L, 1);
	}

	return v;
}

} // lua

#include <Runtime/ReflectMap.h>

RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE(
	"lua::Variant::Map",
	lua,
	Variant::Map,
	map)

	RADREFLECT_CONSTRUCTOR

RADREFLECT_END_NAMESPACE(RADENG_API, lua, Variant::Map)
