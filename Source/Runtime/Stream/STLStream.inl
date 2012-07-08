// STLStream.inl
// Adaptors that allow STL stream objects to be used with our stream system.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace stream {

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits>
inline basic_streambuf_adapter<_Elem, _Traits>::basic_streambuf_adapter(stl_buf &b)
{
	m_b = &b;
}

template<typename _Elem, typename _Traits>
inline basic_streambuf_adapter<_Elem, _Traits>::~basic_streambuf_adapter()
{
}

template<typename _Elem, typename _Traits>
inline UReg basic_streambuf_adapter<_Elem, _Traits>::InCaps() const
{
	return CapSeekInput|CapSizeInput;
}

template<typename _Elem, typename _Traits>
inline UReg basic_streambuf_adapter<_Elem, _Traits>::InStatus() const
{
	return StatusInputOpen;
}

template<typename _Elem, typename _Traits>
inline UReg basic_streambuf_adapter<_Elem, _Traits>::OutCaps() const
{
	return CapSeekOutput;
}

template<typename _Elem, typename _Traits>
inline UReg basic_streambuf_adapter<_Elem, _Traits>::OutStatus() const
{
	return StatusOutputOpen;
}

template<typename _Elem, typename _Traits>
inline bool basic_streambuf_adapter<_Elem, _Traits>::SeekIn(Seek seekType, SPos ofs, UReg* errorCode)
{
	RAD_ASSERT(seekType >= StreamBegin && seekType <= StreamEnd);
	RAD_ASSERT_MSG((ofs%sizeof(typename stl_buf::char_type)) == 0, "Illegal offset! The offset is not a multiple of the underlying STL stream element size!");

	bool s;

	switch(seekType)
	{
	case StreamBegin:
		s = (typename stl_buf::off_type)m_b->pubseekpos((typename stl_buf::pos_type)(ofs / sizeof(typename stl_buf::char_type)), std::ios_base::in) != (typename stl_buf::off_type)(-1);
		break;
	case StreamCur:
	case StreamEnd:
		typename stl_buf::off_type off = (typename stl_buf::off_type)((seekType==StreamCur) ? (ofs / sizeof(typename stl_buf::char_type)) : -(int)(ofs / sizeof(typename stl_buf::char_type)));
		s = (typename stl_buf::off_type)m_b->pubseekoff(off, (seekType==StreamCur)?std::ios_base::cur:std::ios_base::end, std::ios_base::in) != (typename stl_buf::off_type)(-1);
		break;
	};

	SetErrorCode(errorCode, (s) ? Success : ErrorBadSeekPos);

	return s;
}

template<typename _Elem, typename _Traits>
inline SPos basic_streambuf_adapter<_Elem, _Traits>::Size() const
{
	typename stl_buf::off_type p = m_b->pubseekoff(0, std::ios_base::cur, std::ios_base::in);

	if (p == (typename stl_buf::off_type)(-1)) 
		return 0;

	typename stl_buf::off_type e = m_b->pubseekoff(0, std::ios_base::end, std::ios_base::in);

	if (e == (typename stl_buf::off_type)(-1)) 
		return 0;

	m_b->pubseekpos((typename stl_buf::pos_type)p, std::ios_base::in);

	return (SPos)((e-p)*sizeof(typename stl_buf::char_type));
}

template<typename _Elem, typename _Traits>
inline SPos basic_streambuf_adapter<_Elem, _Traits>::InPos() const
{
	return (SPos)(m_b->pubseekoff(0, std::ios_base::cur, std::ios_base::in) * sizeof(typename stl_buf::char_type));
}

template<typename _Elem, typename _Traits>
inline SPos basic_streambuf_adapter<_Elem, _Traits>::Read(void* buff, SPos numBytes, UReg* errorCode)
{
	if (numBytes > 0)
	{
		RAD_ASSERT_MSG((numBytes%sizeof(typename stl_buf::char_type)) == 0, "Illegal byte count. numBytes is not a multiple of the underlying STL stream element size!");
		SPos read = (SPos)m_b->sgetn((typename stl_buf::char_type*)buff, (std::streamsize)(numBytes / sizeof(typename stl_buf::char_type))) * sizeof(typename stl_buf::char_type);
		SetErrorCode(errorCode, (read<numBytes) ? ErrorUnderflow : Success);
		return read;
	}

	return 0;
}

template<typename _Elem, typename _Traits>
inline bool basic_streambuf_adapter<_Elem, _Traits>::SeekOut(Seek seekType, SPos ofs, UReg* errorCode)
{
	RAD_ASSERT(seekType >= StreamBegin && seekType <= StreamEnd);
	RAD_ASSERT_MSG((ofs%sizeof(typename stl_buf::char_type)) == 0, "Illegal offset! The offset is not a multiple of the underlying STL stream element size!");

	bool s;

	switch(seekType)
	{
	case StreamBegin:
		s = (typename stl_buf::off_type)m_b->pubseekpos((typename stl_buf::pos_type)(ofs / sizeof(typename stl_buf::char_type)), std::ios_base::out) != (typename stl_buf::off_type)(-1);
		break;
	case StreamCur:
	case StreamEnd:
		typename stl_buf::off_type off = (typename stl_buf::off_type)((seekType==StreamCur) ? (ofs / sizeof(typename stl_buf::char_type)) : -(int)(ofs / sizeof(typename stl_buf::char_type)));
		s = (typename stl_buf::off_type)m_b->pubseekoff(off, (seekType==StreamCur)?std::ios_base::cur:std::ios_base::end, std::ios_base::out) != (typename stl_buf::off_type)(-1);
		break;
	};

	SetErrorCode(errorCode, (s) ? Success : ErrorBadSeekPos);

	return s;
}

