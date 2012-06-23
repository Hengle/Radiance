// String.h
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "StringDef.h"
#include "StringBase.h"
#include "StringDetails.h"
#include <string>
#include <stdarg.h>

#include "../PushPack.h"

namespace string {

//! Immutable Character Buffer.
/*! This class contains NULL terminated string data in various encodings. There
    are specializations of this class which are used for UTF8, UTF16, UTF32, and wide
    character formats.
 
    The various specializations of this type are used to return string data in
    specific encodings.
 
    Objects of this type are very lightweight and can be copied with almost no
    overhead. It is therefore unecessasry to contain this object in a shared_ptr.
 
    The data stored in this class is considered immutable and should never be changed
    by the user.
 */
template <typename Traits>
class CharBuf {
public:
	typedef CharBuf<Traits> SelfType;
	typedef Traits TraitsType;
	typedef typename Traits::T T;
	typedef void (SelfType::*unspecified_bool_type) ();
	typedef const T *const_iterator;

	CharBuf();
	CharBuf(const SelfType &buf);

	SelfType &operator = (const SelfType &buf);

	operator unspecified_bool_type () const;
	bool operator == (const SelfType &buf) const;
	bool operator != (const SelfType &buf) const;

	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, c_str, const T*);
	//! Defined as c_str
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, begin, const T*);
	//! Defined as c_str + size
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, end, const T*);
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, size, int);
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, empty, bool);

	//! Explicitly release memory.
	void free();

	//! Contructs a new character buffer. The specified data is copied into a new buffer.
	static SelfType create(const T *data, int size, const CopyTag_t&, ::Zone &zone = ZString);
	//! Constructs a new character buffer. The specified data is referenced directly.
	static SelfType create(const T *data, int size, const RefTag_t&, ::Zone &zone = ZString);

private:

	CharBuf(const details::DataBlock::Ref &data, ::Zone &zone);

	friend class String;

	void bool_true() {};

	RAD_DECLARE_GET(c_str, const T*);
	RAD_DECLARE_GET(begin, const T*);
	RAD_DECLARE_GET(end, const T*);
	RAD_DECLARE_GET(size, int);
	RAD_DECLARE_GET(empty, bool);

	details::DataBlock::Ref m_data;
	::Zone *m_zone;
};

//! UTF8 String class.
/*! This string type is reference based, internally immutable, and encoded as a utf8 string. This
    means it can hold ASCII data unmodified. It also means that assignment requires almost no
    overhead. Additionally mechanisms have been provided to construct a String object around
    constant data as an in-place reference allowing string data loaded on disk or compile time 
    static string data to be used without allocation overhead or copying.
 
    Care must be taken by the user when using certain types of operations on unicode strings.
    The upper() and lower() functions may not correctly handle all unicode code points
    depending on your locale and normalization issues. This class does not perform normalization.
    Be aware that normalization issues can also effect sorting. If asthetically correct sort order
    is important you must manually normalize the string before compare() will do what you want.
 
    Where appropriate considerably faster versions of certain functions have been provided that
    work on ASCII strings only (their use on strings containing unicode sequences is undefined).
 
    Converting to/from a UTF8Buf is free as it constructs a buffer around the underlying string
    data. However converting to any other representation is a relatively expensive operation.
    Constructing a UTF8Buf from a String is not free if the string in question was created
    using reference (RefTag) semantics.
 
    In general it is safe to assume that doing any operations on a string where the source
    data is not UTF8 encoded will require encoding the data to utf8 and possibly allocation
    overhead. upper(), lower(), and substr() all require transcoding to UTF32 and back to
    UTF8. Their ASCII equivelents do not.
 
    Mutable operations on a String object are done in-place if possible (i.e. there are no
    other references to the string data). Keep in mind that toUTF8() will usually return
    a reference to the string data, not a copy. Therefore keeping a reference to a returned
    UTF8Buf may force future mutable string operations to create a copy of the data. Additionally
    mutable operations performed on Strings constructed as a RefTag will create a copy of
    the string data.
 
    String data up to 128 bytes is stored in allocation pools to reduce fragmantation and
    improve memory overhead.
 */
class String {
public:
	typedef void (String::*unspecified_bool_type) ();
	typedef const char *const_iterator;

