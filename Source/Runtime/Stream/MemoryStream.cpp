// MemoryStream.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "MemoryStream.h"


namespace stream {

SPos MemInputBuffer::Read(void* buff, SPos numBytes, UReg* errorCode)
{
	RAD_ASSERT(buff);
	RAD_ASSERT(m_ptr);

	SPos readSize = std::min(m_size-m_pos, numBytes);
	if (readSize)
	{
		memcpy(buff, ((const U8*)m_ptr) + m_pos, readSize);
		m_pos += readSize;
	}

	SetErrorCode(errorCode, (readSize<numBytes) ? ErrorUnderflow : Success);
	return readSize;
}

bool MemInputBuffer::SeekIn(Seek seekType, SPos ofs, UReg* errorCode)
{
	bool b = CalcSeekPos(seekType, ofs, m_pos, m_size, &ofs);

	if (b)
	{
		m_pos = ofs;
		SetErrorCode(errorCode, Success);
	}
	else
	{
		SetErrorCode(errorCode, ErrorBadSeekPos);
	}

	return b;
}

SPos FixedMemOutputBuffer::Write(const void* buff, SPos numBytes, UReg* errorCode)
{
	RAD_ASSERT(buff);
	RAD_ASSERT(m_ptr);

	SPos writeSize = std::min(m_size-m_pos, numBytes);

	if (writeSize)
	{
		memcpy(((U8*)m_ptr) + m_pos, buff, writeSize);
		m_pos += writeSize;
	}

	SetErrorCode(errorCode, (writeSize<numBytes) ? ErrorOverflow : Success);
	return writeSize;
}

bool FixedMemOutputBuffer::SeekOut(Seek seekType, SPos ofs, UReg* errorCode)
{
	if (CalcSeekPos(seekType, ofs, m_pos, m_size, &ofs))
	{
		m_pos = ofs;
		SetErrorCode(errorCode, Success);
	}
	else
	{
		SetErrorCode(errorCode, ErrorBadSeekPos);
	}

	return (ofs == m_pos);
}

SPos DynamicMemOutputBuffer::Write(const void* buff, SPos numBytes, UReg* out_errorCode)
{
	UReg errorCode = ErrorOverflow;
	SPos writeSize = (m_buff.Ptr()) ? m_buff.Write(buff, numBytes, &errorCode) : 0;
	if ((writeSize < numBytes))
	{
		if (errorCode == ErrorOverflow)
		{
			if (!Resize((m_buff.Size() + (numBytes-writeSize)) * 2))
			{
				SetErrorCode(out_errorCode, ErrorOverflow);
				return writeSize;
			}
						
			// write the rest.
			writeSize = m_buff.Write(((const U8*)buff) + writeSize, numBytes - writeSize, out_errorCode) + writeSize;
			RAD_ASSERT(writeSize == numBytes);
			errorCode = Success;
		}
	}
	else
	{
		errorCode = Success;
	}

	SetErrorCode(out_errorCode, errorCode);
	return writeSize;
}

void DynamicMemOutputBuffer::FreeMemory()
{
	FixedMemOutputBuffer &buff = OutputBuffer();
	if (buff.Ptr())
	{
		zone_free(buff.Ptr());
		buff.Set(0, 0);
	}
}

bool DynamicMemOutputBuffer::Resize(SPos requestedSize)
{
	FixedMemOutputBuffer &buff = OutputBuffer();
	void* ptr = zone_realloc(m_zone, buff.Ptr(), requestedSize, 0, m_align);
	buff.Set(ptr, ptr ? requestedSize : 0);
	return ptr != 0;
}

} // stream

