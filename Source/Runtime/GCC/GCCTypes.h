// GCCTypes.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <stdint.h>
#include <exception>
#include <string>
#include "../PushPack.h"

#define RAD_NEWLINE "\r\n"

typedef int8_t                     S8,  *PS8;
typedef int16_t                    S16, *PS16;
typedef int32_t                    S32, *PS32;
typedef int64_t                    S64, *PS64;

typedef uint8_t                    U8,  *PU8;
typedef uint16_t                   U16, *PU16;
typedef uint32_t                   U32, *PU32;
typedef uint64_t                   U64, *PU64;

typedef float                      F32, *PF32;
typedef double                     F64, *PF64;

#if RAD_OPT_MACHINE_WORD_SIZE == 4

	typedef U32                        AddrSize; // Can always hold a void*.
	typedef S32                        SAddrSize; // signed version!

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

	#error RAD_ERROR_UNSUP_PLAT

#endif

class RADRT_CLASS exception : public std::exception
{
public:

	exception() throw() : m_type("exception"), m_code(0), m_what(0) {}
	exception(const exception &e) : std::exception(e), m_type("exception"), m_code(e.m_code), m_what(0)
	{
		if (!e.m_noalloc)
		{
			copy(e.m_what);
		}
		else
		{
			m_what = e.m_what;
			m_noalloc = 1;
		}
	}

	explicit exception(int code) throw() : m_type("exception"), m_code(code), m_what(0) {}
	explicit exception(const char *const &s) throw() : m_type("exception"), m_code(0) { copy(s); }
	explicit exception(const char *const &s, int noalloc) throw() : m_type("exception"), m_code(0) { copy(s); }
	explicit exception(int code, const char *const &s) throw() :m_type("exception"),  m_code(code) { copy(s); }
	explicit exception(int code, const char *const &s, int noalloc) throw() : m_type("exception"), m_code(code), m_what((char*)s), m_noalloc(1) {}
	explicit exception(int code, const exception& e) throw() : std::exception(e), m_type("exception"), m_code(code), m_noalloc(0) { copy(e.what()); }

	virtual ~exception() throw() { if (!m_noalloc && m_what) { free(m_what); } }

	exception &operator=(const exception &e) { std::exception::operator=(e); return *this; }

	int code() const { return m_code; }
	const char* what() const throw() { return m_what; }
	const char *type() const throw() { return m_type.c_str(); }

protected:

	struct tag {};

	exception(int code, const char *const &s, int noalloc, const char *type, const tag&) throw() :
		m_what(0),
		m_code(code),
		m_type(type?type:"exception")
		{
			if (!noalloc)
			{
				copy(s);
			}
			else
			{
				m_what = (char*)s;
				m_noalloc = 1;
			}
		}

private:

	int m_code;
	int m_noalloc;
	char *m_what;
	std::string m_type;

	void copy(const char *const &s)
	{
		m_what = 0;
		m_noalloc = 0;
		if (s)
		{
			m_what = (char*)::malloc(::strlen(s)+1);
			strcpy(m_what, s);
		}
	}
};



#include "../PopPack.h"
