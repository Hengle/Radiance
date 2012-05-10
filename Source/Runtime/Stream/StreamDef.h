// StreamDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntStream.h"
#include "../PushPack.h"


namespace stream {

//////////////////////////////////////////////////////////////////////////////////////////
// Stream interfaces.
//////////////////////////////////////////////////////////////////////////////////////////

typedef U32 SPos;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::Error
//////////////////////////////////////////////////////////////////////////////////////////

enum Result
{
	Success,
	ErrorGeneric,
	ErrorUnderflow,
	ErrorOverflow,
	ErrorBadSeekPos,
	ErrorMallocFailed
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::Exception
//////////////////////////////////////////////////////////////////////////////////////////

class Exception;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::ReadException
//////////////////////////////////////////////////////////////////////////////////////////

class ReadException;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::WriteException
//////////////////////////////////////////////////////////////////////////////////////////

class WriteException;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::Seek
//////////////////////////////////////////////////////////////////////////////////////////

enum Seek
{
	StreamBegin,
	StreamCur,
	StreamEnd
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::Caps
//////////////////////////////////////////////////////////////////////////////////////////

enum Caps
{
	RAD_FLAG(CapSeekInput),
	RAD_FLAG(CapSizeInput),
	RAD_FLAG(CapSeekOutput)
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::Status
//////////////////////////////////////////////////////////////////////////////////////////

enum Status
{
	RAD_FLAG(StatusInputOpen),
	RAD_FLAG(StatusOutputOpen),
	STATUS_ALL_OPEN = StatusInputOpen | StatusOutputOpen
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::IInputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

class IInputBuffer;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::IOutputBuffer
//////////////////////////////////////////////////////////////////////////////////////////

class IOutputBuffer;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::PipeFlags
//////////////////////////////////////////////////////////////////////////////////////////

enum PipeFlags
{
	PipeAll,
	PipeSize
};

//////////////////////////////////////////////////////////////////////////////////////////
// stream::IPipeTarget
//////////////////////////////////////////////////////////////////////////////////////////

class IPipeTarget;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::InputStream
//////////////////////////////////////////////////////////////////////////////////////////

class InputStream;

//////////////////////////////////////////////////////////////////////////////////////////
// stream::OutputStream
//////////////////////////////////////////////////////////////////////////////////////////

class OutputStream;

} // stream


#include "../PopPack.h"