/*! \file String.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Runtime
*/

#pragma once

#include "StringDef.h"
#include "StringBase.h"
#include "StringDetails.h"
#include <string>
#include <stdarg.h>

#include "../PushPack.h"

namespace string {

#if !defined(RAD_OPT_SHIP)
/*! In cases where string data is short (31 bytes or less) the string is stored
	locally inside the string object in a small buffer. If the string is longer
	than 31 bytes it is placed inside a shared data block object that can be
	referenced by multiple strings (the immutable reference part of the storage 
	semantics). A string which contains its contents inside this internal buffer
	is called a stack string.

	To keep the implementation orthogonal and simple short RefTag data is also
	copied into the object if it fits, avoiding the need to allocate a boost::shared_ptr
	to track the RefTag. This does mean that a copy of the RefTag data is made
	however.

	RefTag data requires special care by the user (it must not change) and because
	of the nature of a string (stack string or not) is size dependant this can lead
	to some very hard to find bugs when RefTag data is improperly used. In some
	cases modifying the source data of the RefTag may cause no side-effects (because
	the data was moved inside a stack string) and in other cases it may alter the
	string contents (because the RefTag data was too large and the source is indeed
	referenced directly) depending on the particular data being used.

	Enable this define to disable using stack strings for RefTag data. This does
	require an allocation per string in the form of a boost::shared_ptr however
	illegal modification of RefTag data is guaranteed to have a side effect.
*/
#define RAD_STRING_DISABLE_STACK_STRINGS_FOR_REFTAG_DATA
#endif

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

	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, c_str, const T*);
	//! Defined as c_str
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, begin, const T*);
	//! Defined as c_str + size
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, end, const T*);
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, size, int);
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, empty, bool);
	RAD_DECLARE_READONLY_PROPERTY_EX(class CharBuf<Traits>, SelfType, numChars, int);

	//! Clear the buffer.
	void Clear();

	//! Contructs a new character buffer. The specified data is copied into a new buffer.
	static SelfType New(const T *data, int size, const CopyTag_t&, ::Zone &zone = ZString);
	//! Constructs a new character buffer. The specified data is referenced directly.
	static SelfType New(const T *data, int size, const RefTag_t&, ::Zone &zone = ZString);

private:

	enum { 
		kStackSize = 31 
	};

	CharBuf(const details::DataBlock::Ref &data, ::Zone &zone);

	friend class String;

	void bool_true() {};

	RAD_DECLARE_GET(c_str, const T*);
	RAD_DECLARE_GET(begin, const T*);
	RAD_DECLARE_GET(end, const T*);
	RAD_DECLARE_GET(size, int);
	RAD_DECLARE_GET(empty, bool);
	RAD_DECLARE_GET(numChars, int);

	details::DataBlock::Ref m_data;
	::Zone *m_zone;
	char m_stackBytes[kStackSize];
	char m_stackLen;
};

//! UTF8 String class.
/*! This string type is reference based, internally immutable, and encoded as a utf8 string. This
    means it can hold ASCII data unmodified. It also means that assignment requires almost no
    overhead. Additionally mechanisms have been provided to construct a String object around
    constant data as an in-place reference allowing string data loaded on disk or compile time 
    static string data to be used without allocation overhead or copying.
 
    Care must be taken by the user when using certain types of operations on unicode strings.
    The Upper() and Lower() functions may not correctly handle all unicode code points
    depending on your locale and normalization issues. This class does not perform normalization.
    Be aware that normalization issues can also effect sorting. If aesthetically correct sort order
    is important you must manually normalize the string before Compare() will do what you want.
 
    Considerably faster versions of certain functions have been provided that work based on bytes in a 
	string only. By default one byte corresponds to one character in an ASCII encoded string. If a
	string contains UTF8 encoded data then a single character may be represented by multiple bytes.
	It is important to be aware that Byte() versions of functions will only work reliably on ASCII data
	unless the caller has calculated byte offsets for individual characters. \sa ByteForChar() 
	\sa CharForByte()
 
    Converting to/from a UTF8Buf is free as it constructs a buffer around the underlying string
    data. However converting to any other representation is a relatively expensive operation.
    Constructing a UTF8Buf from a String is not free if the string in question was created
    using reference (RefTag) semantics.
 
    In general it is safe to assume that doing any operations on a string where the source
    data is not UTF8 encoded will require encoding the data to UTF8 and possibly allocation
    overhead. Upper(), Lower(), and SubStr() all require transcoding to UTF32 and back to
    UTF8. Their ASCII equivelents do not.
 
    Mutable operations on a String object are done in-place if possible (i.e. there are no
    other references to the string data). Keep in mind that ToUTF8() will usually return
    a reference to the string data, not a copy. Therefore keeping a reference to a returned
    UTF8Buf may force future mutable string operations to create a copy of the data. Additionally
    mutable operations performed on Strings constructed as a RefTag will create a copy of
    the string data.
 
    String data up to 128 bytes is stored in allocation pools to reduce fragmantation and
    improve memory overhead.
 */
