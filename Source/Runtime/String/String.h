// String.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntString.h"
#include "StringDef.h"
#include "StringDetails.h"
#include <string>
#include <stdarg.h>

#include "../PushPack.h"

namespace string {

//! Reference Based String Class.
class String {
public:
	typedef void (*unspecified_bool_type) ();

	String();
	String(const String &s);
	explicit String(const char *sz);
	explicit String(const wchar_t *sz);
	String(const char *sz, int len);
	String(const wchar_t *sz, int len);

	template <typename A>
	explicit String(const std::basic_string<char, std::char_traits<char>, A> &str);
	template <typename A>
	explicit String(const std::basic_stringstream<char, std::char_traits<char>, OtherA> &str);  
	
	template <typename A>
	explicit String(const std::basic_string<wchar_t, std::char_traits<wchar_t>, A> &str);
	template <typename A>
	explicit String(const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &str);  
	
	int Compare(const String &str) const;
	int Compare(const char *sz) const;
	int Compare(const wchar_t *sz) const;

	int NCompare(const String &string) const;
	int NCompare(const char *sz) const;
	int NCompare(const wchar_t *sz) const;

	int Strstr(const String &str) const;
	int Strstr(const char *sz) const;
	int Strstr(const wchar_t *sz) const;

	String Substr(int first, int count) const;
	String Left(int count) const;
	String Right(int count) const;
	String TrimLeft(int count) const;
	String TrimRight(int count) const;

	String Upper() const;
	String Lower() const;
	String Reverse() const;

	String Join(const String &str) const;
	String Join(const char *sz) const;
	String Join(const wchar_t *sz) const;
	String Join(const char c) const;
	String Join(const wchar_t c) const;

	String &Append(const String &str);
	String &Append(const char *sz);
	String &Append(const wchar_t *sz);
	String &Append(const char c);
	String &Append(const wchar_t c);

	unspecified_bool_type operator == (const String &str) const;
	unspecified_bool_type operator == (const char *sz) const;
	unspecified_bool_type operator == (const wchar_t *sz) const;

	template <typename A>
	unspecified_bool_type operator == (const std::basic_string<wchar_t, std::char_traits<wchar_t>, A> &str);
	template <typename A>
	unspecified_bool_type operator == (const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &str);  
	
	unspecified_bool_type operator != (const String &str) const;
	unspecified_bool_type operator != (const char *sz) const;
	unspecified_bool_type operator != (const wchar_t *sz) const;

	template <typename A>
	unspecified_bool_type operator != (const std::basic_string<wchar_t, std::char_traits<wchar_t>, A> &str);
	template <typename A>
	unspecified_bool_type operator != (const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &str);  


};

} // string


#include "../PopPack.h"
