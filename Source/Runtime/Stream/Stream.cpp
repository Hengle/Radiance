// Stream.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Stream.h"
#include "../Utils.h"
#include "../String.h"
#include <stdlib.h>

namespace stream {

bool InputStream::Read(string::String *str, UReg *errorCode) {
	RAD_ASSERT(str);
	S32 l;
	if (!Read(&l, errorCode))
		return false;
	*str = string::String((int)l);
	return Read(const_cast<char*>(str->c_str.get()), (SPos)l, errorCode) == (SPos)l;
}

bool OutputStream::Write(const string::String &str, UReg *errorCode) {
	SPos l = (SPos)str.length.get();
	if (!Write((U32)l))
		return false;
	return Write(str.c_str.get(), l, errorCode) == l;
}

bool OutputStream::Write(const char *sz, UReg *errorCode) {
	return Write(CStr(sz), errorCode);
}

//////////////////////////////////////////////////////////////////////////////////////////
// Calculate position
//////////////////////////////////////////////////////////////////////////////////////////

RADRT_API bool RADRT_CALL CalcSeekPos(Seek seekType, SPos dstOfs, SPos curPos, SPos size, SPos* newOfs)
{
	RAD_ASSERT(seekType==StreamBegin||seekType==StreamCur||seekType==StreamEnd);
	RAD_ASSERT(newOfs);

	switch (seekType)
	{
	//case StreamBegin: *newOfs = dstOfs; break;
	case StreamCur: dstOfs += curPos; break;
	case StreamEnd: if (size < dstOfs) return false; dstOfs = size - dstOfs; break;
	}

	*newOfs = dstOfs;
	return dstOfs <= size;
}

//////////////////////////////////////////////////////////////////////////////////////////
// stream::InputStream::PipeToStream()
//////////////////////////////////////////////////////////////////////////////////////////
// pipe to a buffer.

class Pump : public IPipeTarget
{
public:
	UReg operator () (const void* buff, SPos size)
	{
		UReg errorCode;
		out->Write(buff, size, &errorCode);
		return errorCode;
	}

	OutputStream* out;
};

UReg InputStream::PipeToStream(OutputStream& outputStream, SPos* bytesRead, SPos numBytesToPipe, PipeFlags pipeFlags)
{
	const int SIZE = 256;
	S8 bytes[SIZE];
	Pump np;

	np.out = &outputStream;
	return PipeToTarget(np, bytes, SIZE, bytesRead, numBytesToPipe, pipeFlags);
}

//////////////////////////////////////////////////////////////////////////////////////////
// stream::InputStream::PipeToTarget()
//////////////////////////////////////////////////////////////////////////////////////////

UReg InputStream::PipeToTarget(IPipeTarget& pipeTarget, void* tempBuffer, SPos tempBufferSize, SPos* pbytesRead, SPos numBytesToPipe, PipeFlags pipeFlags)
{
	RAD_ASSERT(tempBuffer);
	RAD_ASSERT(pipeFlags == PipeAll || pipeFlags == PipeSize);
	RAD_ASSERT_MSG(!numBytesToPipe || (pipeFlags == PipeSize), "numBytesToPipe must be Zero if PipeAll is specified");

	// pipe until a read error or we hit our size.
	SPos bytesRead, readSize, totalRead;
	UReg errorCode = Success;

	if (pipeFlags == PipeAll)
		numBytesToPipe = tempBufferSize;

	bytesRead = totalRead = 0;

	for (;;)
	{
		readSize = (numBytesToPipe < tempBufferSize) ? numBytesToPipe : tempBufferSize;
		if (!readSize)
			break;

		bytesRead = Read(tempBuffer, readSize, &errorCode);
		totalRead += bytesRead;

		if (pipeFlags == PipeSize)
		{
			if (bytesRead != readSize)
				break;
			numBytesToPipe -= readSize;
		}
		else
		{
			if (bytesRead)
			{
				UReg err2 = pipeTarget(tempBuffer, bytesRead);
				if (err2 != Success)
				{
					errorCode = err2;
					break;
				}
			}
			else
			{
				break;
			}
		}
	}

	if (pbytesRead) 
		*pbytesRead = totalRead;

	if (pipeFlags == PipeAll && errorCode == ErrorUnderflow)
		errorCode = Success;

	return errorCode;
}

} // stream