template<typename _Elem, typename _Traits>
inline SPos basic_streambuf_adapter<_Elem, _Traits>::OutPos() const
{
	return (SPos)m_b->pubseekoff(0, std::ios_base::cur, std::ios_base::out) * sizeof(typename stl_buf::char_type);
}

template<typename _Elem, typename _Traits>
inline SPos basic_streambuf_adapter<_Elem, _Traits>::Write(const void* buff, SPos numBytes, UReg* errorCode)
{
	if (numBytes > 0)
	{
		RAD_ASSERT_MSG((numBytes%sizeof(_Elem)) == 0, "Illegal byte count. numBytes is not a multiple of the underlying STL stream element size!");
		SPos write = (SPos)m_b->sputn((const typename stl_buf::char_type*)buff, (std::streamsize)(numBytes / sizeof(typename stl_buf::char_type))) * sizeof(typename stl_buf::char_type);
		SetErrorCode(errorCode, (write<numBytes) ? ErrorOverflow : Success);
		return write;
	}

	return 0;
}

template<typename _Elem, typename _Traits>
inline void basic_streambuf_adapter<_Elem, _Traits>::Flush()
{
	m_b->pubsync();
}

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits>
inline basic_istream_adapter<_Elem, _Traits>::basic_istream_adapter(stl_stream &str) : m_b(*str.rdbuf())
{
	SetBuffer(m_b);
}

template<typename _Elem, typename _Traits>
inline basic_istream_adapter<_Elem, _Traits>::~basic_istream_adapter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits>
inline basic_ostream_adapter<_Elem, _Traits>::basic_ostream_adapter(stl_stream& str) : m_b(*str.rdbuf())
{
	SetBuffer(m_b);
}

template<typename _Elem, typename _Traits>
inline basic_ostream_adapter<_Elem, _Traits>::~basic_ostream_adapter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits>
inline basic_iostream_adapter<_Elem, _Traits>::basic_iostream_adapter(stl_stream& str) : m_b(*str.rdbuf())
{
	InputStream::SetBuffer(m_b);
	OutputStream::SetBuffer(m_b);
}

template<typename _Elem, typename _Traits>
inline basic_iostream_adapter<_Elem, _Traits>::~basic_iostream_adapter()
{
}

//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits>
inline basic_streambuf<_Elem, _Traits>::basic_streambuf(IInputBuffer *in, IOutputBuffer *out) :
m_in(in),
m_out(out)
{
}

template<typename _Elem, typename _Traits>
typename basic_streambuf<_Elem, _Traits>::int_type basic_streambuf<_Elem, _Traits>::overflow(int_type x)
{
	if (m_out && !_Traits::eq_int_type(_Traits::eof(), x))
	{
		char_type c = _Traits::to_char_type(x);
		SPos r = m_out->Write(&c, sizeof(char_type), 0);
		return r ? _Traits::not_eof(x) : _Traits::eof();
	}

	return m_out ? _Traits::not_eof(x) : _Traits::eof();
}

template<typename _Elem, typename _Traits>
typename basic_streambuf<_Elem, _Traits>::int_type basic_streambuf<_Elem, _Traits>::underflow()
{
	if (!m_in) return _Traits::eof();
	SPos r = m_in->Read(m_e, sizeof(char_type)*BufSize, 0);
	if (r)
	{
		this->setg(m_e, m_e, m_e+r);
	}
	return r ? _Traits::to_int_type(*m_e) : _Traits::eof();
}

template<typename _Elem, typename _Traits>
std::streamsize basic_streambuf<_Elem, _Traits>::xsgetn(_Elem *ptr, std::streamsize count)
{
	SPos r = 0;
	if (m_in)
	{
		r = m_in->Read(ptr, (SPos)count*sizeof(_Elem), 0) / sizeof(_Elem);
		this->setg(0, 0, 0);
	}
	return (std::streamsize)r;
}

template<typename _Elem, typename _Traits>
std::streamsize basic_streambuf<_Elem, _Traits>::xsputn(const _Elem *ptr, std::streamsize count)
{
	SPos r = 0;
	if (m_out)
	{
		r = m_out->Write(ptr, (SPos)count*sizeof(_Elem), 0) / sizeof(_Elem);
	}
	return (std::streamsize)r;
}

template<typename _Elem, typename _Traits>
typename basic_streambuf<_Elem, _Traits>::pos_type basic_streambuf<_Elem, _Traits>::seekoff(off_type x, std::ios_base::seekdir d, std::ios_base::openmode m)
{
	Seek sd = (d==std::ios_base::beg) ? StreamBegin :
		(d==std::ios_base::end) ? StreamEnd : StreamCur;
	if (x < 0) { x = -x; }

	if (m & std::ios_base::in)
	{
		if (!m_in->SeekIn(sd, (SPos)x, 0))
		{
			return std::streampos(std::_BADOFF);
		}
	}

	if (m & std::ios_base::out)
	{
		if (!m_out->SeekOut(sd, (SPos)x, 0))
		{
			return std::streampos(std::_BADOFF);
		}
	}

	return m_in ? 
		(std::streampos)m_in->InPos() : m_out ? 
		(std::streampos)m_out->OutPos() : 
		std::streampos(std::_BADOFF);
}

template<typename _Elem, typename _Traits>
inline typename basic_streambuf<_Elem, _Traits>::pos_type basic_streambuf<_Elem, _Traits>::seekpos(pos_type x, std::ios_base::openmode m)
{
	return this->seekoff((off_type)x, std::ios_base::beg, m);
}

} // stream

