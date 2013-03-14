// Stream.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntStream.h"
#include "StreamDef.h"
#include "../StringDef.h"
#include "../PushPack.h"

namespace stream {

//////////////////////////////////////////////////////////////////////////////////////////
// stream::Exception
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Exception : public exception
{
public:
	Exception(UReg code = ErrorGeneric);
	Exception(const Exception& e);
	virtual ~Exception() throw();

	UReg ErrorCode() const;

protected:

	void SetErrorCode(UReg code);

private:

	UReg m_code;
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::ReadException
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS ReadException : public Exception
{
public:
	ReadException(UReg code = ErrorGeneric);
	virtual ~ReadException() throw();
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::WriteException
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS WriteException : public Exception
{
public:
	WriteException(UReg code = ErrorGeneric);
	virtual ~WriteException() throw();
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::IInputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS IInputBuffer
{
public:
	IInputBuffer();
	virtual ~IInputBuffer();

	//
	// Read (returns # of bytes read). If there was no error, the bytes read will equal the
	// requested number. If the number of bytes returned is less that the requested number,
	// then ErrorUnderflow is returned (and may be a fatal error depending on the context).
	//
	// Zero length reads are legal.
	//
	// The error code parameter can be null. Authors of an input buffer
	// can use the SetErrorCode() function which handles a null errorCode pointer.
	//
	virtual SPos Read(void* buff, SPos numBytes, UReg* errorCode) = 0;
	// note: if StreamEnd is specified, offset is interpreted as a negative number!
	// All other seek types can only accept positive offsets!
	virtual bool SeekIn(Seek seekType, SPos ofs, UReg* errorCode) = 0;
	virtual SPos InPos() const = 0;
	virtual SPos Size() const = 0;

	virtual UReg InCaps() const = 0;
	virtual UReg InStatus() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::IOutputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS IOutputBuffer
{
public:
	IOutputBuffer();
	virtual ~IOutputBuffer();

	//
	// Write (returns # of bytes written). If there was no error, the bytes written will equal the
	// requested number. If the number of bytes returned is less that the requested number,
	// then ErrorOverflow is returned (and may be a fatal error depending on the context).
	//
	// Zero length writes are legal.
	//
	// The error code parameter can be null. Authors of an output buffer
	// can use the SetErrorCode() function which handles a null errorCode pointer.
	//
	virtual SPos Write(const void* buff, SPos numBytes, UReg* errorCode) = 0;
	// note: if StreamEnd is specified, offset is interpreted as a negative number!
	// All other seek types can only accept positive offsets!
	virtual bool SeekOut(Seek seekType, SPos ofs, UReg* errorCode) = 0;
	virtual SPos OutPos() const = 0;
	virtual void Flush() = 0;

	virtual UReg OutCaps() const = 0;
	virtual UReg OutStatus() const = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::IPipeTarget
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS IPipeTarget
{
public:
	IPipeTarget();
	virtual ~IPipeTarget();

	//
	// Return: Success on success, or an appropriate error code otherwise.
	//
	virtual UReg operator () (const void* buffer, SPos size) = 0;
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::InputStream
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS InputStream
{
public:
	InputStream();
	InputStream(IInputBuffer &buff);
	virtual ~InputStream();

	//
	// Read (returns # of bytes read). If there was no error, the bytes read will equal the
	// requested number. If the number of bytes returned is less that the requested number,
	// then ErrorUnderflow is returned (and may be a fatal error depending on the context).
	//
	// Zero length reads are legal.
	//
	// The error code parameter can be null.
	//
	virtual SPos Read(void* buff, SPos numBytes, UReg* errorCode);
	// pipe to a target (returns Success on success, otherwise an ERROR_* code.
	UReg PipeToTarget(IPipeTarget& pipeTarget, void *tempBuffer, SPos tempBufferSize, SPos* numBytesPiped, SPos numBytesToPipe, PipeFlags pipeFlags);
	// pipe to a stream.
	UReg PipeToStream(OutputStream& outputStream, SPos* numBytesPiped, SPos numBytesToPipe, PipeFlags pipeFlags);

	// note: if StreamEnd is specified, offset is interpreted as a negative number!
	// All other seek types can only accept positive offsets!
	bool SeekIn(Seek seekType, SPos ofs, UReg* errorCode);
	SPos InPos() const;
	SPos Size() const;

	UReg InCaps() const;
	UReg InStatus() const;

	virtual bool Read(S8*  var, UReg* errorCode = 0);
	virtual bool Read(U8*  var, UReg* errorCode = 0);
	virtual bool Read(S16* var, UReg* errorCode = 0);
	virtual bool Read(U16* var, UReg* errorCode = 0);
	virtual bool Read(S32* var, UReg* errorCode = 0);
	virtual bool Read(U32* var, UReg* errorCode = 0);
	virtual bool Read(S64* var, UReg* errorCode = 0);
	virtual bool Read(U64* var, UReg* errorCode = 0);
	virtual bool Read(F32* var, UReg* errorCode = 0);
	virtual bool Read(F64* var, UReg* errorCode = 0);

	bool Read(string::String *str, UReg *errorCode = 0);

	// >> operators.
	InputStream &operator >> (S8& var);  // throw(ReadException);
	InputStream &operator >> (U8& var);  // throw(ReadException);
	InputStream &operator >> (S16& var); // throw(ReadException);
	InputStream &operator >> (U16& var); // throw(ReadException);22
	InputStream &operator >> (S32& var); // throw(ReadException);
	InputStream &operator >> (U32& var); // throw(ReadException);
	InputStream &operator >> (S64& var); // throw(ReadException);
	InputStream &operator >> (U64& var); // throw(ReadException);
	InputStream &operator >> (F32& var); // throw(ReadException);
	InputStream &operator >> (F64& var); // throw(ReadException);
	InputStream &operator >> (string::String &str); // throw(ReadException);

	IInputBuffer &Buffer();
	void SetBuffer(IInputBuffer &buff);

protected:

	virtual void InByteSwapWideChars(U16 *chars) {}

private:

	InputStream(const InputStream&);
	InputStream& operator = (const InputStream&);

	template<typename T>
	InputStream& StreamType(T& var);// throw(ReadException);

	IInputBuffer* m_buff;
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::OutputStream
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS OutputStream
{
public:
	OutputStream();
	OutputStream(IOutputBuffer &buff);
	virtual ~OutputStream();

	//
	// write. If there was no error, the bytes written will equal the
	// requested number. If the number of bytes returned is less that the requested number,
	// then ErrorOverflow is returned (and may be a fatal error depending on the context).
	//
	// Zero length writes are legal.
	//
	// The error code parameter can be null.
	//
	virtual SPos Write(const void* buff, SPos numBytes, UReg* errorCode);
	// note: if StreamEnd is specified, offset is interpreted as a negative number!
	// All other seek types can only accept positive offsets!
	bool SeekOut(Seek seekType, SPos ofs, UReg* errorCode);
	SPos OutPos() const;
	void Flush();

	UReg OutCaps() const;
	UReg OutStatus() const;

	virtual bool Write(const S8&  var, UReg* errorCode = 0);
	virtual bool Write(const U8&  var, UReg* errorCode = 0);
	virtual bool Write(const S16& var, UReg* errorCode = 0);
	virtual bool Write(const U16& var, UReg* errorCode = 0);
	virtual bool Write(const S32& var, UReg* errorCode = 0);
	virtual bool Write(const U32& var, UReg* errorCode = 0);
	virtual bool Write(const S64& var, UReg* errorCode = 0);
	virtual bool Write(const U64& var, UReg* errorCode = 0);
	virtual bool Write(const F32& var, UReg* errorCode = 0);
	virtual bool Write(const F64& var, UReg* errorCode = 0);
	
	bool Write(const char *sz, UReg *errorCode = 0);
	bool Write(const string::String &str, UReg *errorCode = 0);

	// << operators.
	OutputStream &operator << (const S8& var);  // throw(WriteException);
	OutputStream &operator << (const U8& var);  // throw(WriteException);
	OutputStream &operator << (const S16& var); // throw(WriteException);
	OutputStream &operator << (const U16& var); // throw(WriteException);
	OutputStream &operator << (const S32& var); // throw(WriteException);
	OutputStream &operator << (const U32& var); // throw(WriteException);
	OutputStream &operator << (const S64& var); // throw(WriteException);
	OutputStream &operator << (const U64& var); // throw(WriteException);
	OutputStream &operator << (const F32& var); // throw(WriteException);
	OutputStream &operator << (const F64& var); // throw(WriteException);
	OutputStream &operator << (const string::String &str); // throw(WriteException);
	OutputStream &operator << (const char *sz);

	IOutputBuffer &Buffer();
	void SetBuffer(IOutputBuffer &buff);

protected:

	virtual void OutByteSwapWideChars(U16 *chars) {}

private:

	OutputStream(const OutputStream&);
	OutputStream& operator = (const OutputStream&);

	template<typename T>
	OutputStream& StreamType(T& var);// throw(WriteException);

	IOutputBuffer* m_buff;
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::SetErrorCode()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API void RADRT_CALL SetErrorCode(UReg* outCode, UReg code);

//////////////////////////////////////////////////////////////////////////////////////////
// stream::CalcSeekPos()
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL CalcSeekPos(Seek seekType, SPos dstOfs, SPos curPos, SPos size, SPos* newOfs);

} // stream


#include "../PopPack.h"
#include "Stream.inl"
