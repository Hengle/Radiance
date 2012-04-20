// String.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntString.h"
#include "StringDef.h"
#include <string>
#include <stdarg.h>
#include "../PushPack.h"


namespace string {

RAD_ZONE_DEC(RADRT_API, ZString);

//////////////////////////////////////////////////////////////////////////////////////////
// string::string_base
//////////////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
class string_base :
public std::basic_string<T, std::char_traits<T>, A>
{
public:

	typedef string_base<T, S, A> string_base_type;
	typedef std::basic_string<T, std::char_traits<T>, A> basic_string_type;
	typedef typename basic_string_type::size_type size_type;
	typedef typename basic_string_type::traits_type traits_type;

	static const size_type npos;

	// Constructors can convert allocator types

	string_base();
	template<typename OtherA>
	string_base(const string_base<T, S, OtherA> &str);
	template<typename OtherA>
	explicit string_base(const std::basic_string<T, std::char_traits<T>, OtherA> &str);
	template<typename OtherA>
	explicit string_base(const std::basic_stringstream<T, std::char_traits<T>, OtherA> &ss);
	string_base(T ch, size_type repeat = 1);
	string_base(const T *sz);
	string_base(const T *sz, size_type len);

	// Destructor

	~string_base();

   // Comparison

	template<typename OtherA>
	bool operator==(const string_base<T, S, OtherA>& str) const;

	int compare_no_case(const T *sz) const;
	int collate(const T *sz) const;

	//! Finds the index of the first occurance of a string.
	/*! Finds the index of the first occurance of a string.
	**  \returns the index of the first occurance of the string or -1 if no substring exists.
	*/
	int contains(const T *sz) const;
	int contains(const string_base_type &str) const;

	// Format

	void format(const T *fmt, ...);
	void format_argv(const T *fmt, va_list args);

	// allocation (allocates from allocator).
	// this has nothing to do with what the string contains, it is just
	// new memory from the allocator.
	T* allocate(size_type len) const;
	void deallocate(T* ptr, size_type len) const;

protected:

	// Assignment

	template<typename OtherA>
	string_base<T, S, A>& operator=(const string_base<T, S, OtherA> &str);
	template<typename OtherA>
	string_base<T, S, A> &operator=(const std::basic_string<T, std::char_traits<T>, OtherA> &str);
	template<typename OtherA>
	string_base<T, S, A> &operator=(const std::basic_stringstream<T, std::char_traits<T>, OtherA> &ss);

	string_base<T, S, A>& operator=(const T *sz);
	string_base<T, S, A>& operator=(T ch);

   // Substring Extraction

	const T *substr(size_type &len, size_type first = 0, size_type count = npos) const;
	const T *mid(size_type &len, size_type first = 0) const;
	const T *left(size_type &len, size_type count = npos) const;
	const T *right(size_type &len, size_type count = npos) const;
	const T *span_including(size_type &len, const T *charSet) const;
	const T *span_excluding(size_type &len, const T *charSet) const;
	const T *trim_right(size_type &len) const;
	const T *trim_left(size_type &len) const;

	// The following routines should all use this constructor, which deletes
	// the pointer that was passed into it once it uses it.  This is done to
	// reduce the amount of copying that's going on.

	typedef int CON;

	enum
	{ 
		COPYDEL
	};

	string_base(T *sz, size_type len, CON copydel, A& allocator);

	// Case conversion

	T *upper() const;
	T *lower() const;
	T *reverse() const;

private:

	const T *substring(size_type &len, size_type first, size_type count) const;
};

//////////////////////////////////////////////////////////////////////////////////////////
// string::string
//////////////////////////////////////////////////////////////////////////////////////////

template<typename A = zone_allocator<char, ZStringT> > // allocator type
class string :
public string_base<char, char, A>
{
public:

	typedef string_base<char, char, A> string_base_type;
	typedef typename string_base_type::size_type size_type;
	typedef typename string_base_type::traits_type traits_type;

	static const size_type npos;

	// Constructors can convert allocator types

	string();
	template<typename OtherA>
	string(const string<OtherA> &str);
	template<typename OtherA>
	explicit string(const std::basic_string<char, std::char_traits<char>, OtherA> &str);
	template<typename OtherA>
	explicit string(const std::basic_stringstream<char, std::char_traits<char>, OtherA> &str);  
	string(char ch, size_type repeat = 1);
	explicit string(const char *sz);
	string(const char *sz, size_type len);
	template<typename OtherA>
	explicit string(const wstring<OtherA> &str); 
	template<typename OtherA>
	explicit string(const std::basic_string<wchar_t, std::char_traits<wchar_t>, OtherA> &str); 

	// Destructor

	~string();

	// Assignment

	template<typename OtherA>
	string<A>& operator=(const string<OtherA>& str);
	template<typename OtherA>
	string<A> &operator=(const std::basic_string<char, std::char_traits<char>, OtherA> &str);
	template<typename OtherA>
	string<A> &operator=(const std::basic_stringstream<char, std::char_traits<char>, OtherA> &ss);
	string<A>& operator=(const char *sz);
	string<A>& operator=(char ch);

	// Substring Extraction

	string<A> substr(size_type first = 0, size_type count = npos) const;
	string<A> mid(size_type first = 0) const;
	string<A> left(size_type count = npos) const;
	string<A> right(size_type count = npos) const;
	string<A> span_including(const char *charSet) const;
	string<A> span_excluding(const char *charSet) const;
	string<A> trim_right() const;
	string<A> trim_left() const;

	// Case conversion

	string<A> upper() const;
	string<A> lower() const;
	string<A> reverse() const;

protected:

	typedef int CON;

	enum
	{ 
		COPYDEL = string_base_type::COPYDEL
	};

private:

	string(char *sz, size_type len, CON copydel, A& allocator);
};

template<typename A, typename OtherA>
string<A> operator+(const string<A> &l, const string<OtherA> &r);
template<typename A>
string<A> operator+(const string<A> &l, const char *r);
template<typename A>
string<A> operator+(const string<A> &l, char r);
template<typename A>
string<A> operator+(const char *l, const string<A> &r);
template<typename A>
string<A> operator+(char l, const string<A> &r);

//////////////////////////////////////////////////////////////////////////////////////////
// string::wstring
//////////////////////////////////////////////////////////////////////////////////////////

template<typename A = zone_allocator<wchar_t, ZStringT> > // allocator type
class wstring :
public string_base<wchar_t, wchar_t, A>
{

public:

	typedef string_base<wchar_t, wchar_t, A> string_base_type;
	typedef typename string_base_type::size_type size_type;
	typedef typename string_base_type::traits_type traits_type;

	static const size_type npos;

	// Constructors

	wstring();
	template<typename OtherA>
	wstring(const wstring<OtherA> &str);
	template<typename OtherA>
	explicit wstring(const std::basic_string<wchar_t, std::char_traits<wchar_t>, OtherA> &str);
	template<typename OtherA>
	explicit wstring(const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &ss);
	wstring(wchar_t ch, size_type repeat = 1);
	explicit wstring(const wchar_t *sz);
	wstring(const wchar_t *sz, size_type len);
	template<typename OtherA>
	explicit wstring(const string<OtherA> &str);
	template<typename OtherA>
	explicit wstring(const std::basic_string<char, std::char_traits<char>, OtherA> &str); 

	// Destructor

	~wstring();

	// Assignment

	template <typename OtherA>
	wstring<A>& operator=(const wstring<OtherA>& str);
	template<typename OtherA>
	wstring<A> &operator=(const std::basic_string<wchar_t, std::char_traits<wchar_t>, OtherA> &str);
	template<typename OtherA>
	wstring<A> &operator=(const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &ss);
	wstring<A>& operator=(const wchar_t *sz);
	wstring<A>& operator=(wchar_t ch);

	// Substring Extraction

	wstring<A> substr(size_type first = 0, size_type count = npos) const;
	wstring<A> mid(size_type first = 0) const;
	wstring<A> left(size_type count = npos) const;
	wstring<A> right(size_type count = npos) const;
	wstring<A> span_including(const wchar_t *charSet) const;
	wstring<A> span_excluding(const wchar_t *charSet) const;
	wstring<A> trim_right() const;
	wstring<A> trim_left() const;

	// Case conversion

	wstring<A> upper() const;
	wstring<A> lower() const;
	wstring<A> reverse() const;

protected:

	enum CON 
	{ 
		COPYDEL = string_base_type::COPYDEL
	};

private:

	wstring(wchar_t *sz, size_type len, CON copydel, A& allocator);
};

template<typename A>
wstring<A> operator+(const wstring<A> &l, const wstring<A> &r);
template<typename A>
wstring<A> operator+(const wstring<A> &l, const wchar_t *r);
template<typename A>
wstring<A> operator+(const wstring<A> &l, wchar_t r);
template<typename A>
wstring<A> operator+(const wchar_t *l, const wstring<A> &r);
template<typename A>
wstring<A> operator+(wchar_t l, const wstring<A> &r);

typedef string<> String;
typedef wstring<> WString;

String Shorten(const wchar_t *str);
WString Widen(const char *str);

#if defined(RAD_OPT_WCHAR_4)
inline U16 Shorten(wchar_t c)
{
	RAD_ASSERT(c<=0xffff);
	return (U16)(c&0xffff);
}
inline wchar_t Widen(U16 c)
{
	return (wchar_t)c;
}
#else
inline U16 Shorten(wchar_t c)
{
	return (U16)c;
}
inline wchar_t Widen(U16 c)
{
	return (wchar_t)c;
}
#endif

} // string

#include "../PopPack.h"
#include "String.inl"
