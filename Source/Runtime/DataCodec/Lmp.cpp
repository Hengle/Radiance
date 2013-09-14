// Lmp.cpp
// Lmp file format API.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "IntDataCodec.h"
#include "LmpWriter.h"
#include "LmpReader.h"
#include "../Stream.h"
#include "../Endian.h"
#include "../StringBase.h"
#include "../Utils.h"

#include <algorithm>

namespace data_codec {
namespace lmp {

RAD_ZONE_DEC(RADRT_API, ZLmp);
RAD_ZONE_DEF(RADRT_API, ZLmp, "LuMP", ZRuntime);

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::lmp::Writer
//////////////////////////////////////////////////////////////////////////////////////////

inline Writer::Lump::Lump() : m_next(0), m_size(0), m_ofs(0), m_tagSize(0), m_tagData(0)
{
}

inline Writer::Lump::~Lump()
{
	if (m_tagData)
	{
		zone_free(m_tagData);
	}
}

const char* Writer::Lump::Name() const
{
	return m_name.c_str;
}

LOfs Writer::Lump::Size() const
{
	return m_size;
}

LOfs Writer::Lump::Ofs() const
{
	return m_ofs;
}

LOfs Writer::Lump::TagDataSize() const
{
	return m_tagSize;
}

void* Writer::Lump::AllocateTagData(LOfs size)
{
	RAD_ASSERT(m_tagData==0);
	RAD_ASSERT(size);

	m_tagData = zone_malloc(ZLmp, size, 0);
	if (m_tagData) m_tagSize = size;
	return m_tagData;
}

void* Writer::Lump::TagData() const
{
	return m_tagData;
}

Writer::Writer() : m_lumpList(0), m_lumpCount(0)
#if defined(RAD_OPT_DEBUG)
, m_active(false)
#endif
{
}

Writer::~Writer()
{
	FreeLumps();
}

bool Writer::Begin(U32 sig, U32 magic, stream::OutputStream &stream)
{
	RAD_ASSERT_MSG(!m_active, "lmp is being created! Please End() this lmp writer before Begin()'ing!");
	m_stream = &stream;

	RAD_DEBUG_ONLY(m_active = true);

	// write the header
	if (!m_stream->Write(sig, 0) ||
		!m_stream->Write(magic, 0)) 
		return false;

	RAD_ASSERT(m_lumpCount == 0);
	RAD_ASSERT(m_lumpList == 0);

	return true;
}

Writer::Lump* Writer::WriteLump(const char* name, const void* data, AddrSize dataSize, LOfs alignment)
{
	RAD_ASSERT_MSG(m_active, "Please Begin() this lmp writer before writing entries!");
	RAD_ASSERT(m_stream->OutPos() < kMaxLOfs);
	RAD_ASSERT((alignment > 0) && IsPowerOf2(alignment));

	if (string::len(name) > kMaxLumpNameLen)
	{
		return 0;
	}

	Lump* l = new (ZLmp) Lump();
	l->m_name = name;
	l->m_size = (LOfs)dataSize;
	l->m_ofs  = (LOfs)m_stream->OutPos();

	if (dataSize)
	{
		int bits = (l->m_ofs) & (alignment-1);
		if (bits) // need to align this.
		{
			alignment -= bits; // number of bytes we need to write.
			const int NUM_BYTES = 5;
			U8 bytes[NUM_BYTES] = {'A', 'L', 'I', 'G', 'N'};
			LOfs rep = alignment / NUM_BYTES;
			while (rep--)
			{
				if (m_stream->Write(bytes, 5, 0) != 5)
				{
					delete l;
					return 0;
				}
				alignment -= NUM_BYTES;
			}
			if (alignment)
			{
				if (m_stream->Write(bytes, (stream::SPos)alignment, 0) != (stream::SPos)alignment)
				{
					delete l;
					return 0;
				}
			}
		}

		l->m_ofs = (LOfs)m_stream->OutPos();

		if (m_stream->Write(data, (stream::SPos)dataSize, 0) != (stream::SPos)dataSize)
		{
			delete l;
			return 0;
		}
	}

	AddLump(l);
	return l;
}

Writer::String& Writer::LumpName(Lump* l)
{
	return l->m_name;
}

inline bool Writer::SortLumpsFn::operator() (Lump* e1, Lump* e2)
{
	return Writer::LumpName(e1) <= Writer::LumpName(e2);
}

inline bool Writer::FindLumpFn::operator() (const String& e1, Lump* e2)
{
	return e1 <= Writer::LumpName(e2);
}

inline bool Writer::FindLumpFn::operator() (Lump* e1, const String& e2)
{
	return Writer::LumpName(e1) <= e2;
}

inline bool Writer::FindLumpFn::operator() (Lump* e1, Lump* e2)
{
	return Writer::LumpName(e1) <= Writer::LumpName(e2);
}

void Writer::SortLumps()
{
	m_lumps.clear();
	m_lumps.reserve(m_lumpCount);
	for (Lump* l = m_lumpList; l; l = l->m_next)
	{
		m_lumps.push_back(l);
	}
	std::sort(m_lumps.begin(), m_lumps.end(), SortLumpsFn());
}

Writer::Lump* Writer::LumpForName(const char* name)
{
	RAD_ASSERT(name);
	if (m_lumps.empty()) 
		return 0;
	String s(name);
	LumpList::iterator it = std::upper_bound(m_lumps.begin(), m_lumps.end(), s, FindLumpFn());
	if (it != m_lumps.begin()) 
		--it;
	if ((*it)->m_name == s) 
		return *it;
	return 0;
}

bool Writer::End()
{
	U8 nulls[kLumpTagAlignment-1];
	memset(nulls, 0, kLumpTagAlignment-1);

	RAD_ASSERT(m_stream->OutPos() <= kMaxLOfs);
	LOfs dirStart = (LOfs)m_stream->OutPos();

	// write out how many lumps there are...
	if (!m_stream->Write(m_lumpCount, 0)) 
		return false;

	// write information about each lump and keep track of where it is...
	for (LumpList::iterator it = m_lumps.begin(); it != m_lumps.end(); ++it)
	{
		Lump* l = *it;

		if(!m_stream->Write(l->m_ofs, 0) ||   // offset (4 bytes)
		   !m_stream->Write(l->m_size, 0) ||  // size (4 bytes)
		   !m_stream->Write(l->m_tagSize, 0)) // tagSize (4 bytes)
		{
			return false;
		}

		U32 nameLen = (U32)l->m_name.numBytes+1;
		RAD_ASSERT(nameLen <= kMaxU16);

		if (!m_stream->Write((U16)nameLen, 0) ||  // name length (including null) (2 bytes)
			(m_stream->Write(l->m_name.c_str.get(), nameLen, 0) != nameLen))   // name data (plus null)
		{
			return false;
		}

		nameLen += 14; // the 14 bytes of header we wrote before the name.

		U32 aligned = (nameLen + (kLumpTagAlignment-1)) & ~(kLumpTagAlignment-1);

		// pad to LumpTagAlignment alignment!
		if (nameLen != aligned)
		{
			if (m_stream->Write(nulls, aligned - nameLen, 0) != (aligned - nameLen)) 
				return false;
		}

		if (l->m_tagSize)
		{
			if (m_stream->Write(l->m_tagData, l->m_tagSize, 0) != l->m_tagSize) 
				return false;

			aligned = (l->m_tagSize + (kLumpTagAlignment-1)) & ~(kLumpTagAlignment-1);

			// pad to LumpTagAlignment alignment!
			if (l->m_tagSize != aligned)
			{
				if (m_stream->Write(nulls, aligned - l->m_tagSize, 0) != (aligned - l->m_tagSize)) 
					return false;
			}
		}
	}

	// store the directory offset.
	if (!m_stream->Write(dirStart, 0)) 
		return false;

	FreeLumps();

	return true;
}

void Writer::AddLump(Lump* l)
{
	l->m_next = m_lumpList;
	m_lumpList = l;
	m_lumpCount++;
}

void Writer::FreeLumps()
{
	while (m_lumpList)
	{
		Lump* next = m_lumpList->m_next;
		delete m_lumpList;
		m_lumpList = next;
	}

	m_lumpCount = 0;

	m_lumps.clear();
	STLContainerShrinkToSize(m_lumps);
}

U32 Writer::NumLumps() const
{
	return m_lumpCount;
}

//////////////////////////////////////////////////////////////////////////////////////////
// data_codec::lmp::StreamReader
//////////////////////////////////////////////////////////////////////////////////////////

StreamReader::Lump::Lump() : m_info(0), m_name(0), m_tagData(0), m_idx(kMaxU32)
{
}

StreamReader::Lump::~Lump()
{
}

const char* StreamReader::Lump::Name() const
{
	return m_name;
}

LOfs StreamReader::Lump::Size() const
{
	RAD_ASSERT(m_info);
	return m_info->size;
}

LOfs StreamReader::Lump::Ofs() const
{
	RAD_ASSERT(m_info);
	return m_info->ofs;
}

U32 StreamReader::Lump::Idx() const
{
	return m_idx;
}

LOfs StreamReader::Lump::TagSize() const
{
	RAD_ASSERT(m_info);
	return m_info->tagSize;
}

const void* StreamReader::Lump::TagData() const
{
	return m_tagData;
}

StreamReader::StreamReader() : m_lumpData(0), m_numLumps(0), m_lumps(0)
{
}

StreamReader::~StreamReader()
{
	Free();
}

// After LoadDir() the stream is no longer needed.
bool StreamReader::LoadLumpInfo(U32 sig, U32 magic, stream::InputStream &stream, EndianMode endianMode)
{
	RAD_ASSERT(m_lumpData == 0);
	RAD_ASSERT(m_lumps == 0);
	RAD_ASSERT(m_numLumps == 0);

	if (!stream.SeekIn(stream::StreamBegin, 0, 0)) 
		return false;

	U32 mysig, mymagic;

	if (!stream.Read(&mysig, 0) ||
		!stream.Read(&mymagic, 0)) 
		return false;

#if defined(RAD_OPT_LITTLE_ENDIAN)
	if (endianMode == BigEndian)
#else
	if (endianMode == LittleEndian)
#endif
	{
		mysig = endian::Swap(mysig);
		mymagic = endian::Swap(mymagic);
	}

	if (mysig != sig || mymagic != magic) 
		return false;

	if (!stream.SeekIn(stream::StreamEnd, sizeof(LOfs), 0)) 
		return false;

	LOfs dirEnd = (LOfs)stream.InPos();

	// find out where the directory is.
	LOfs dirOfs;
	if (!stream.Read(&dirOfs, 0)) 
		return false;

#if defined(RAD_OPT_LITTLE_ENDIAN)
	if (endianMode == BigEndian)
#else
	if (endianMode == LittleEndian)
#endif
	{
		dirOfs = endian::Swap(dirOfs);
	}

	if (!stream.SeekIn(stream::StreamBegin, dirOfs, 0)) 
		return false;

	// read the number of lumps.

	if (!stream.Read(&m_numLumps, 0)) 
		return false;

#if defined(RAD_OPT_LITTLE_ENDIAN)
	if (endianMode == BigEndian)
#else
	if (endianMode == LittleEndian)
#endif
	{
		m_numLumps = endian::Swap(m_numLumps);
	}

	LOfs dirStart = (LOfs)stream.InPos();
	LOfs dirSize = dirEnd - dirStart;

	if (dirSize < 13) 
		return false; // smallest directory possible

	m_lumpData = zone_malloc(ZLmp, dirSize, 0);
	if (!m_lumpData) 
		return false;

	// load it.
	if (stream.Read(m_lumpData, dirSize, 0) != dirSize)
	{
		zone_free(m_lumpData);
		m_lumpData = 0;
		return false;
	}

	m_lumps = (Lump*)zone_malloc(ZLmp, sizeof(Lump)*m_numLumps, 0);
	if (!m_lumps)
	{
		zone_free(m_lumpData);
		m_lumpData = 0;
		return false;
	}

	SetupLumps(endianMode);

	return true;
}

void StreamReader::SetupLumps(EndianMode endianMode)
{
	RAD_STATIC_ASSERT(sizeof(Lump::Info)==12);

	U8* rawLump = (U8*)m_lumpData;
	U16 nameLen;
	LOfs alignedTagSize;

	for (U32 i = 0; i < m_numLumps; i++)
	{
		Lump* lump = &m_lumps[i];

		lump->m_info = (Lump::Info*)rawLump;
		nameLen = *((U16*)(rawLump+12));

#if defined(RAD_OPT_LITTLE_ENDIAN)
		if (endianMode == BigEndian)
#else
		if (endianMode == LittleEndian)
#endif
		{
			lump->m_info->size = endian::Swap(lump->m_info->size);
			lump->m_info->ofs = endian::Swap(lump->m_info->ofs);
			lump->m_info->tagSize = endian::Swap(lump->m_info->tagSize);

			nameLen = endian::Swap(nameLen);
		}

		lump->m_name = (const char*)(rawLump + 14);

		// aligned to LumpTagAlignment bytes....
		nameLen = (nameLen + 14 + (kLumpTagAlignment-1)) & ~(kLumpTagAlignment-1);
		rawLump += nameLen;

		alignedTagSize = (lump->m_info->tagSize + (kLumpTagAlignment-1)) & ~(kLumpTagAlignment-1);

		if (alignedTagSize)
		{
			lump->m_tagData = rawLump;
			rawLump += alignedTagSize;
		}
		else
		{
			lump->m_tagData = 0;
		}

		lump->m_idx = i;
	}
}

void StreamReader::Free()
{
	if (m_lumpData)
	{
		zone_free(m_lumpData);
		m_lumpData = 0;
	}
	if (m_lumps)
	{
		zone_free(m_lumps);
		m_lumps = 0;
		m_numLumps = 0;
	}
}

U32 StreamReader::NumLumps() const
{
	return m_numLumps;
}

const StreamReader::Lump* StreamReader::GetByIndex(U32 i) const
{
	if (i < m_numLumps)
		return &m_lumps[i];
	return 0;
}

inline bool StreamReader::FindLumpFn::operator() (const char* e1, Lump& e2)
{
	return string::cmp(e1, e2.Name()) < 0;
}

inline bool StreamReader::FindLumpFn::operator() (Lump& e1, const char* e2)
{
	return string::cmp(e1.Name(), e2) < 0;
}

inline bool StreamReader::FindLumpFn::operator() (Lump& e1, Lump& e2)
{
	return string::cmp(e1.Name(), e2.Name()) < 0;
}

// case sensitive.
const StreamReader::Lump* StreamReader::GetByName(const char* name) const
{
	RAD_ASSERT(name&&name[0]);

	if (m_numLumps == 0)
		return 0;

	Lump* start = m_lumps;
	Lump* end   = m_lumps+m_numLumps;

	Lump* l = std::upper_bound(start, end, name, FindLumpFn());
	if (l != start)
		--l;
	if (!string::cmp(l->m_name, name))
		return l;

	return 0;
}

} // lmp
} // data_codec

