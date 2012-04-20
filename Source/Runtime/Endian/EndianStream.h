// EndianStream.h
// Endian Streams.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntEndian.h"
#include "../Stream.h"
#include "../PushPack.h"


namespace stream {

//////////////////////////////////////////////////////////////////////////////////////////
// endian::EndianSwapStream
//////////////////////////////////////////////////////////////////////////////////////////

class EndianSwapInputStream : public InputStream
{
public:

	EndianSwapInputStream();
	EndianSwapInputStream(IInputBuffer &b);
	virtual ~EndianSwapInputStream();

	bool Read(S8*  var, UReg* errorCode = 0);
	bool Read(U8*  var, UReg* errorCode = 0);
	bool Read(S16* var, UReg* errorCode = 0);
	bool Read(U16* var, UReg* errorCode = 0);
	bool Read(S32* var, UReg* errorCode = 0);
	bool Read(U32* var, UReg* errorCode = 0);
	bool Read(S64* var, UReg* errorCode = 0);
	bool Read(U64* var, UReg* errorCode = 0);
	bool Read(F32* var, UReg* errorCode = 0);
	bool Read(F64* var, UReg* errorCode = 0);

protected:

	virtual void InByteSwapWideChars(U16 *chars);
};

class EndianSwapOutputStream : public OutputStream
{
public:

	EndianSwapOutputStream();
	EndianSwapOutputStream(IOutputBuffer &b);
	virtual ~EndianSwapOutputStream();

	bool Write(const S8&  var, UReg* errorCode = 0);
	bool Write(const U8&  var, UReg* errorCode = 0);
	bool Write(const S16& var, UReg* errorCode = 0);
	bool Write(const U16& var, UReg* errorCode = 0);
	bool Write(const S32& var, UReg* errorCode = 0);
	bool Write(const U32& var, UReg* errorCode = 0);
	bool Write(const S64& var, UReg* errorCode = 0);
	bool Write(const U64& var, UReg* errorCode = 0);
	bool Write(const F32& var, UReg* errorCode = 0);
	bool Write(const F64& var, UReg* errorCode = 0);

protected:

	virtual void OutByteSwapWideChars(U16 *chars);
};

//////////////////////////////////////////////////////////////////////////////////////////
// endian::LittleStream
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_BIG_ENDIAN)
	typedef EndianSwapInputStream LittleInputStream;
	typedef EndianSwapOutputStream LittleOutputStream;
#else
	typedef InputStream LittleInputStream;
	typedef OutputStream LittleOutputStream;
#endif

//////////////////////////////////////////////////////////////////////////////////////////
// endian::BigStream
//////////////////////////////////////////////////////////////////////////////////////////

#if defined(RAD_OPT_LITTLE_ENDIAN)
	typedef EndianSwapInputStream BigInputStream;
	typedef EndianSwapOutputStream BigOutputStream;
#else
	typedef InputStream BigInputStream;
	typedef OutputStream BigOutputStream;
#endif

} // stream


#include "../PopPack.h"
#include "EndianStream.inl"
