// EndianStream.inl
// Endian Streams.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Endian.h"


namespace stream {

inline EndianSwapInputStream::EndianSwapInputStream()
{
}

inline EndianSwapInputStream::EndianSwapInputStream(IInputBuffer &b) : InputStream(b)
{
}

inline EndianSwapInputStream::~EndianSwapInputStream()
{
}

inline bool EndianSwapInputStream::Read(S8*  var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(U8*  var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(S16* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(U16* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(S32* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(U32* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(S64* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(U64* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(F32* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline bool EndianSwapInputStream::Read(F64* var, UReg* errorCode)
{
	bool s;
	if( s=InputStream::Read(var, errorCode) )
	{
		*var = endian::Swap(*var);
	}

	return s;
}

inline void EndianSwapInputStream::InByteSwapWideChars(U16 *chars)
{
	while (*chars)
	{
		*chars = endian::Swap(*chars);
		++chars;
	}
}

inline EndianSwapOutputStream::EndianSwapOutputStream()
{
}

inline EndianSwapOutputStream::EndianSwapOutputStream(IOutputBuffer &b) : OutputStream(b)
{
}

inline EndianSwapOutputStream::~EndianSwapOutputStream()
{
}

inline bool EndianSwapOutputStream::Write(const S8& var, UReg* errorCode)
{
	S8 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const U8& var, UReg* errorCode)
{
	U8 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const S16& var, UReg* errorCode)
{
	S16 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const U16& var, UReg* errorCode)
{
	U16 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const S32& var, UReg* errorCode)
{
	S32 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const U32& var, UReg* errorCode)
{
	U32 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const S64& var, UReg* errorCode)
{
	S64 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const U64& var, UReg* errorCode)
{
	U64 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const F32& var, UReg* errorCode)
{
	F32 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline bool EndianSwapOutputStream::Write(const F64& var, UReg* errorCode)
{
	F64 swap = endian::Swap(var);
	return OutputStream::Write(swap, errorCode);	
}

inline void EndianSwapOutputStream::OutByteSwapWideChars(U16 *chars)
{
	while (*chars)
	{
		*chars = endian::Swap(*chars);
		++chars;
	}
}

} // stream