class String {
public:
	typedef const char *const_iterator;

	//! Constructs an empty string.
	String(::Zone &zone = ZString);
	//! Constructs a copy of a string.
	String(const String &s);
	//! Constructs a copy of a string and forces a copy to be made
	//! if the source string was constructed with a RefTag.
	explicit String(const String &s, const CopyTag_t&, ::Zone &zone = ZString);
	//! Contructs a copy of utf8 encoded string data.
	explicit String(const UTF8Buf &buf);
	//! Constructs a string from UTF16 data.
	explicit String(const UTF16Buf &buf, ::Zone &zone = ZString);
	//! Constructs a string from UTF32 data.
	explicit String(const UTF32Buf &buf, ::Zone &zone = ZString);
	//! Constructs a string from wchar_t data.
	explicit String(const WCharBuf &buf, ::Zone &zone = ZString);

	//! Constructs a string from a copy of a UTF8 encoded string.
	explicit String(const char *sz, ::Zone &zone = ZString);
	//! Constructs a string as an in-place reference of a UTF8 encoded string.
	explicit String(const char *sz, const RefTag_t&, ::Zone &zone = ZString);
	//! Construct a string from a copy of a UTF8 encoded string of the specified length.
	explicit String(const char *sz, int len, const CopyTag_t&, ::Zone &zone = ZString);
	//! Constructs a string as an in-place reference of a UTF8 encoded string of a the specified length.
	/*! The referenced string must be null terminated, however the supplied length should only
	    enclose the actualy character bytes (therefore the null byte should be at len+1) 
	 */
	explicit String(const char *sz, int len, const RefTag_t&, ::Zone &zone = ZString);

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
	//! Constructs a string from a wide-character encoded string.
	//! The source data is re-encoded as a UTF8 string internally.
	explicit String(const wchar_t *sz, ::Zone &zone = ZString);
	//! Constructs a string from a wide-character encoded string of the specified length.
	//! The source data is re-encoded as a UTF8 string internally.
	explicit String(const wchar_t *sz, int len, ::Zone &zone = ZString);
#endif

	//! Constructs a string from a UTF16 encoded string.
	//! The source data is re-encoded as a UTF8 string internally.
	explicit String(const U16 *sz, ::Zone &zone = ZString);
	//! Constructs a string from a UTF16 encoded string of the specified length.
	//! The source data is re-encoded as a UTF8 string internally.
	explicit String(const U16 *sz, int len, ::Zone &zone = ZString);

	//! Constructs a string from a UTF32 encoded string.
	//! The source data is re-encoded as a UTF8 string internally.
	explicit String(const U32 *sz, ::Zone &zone = ZString);
	//! Constructs a string from a UTF32 encoded string of the specified length.
	//! The source data is re-encoded as a UTF8 string internally.
	explicit String(const U32 *sz, int len, ::Zone &zone = ZString);

	//! Constructs a string from a single character.
	explicit String(char c, ::Zone &zone = ZString);

#if defined(RAD_NATIVE_WCHAR_T_DEFINED)
	explicit String(wchar_t c, ::Zone &zone = ZString);
#endif

