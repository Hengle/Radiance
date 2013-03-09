// MemoryStream.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once
#include "Stream.h"
#include "../PushPack.h"

namespace stream {

class RADRT_CLASS MemInputBuffer : public IInputBuffer
{
public:
	MemInputBuffer();
	MemInputBuffer(const void* buff, stream::SPos size);
	virtual ~MemInputBuffer();

	void Set(const void* buff, stream::SPos size);
	const void* Ptr() const;

	stream::SPos Read(void* buff, stream::SPos numBytes, UReg* errorCode);
	// note: if STREAM_END is specified, offset is interpreted as a negative number!
	bool SeekIn(stream::Seek seekType, stream::SPos ofs, UReg* errorCode);
	stream::SPos InPos() const;
	stream::SPos Size()  const;
	UReg InCaps() const;
	UReg InStatus() const;

private:

	const void* m_ptr;
	stream::SPos m_size;
	stream::SPos m_pos;
};

class RADRT_CLASS FixedMemOutputBuffer : public stream::IOutputBuffer
{
public:
	FixedMemOutputBuffer();
	FixedMemOutputBuffer(void* buff, stream::SPos size);
	virtual ~FixedMemOutputBuffer();

	void Set(void* buff, stream::SPos size);
	void* Ptr() const;
	stream::SPos Size() const;

	stream::SPos Write(const void* buff, stream::SPos numBytes, UReg* errorCode);
	// note: if STREAM_END is specified, offset is interpreted as a negative number!
	bool SeekOut(stream::Seek seekType, stream::SPos ofs, UReg* errorCode);
	stream::SPos OutPos() const;
	void Flush();

	UReg OutCaps() const;
	UReg OutStatus() const;

private:

	void* m_ptr;
	stream::SPos m_size;
	stream::SPos m_pos;
};

class RADRT_CLASS DynamicMemOutputBuffer : public stream::IOutputBuffer
{
public:
	DynamicMemOutputBuffer(Zone &zone, AddrSize alignment=kDefaultAlignment);
	virtual ~DynamicMemOutputBuffer();

	stream::SPos Size() const;

	stream::SPos Write(const void* buff, stream::SPos numBytes, UReg* errorCode);
	// note: if STREAM_END is specified, offset is interpreted as a negative number!
	bool SeekOut(stream::Seek seekType, stream::SPos ofs, UReg* errorCode);
	stream::SPos OutPos() const;
	void Flush();

	UReg OutCaps() const;
	UReg OutStatus() const;
	
	// will release memory, and reset stream for use.
	void FreeMemory();

	FixedMemOutputBuffer &OutputBuffer();

protected:

	bool Resize(stream::SPos requestedSize);

private:

	Zone &m_zone;
	AddrSize m_align;
	FixedMemOutputBuffer m_buff;
};

} // stream


#include "../PopPack.h"

#include "MemoryStream.inl"