	//! Constructs an empty string.
	String(::Zone &zone = ZString);
	//! Constructs a copy of a string.
	String(const String &s);
	//! Constructs a copy of a string and forces a copy to be made
	//! if the source string was constructed with a RefTag.
	String(const String &s, const CopyTag_t&, ::Zone &zone = ZString);
	//! Contructs a copy of utf8 encoded string data.
	explicit String(const UTF8Buf &buf);
	//! Constructs a string from UTF16 data.
	String(const UTF16Buf &buf, ::Zone &zone = ZString);
	//! Constructs a string from UTF32 data.
	String(const UTF32Buf &buf, ::Zone &zone = ZString);
	//! Constructs a string from wchar_t data.
	String(const WCharBuf &buf, ::Zone &zone = ZString);

	//! Constructs a string from a copy of a UTF8 encoded string.
	String(const char *sz, const CopyTag_t&, ::Zone &zone = ZString);
	//! Constructs a string as an in-place reference of a UTF8 encoded string.
	String(const char *sz, const RefTag_t&, ::Zone &zone = ZString);
	//! Construct a string from a copy of a UTF8 encoded string of the specified length.
	String(const char *sz, int len, const CopyTag_t&, ::Zone &zone = ZString);
	//! Constructs a string as an in-place reference of a UTF8 encoded string of a the specified length.
	/*! The referenced string must be null terminated, however the supplied length should only
	    enclose the actualy character bytes (therefore the null byte should be at len+1) 
	 */
	String(const char *sz, int len, const RefTag_t&, ::Zone &zone = ZString);

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
	//! Constructs a string from a wide-character encoded string.
	//! The source data is re-encoded as a UTF8 string internally.
	String(const wchar_t *sz, ::Zone &zone = ZString);
	//! Constructs a string from a wide-character encoded string of the specified length.
	//! The source data is re-encoded as a UTF8 string internally.
	String(const wchar_t *sz, int len, ::Zone &zone = ZString);
#endif

	//! Constructs a string from a UTF16 encoded string.
	//! The source data is re-encoded as a UTF8 string internally.
	String(const U16 *sz, ::Zone &zone = ZString);
	//! Constructs a string from a UTF16 encoded string of the specified length.
	//! The source data is re-encoded as a UTF8 string internally.
	String(const U16 *sz, int len, ::Zone &zone = ZString);

	//! Constructs a string from a UTF32 encoded string.
	//! The source data is re-encoded as a UTF8 string internally.
	String(const U32 *sz, ::Zone &zone = ZString);
	//! Constructs a string from a UTF32 encoded string of the specified length.
	//! The source data is re-encoded as a UTF8 string internally.
	String(const U32 *sz, int len, ::Zone &zone = ZString);

	//! Constructs a string from a single character.
	String(char c, ::Zone &zone = ZString);

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
	String(wchar_t c, ::Zone &zone = ZString);
#endif

	String(U16 c, ::Zone &zone = ZString);
	String(U32 c, ::Zone &zone = ZString);

	// STL compatible constructors.
	explicit String(const std::string &str, ::Zone &zone = ZString);
	explicit String(const std::wstring &str, ::Zone &zone = ZString);

	//! Allocates a string of N bytes in length including the null terminator.
	/*! The string contains garbase but will be null terminated. */
	explicit String(int len, ::Zone &zone = ZString);

	// Immutable operations.

	RAD_DECLARE_READONLY_PROPERTY(String, length, int);
	//! Defined as c_str
	RAD_DECLARE_READONLY_PROPERTY(String, begin, const char*);
	//! Defined as c_str + length
	RAD_DECLARE_READONLY_PROPERTY(String, end, const char*);
	RAD_DECLARE_READONLY_PROPERTY(String, numChars, int);
	RAD_DECLARE_READONLY_PROPERTY(String, c_str, const char*);
	RAD_DECLARE_READONLY_PROPERTY(String, empty, bool);

	//! Retrieves the string data as a UTF8 encoded buffer.
	/*! This operation is inexpensive since the original string data is simply referenced by the UTF8Buf object. */
	UTF8Buf toUTF8() const;
	//! Retrieves the string data as a UTF16 encoded buffer.
	UTF16Buf toUTF16() const;
	//! Retrieves the string data as a UTF32 encoded buffer.
	UTF32Buf toUTF32() const;
	//! Retrieves the string data as a wide-character encoded buffer.
	WCharBuf toWChar() const;

	std::string toStdString() const;
	std::wstring toStdWString() const;

	int compare(const String &str) const;
	int compare(const char *sz) const;
	int compare(const wchar_t *sz) const;

	int comparei(const String &str) const;
	int comparei(const char *sz) const;
	int comparei(const wchar_t *sz) const;

	int nCompare(const String &str, int len) const;
	int nCompare(const char *sz, int len) const;
	int nCompare(const wchar_t *sz, int len) const;