	explicit String(U16 c, ::Zone &zone = ZString);
	explicit String(U32 c, ::Zone &zone = ZString);

	// STL compatible constructors.
	explicit String(const std::string &str, ::Zone &zone = ZString);
	explicit String(const std::wstring &str, ::Zone &zone = ZString);

	//! Allocates a string of N bytes in length including the null terminator.
	/*! The string contains garbage but will be null terminated. */
	explicit String(int len, ::Zone &zone = ZString);

	// Immutable operations.

	RAD_DECLARE_READONLY_PROPERTY(String, numBytes, int);
	//! Defined as c_str
	RAD_DECLARE_READONLY_PROPERTY(String, begin, const char*);
	//! Defined as c_str + length
	RAD_DECLARE_READONLY_PROPERTY(String, end, const char*);
	RAD_DECLARE_READONLY_PROPERTY(String, numChars, int);
	RAD_DECLARE_READONLY_PROPERTY(String, c_str, const char*);
	RAD_DECLARE_READONLY_PROPERTY(String, empty, bool);

	//! Retrieves the string data as a UTF8 encoded buffer.
	/*! This operation is inexpensive since the original string data is simply referenced by the UTF8Buf object. */
	UTF8Buf ToUTF8() const;
	//! Retrieves the string data as a UTF16 encoded buffer.
	UTF16Buf ToUTF16() const;
	//! Retrieves the string data as a UTF32 encoded buffer.
	UTF32Buf ToUTF32() const;
	//! Retrieves the string data as a wide-character encoded buffer.
	WCharBuf ToWChar() const;

	std::string ToStdString() const;
	std::wstring ToStdWString() const;

	// Compares strings, supporting unicode code-points.
	// Lengths are in characters not bytes.

	int Compare(const String &str) const;
	int Compare(const char *sz) const;
	int Compare(const wchar_t *sz) const;

	int Comparei(const String &str) const;
	int Comparei(const char *sz) const;
	int Comparei(const wchar_t *sz) const;

	int NCompare(const String &str, int len) const;
	int NCompare(const char *sz, int len) const;
	int NCompare(const wchar_t *sz, int len) const;

	int NComparei(const String &str, int len) const;
	int NComparei(const char *sz, int len) const;
	int NComparei(const wchar_t *sz, int len) const;

	// Compares string, only supports ASCII

	int CompareASCII(const String &str) const;
	int CompareASCII(const char *sz) const;
	
	int CompareiASCII(const String &str) const;
	int CompareiASCII(const char *sz) const;
	
	int NCompareASCII(const String &str, int len) const;
	int NCompareASCII(const char *sz, int len) const;
	
	int NCompareiASCII(const String &str, int len) const;
	int NCompareiASCII(const char *sz, int len) const;
	
	int StrStr(const String &str) const;
	int StrStr(const char *sz) const;

	StringVec Split(const String &sep) const;
	StringVec Split(const char *sep) const;

	String Join(const String &str) const;
	String Join(const char *sz) const;
	String Join(const wchar_t *sz) const;
	String Join(const char c) const;
	String Join(const wchar_t c) const;

	String NJoin(const String &str, int len) const;
	String NJoin(const char *sz, int len) const;
	String NJoin(const wchar_t *sz, int len) const;

	//! Returns a substring.
	/*! \param first The first \em character to include in the substring.
	    \param count The number of \em characters to include in the substring.
	    \remarks Since the string is UTF8 encoded there is a difference between
	    a character index and a byte position in the string. For strings that
	    only contain ASCII the character index and byte position are the same.
	    \sa CharForByte()
	    \sa ByteForChar()
	 */
	String SubStr(int first, int count) const;

	//! Returns a substring.
	/*! Returns the remaining string starting at the specified \em character.
		Equivelent to Right(NumChars() - ofs).
		\param ofs The first \em character to include in the substring.
		\remarks Since the string is UTF8 encoded there is a difference between
	    a character index and a byte position in the string. For strings that
	    only contain ASCII the character index and byte position are the same.
	    \sa CharForByte()
	    \sa ByteForChar()
	 */
	String SubStr(int ofs) const;

