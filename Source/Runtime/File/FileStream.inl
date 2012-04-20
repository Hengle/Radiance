// FileStream.inl
// Platform Agnostic File System Stream
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.


namespace file {
namespace stream {

//////////////////////////////////////////////////////////////////////////////////////////
// file::::stream::InputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

inline InputBuffer::InputBuffer() : m_file(0), m_fp(0), m_pos(0)
{
}

inline InputBuffer::InputBuffer(file::File &file) : m_file(&file), m_fp(0), m_pos(0)
{
}

inline InputBuffer::InputBuffer(FILE *file) : m_file(0), m_fp(file), m_pos(0)
{
}

inline InputBuffer::~InputBuffer()
{
}

inline void InputBuffer::SetFile(file::File &file)
{
	m_fp = 0;
	m_file = &file;
}

inline void InputBuffer::SetFile(FILE *fp)
{
	m_file = 0;
	m_fp = fp;
}

inline file::File &InputBuffer::File() const
{
	RAD_ASSERT(m_file);
	return *m_file;
}

inline FILE *InputBuffer::FilePtr() const
{
	return m_fp;
}

inline ::stream::SPos InputBuffer::InPos() const
{
	RAD_ASSERT(m_file||m_fp);
	return m_pos;
}

inline UReg InputBuffer::InCaps() const
{
	return ::stream::CapSeekInput|::stream::CapSizeInput;
}

inline UReg InputBuffer::InStatus() const
{
	return (m_file||m_fp) ? ::stream::StatusInputOpen : 0;
}

//////////////////////////////////////////////////////////////////////////////////////////
// file::::stream::OutputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

inline OutputBuffer::OutputBuffer() : m_file(0), m_fp(0), m_pos(0)
{
}

inline OutputBuffer::OutputBuffer(file::File &file) : m_file(&file), m_fp(0), m_pos(0)
{
}

inline OutputBuffer::OutputBuffer(FILE *file) : m_file(0), m_fp(file), m_pos(0)
{
}

inline OutputBuffer::~OutputBuffer()
{
}

inline void OutputBuffer::SetFile(file::File &file)
{
	m_fp = 0;
	m_file = &file;
}

inline void OutputBuffer::SetFile(FILE *file)
{
	m_file = 0;
	m_fp = file;
}

inline file::File &OutputBuffer::File() const
{
	RAD_ASSERT(m_file);
	return *m_file;
}

inline FILE *OutputBuffer::FilePtr() const
{
	return m_fp;
}

inline ::stream::SPos OutputBuffer::OutPos() const
{
	RAD_ASSERT(m_file||m_fp);
	return m_pos;
}

inline void OutputBuffer::Flush()
{
	RAD_ASSERT(m_file||m_fp);
	if (m_file)
	{
		m_file->Flush();
	}
	else
	{
		fflush(m_fp);
	}
}

inline UReg OutputBuffer::OutCaps() const
{
	RAD_ASSERT(m_file||m_fp);
	return ::stream::CapSeekOutput;
}

inline UReg OutputBuffer::OutStatus() const
{
	return (m_file||m_fp) ? ::stream::StatusOutputOpen : 0;
}

} // stream
} // file

