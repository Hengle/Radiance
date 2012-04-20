// Stream.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../StringBase.h"
#include "../Utils.h"
#include "../PushSystemMacros.h"

namespace stream {

inline Exception::Exception(UReg code) : m_code(code)
{
}

inline Exception::Exception(const Exception& e) : exception(e), m_code(e.m_code)
{
}

inline Exception::~Exception() throw()
{
}

UReg inline Exception::ErrorCode() const
{
	return m_code;
}

void inline Exception::SetErrorCode(UReg code)
{
	m_code = code;
}

inline ReadException::ReadException(UReg code) : Exception(code)
{
}

inline ReadException::~ReadException() throw()
{
}

inline WriteException::WriteException(UReg code) : Exception(code)
{
}
inline WriteException::~WriteException() throw()
{
}

inline IInputBuffer::IInputBuffer()
{
}

inline IInputBuffer::~IInputBuffer()
{
}

inline IOutputBuffer::IOutputBuffer()
{
}

inline IOutputBuffer::~IOutputBuffer()
{
}

inline IPipeTarget::IPipeTarget()
{
}

inline IPipeTarget::~IPipeTarget()
{
}

RADRT_API inline void RADRT_CALL SetErrorCode(UReg* outCode, UReg code)
{
	if (outCode) *outCode = code;
}

inline InputStream::InputStream(IInputBuffer &buff) : m_buff(0)
{
	SetBuffer(buff);
}

inline InputStream::InputStream() : m_buff(0)
{
}

inline InputStream::~InputStream()
{
}

inline void InputStream::SetBuffer(IInputBuffer &buff)
{
	m_buff = &buff;
}

inline IInputBuffer &InputStream::Buffer()
{
	RAD_ASSERT(m_buff);
	return *m_buff;
}

inline UReg InputStream::InCaps() const
{
	RAD_ASSERT(m_buff);
	return m_buff->InCaps();
}

inline UReg InputStream::InStatus() const
{
	RAD_ASSERT(m_buff);
	return m_buff->InStatus();
}

inline SPos InputStream::Size() const
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(InStatus()&StatusInputOpen);
	RAD_ASSERT(InCaps()&CapSeekInput);
	return m_buff->Size();
}

inline SPos InputStream::InPos() const
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(InStatus()&StatusInputOpen);
	return m_buff->InPos();
}

inline SPos InputStream::Read(void* buff, SPos numBytes, UReg* errorCode)
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(InStatus()&StatusInputOpen);
	if (numBytes > 0) return m_buff->Read(buff, numBytes, errorCode);
	return 0;
}

inline bool InputStream::SeekIn(Seek seekType, SPos ofs, UReg* errorCode)
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(InStatus()&StatusInputOpen);
	return m_buff->SeekIn(seekType, ofs, errorCode);
}

inline bool InputStream::Read(S8*  var, UReg* errorCode)
{
	return Read(var, sizeof(S8), errorCode) == sizeof(S8);
}

inline bool InputStream::Read(U8*  var, UReg* errorCode)
{
	return Read(var, sizeof(U8), errorCode) == sizeof(U8);
}

inline bool InputStream::Read(S16* var, UReg* errorCode)
{
	return Read(var, sizeof(S16), errorCode) == sizeof(S16);
}

inline bool InputStream::Read(U16* var, UReg* errorCode)
{
	return Read(var, sizeof(U16), errorCode) == sizeof(U16);
}

inline bool InputStream::Read(S32* var, UReg* errorCode)
{
	return Read(var, sizeof(S32), errorCode) == sizeof(S32);
}

inline bool InputStream::Read(U32* var, UReg* errorCode)
{
	return Read(var, sizeof(U32), errorCode) == sizeof(U32);
}

inline bool InputStream::Read(S64* var, UReg* errorCode)
{
	return Read(var, sizeof(S64), errorCode) == sizeof(S64);
}

inline bool InputStream::Read(U64* var, UReg* errorCode)
{
	return Read(var, sizeof(U64), errorCode) == sizeof(U64);
}

inline bool InputStream::Read(F32* var, UReg* errorCode)
{
	return Read(var, sizeof(F32), errorCode) == sizeof(F32);
}

inline bool InputStream::Read(F64* var, UReg* errorCode)
{
	return Read(var, sizeof(F64), errorCode) == sizeof(F64);
}

