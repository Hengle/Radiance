// LmpReader.h
// LMP file format reader. The LMP format is simply a set of named lumps of data. The
// format of the lumps (i.e. what they are) is entirely up to the application using/creating
// the data. The LMP API's allow for the creation of the lump file.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntDataCodec.h"
#include "LmpDef.h"
#include "../StreamDef.h"
#include <functional>

#include "../PushPack.h"


namespace data_codec {
namespace lmp {

enum EndianMode
{
	LittleEndian,
	BigEndian
};

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::lmp::StreamReader
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS StreamReader
{
public:
	StreamReader();
	~StreamReader();

	// After LoadLumpInfo() the stream is no longer needed. The input stream cannot be adapted
	// because data is loaded in one large block (so endian-ness cannot be handled with
	// an endian stream like the Writer can be). Use the EndianMode instead.
	bool LoadLumpInfo(U32 sig, U32 magic, stream::InputStream &stream, EndianMode endianMode);
	void Free();

	//////////////////////////////////////////////////////////////////////////////////////////
	// data_codec::lmp::Reader::Lump
	//////////////////////////////////////////////////////////////////////////////////////////
	class RADRT_CLASS Lump
	{
	public:
		Lump();
		~Lump();
		const char* Name() const;
		LOfs Size() const;
		LOfs Ofs() const;
		LOfs TagSize() const;
		U32  Idx() const;

		// LumpTagAlignment bytes aligned, if you need more alignment you have to pad it manually.
		const void* TagData() const;

	private:
		Lump(const Lump&);
		Lump& operator = (const Lump&);

		struct Info
		{
			LOfs ofs;
			LOfs size;
			LOfs tagSize;
		};

		Info* m_info;
		const char* m_name;
		const void* m_tagData;
		U32 m_idx;

		friend class StreamReader;
	};

	U32 NumLumps() const;
	const Lump* GetByIndex(U32 i) const;

	// case sensitive.
	const Lump* GetByName(const char* name) const;

private:

	struct FindLumpFn : public std::binary_function<const char*, Lump&, bool>
	{
		bool operator() (const char* e1, Lump& e2);
		bool operator() (Lump& e1, const char* e2);
		bool operator() (Lump& e1, Lump& e2);
	};

	void* m_lumpData;
	U32 m_numLumps;
	Lump* m_lumps;

	void SetupLumps(EndianMode endianMode);
};

} // lmp
} // data_codec


#include "../PopPack.h"