	int nComparei(const String &str, int len) const;
	int nComparei(const char *sz, int len) const;
	int nComparei(const wchar_t *sz, int len) const;

	int strstr(const String &str) const;
	int strstr(const char *sz) const;

	String join(const String &str) const;
	String join(const char *sz) const;
	String join(const wchar_t *sz) const;
	String join(const char c) const;
	String join(const wchar_t c) const;

	String nJoin(const String &str, int len) const;
	String nJoin(const char *sz, int len) const;
	String nJoin(const wchar_t *sz, int len) const;

	//! Returns a substring.
	/*! \param first The first \em character to include in the substring.
	    \param count The number of \em characters to include in the substring.
	    \remarks Since the string is UTF8 encoded there is a difference between
	    a character index and a byte position in the string. For strings that
	    only contain ASCII the character index and byte position are the same.
	    \sa CharIndexForBytePos()
	    \sa BytePosForCharIndex()
	 */
	String substr(int first, int count) const;

	//! Returns a substring.
	/*! Returns the remaining string after the specified \em character.
		Equivelent to right(numChars() - ofs).
		\param ofs The first \em character to include in the substring.
		\remarks Since the string is UTF8 encoded there is a difference between
	    a character index and a byte position in the string. For strings that
	    only contain ASCII the character index and byte position are the same.
	    \sa CharIndexForBytePos()
	    \sa BytePosForCharIndex()
	 */
	String substr(int ofs) const;

	//! Returns a substring. This function is intended for use with ASCII strings.
	/*! \param first The first \em character to include in the substring.
	    \param count The number of \em characters to include in the substring.
	    \remarks This function is faster than the generic substr() function, 
	    however it is only intended for use with strings that only contains
	    ASCII characters. If used on a string with unicode characters the results
	    are undefined.
	 */
	String substrASCII(int first, int count) const;
		
	//! Returns a substring. This function is intended for use with ASCII strings.
	/*! Equivelent to rightASCII(length - ofs).
		\param ofs The first \em character to include in the substring.
		\remarks This function is faster than the generic substr() function, 
	    however it is only intended for use with strings that only contains
	    ASCII characters. If used on a string with unicode characters the results
	    are undefined.
	 */
	String substrASCII(int ofs) const;

	//! Returns a substring containing the leftmost characters.
	/*! \param count The number of characters to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	 */
	String left(int count) const;

	//! Returns a substring containing the rightmost characters.
	/*! \param count The number of characters to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	 */
	String right(int count) const;

	//! Returns a substring containing the leftmost characters.
	/*! \param count The number of characters to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	    This function is intended for use on strings that only contain ASCII characters.
	    If used on a string that contains unicode the results are undefined.
	 */
	String leftASCII(int count) const;

	//! Returns a substring containing the rightmost characters.
	/*! \param count The number of characters to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	    If used on a string that contains unicode the results are undefined.
	 */
	String rightASCII(int count) const;

	//! Boolean operator returns true if string is non-empty.
	operator unspecified_bool_type () const;

	//! Case sensitive equality test.
	bool operator == (const String &str) const;
	bool operator == (const char *sz) const;
	bool operator == (const wchar_t *sz) const; 
	
	//! Case sensitive inequality test.
	bool operator != (const String &str) const;
	bool operator != (const char *sz) const;
	bool operator != (const wchar_t *sz) const;

	//! Case sensitive greater than test.
	bool operator > (const String &str) const;
	bool operator > (const char *sz) const;
	bool operator > (const wchar_t *sz) const;

	//! Case sensitive gequal test.
	bool operator >= (const String &str) const;
	bool operator >= (const char *sz) const;
	bool operator >= (const wchar_t *sz) const;

	//! Case sensitive less test.
	bool operator < (const String &str) const;
	bool operator < (const char *sz) const;
	bool operator < (const wchar_t *sz) const;

	//! Case sensitive lequal test.
	bool operator <= (const String &str) const;
	bool operator <= (const char *sz) const;
	bool operator <= (const wchar_t *sz) const;

	//! Returns the character at the specified position.
	char operator [] (int ofs) const;

	//! Returns true if the string objects reference the same string data.
	bool equalInstance(const String &str) const;
	//! Returns true if the string objects reference the same string data.
	bool equalInstance(const UTF8Buf &buf) const;

	//! Returns the character index corresponding to the specified byte offset.
	/*! \returns A character index or -1 if the byte position is invalid 
	    (off the end of the string). 
	 */
	int charIndexForBytePos(int pos) const;

