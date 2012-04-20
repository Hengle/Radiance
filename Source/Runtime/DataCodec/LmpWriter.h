// LmpWriter.h
// LMP file format writer. The LMP format is simply a set of named lumps of data. The
// format of the lumps (i.e. what they are) is entirely up to the application using/creating
// the data. The LMP API's allow for the creation of the lump file.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "IntDataCodec.h"
#include "LmpDef.h"
#include "../StreamDef.h"
#include "../String.h"
#include "../Container/ZoneVector.h"
#include <functional>

#include "../PushPack.h"


namespace data_codec {
namespace lmp {

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::lmp::Writer
//////////////////////////////////////////////////////////////////////////////////////////

class RADRT_CLASS Writer
{
public:
	typedef string::string<> String;

	Writer();
	~Writer();

	//////////////////////////////////////////////////////////////////////////////////////////
	// data_codec::lmp::Writer::Lump
	//////////////////////////////////////////////////////////////////////////////////////////
	class RADRT_CLASS Lump
	{
	public:
		Lump();
		~Lump();
		const char* Name() const;
		LOfs Size() const;
		LOfs Ofs() const;
		LOfs TagDataSize() const;

		void* AllocateTagData(LOfs size);
		void* TagData() const;

	private:
		Lump(const Lump&);
		Lump& operator = (const Lump&);

		Lump* m_next;
		String m_name;
		LOfs m_size;
		LOfs m_ofs;
		LOfs m_tagSize;
		void* m_tagData;

		friend class Writer;
	};

	// The output stream can be an adapted endian stream.
	bool Begin(U32 sig, U32 magic, stream::OutputStream &stream);
	Lump* WriteLump(const char* name, const void* data, AddrSize dataSize, LOfs alignment);
	U32 NumLumps() const;

	// Must be called before End()!
	void SortLumps();
	// Cannot be called before SortLumps(). Case sensitive.
	Lump* LumpForName(const char* name);
	// Ends the file.
	bool End();

private:

	typedef zone_vector<Lump*, ZRuntimeT>::type LumpList;

	stream::OutputStream* m_stream;
	Lump* m_lumpList;
	U32 m_lumpCount;
	void AddLump(Lump* e);
	void FreeLumps();

	RAD_DEBUG_ONLY(bool m_active);

	struct SortLumpsFn : public std::binary_function<Lump*, Lump*, bool>
	{
		bool operator() (Lump* e1, Lump* e2);
	};

	struct FindLumpFn : public std::binary_function<String&, Lump*, bool>
	{
		bool operator() (const String& e1, Lump* e2);
		bool operator() (Lump* e1, const String& e2);
		bool operator() (Lump* e1, Lump* e2);
	};

	static String& LumpName(Lump* l);

	LumpList m_lumps;
};

} // lmp
} // data_codec


#include "../PopPack.h"