template <typename T, typename S, typename A>
bool InputStream::ReadStringHelper<T, S, A>::Read(stream::InputStream &stream, std::basic_string<T, S, A> *str, UReg *errorCode)
{
	{
		AddrSize assertTIsOneByte[sizeof(T) == 1 ? 1 : 0];
		assertTIsOneByte;
	}
	U8 *workBytes;
	U16 numBytes;

	if (!stream.Read(&numBytes, errorCode)) return false;
	if (numBytes == 0)
	{
		*str = std::basic_string<T, S, A>();
	}
	else
	{
		workBytes = (U8*)stack_alloc(numBytes+1);
		if (!stream.Read(workBytes, numBytes, errorCode)) 
			return false;
		U16 numChars = numBytes;
		reinterpret_cast<T*>(workBytes)[numChars] = T(0);
		*str = reinterpret_cast<const T*>(workBytes);
	}
	return true;
}

template <typename S, typename A>
bool InputStream::ReadStringHelper<wchar_t, S, A>::Read(stream::InputStream &stream, std::basic_string<wchar_t, S, A> *str, UReg *errorCode)
{
	{
#if defined(RAD_OPT_4BYTE_WCHAR)
		U32 assertWCharIsFourBytes[sizeof(wchar_t) == 4 ? 1 : 0];
		assertWCharIsFourBytes;
#else
		U32 assertWCharIsTwoBytes[sizeof(wchar_t) == 2 ? 1 : 0];
		assertWCharIsTwoBytes;
#endif
	}
	U8 *workBytes;
	U16 numChars;
	if (!stream.Read(&numChars, errorCode)) 
		return false;
#if defined(RAD_OPT_4BYTE_WCHAR)
	workBytes = (U8*)stack_alloc(numChars*4+4);
#else
	workBytes = (U8*)stack_alloc(numChars*2+2);
#endif
	if (stream.Read(workBytes, numChars*2, errorCode) != numChars*2) 
		return false;
	reinterpret_cast<U16*>(workBytes)[numChars] = 0;
	stream.InByteSwapWideChars(reinterpret_cast<U16*>(workBytes));

#if defined(RAD_OPT_4BYTE_WCHAR)
	{
		const U16 *src = reinterpret_cast<const U16*>(workBytes + numChars * 2 - 2);
		wchar_t *dst   = reinterpret_cast<wchar_t*>(workBytes + numChars * 4 - 4);
		for (U16 i = 0; i < numChars; ++i)
		{
			*dst = static_cast<wchar_t>(*src);
			--dst;
			--src;
		}
		RAD_ASSERT((void*)dst == (void*)workBytes);
		reinterpret_cast<wchar_t*>(workBytes)[numChars] = 0; // null
	}
#endif

	*str = reinterpret_cast<const wchar_t*>(workBytes);
	return true;
}

template <typename T, typename S, typename A>
inline bool InputStream::Read(std::basic_string<T, S, A> *str, UReg *errorCode)
{
	RAD_ASSERT(str);
	return ReadStringHelper<T, S, A>::Read(*this, str, errorCode);
}

template<typename T>
inline InputStream& InputStream::StreamType(T& var)// throw(ReadException)
{
	UReg err;
	if (!Read(&var, &err)) 
		throw ReadException(err);
	return *this;
}

inline InputStream& InputStream::operator >> (S8& var)//  throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (U8& var)//  throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (S16& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (U16& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (S32& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (U32& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (S64& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (U64& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (F32& var)// throw(ReadException)
{
	return StreamType(var);
}

inline InputStream& InputStream::operator >> (F64& var)// throw(ReadException)
{
	return StreamType(var);
}

template <typename T, typename S, typename A>
inline InputStream& InputStream::operator >> (std::basic_string<T, S, A> &str)// throw(ReadException)
{
	UReg errorCode;
	if (!ReadStringHelper<T, S, A>::Read(*this, &str, &errorCode)) 
		throw ReadException(errorCode);
	return *this;
}

inline OutputStream::OutputStream(IOutputBuffer &buff) : m_buff(0)
{
	SetBuffer(buff);
}

inline OutputStream::OutputStream() : m_buff(0)
{
}

inline OutputStream::~OutputStream()
{
}

inline void OutputStream::SetBuffer(IOutputBuffer &buff)
{
	m_buff = &buff;
}

inline IOutputBuffer &OutputStream::Buffer()
{
	RAD_ASSERT(m_buff);
	return *m_buff;
}

