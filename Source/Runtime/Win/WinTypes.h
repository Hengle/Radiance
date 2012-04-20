// WinTypes.h
// Defines basic types for Windows/Xenon.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <exception>
#include <string>
#include "../PushPack.h"

#define RAD_NEWLINE "\n"

//////////////////////////////////////////////////////////////////////////////////////////
// Basic Types                                                                         
//////////////////////////////////////////////////////////////////////////////////////////

typedef signed __int8              S8,  *PS8;
typedef signed __int16             S16, *PS16;
typedef signed __int32             S32, *PS32;
typedef signed __int64             S64, *PS64;

typedef unsigned __int8            U8,  *PU8;
typedef unsigned __int16           U16, *PU16;
typedef unsigned __int32           U32, *PU32;
typedef unsigned __int64           U64, *PU64;

typedef float                      F32, *PF32;
typedef double                     F64, *PF64;

#if RAD_OPT_MACHINE_WORD_SIZE == 4

	typedef __w64 U32                  AddrSize; // Can always hold a void*.
	typedef __w64 S32                  SAddrSize; // signed version!       

	typedef S32                        SReg;   // Signed register size.
	typedef S16                        SHReg;  // Half a signed register size.
	typedef U32                        UReg;   // Unsigned register size.
	typedef U16                        UHReg;  // Half unsigned register size.
	typedef F32                        FReg;   // floating point register size.

#elif RAD_OPT_MACHINE_WORD_SIZE == 8

	typedef U64                        AddrSize; // Can always hold a void*.
	typedef S64                        SAddrSize; // signed version!

	typedef S64                        SReg;   // Signed register size.
	typedef S32                        SHReg;  // Half a signed register size.
	typedef U64                        UReg;   // Unsigned register size.
	typedef U32                        UHReg;  // Half unsigned register size.
	typedef F64                        FReg;   // floating point register size.

#else

	#error "unrecognized machine size!"

#endif

#pragma warning (push)
#pragma warning (disable:4275) // non dll-interface class 'std::exception' used as base for dll-interface class 'X'

class RADRT_CLASS exception : public std::exception
{
public:

	exception() : m_type("exception"), m_code(0) {}
	exception(const exception& e) : std::exception(e), m_type(e.m_type), m_code(e.m_code) {}
	explicit exception(int code) : m_type("exception"), m_code(code) {}
	explicit exception(const char *const &s) : std::exception(s), m_type("exception"), m_code(0) {}
	explicit exception(const char *const &s, int noalloc) : std::exception(s, noalloc), m_type("exception"), m_code(0) {}
	explicit exception(int code, const char *const &s) : std::exception(s), m_type("exception"), m_code(code) {}
	explicit exception(int code, const char *const &s, int noalloc) : std::exception(s, noalloc), m_type("exception"), m_code(code) {}
	explicit exception(int code, const exception& e) : std::exception(e), m_type("exception"), m_code(code) {}

	virtual ~exception() {}

	exception &operator=(const exception &e) { std::exception::operator=(e); return *this; }

	int code() const { return m_code; }
	const char *type() const { return m_type.c_str(); }

protected:

	struct tag {};

	exception(int code, const char *const &s, int noalloc, const char *type, const tag&) : 
		std::exception(s, noalloc), 
			m_code(code), 
			m_type(type?type:"exception") 
		{
		}


private:

	int m_code;
	std::string m_type;
};

#pragma warning(pop)

#include "../PopPack.h"