	//! Returns the byte position corresponding to the specified character index.
	/*! \returns A byte position or -1 if the character index is invalid 
	    (off the end of the string). 
	 */
	int bytePosForCharIndex(int idx) const;

	// Mutable Operations

	String &upper();
	String &lower();
	String &reverse();

	String &upperASCII();
	String &lowerASCII();
	String &reverseASCII();

	String &trimSubstr(int ofs, int count);
	String &trimSubstrASCII(int ofs, int count);

	String &trimLeft(int count);
	String &trimRight(int count);

	String &trimLeftASCII(int count);
	String &trimRightASCII(int count);

	String &erase(int ofs, int count = 1);
	String &eraseASCII(int ofs, int count = 1);

	String &append(const String &str);
	String &append(const char *sz);
	String &append(const wchar_t *sz);
	String &append(const char c);
	String &append(const wchar_t c);

	String &nAppend(const String &str, int len);
	String &nAppend(const char *sz, int len);
	String &nAppend(const wchar_t *sz, int len);

	String &replace(const String &src, const String &dst);
	String &replace(char src, char dst);
	String &replace(char src, const char *dst);
	String &replace(char src, wchar_t dst);
	String &replace(char src, const wchar_t *dst);
	String &replace(const char *src, char dst);
	String &replace(const char *src, const char *dst);
	String &replace(const char *src, wchar_t dst);
	String &replace(const char *src, const wchar_t *dst);
	String &replace(wchar_t src, char dst);
	String &replace(wchar_t src, const char *dst);
	String &replace(wchar_t src, wchar_t dst);
	String &replace(wchar_t src, const wchar_t *dst);
	String &replace(const wchar_t *src, char dst);
	String &replace(const wchar_t *src, const char *dst);
	String &replace(const wchar_t *src, wchar_t dst);
	String &replace(const wchar_t *src, const wchar_t *dst);

	String &printf(const char *fmt, ...);
	String &printf(const char *fmt, va_list args);

	String &operator = (const String &string);
	String &operator = (const char *sz);
	String &operator = (const wchar_t *sz);
	String &operator = (char c);
	String &operator = (wchar_t c);

	String &operator += (const String &string);
	String &operator += (const char *sz);
	String &operator += (const wchar_t *sz);
	String &operator += (char c);
	String &operator += (wchar_t c);

	//! Writes a character to a byte position.
	String &write(int pos, char sz);
	//! Writes a character string to a byte position (including the null terminator).
	String &write(int pos, const char *sz);
	//! Writes \em len bytes of a character string to a byte position.
	/*! This operation is performed like an strncpy, meaing that if \em sz
	    terminates before len bytes are written the write stops at that point
		and the string is terminated.

		\remarks Bounds checking is not performed on the destination string.
		Make sure to construct a string using the String(len) constructor to 
		pre-allocate a string large enough to contain the operations of the write. */
	String &write(int pos, const char *sz, int len);

	//! Writes a \em len bytes of a character string to a byte position.
	String &write(int pos, const String &str);
	//! Writes a \em len bytes of a character string to a byte position.
	String &write(int pos, const String &str, int len);
	
	String &clear();

private:

	void bool_true() {}

	RAD_DECLARE_GET(length, int);
	RAD_DECLARE_GET(begin, const char*);
	RAD_DECLARE_GET(end, const char*);
	RAD_DECLARE_GET(c_str, const char *);
	RAD_DECLARE_GET(numChars, int);
	RAD_DECLARE_GET(empty, bool);

	details::DataBlock::Ref m_data;
	::Zone *m_zone;
};

String operator + (const String &a, const String &b);
String operator + (const String &a, const char *sz);
String operator + (const String &a, const wchar_t *sz);
String operator + (const char *sz, const String &b);
String operator + (const wchar_t *sz, const String &b);

} // string

//! Constructs a RefTag string around sz.
/*! This is a helper function used to wrap constant strings.
    It is critical that the string referenced by \em sz be
	a compile time string or otherwise be immutable during
	the lifetime of the returned string. */
string::String CStr(const char *sz);

namespace std {
template <class C, class T> class basic_istream;
template <class C, class T> class basic_ostream;
}

//! Streaming support for strings.
/*! Internally the string is converted to/from a std::string. */
template<class CharType, class Traits>
std::basic_istream<CharType, Traits>& operator >> (std::basic_istream<CharType, Traits> &stream, string::String &string);

//! Streaming support for strings.
template<class CharType, class Traits>
std::basic_ostream<CharType, Traits>& operator << (std::basic_ostream<CharType, Traits> &stream, const string::String &string);


#include "../PopPack.h"

#include "String.inl"