inline UReg OutputStream::OutCaps() const
{
	RAD_ASSERT(m_buff);
	return m_buff->OutCaps();
}

inline UReg OutputStream::OutStatus() const
{
	RAD_ASSERT(m_buff);
	return m_buff->OutStatus();
}

inline SPos OutputStream::Write(const void* buff, SPos numBytes, UReg* errorCode)
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(OutStatus()&StatusOutputOpen);
	if (numBytes > 0) 
		return m_buff->Write(buff, numBytes, errorCode);
	return 0;
}

inline bool OutputStream::SeekOut(Seek seekType, SPos ofs, UReg* errorCode)
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(OutStatus()&StatusOutputOpen);
	return m_buff->SeekOut(seekType, ofs, errorCode);
}

inline SPos OutputStream::OutPos() const
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(OutStatus()&StatusOutputOpen);
	return m_buff->OutPos();
}

inline void OutputStream::Flush()
{
	RAD_ASSERT(m_buff);
	RAD_ASSERT(OutStatus()&StatusOutputOpen);
	m_buff->Flush();
}

inline bool OutputStream::Write(const S8& var, UReg* errorCode)
{
	return Write(&var, sizeof(S8), errorCode) == sizeof(S8);
}

inline bool OutputStream::Write(const U8& var, UReg* errorCode)
{
	return Write(&var, sizeof(U8), errorCode) == sizeof(U8);
}

inline bool OutputStream::Write(const S16& var, UReg* errorCode)
{
	return Write(&var, sizeof(S16), errorCode) == sizeof(S16);
}

inline bool OutputStream::Write(const U16& var, UReg* errorCode)
{
	return Write(&var, sizeof(U16), errorCode) == sizeof(U16);
}

inline bool OutputStream::Write(const S32& var, UReg* errorCode)
{
	return Write(&var, sizeof(S32), errorCode) == sizeof(S32);
}

inline bool OutputStream::Write(const U32& var, UReg* errorCode)
{
	return Write(&var, sizeof(U32), errorCode) == sizeof(U32);
}

inline bool OutputStream::Write(const S64& var, UReg* errorCode)
{
	return Write(&var, sizeof(S64), errorCode) == sizeof(S64);
}

inline bool OutputStream::Write(const U64& var, UReg* errorCode)
{
	return Write(&var, sizeof(U64), errorCode) == sizeof(U64);
}

inline bool OutputStream::Write(const F32& var, UReg* errorCode)
{
	return Write(&var, sizeof(F32), errorCode) == sizeof(F32);
}

inline bool OutputStream::Write(const F64& var, UReg* errorCode)
{
	return Write(&var, sizeof(F64), errorCode) == sizeof(F64);
}

template <typename TChar>
bool OutputStream::WriteStringHelper<TChar>::Write(OutputStream &stream, const TChar *str, UReg *errorCode)
{
	{
		AddrSize assertTIsOneByte[sizeof(TChar) == 1 ? 1 : 0];
		assertTIsOneByte;
	}
	RAD_ASSERT(str);
	size_t len = string::len(str);
	RAD_VERIFY(len <= (size_t)std::numeric_limits<U16>::max());
	U16 numBytes = (U16)len;
	if (!stream.Write(numBytes, errorCode)) 
		return false;
	return stream.Write(str, (SPos)len, errorCode) == (SPos)len;
}

template <typename TChar>
inline bool OutputStream::Write(const TChar *str, UReg *errorCode)
{
	return WriteStringHelper<TChar>::Write(*this, str, errorCode);
}

template<typename T>
inline OutputStream& OutputStream::StreamType(T& var)// throw(WriteException)
{
	UReg err;
	if (!Write(var, &err)) 
		throw WriteException(err);
	return *this;
}

inline OutputStream& OutputStream::operator << (const S8& var)//  throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const U8& var)//  throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const S16& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const U16& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const S32& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const U32& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const S64& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const U64& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const F32& var)// throw(WriteException)
{
	return StreamType(var);
}

inline OutputStream& OutputStream::operator << (const F64& var)// throw(WriteException)
{
	return StreamType(var);
}

template <typename TChar>
inline OutputStream& OutputStream::operator << (const TChar *str)// throw(WriteException)
{
	UReg errorCode;
	if (!WriteStringHelper<TChar>::Write(*this, str, &errorCode)) throw WriteException(errorCode);
	return *this;
}

} // stream

#include "../PopSystemMacros.h"
