/*! \file null_CloudStorage.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#include <Engine/Persistence.h>

bool CloudStorage::Enabled() {
	return false;
}

CloudFile::Vec CloudStorage::Resolve(const char *name) {
	return CloudFile::Vec();
}

void CloudStorage::ResolveConflict(const CloudFile::Ref &version) {
}

bool CloudStorage::StartDownloadingLatestVersion(const char *name) {
	return true;
}

CloudFile::Status CloudStorage::FileStatus(const char *name) {
	return CloudFile::Ready;
}
