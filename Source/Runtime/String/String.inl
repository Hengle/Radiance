// String.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Mike Songy & Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../StringBase.h"
#include "../PushPack.h"


namespace string {

////////////////////////////////////////////////////////////////////////////////
// string::string::string_base explicit instantiations
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const typename string_base<T, S, A>::size_type string_base<T, S, A>::npos = string_base<T, S, A>::basic_string_type::npos;

template<typename A>
const typename string<A>::size_type string<A>::npos = string<A>::string_base_type::npos;

template<typename A>
const typename wstring<A>::size_type wstring<A>::npos = wstring<A>::string_base_type::npos;

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::string_base()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
inline string_base<T, S, A>::string_base() :
std::basic_string<T, traits_type, A>()
{
}

// Constructors that convert allocator types

template<typename T, typename S, typename A>
template<typename OtherA>
inline string_base<T, S, A>::string_base(const string_base<T, S, OtherA>& string) :
std::basic_string<T, traits_type, A>(string.c_str())
{
}

template<typename T, typename S, typename A>
template<typename OtherA>
inline string_base<T, S, A>::string_base(const std::basic_string<T, std::char_traits<T>, OtherA> &string) :
std::basic_string<T, traits_type, A>(string.c_str())
{
}

template<typename T, typename S, typename A>
template<typename OtherA>
inline string_base<T, S, A>::string_base(const std::basic_stringstream<T, std::char_traits<T>, OtherA> &ss) :
std::basic_string<T, traits_type, A>(ss.str().c_str())
{
}

template<typename T, typename S, typename A>
string_base<T, S, A>::string_base(T ch, size_type nRepeat) :
std::basic_string<T, traits_type, A>()
{
	if (nRepeat >= 1)
	{
		reserve(nRepeat);
		for (size_type i = 0; i < nRepeat; i++)
		{
			*this += ch;
		}
	}
}

template<typename T, typename S, typename A>
inline string_base<T, S, A>::string_base(const T *psz) :
std::basic_string<T, traits_type, A>(psz)
{
}

template<typename T, typename S, typename A>
inline string_base<T, S, A>::string_base(const T *pch, size_type nLength) :
std::basic_string<T, traits_type, A>(pch, nLength)
{
}

template<typename T, typename S, typename A>
inline string_base<T, S, A>::string_base(T *psz, size_type len, typename string_base<T, S, A>::CON copydel, A& allocator) :
std::basic_string<T, traits_type, A>(psz)
{
	RAD_ASSERT(psz);
	switch(copydel)
	{
	case COPYDEL:
		if (len)
		{
			allocator.deallocate(psz, len);
		}
		break;
	}
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::~string_base()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
string_base<T, S, A>::~string_base()
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::allocate()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
inline T* string_base<T, S, A>::allocate(size_type len) const
{
	return this->get_allocator().allocate(len);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::deallocate()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
inline void string_base<T, S, A>::deallocate(T* ptr, size_type len) const
{
	this->get_allocator().deallocate(ptr, len);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::operator=()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
template<typename OtherA>
inline string_base<T, S, A>& string_base<T, S, A>::operator=(const string_base<T, S, OtherA> &str)
{
	*static_cast<std::basic_string<T, traits_type, A>*>(this) = str.c_str();
	return *this;
}

template<typename T, typename S, typename A>
template<typename OtherA>
inline string_base<T, S, A> &string_base<T, S, A>::operator=(const std::basic_string<T, std::char_traits<T>, OtherA> &str)
{
	*static_cast<std::basic_string<T, traits_type, A>*>(this) = str.c_str();
	return *this;
}

template<typename T, typename S, typename A>
template<typename OtherA>
inline string_base<T, S, A> &string_base<T, S, A>::operator=(const std::basic_stringstream<T, std::char_traits<T>, OtherA> &ss)
{
	*static_cast<std::basic_string<T, traits_type, A>*>(this) = ss.str().c_str();
	return *this;
}

template<typename T, typename S, typename A>
inline string_base<T, S, A>& string_base<T, S, A>::operator=(const T *psz)
{
  *static_cast<std::basic_string<T, traits_type, A>*>(this) = psz;
	return *this;
}

template<typename T, typename S, typename A>
inline string_base<T, S, A>& string_base<T, S, A>::operator=(T ch)
{
  *static_cast<std::basic_string<T, traits_type, A>*>(this) = ch;
  return *this;
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::operator==()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
template<typename OtherA>
inline bool string_base<T, S, A>::operator==(const string_base<T, S, OtherA>& string) const
{
	return (0 == compare(string.c_str()));
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::compare_no_case()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
inline int string_base<T, S, A>::compare_no_case(const T *psz) const
{
	return icmp<S>(reinterpret_cast<const S *>(this->c_str()), reinterpret_cast<const S *>(psz));
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::collate()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
int string_base<T, S, A>::collate(const T *psz) const
{
	return coll<S>(reinterpret_cast<const S *>(this->c_str()), reinterpret_cast<const S *>(psz));
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::contains()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
int string_base<T, S, A>::contains(const T *sz) const
{
	const T *x = this->c_str();
	const T *p = strstr<T>(x, sz);
	return p ? (int)(p-x) : -1;
}

template<typename T, typename S, typename A>
int string_base<T, S, A>::contains(const string_base_type &str) const
{
	return contains(str.c_str());
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::format()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
void string_base<T, S, A>::format(const T *szFormat, ...)
{
	va_list args;
	va_start(args, szFormat);
	format_argv(szFormat, args);
	va_end(args);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::format_argv()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
void string_base<T, S, A>::format_argv(const T *szFormat, va_list args)
{
	size_type len = vscprintf(szFormat, args)+1;
	T* buff = this->allocate(len);
	vsprintf(buff, szFormat, args);
	string_base<T, S, A>::operator=(buff);
	this->deallocate(buff, len);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::substring()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::substring(size_type &nRLen, size_type nFirst, size_type nCount) const
{
	RAD_ASSERT(nCount >= 0);
	RAD_ASSERT(nCount <= this->length());
	RAD_ASSERT(nFirst >= 0);
	RAD_ASSERT((this->empty() && nFirst == 0) || (nFirst <= this->length()));

	const T	*p = this->c_str();

	size_type len = this->length();
	if (nFirst + nCount > len)
	{
		nCount = len - nFirst;
	}

	if (nCount && p)
	{
		nRLen = nCount;
		return (p + nFirst);
	}
	else
	{
		nRLen = 0;
		return NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::substr()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::substr(size_type &nRLen, size_type nFirst, size_type nCount) const
{
	size_type len = this->length();

	nFirst = Clamp(nFirst, size_type(0), len);
	nCount = Clamp(nCount, size_type(0), len - nFirst);

	return substring(nRLen, nFirst, nCount);
}

template<typename T, typename S, typename A>
inline const T *string_base<T, S, A>::mid(size_type &nRLen, size_type nFirst) const
{
	size_type len = this->length();

	nFirst = Clamp(nFirst, size_type(0), len);

	return substring(nRLen, nFirst, len - nFirst);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::left()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::left(size_type &nRLen, size_type nCount) const
{
	size_type len = this->length();
	nCount = Clamp(nCount, size_type(0), len);
	return substring(nRLen, 0, nCount);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::right()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::right(size_type &nRLen, size_type nCount) const
{
	size_type len = this->length();
	nCount = Clamp(nCount, size_type(0), len);
	return substring(nRLen, len - nCount, nCount);
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::span_including()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::span_including(size_type &nRLen, const T *pszCharSet) const
{
	return left(nRLen, spn<S>(reinterpret_cast<const S *>(this->c_str()), reinterpret_cast<const S *>(pszCharSet)));
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::span_excluding()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::span_excluding(size_type &nRLen, const T *pszCharSet) const
{
	return left(nRLen, cspn<S>(reinterpret_cast<const S *>(this->c_str()), reinterpret_cast<const S *>(pszCharSet)));
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::trim_right()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::trim_right(size_type &nRLen) const
{
	size_type len = this->length();
	const T	*buffer = this->c_str();

	if (len && buffer)
	{
		const T *psz = buffer,	*pszLast = NULL;

		// Find beginning of trailing spaces by starting at beginning

		while (*psz != static_cast<T>('\0'))
		{
			if (isspace<S>(*reinterpret_cast<const S *>(psz)))
			{
				if (!pszLast) { pszLast = psz; }
			}
			else
			{
				pszLast = NULL;
			}
			++psz;
		}

		// truncate at trailing space start

		nRLen = pszLast ? pszLast - buffer : len;
		return buffer;
	}
	else
	{
		nRLen = 0;
		return NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::trim_left()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
const T *string_base<T, S, A>::trim_left(size_type &nRLen) const
{
	size_type len = this->length();

	const T	*buffer = this->c_str();

	if (len && buffer)
	{
		const T *psz = buffer;

		// Find first non-space character

		while (isspace<S>(*reinterpret_cast<const S *>(psz)))
		{
			++psz;
		}

		// Get data and length

		nRLen = len - (psz - buffer);
		return psz;
	}
	else
	{
		nRLen = 0;
		return NULL;
	}
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::upper()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
T *string_base<T, S, A>::upper() const
{
	size_type len = this->length();

	if (len > 0)
	{
		T	*buffer = this->allocate(len + 1);

		if (buffer)
		{
			copy(buffer, len);
			buffer[len] = static_cast<T>('\0');
			toupper(reinterpret_cast<S *>(buffer));
			return buffer;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::lower()
////////////////////////////////////////////////////////////////////////////////

template<typename T, typename S, typename A>
T *string_base<T, S, A>::lower() const
{
	size_type	len = this->length();

	if (len > 0)
	{
		T	*buffer = this->allocate(len + 1);

		if (buffer)
		{
			copy(buffer, len);
			buffer[len] = static_cast<T>('\0');
			tolower(reinterpret_cast<S *>(buffer));
			return buffer;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// string::string_base<T, S, A>::reverse()
//////////////////////////////////////////////////////////////////////w//////////

template<typename T, typename S, typename A>
T *string_base<T, S, A>::reverse() const
{
	size_type len = this->length();

	if (len > 0)
	{
		T *buffer = this->allocate(len + 1);

		if (buffer)
		{
			copy(buffer, len);
			buffer[len] = static_cast<T>('\0');
			::string::reverse(reinterpret_cast<S *>(buffer));
			return buffer;
		}
	}

	return NULL;
}

////////////////////////////////////////////////////////////////////////////////
// string::string inline function definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A>::string() :
string_base<char, char, A>()
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline string<A>::string(const string<OtherA> &str) :
string_base<char, char, A>(str)
{
}

///////////////////////////////////////////////////////////////////////////////
// string::string()
///////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline string<A>::string(const wstring<OtherA> &str) :
string_base<char, char, A>()
{
#define CAWARN_DISABLE 6309 6387
#include <Runtime/PushCAWarnings.h>
	size_t size = ::wcstombs(NULL, str.c_str(), str.length() * 2 + 1) + 1;
#include <Runtime/PopCAWarnings.h>
	char *buff = this->allocate(size);
	::wcstombs(buff, str.c_str(), size);
	string_base<char, char, A>::operator=(buff);
	this->deallocate(buff, size);
}

template<typename A>
template<typename OtherA>
inline string<A>::string(const std::basic_string<wchar_t, std::char_traits<wchar_t>, OtherA> &str) :
string_base<char, char, A>()
{
#define CAWARN_DISABLE 6309 6387
#include <Runtime/PushCAWarnings.h>
	size_t size = ::wcstombs(NULL, str.c_str(), str.length() * 2 + 1) + 1;
#include <Runtime/PopCAWarnings.h>
	char *buff = this->allocate(size);
	::wcstombs(buff, str.c_str(), size);
	string_base<char, char, A>::operator=(buff);
	this->deallocate(buff, size);
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline string<A>::string(const std::basic_string<char, std::char_traits<char>, OtherA> &str) :
string_base<char, char, A>(str)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline string<A>::string(const std::basic_stringstream<char, std::char_traits<char>, OtherA> &ss) :
string_base<char, char, A>(ss)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A>::string(char ch, size_type nRepeat) :
string_base<char, char, A>(ch, nRepeat)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A>::string(const char *psz) :
string_base<char, char, A>(psz)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A>::string(const char *psz, size_type nLength) :
string_base<char, char, A>(psz, nLength)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A>::string(char *psz, size_type len, CON copydel, A& allocator) :
string_base<char, char, A>(psz, len, copydel, allocator)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::~string()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A>::~string()
{
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::operator=()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline string<A>& string<A>::operator=(const string<OtherA>& str)
{
	string_base<char, char, A>::operator=(str);
	return *this;
}

template<typename A>
template<typename OtherA>
inline string<A> &string<A>::operator=(const std::basic_string<char, std::char_traits<char>, OtherA> &str)
{
	string_base<char, char, A>::operator=(str);
	return *this;
}

template<typename A>
template<typename OtherA>
inline string<A> &string<A>::operator=(const std::basic_stringstream<char, std::char_traits<char>, OtherA> &ss)
{
	string_base<char, char, A>::operator=(ss);
	return *this;
}

template<typename A>
inline string<A>& string<A>::operator=(char ch)
{
	string_base<char, char, A>::operator=(ch);
	return *this;
}

template<typename A>
inline string<A>& string<A>::operator=(const char *psz)
{
	string_base<char, char, A>::operator=(psz);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::substr()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::substr(size_type nFirst, size_type nCount) const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::substr(n, nFirst, nCount);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::mid()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::mid(size_type nFirst) const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::mid(n, nFirst);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::left()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::left(size_type nCount) const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::left(n, nCount);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::right()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::right(size_type nCount) const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::right(n, nCount);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string:string<A>:::span_including()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::span_including(const char *pszCharSet) const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::span_including(n, pszCharSet);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::span_excluding()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::span_excluding(const char *pszCharSet) const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::span_excluding(n, pszCharSet);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::trim_right()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::trim_right() const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::trim_right(n);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::trim_left()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::trim_left() const
{
	size_type n = 0;
	const char *p = string_base<char, char, A>::trim_left(n);
	return ((p && n) ? string<A>(p, n) : string());
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::upper()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::upper() const
{
	char *p = string_base<char, char, A>::upper();
	A a = this->get_allocator();
	return (p) ? string<A>(p, this->length(), COPYDEL, a) : string<A>();
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::lower()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::lower() const
{
	char *p = string_base<char, char, A>::lower();
	A a = this->get_allocator();
	return (p) ? string<A>(p, this->length(), COPYDEL, a) : string<A>();
}

////////////////////////////////////////////////////////////////////////////////
// string::string<A>::reverse()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> string<A>::reverse() const
{
	char *p = string_base<char, char, A>::reverse();
	A a = this->get_allocator();
	return (p) ? string<A>(p, this->length(), COPYDEL, a) : string<A>();
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A, typename OtherA>
inline string<A> operator+(const string<A> &l, const string<OtherA> &r)
{
	string<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> operator+(const string<A> &l, const char *r)
{
	string<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> operator+(const string<A> &l, char r)
{
	string<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> operator+(const char *l, const string<A> &r)
{
	string<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline string<A> operator+(char l, const string<A> &r)
{
	string<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring inline function definitions
////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A>::wstring() :
string_base<wchar_t, wchar_t, A>()
{
}

///////////////////////////////////////////////////////////////////////////////
// wstring::wstring()
///////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline wstring<A>::wstring(const string<OtherA> &str) :
string_base<wchar_t, wchar_t, A>()
{
#define CAWARN_DISABLE 6309 6387
#include <Runtime/PushCAWarnings.h>
	size_t size = ::mbstowcs(NULL, str.c_str(), str.length() + 1) + 1;
#include <Runtime/PopCAWarnings.h>
	wchar_t* buff = this->allocate(size);
	::mbstowcs(buff, str.c_str(), size);
	string_base<wchar_t, wchar_t, A>::operator=(buff);
	this->deallocate(buff, size);
}

template<typename A>
template<typename OtherA>
inline wstring<A>::wstring(const std::basic_string<char, std::char_traits<char>, OtherA> &str) :
string_base<wchar_t, wchar_t, A>()
{
	size_t size = ::mbstowcs(NULL, str.c_str(), str.length() + 1) + 1;
	wchar_t* buff = this->allocate(size);
	::mbstowcs(buff, str.c_str(), size);
	string_base<wchar_t, wchar_t, A>::operator=(buff);
	this->deallocate(buff, size);
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline wstring<A>::wstring(const wstring<OtherA> &string) :
string_base<wchar_t, wchar_t, A>(string)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline wstring<A>::wstring(const std::basic_string<wchar_t, std::char_traits<wchar_t>, OtherA> &str) :
string_base<wchar_t, wchar_t, A>(str)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline wstring<A>::wstring(const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &ss) :
string_base<wchar_t, wchar_t, A>(ss)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A>::wstring(wchar_t ch, size_type nRepeat) :
string_base<wchar_t, wchar_t, A>(ch, nRepeat)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A>::wstring(const wchar_t *psz) :
string_base<wchar_t, wchar_t, A>(psz)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A>::wstring(const wchar_t *psz, size_type nLength) :
string_base<wchar_t, wchar_t, A>(psz, nLength)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A>::wstring(wchar_t *psz, size_type len, CON copydel, A& allocator) :
string_base<wchar_t, wchar_t, A>(psz, len, copydel, allocator)
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::~wstring()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A>::~wstring()
{
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::operator=()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
template<typename OtherA>
inline wstring<A>& wstring<A>::operator=(const wstring<OtherA>& string)
{
	string_base<wchar_t, wchar_t, A>::operator=(string);
	return *this;
}

template<typename A>
template<typename OtherA>
inline wstring<A> &wstring<A>::operator=(const std::basic_string<wchar_t, std::char_traits<wchar_t>, OtherA> &str)
{
	string_base<wchar_t, wchar_t, A>::operator=(str);
	return *this;
}

template<typename A>
template<typename OtherA>
inline wstring<A> &wstring<A>::operator=(const std::basic_stringstream<wchar_t, std::char_traits<wchar_t>, OtherA> &ss)
{
	string_base<wchar_t, wchar_t, A>::operator=(ss);
	return *this;
}

template<typename A>
inline wstring<A>& wstring<A>::operator=(wchar_t ch)
{
	string_base<wchar_t, wchar_t, A>::operator=(ch);
	return *this;
}

template<typename A>
inline wstring<A>& wstring<A>::operator=(const wchar_t *psz)
{
	string_base<wchar_t, wchar_t, A>::operator=(psz);
	return *this;
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::substr()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::substr(size_type nFirst, size_type nCount) const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::substr(n, nFirst, nCount);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::mid()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::mid(size_type nFirst) const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::mid(n, nFirst);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::left()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::left(size_type nCount) const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::left(n, nCount);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::right()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::right(size_type nCount) const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::right(n, nCount);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::span_including()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::span_including(const wchar_t *pszCharSet) const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::span_including(n, pszCharSet);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::span_excluding()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::span_excluding(const wchar_t *pszCharSet) const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::span_excluding(n, pszCharSet);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::trim_right()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::trim_right() const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::trim_right(n);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::trim_left()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::trim_left() const
{
	size_type n = 0;
	const wchar_t *p = string_base<wchar_t, wchar_t, A>::trim_left(n);
	return ((p && n) ? wstring<A>(p, n) : wstring<A>());
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::upper()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::upper() const
{
	wchar_t *p = string_base<wchar_t, wchar_t, A>::upper();
	A a = this->get_allocator();
	return (p) ? wstring<A>(p, this->length(), COPYDEL, a) : wstring<A>();
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::lower()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::lower() const
{
	wchar_t *p = string_base<wchar_t, wchar_t, A>::lower();
	A a = this->get_allocator();
	return (p) ? wstring<A>(p, this->length(), COPYDEL, a) : wstring<A>();
}

////////////////////////////////////////////////////////////////////////////////
// string::wstring<A>::reverse()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> wstring<A>::reverse() const
{
	wchar_t *p = string_base<wchar_t, wchar_t, A>::reverse();
	A a = this->get_allocator();
	return (p) ? wstring<A>(p, this->length(), COPYDEL, a) : wstring<A>();
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> operator+(const wstring<A> &l, const wstring<A> &r)
{
	wstring<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> operator+(const wstring<A> &l, const wchar_t *r)
{
	wstring<A> res(l);
	res += r;
	return res;
}
////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> operator+(const wstring<A> &l, wchar_t r)
{
	wstring<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> operator+(const wchar_t *l, const wstring<A> &r)
{
	wstring<A> res(l);
	res += r;
	return res;
}

////////////////////////////////////////////////////////////////////////////////
// string::operator+()
////////////////////////////////////////////////////////////////////////////////

template<typename A>
inline wstring<A> operator+(wchar_t l, const wstring<A> &r)
{
	wstring<A> res(l);
	res += r;
	return res;
}

inline String Shorten(const wchar_t *str)
{
	WString x(str ? str : L"");
	return String(x);
}

inline WString Widen(const char *str)
{
	String x(str ? str : "");
	return WString(x);
}

} // string


#include "../PopPack.h"
