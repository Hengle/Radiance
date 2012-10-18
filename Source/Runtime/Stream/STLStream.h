// STLStream.h
// Adaptors that allow STL stream objects to be used with our stream system.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "STLStreamDef.h"
#include "Stream.h"
#include <iostream>
#include <sstream>
#include "../PushPack.h"

#if defined(RAD_OPT_GCC)
namespace std { enum { _BADOFF = -1 }; }
#endif

namespace stream {

// with the exception of the string buff, these streams are not text formatted by default.

//////////////////////////////////////////////////////////////////////////////////////////

template <typename _Elem, typename _Traits = std::char_traits<_Elem>, typename _Alloc = std::allocator<_Elem> >
class basic_stringbuf : public std::basic_stringbuf<_Elem, _Traits, _Alloc>
{
public:
	typedef std::basic_stringbuf<_Elem> SuperType;
	typedef std::basic_string<_Elem, _Traits, _Alloc> StringType;

	explicit basic_stringbuf(std::ios_base::openmode _Mode = std::ios_base::out)
		: SuperType(_Mode), m_mode(_Mode)
	{
	}

	explicit basic_stringbuf(const StringType &_Str, std::ios_base::openmode _Mode = std::ios_base::in | std::ios_base::out)
		: SuperType(_Str, _Mode), m_mode(_Mode)
	{
	}

	virtual ~basic_stringbuf()
	{
	}

protected:

	virtual int Flush(const StringType &str) = 0;

	virtual int sync()
	{
		StringType x(this->str());
#if defined(RAD_OPT_WINX)
		_Tidy();
		_Init(0, 0, _Getstate(m_mode));
#else // GCC
		this->str(StringType());
#endif
		return Flush(x);
	}

private:

#if defined(RAD_OPT_WINX)
	typedef typename SuperType::_Strstate _Strstate;

	_Strstate _Getstate(std::ios_base::openmode _Mode)
	{	// convert open mode to stream state bits
	_Strstate _State = (_Strstate)0;
	if (!(_Mode & std::ios_base::in))
		_State |= SuperType::_Noread;
	if (!(_Mode & std::ios_base::out))
		_State |= SuperType::_Constant;
	if (_Mode & std::ios_base::app)
		_State |= SuperType::_Append;
	if (_Mode & std::ios_base::ate)
		_State |= SuperType::_Atend;
	return (_State);
	}
#endif
	std::ios_base::openmode m_mode;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits = std::char_traits<_Elem> >
class basic_streambuf_adapter : public IInputBuffer, public IOutputBuffer
{
public:

	typedef std::basic_streambuf<_Elem, _Traits> stl_buf;

	basic_streambuf_adapter(stl_buf &b);
	virtual ~basic_streambuf_adapter();

	UReg InCaps() const;
	UReg InStatus() const;

	UReg OutCaps() const;
	UReg OutStatus() const;

	bool SeekIn(Seek seekType, SPos ofs, UReg* errorCode);
	SPos InPos() const;
	SPos Size()  const;

	bool SeekOut(Seek seekType, SPos ofs, UReg* errorCode);
	SPos OutPos() const;

	void Flush();

	SPos Read(void* buff, SPos numBytes, UReg* errorCode);
	SPos Write(const void* buff, SPos numBytes, UReg* errorCode);

private:

	basic_streambuf_adapter(const basic_streambuf_adapter<_Elem, _Traits>&);
	basic_streambuf_adapter<_Elem, _Traits>& operator = (const basic_streambuf_adapter<_Elem, _Traits>&);

	mutable stl_buf* m_b;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits = std::char_traits<_Elem> >
class basic_istream_adapter : public InputStream
{
public:

	typedef std::basic_istream<_Elem, _Traits> stl_stream;

	basic_istream_adapter(stl_stream &str);
	virtual ~basic_istream_adapter();

private:

	basic_istream_adapter(const basic_istream_adapter<_Elem, _Traits>&);
	basic_istream_adapter<_Elem, _Traits>& operator = (const basic_istream_adapter<_Elem, _Traits>&);

	basic_streambuf_adapter<_Elem, _Traits> m_b;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits = std::char_traits<_Elem> >
class basic_ostream_adapter : public OutputStream
{
public:

	typedef std::basic_ostream<_Elem, _Traits> stl_stream;

	basic_ostream_adapter(stl_stream &str);
	virtual ~basic_ostream_adapter();

private:

	basic_ostream_adapter(const basic_ostream_adapter<_Elem, _Traits>&);
	basic_ostream_adapter<_Elem, _Traits>& operator = (const basic_ostream_adapter<_Elem, _Traits>&);

	basic_streambuf_adapter<_Elem, _Traits> m_b;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits = std::char_traits<_Elem> >
class basic_iostream_adapter : public InputStream, public OutputStream
{
public:

	typedef std::basic_iostream<_Elem, _Traits> stl_stream;

	basic_iostream_adapter(stl_stream &str);
	virtual ~basic_iostream_adapter();

private:

	basic_iostream_adapter(const basic_iostream_adapter<_Elem, _Traits>&);
	basic_iostream_adapter<_Elem, _Traits>& operator = (const basic_iostream_adapter<_Elem, _Traits>&);

	basic_streambuf_adapter<_Elem, _Traits> m_b;
};

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits = std::char_traits<_Elem> >
class basic_streambuf : public std::basic_streambuf<_Elem, _Traits>
{
public:

	typedef typename _Traits::int_type int_type;
	typedef typename _Traits::pos_type pos_type;
	typedef typename _Traits::off_type off_type;
	typedef _Elem char_type;
	typedef _Traits traits_type;
	typedef basic_streambuf<_Elem, _Traits> self_type;

	basic_streambuf(IInputBuffer *in, IOutputBuffer *out);

protected:

	virtual int_type overflow(int_type);
	virtual int_type underflow();
	virtual std::streamsize xsgetn(_Elem *ptr, std::streamsize count);
	virtual std::streamsize xsputn(const _Elem *ptr, std::streamsize count);
	virtual pos_type seekoff(off_type, std::ios_base::seekdir, std::ios_base::openmode);
	virtual pos_type seekpos(pos_type, std::ios_base::openmode);

private:

	basic_streambuf(const self_type &);
	self_type &operator = (const self_type&) const;

	enum { BufSize = 1024 };

	IInputBuffer *m_in;
	IOutputBuffer *m_out;
	_Elem m_e[BufSize];
};


} // stream


#include "../PopPack.h"
#include "STLStream.inl"