	//! Returns a substring. This function is intended for use with ASCII strings.
	/*! \param first The first \em byte to include in the substring.
	    \param count The number of \em bytes to include in the substring.
	    \remarks This function is faster than the generic SubStr() function, 
	     however its arguments are expressed as bytes not characters.
	 */
	String SubStrBytes(int first, int count) const;
		
	//! Returns a substring.
	/*! Equivelent to RightBytes(length - ofs).
		\param ofs The first \em byte to include in the substring.
		\remarks This function is faster than the generic SubStr() function, 
	    however its arguments are expressed as bytes not characters.
	 */
	String SubStrBytes(int ofs) const;

	//! Returns a substring containing the leftmost characters.
	/*! \param count The number of \em characters to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	 */
	String Left(int count) const;

	//! Returns a substring containing the rightmost characters.
	/*! \param count The number of \em characters to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	 */
	String Right(int count) const;

	//! Returns a substring containing the leftmost characters.
	/*! \param count The number of \em bytes to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	    This function is faster than the generic Left() function, however its arguments are 
		expressed as bytes not characters.
	 */
	String LeftBytes(int count) const;

	//! Returns a substring containing the rightmost characters.
	/*! \param count The number of \em bytes to return. 
	    \remarks Care must be used as bounds checking is not performed. Requesting more
	    characters than there are in a string will most likely result in an invalid access.
	    This function is faster than the generic SubStr() function, however its arguments are 
		expressed as bytes not characters.
	 */
	String RightBytes(int count) const;

	// NOTE: comparison operators ONLY support ASCII character sets.

	//! Case sensitive equality test.
	bool operator == (const String &str) const;
	bool operator == (const char *sz) const;
	
	//! Case sensitive inequality test.
	bool operator != (const String &str) const;
	bool operator != (const char *sz) const;

	//! Case sensitive greater than test.
	bool operator > (const String &str) const;
	bool operator > (const char *sz) const;

	//! Case sensitive gequal test.
	bool operator >= (const String &str) const;
	bool operator >= (const char *sz) const;

	//! Case sensitive less test.
	bool operator < (const String &str) const;
	bool operator < (const char *sz) const;

	//! Case sensitive lequal test.
	bool operator <= (const String &str) const;
	bool operator <= (const char *sz) const;

	//! Returns the character at the specified position.
	char operator [] (int ofs) const;

	//! Returns true if the string objects reference the same string data.
	bool EqualsInstance(const String &str) const;
	//! Returns true if the string objects reference the same string data.
	bool EqualsInstance(const UTF8Buf &buf) const;

	//! Returns the character index corresponding to the specified byte offset.
	/*! \returns A character index or -1 if the byte position is invalid 
	    (off the end of the string). 
	 */
	int CharForByte(int byte) const;

	//! Returns the byte position corresponding to the specified character index.
	/*! \returns A byte position or -1 if the character index is invalid 
	    (off the end of the string). 
	 */
	int ByteForChar(int pos) const;

	// Mutable Operations

	String &Upper();
	String &Lower();
	String &Reverse();

	String &UpperASCII();
	String &LowerASCII();
	String &ReverseASCII();

	String &TrimSubStr(int ofs, int count);
	String &TrimSubStrBytes(int ofs, int count);

	String &TrimLeft(int count);
	String &TrimRight(int count);

	String &TrimLeftBytes(int count);
	String &TrimRightBytes(int count);

	String &Erase(int ofs, int count = 1);
	String &EraseBytes(int ofs, int count = 1);

	String &Append(const String &str);
	String &Append(const char *sz);
	String &Append(const wchar_t *sz);
	String &Append(const char c);
	String &Append(const wchar_t c);

	String &NAppend(const String &str, int len);
	String &NAppend(const char *sz, int len);
	String &NAppend(const wchar_t *sz, int len);

	String &NAppendBytes(const String &str, int len);
	String &NAppendBytes(const char *sz, int len);
	String &NAppendBytes(const wchar_t *sz, int len);

