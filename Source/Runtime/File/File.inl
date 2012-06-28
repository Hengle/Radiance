// File.inl
// Copyright (c) 2012 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace file {

FileSystem::~FileSystem() {
}

inline void FileSystem::setAlias(
	const char *name,
	const char *path
) {
	RAD_ASSERT(name);
	RAD_ASSERT(path);
	RAD_ASSERT(name[1] == 0); // only single characters allowed!
	m_aliasTable[name[0]] = path;
}

inline String FileSystem::alias(const char *name) {
	RAD_ASSERT(name);
	RAD_ASSERT(name[1] == 0); // only single characters allowed!
	return m_aliasTable[name[0]];
}

inline int FileSystem::RAD_IMPLEMENT_GET(globalMask) {
	return m_globalMask;
}

inline void FileSystem::RAD_IMPLEMENT_SET(globalMask) (int value) {
	m_globalMask = value;
}

} // file
