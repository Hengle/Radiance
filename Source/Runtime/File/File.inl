// File.inl
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../PushSystemMacros.h"

namespace file {

inline FileSystem::~FileSystem() {
}

inline String FileSystem::Alias(char name) {
	return m_aliasTable[name];
}

inline int FileSystem::RAD_IMPLEMENT_GET(globalMask) {
	return m_globalMask;
}

inline void FileSystem::RAD_IMPLEMENT_SET(globalMask) (int value) {
	m_globalMask = value;
}

///////////////////////////////////////////////////////////////////////////////

inline MMFile::MMFile() {
}

///////////////////////////////////////////////////////////////////////////////

inline MMapping::MMapping(
	const void *data, 
	AddrSize size, 
	AddrSize offset, 
	AddrSize backingSize,
	::Zone &zone
) : m_data(data), m_size(size), m_offset(offset), m_backingSize(backingSize), m_zone(zone) {
	m_zone.Inc(backingSize, 0);
}

inline MMapping::~MMapping() {
	m_zone.Dec(m_backingSize, 0);
}

inline const void *MMapping::RAD_IMPLEMENT_GET(data) {
	return m_data;
}

inline AddrSize MMapping::RAD_IMPLEMENT_GET(size) {
	return m_size;
}

inline AddrSize MMapping::RAD_IMPLEMENT_GET(offset) {
	return m_offset;
}

///////////////////////////////////////////////////////////////////////////////

inline FileSearch::FileSearch() {
}

///////////////////////////////////////////////////////////////////////////////

inline stream::SPos MMFileInputBuffer::InPos() const {
	return m_pos;
}

inline stream::SPos MMFileInputBuffer::Size()  const {
	return (stream::SPos)m_file->size.get();
}

inline UReg MMFileInputBuffer::InCaps() const {
	return stream::CapSeekInput | stream::CapSizeInput;
}

inline UReg MMFileInputBuffer::InStatus() const {
	return m_file ? stream::StatusInputOpen : 0;
}

///////////////////////////////////////////////////////////////////////////////

inline stream::SPos FILEInputBuffer::InPos() const {
	return m_pos;
}

inline UReg FILEInputBuffer::InCaps() const {
	return stream::CapSeekInput|stream::CapSizeInput;
}

inline UReg FILEInputBuffer::InStatus() const {
	return m_fp ? stream::StatusInputOpen : 0;
}

inline FILE *FILEInputBuffer::RAD_IMPLEMENT_GET(fp) {
	return m_fp;
}

///////////////////////////////////////////////////////////////////////////////

inline stream::SPos FILEOutputBuffer::OutPos() const {
	return m_pos;
}

inline void FILEOutputBuffer::Flush() {
	fflush(m_fp);
}

inline UReg FILEOutputBuffer::OutCaps() const {
	return stream::CapSeekOutput;
}

inline UReg FILEOutputBuffer::OutStatus() const {
	return m_fp ? stream::StatusOutputOpen : 0;
}

inline FILE *FILEOutputBuffer::RAD_IMPLEMENT_GET(fp) {
	return m_fp;
}

} // file

#include "../PopSystemMacros.h"