	String &Replace(const String &src, const String &dst);
	String &Replace(char src, char dst);
	String &Replace(char src, const char *dst);
	String &Replace(char src, wchar_t dst);
	String &Replace(char src, const wchar_t *dst);
	String &Replace(const char *src, char dst);
	String &Replace(const char *src, const char *dst);
	String &Replace(const char *src, wchar_t dst);
	String &Replace(const char *src, const wchar_t *dst);
	String &Replace(wchar_t src, char dst);
	String &Replace(wchar_t src, const char *dst);
	String &Replace(wchar_t src, wchar_t dst);
	String &Replace(wchar_t src, const wchar_t *dst);
	String &Replace(const wchar_t *src, char dst);
	String &Replace(const wchar_t *src, const char *dst);
	String &Replace(const wchar_t *src, wchar_t dst);
	String &Replace(const wchar_t *src, const wchar_t *dst);

	String &Printf(const char *fmt, ...);
	String &Printf_valist(const char *fmt, va_list args);

	String &PrintfASCII(const char *fmt, ...);
	String &PrintfASCII_valist(const char *fmt, va_list args);

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
	String &Write(int pos, char sz);
	//! Writes a character string to a byte position (including the null terminator).
	String &Write(int pos, const char *sz);
	//! Writes \em len bytes of a character string to a byte position.
	/*! This operation is performed like a strncpy, meaning that if \em sz
	    terminates before len bytes are written the write stops at that point
		and the string is terminated.

		\remarks Bounds checking is not performed on the destination string.
		Make sure to construct a string using the String(len) constructor to 
		pre-allocate a string large enough to contain the operations of the write. */
	String &Write(int pos, const char *sz, int len);

	//! Writes a \em len bytes of a character string to a byte position.
	String &Write(int pos, const String &str);
	//! Writes a \em len bytes of a character string to a byte position.
	String &Write(int pos, const String &str, int len);
	
	String &Clear();

private:

	enum { kStackSize = UTF8Buf::kStackSize };

	void bool_true() {}

	RAD_DECLARE_GET(numBytes, int);
	RAD_DECLARE_GET(begin, const char*);
	RAD_DECLARE_GET(end, const char*);
	RAD_DECLARE_GET(c_str, const char *);
	RAD_DECLARE_GET(numChars, int);
	RAD_DECLARE_GET(empty, bool);

	details::DataBlock::Ref m_data;
	::Zone *m_zone;
	char m_stackBytes[kStackSize];
	U8 m_stackLen;
};

String operator + (const String &a, const String &b);
String operator + (const String &a, const char *sz);
String operator + (const String &a, const wchar_t *sz);
String operator + (const char *sz, const String &b);
String operator + (const wchar_t *sz, const String &b);
String operator + (char s, const String &b);
String operator + (wchar_t s, const String &b);
String operator + (const String &b, char s);
String operator + (const String &b, wchar_t s);

} // string

//! Constructs a RefTag string around sz.
/*! This is a helper function used to wrap constant strings.
    It is critical that the string referenced by \em sz be
	a compile time string or otherwise be immutable during
	the lifetime of the returned string. */
string::String CStr(const char *sz);

RAD_BEGIN_STD_NAMESPACE
template <class C, class T> class basic_istream;
template <class C, class T> class basic_ostream;
RAD_END_STD_NAMESPACE

namespace string {

bool operator == (const char *sz, const String &s);
bool operator != (const char *sz, const String &s);

//! Streaming support for strings.
/*! Internally the string is converted to/from a std::string. */
template<class CharType, class Traits>
std::basic_istream<CharType, Traits>& operator >> (std::basic_istream<CharType, Traits> &stream, String &string);

//! Streaming support for strings.
template<class CharType, class Traits>
std::basic_ostream<CharType, Traits>& operator << (std::basic_ostream<CharType, Traits> &stream, const String &string);

} //string

#include "../PopPack.h"

#include "String_inl.h"

typedef ::string::String String;
typedef ::string::StringVec StringVec;