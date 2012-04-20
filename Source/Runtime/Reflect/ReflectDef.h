// ReflectDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntReflect.h"
#include "Private/ReflectPrivateDef.h"
#include "../String.h"
#include "../PushPack.h"

//////////////////////////////////////////////////////////////////////////////////////////
// Public reflection declaration macro
//////////////////////////////////////////////////////////////////////////////////////////
//
// This should be used inside of a reflected class if you want to be able to
// reflect protected and private data
//
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_EXPOSE_PRIVATES(_type) \
	PRIVATE_RADREFLECT_EXPOSE_PRIVATES(_type)

//////////////////////////////////////////////////////////////////////////////////////////
// Public reflection declaration macro
//////////////////////////////////////////////////////////////////////////////////////////
//
// These use template specialization to create the reflect::Type<_type>()
// specialization and to automatically register the Class for complex types.
//
// These must be used in the global namespace.
//
//////////////////////////////////////////////////////////////////////////////////////////

#define RADREFLECT_DECLARE(_api, _type) \
	PRIVATE_RADREFLECT_DECLARE(_api, _type)


namespace reflect {

class Attribute;

class Class;

class ConstReflected;
class Reflected;

class ConstArray;
class Array;

template <typename T> const Class *Type();

//////////////////////////////////////////////////////////////////////////////////////////
// reflect type definitions
//////////////////////////////////////////////////////////////////////////////////////////

typedef ::reflect::details::Name                         NAME;

typedef Attribute                                                 ATTRIBUTE;
typedef ::reflect::details::ConstPointerArray<ATTRIBUTE> ATTRIBUTEARRAY;

//////////////////////////////////////////////////////////////////////////////////////////
// Type Export Helpers
//////////////////////////////////////////////////////////////////////////////////////////

//////////////////////////////////////////////////////////////////////////////////////////
// reflect::RADREFLECT_TYPE_EXPORT_FN
//////////////////////////////////////////////////////////////////////////////////////////

typedef const Class* (RAD_STDCALL *RADREFLECT_TYPE_EXPORT_FN) (int num);

#define RADREFLECT_BEGIN_TYPES_EXPORT\
	extern "C" RAD_SYMEXPORT const reflect::Class* RAD_STDCALL __RAD_ReflectedTypesExport(int num) {\
	static const reflect::Class *s_types[] = {
#define RADREFLECT_EXPORT_TYPE(_class_) (::reflect::Type<_class_>()),
#define RADREFLECT_END_TYPES_EXPORT 0}; return s_types[num]; }

} // reflect

#if !defined(RAD_OPT_NO_REFLECTION)

//////////////////////////////////////////////////////////////////////////////////////////
// Reflected type declarations
//////////////////////////////////////////////////////////////////////////////////////////

RADREFLECT_DECLARE(RADRT_API, void)
RADREFLECT_DECLARE(RADRT_API, S8)
RADREFLECT_DECLARE(RADRT_API, S8*)
RADREFLECT_DECLARE(RADRT_API, U8)
RADREFLECT_DECLARE(RADRT_API, U8*)
RADREFLECT_DECLARE(RADRT_API, S16)
RADREFLECT_DECLARE(RADRT_API, S16*)
RADREFLECT_DECLARE(RADRT_API, U16)
RADREFLECT_DECLARE(RADRT_API, U16*)
RADREFLECT_DECLARE(RADRT_API, S32)
RADREFLECT_DECLARE(RADRT_API, S32*)
RADREFLECT_DECLARE(RADRT_API, U32)
RADREFLECT_DECLARE(RADRT_API, U32*)
RADREFLECT_DECLARE(RADRT_API, S64)
RADREFLECT_DECLARE(RADRT_API, S64*)
RADREFLECT_DECLARE(RADRT_API, U64)
RADREFLECT_DECLARE(RADRT_API, U64*)
RADREFLECT_DECLARE(RADRT_API, F32)
RADREFLECT_DECLARE(RADRT_API, F32*)
RADREFLECT_DECLARE(RADRT_API, F64)
RADREFLECT_DECLARE(RADRT_API, F64*)
RADREFLECT_DECLARE(RADRT_API, bool);
RADREFLECT_DECLARE(RADRT_API, bool*);
RADREFLECT_DECLARE(RADRT_API, char*)
RADREFLECT_DECLARE(RADRT_API, wchar_t);
RADREFLECT_DECLARE(RADRT_API, wchar_t*)
RADREFLECT_DECLARE(RADRT_API, const char*)
RADREFLECT_DECLARE(RADRT_API, const wchar_t*)
RADREFLECT_DECLARE(RADRT_API, void*)
RADREFLECT_DECLARE(RADRT_API, const void*)
RADREFLECT_DECLARE(RADRT_API, string::string<>)
RADREFLECT_DECLARE(RADRT_API, string::wstring<>)
RADREFLECT_DECLARE(RADRT_API, std::string)
RADREFLECT_DECLARE(RADRT_API, std::wstring)

#endif // !defined(RAD_OPT_NO_REFLECTION)

#include "../PopPack.h"
