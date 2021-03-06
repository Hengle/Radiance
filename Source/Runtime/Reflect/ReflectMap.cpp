// ReflectMap.cpp
// Reflection maps for base types.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if !defined(RAD_OPT_NO_REFLECTION)

#include "ReflectMap.h"

RADREFLECT_TYPE_NULL(RADRT_API, void)
RADREFLECT_TYPE(RADRT_API, "void*", void*)
RADREFLECT_TYPE(RADRT_API, "bool", bool)
RADREFLECT_TYPE(RADRT_API, "bool*", bool*)
RADREFLECT_TYPE(RADRT_API, "const bool*", const bool*)
RADREFLECT_TYPE(RADRT_API, "char", char)
RADREFLECT_TYPE(RADRT_API, "char*", char*)
RADREFLECT_TYPE(RADRT_API, "const char*", const char*)
RADREFLECT_TYPE(RADRT_API, "const void*", const void*)

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
RADREFLECT_TYPE(RADRT_API, "wchar_t", wchar_t)
RADREFLECT_TYPE(RADRT_API, "wchar_t*", wchar_t*)
RADREFLECT_TYPE(RADRT_API, "const wchar_t*", const wchar_t*)
#endif

RADREFLECT_TYPE(RADRT_API, "S8", S8)
RADREFLECT_TYPE(RADRT_API, "S8*", S8*)
RADREFLECT_TYPE(RADRT_API, "const S8*", const S8*)
RADREFLECT_TYPE(RADRT_API, "U8", U8)
RADREFLECT_TYPE(RADRT_API, "U8*", U8*)
RADREFLECT_TYPE(RADRT_API, "const U8*", const U8*)
RADREFLECT_TYPE(RADRT_API, "S16", S16)
RADREFLECT_TYPE(RADRT_API, "S16*", S16*)
RADREFLECT_TYPE(RADRT_API, "const S16*", const S16*)
RADREFLECT_TYPE(RADRT_API, "U16", U16)
RADREFLECT_TYPE(RADRT_API, "U16*", U16*)
RADREFLECT_TYPE(RADRT_API, "const U16*", const U16*)
RADREFLECT_TYPE(RADRT_API, "S32", S32)
RADREFLECT_TYPE(RADRT_API, "S32*", S32*)
RADREFLECT_TYPE(RADRT_API, "const S32*", const S32*)
RADREFLECT_TYPE(RADRT_API, "U32", U32)
RADREFLECT_TYPE(RADRT_API, "U32*", U32*)
RADREFLECT_TYPE(RADRT_API, "const U32*", const U32*)
RADREFLECT_TYPE(RADRT_API, "S64", S64)
RADREFLECT_TYPE(RADRT_API, "S64*", S64*)
RADREFLECT_TYPE(RADRT_API, "const S64*", const S64*)
RADREFLECT_TYPE(RADRT_API, "U64", U64)
RADREFLECT_TYPE(RADRT_API, "U64*", U64*)
RADREFLECT_TYPE(RADRT_API, "const U64*", const U64*)
RADREFLECT_TYPE(RADRT_API, "F32", F32)
RADREFLECT_TYPE(RADRT_API, "F32*", F32*)
RADREFLECT_TYPE(RADRT_API, "const F32*", const F32*)
RADREFLECT_TYPE(RADRT_API, "F64", F64)
RADREFLECT_TYPE(RADRT_API, "F64*", F64*)
RADREFLECT_TYPE(RADRT_API, "const F64*", const F64*)

using namespace reflect;

RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE("string::String", string, String, String)
	RADREFLECT_CONSTRUCTOR
RADREFLECT_END_NAMESPACE(RADRT_API, string, String)

RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE("std::string", std, string, basic_string)
	RADREFLECT_CONSTRUCTOR
RADREFLECT_END_NAMESPACE(RADRT_API, std, string)

RADREFLECT_BEGIN_CLASS_DESTRUCTOR_NAMESPACE("std::wstring", std, wstring, basic_string)
	RADREFLECT_CONSTRUCTOR
RADREFLECT_END_NAMESPACE(RADRT_API, std, wstring)

RADREFLECT_CLASS(RADRT_API, "reflect::Enum", Enum)
RADREFLECT_CLASS(RADRT_API, "reflect::EnumFlags", EnumFlags)
RADREFLECT_CLASS(RADRT_API, "reflect::EnumValue", EnumValue)
RADREFLECT_CLASS(RADRT_API, "reflect::ConstReflected", ConstReflected)

RADREFLECT_BEGIN_CLASS("reflect::Reflected", Reflected)
	RADREFLECT_SUPER(ConstReflected)
RADREFLECT_END(RADRT_API, Reflected)

RADREFLECT_TYPE(RADRT_API, "reflect::Class*", Class*)
RADREFLECT_TYPE(RADRT_API, "reflect::Class*", const Class*)
RADREFLECT_TYPE(RADRT_API, "reflect::Class::STATICMEMBER*", Class::STATICMEMBER*)
RADREFLECT_TYPE(RADRT_API, "const reflect::Class::STATICMEMBER*", const Class::STATICMEMBER*)
RADREFLECT_TYPE(RADRT_API, "reflect::Class::STATICCONSTANT*", Class::STATICCONSTANT*)
RADREFLECT_TYPE(RADRT_API, "const reflect::Class::STATICCONSTANT*", const Class::STATICCONSTANT*)
RADREFLECT_TYPE(RADRT_API, "reflect::Class::MUTABLEMETHOD*", Class::MUTABLEMETHOD*)
RADREFLECT_TYPE(RADRT_API, "const reflect::Class::MUTABLEMETHOD*", const Class::MUTABLEMETHOD*)
RADREFLECT_TYPE(RADRT_API, "reflect::Class::CONSTMETHOD*", Class::CONSTMETHOD*)
RADREFLECT_TYPE(RADRT_API, "const reflect::Class::CONSTMETHOD*", const Class::CONSTMETHOD*)
RADREFLECT_TYPE(RADRT_API, "reflect::Class::STATICMETHOD*", Class::STATICMETHOD*)
RADREFLECT_TYPE(RADRT_API, "const reflect::Class::STATICMETHOD*", const Class::STATICMETHOD*)

#endif // !defined(RAD_OPT_NO_REFLECTION)

