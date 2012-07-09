// FileStream.cpp
// Platform Agnostic File System Stream
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "FileStream.h"

using namespace stream;


namespace file {
namespace stream {

::stream::SPos InputBuffer::Read(void* buff, ::stream::SPos numBytes, UReg* errorCode)
{
	RAD_ASSERT(m_file||m_fp);
	RAD_ASSERT(m_pos <= Size());
	FPos read = 0;

	if (m_file)
	{
		Result r = m_file->Read(buff, (FPos)numBytes, &read, (FPos)m_pos, 0); // synchronous read.
		if (r == file::Success || r == file::ErrorPartial)
		{
			SetErrorCode(errorCode, (r==file::Success) ? ::stream::Success : ::stream::ErrorUnderflow);
			m_pos += read;
			return (SPos)read;
		}
	}
	else
	{
		fseek(m_fp, m_pos, SEEK_SET);
		read = (SPos)fread(buff, 1, numBytes, m_fp);
		SetErrorCode(errorCode, (read<numBytes) ? ::stream::Success : ::stream::ErrorUnderflow);
		m_pos += read;
		return read;
	}

	SetErrorCode(errorCode, ::stream::ErrorGeneric);
	return (SPos)read;
}

bool InputBuffer::SeekIn(::stream::Seek seekType, ::stream::SPos ofs, UReg* errorCode)
{
	bool b = CalcSeekPos(seekType, ofs, m_pos, Size(), &ofs);
	if (b)
	{
		m_pos = ofs;
		SetErrorCode(errorCode, ::stream::Success);
	}
	else
	{
		SetErrorCode(errorCode, ::stream::ErrorBadSeekPos);
	}

	return b;
}

::stream::SPos InputBuffer::Size()  const
{
	RAD_ASSERT(m_file||m_fp);

	if (m_file)
	{
		return m_file->Size();
	}
	
	size_t x = ftell(m_fp);
	fseek(m_fp, 0, SEEK_END);
	size_t s = ftell(m_fp);
	fseek(m_fp, (long)x, SEEK_SET);
	return (::stream::SPos)s;
}

::stream::SPos OutputBuffer::Write(const void* buff, ::stream::SPos numBytes, UReg* errorCode)
{
	RAD_ASSERT(m_file||m_fp);
	RAD_ASSERT(m_pos <= Size());
	FPos write = 0;

	if (m_file)
	{
		Result r = m_file->Write(buff, (FPos)numBytes, &write, (FPos)m_pos, 0); // synchronous write.
		if (r == file::Success || r == file::ErrorPartial)
		{
			SetErrorCode(errorCode, (r==file::Success) ? ::stream::Success : ::stream::ErrorUnderflow);
			m_pos += write;
			return (SPos)write;
		}
	}
	else
	{
		fseek(m_fp, m_pos, SEEK_SET);
		write = (SPos)fwrite(buff, 1, numBytes, m_fp);
		SetErrorCode(errorCode, (write<numBytes) ? ::stream::ErrorUnderflow : ::stream::Success );
		m_pos += write;
		return write;
	}

	SetErrorCode(errorCode, ::stream::ErrorGeneric);
	return (SPos)write;
}

bool OutputBuffer::SeekOut(::stream::Seek seekType, ::stream::SPos ofs, UReg* errorCode)
{
	bool b = CalcSeekPos(seekType, ofs, m_pos, Size(), &ofs);
	if (b)
	{
		m_pos = ofs;
		SetErrorCode(errorCode, ::stream::Success);
	}
	else
	{
		SetErrorCode(errorCode, ::stream::ErrorBadSeekPos);
	}

	return b;
}

::stream::SPos OutputBuffer::Size()  const
{
	RAD_ASSERT(m_file||m_fp);

	if (m_file)
	{
		return m_file->Size();
	}
	
	size_t x = ftell(m_fp);
	fseek(m_fp, 0, SEEK_END);
	size_t s = ftell(m_fp);
	fseek(m_fp, (long)x, SEEK_SET);
	return (::stream::SPos)s;
}

} // stream
} // file

