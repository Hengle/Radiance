// MemoryStream.inl
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.


namespace stream {

inline MemInputBuffer::MemInputBuffer() : m_ptr(0), m_size(0), m_pos(0)
{
}

inline MemInputBuffer::MemInputBuffer(const void* buff, SPos size) : m_ptr(buff), m_size(size), m_pos(0)
{
	RAD_ASSERT(buff && size);
}

inline MemInputBuffer::~MemInputBuffer()
{
}

inline void MemInputBuffer::Set(const void* buff, SPos size)
{
	m_ptr = buff;
	m_size = size;
}

inline SPos MemInputBuffer::InPos() const
{
	return m_pos;
}

inline SPos MemInputBuffer::Size()  const
{
	return m_size;
}

inline UReg MemInputBuffer::InCaps() const
{
	return CapSeekInput | CapSizeInput;
}

inline UReg MemInputBuffer::InStatus() const
{
	return (m_ptr) ? StatusInputOpen : 0;
}

inline const void* MemInputBuffer::Ptr() const
{
	return m_ptr;
}

inline FixedMemOutputBuffer::FixedMemOutputBuffer() : m_ptr(0), m_size(0), m_pos(0)
{
}

inline FixedMemOutputBuffer::FixedMemOutputBuffer(void* buff, SPos size) : m_ptr(buff), m_size(size), m_pos(0)
{
}

inline FixedMemOutputBuffer::~FixedMemOutputBuffer()
{
}

inline void FixedMemOutputBuffer::Set(void* buff, SPos size)
{
	m_ptr = buff;
	m_size = size;
}

inline SPos FixedMemOutputBuffer::OutPos() const
{
	return m_pos;
}

inline void FixedMemOutputBuffer::Flush()
{
}

inline UReg FixedMemOutputBuffer::OutCaps() const
{
	return (m_ptr) ? CapSeekOutput : 0;
}

inline UReg FixedMemOutputBuffer::OutStatus() const
{
	return (m_ptr) ? StatusOutputOpen : 0;
}

inline void* FixedMemOutputBuffer::Ptr() const
{
	return m_ptr;
}

inline SPos FixedMemOutputBuffer::Size() const
{
	return m_size;
}

inline DynamicMemOutputBuffer::DynamicMemOutputBuffer(Zone &zone, AddrSize alignment) :
m_zone(zone), m_align(alignment)
{
}

inline DynamicMemOutputBuffer::~DynamicMemOutputBuffer()
{
}

inline FixedMemOutputBuffer &DynamicMemOutputBuffer::OutputBuffer()
{
	return m_buff;
}

inline SPos DynamicMemOutputBuffer::Size() const
{
	return m_buff.Size();
}

inline bool DynamicMemOutputBuffer::SeekOut(Seek seekType, SPos ofs, UReg* errorCode)
{
	return m_buff.SeekOut(seekType, ofs, errorCode);
}

inline SPos DynamicMemOutputBuffer::OutPos() const
{
	return m_buff.OutPos();
}

inline void DynamicMemOutputBuffer::Flush()
{
	return m_buff.Flush();
}

inline UReg DynamicMemOutputBuffer::OutCaps() const
{
	return CapSeekOutput;
}

inline UReg DynamicMemOutputBuffer::OutStatus() const
{
	return StatusOutputOpen;
}

} // stream

