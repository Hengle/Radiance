// FileStream.h
// Platform Agnostic File System Stream
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "File.h"
#include "FileStreamDef.h"
#include "../Stream.h"
#include "../PushPack.h"
#include <stdio.h>


namespace file {
namespace stream {

class RADRT_CLASS InputBuffer : public ::stream::IInputBuffer
{
public:

	InputBuffer();
	InputBuffer(file::File &file);
	InputBuffer(FILE *fp);
	virtual ~InputBuffer();

	void SetFile(file::File &file);
	void SetFile(FILE *fp);
	file::File &File() const;
	FILE *FilePtr() const;

	::stream::SPos Read(void* buff, ::stream::SPos numBytes, UReg* errorCode);

	bool SeekIn(::stream::Seek seekType, ::stream::SPos ofs, UReg* errorCode) ;

	::stream::SPos InPos() const;
	::stream::SPos Size()  const;

	UReg InCaps() const;
	UReg InStatus() const;

private:

	FILE *m_fp;
	file::File *m_file;
	::stream::SPos m_pos;
};

class RADRT_CLASS OutputBuffer : public ::stream::IOutputBuffer
{
public:

	OutputBuffer();
	OutputBuffer(file::File &file);
	OutputBuffer(FILE *fp);
	virtual ~OutputBuffer();

	void SetFile(file::File &file);
	void SetFile(FILE *fp);
	file::File &File() const;
	FILE *FilePtr() const;

	::stream::SPos Write(const void* buff, ::stream::SPos numBytes, UReg* errorCode);

	bool SeekOut(::stream::Seek seekType, ::stream::SPos ofs, UReg* errorCode);

	::stream::SPos OutPos() const;
	void Flush();

	UReg OutCaps() const;
	UReg OutStatus() const;

private:

	::stream::SPos Size()  const;

	FILE *m_fp;
	file::File *m_file;
	::stream::SPos m_pos;
};

} // stream
} // file


#include "../PopPack.h"
#include "FileStream.inl"
