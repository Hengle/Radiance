// StringTest.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../UTCommon.h"
#include <Runtime/File.h>

using namespace file;
using namespace xtime;
using namespace string;

namespace ut
{
namespace
{
	void FileSearch(const FileSystem::Ref &fs) {
		std::cout << "Searchin @r:/*.*" << std::endl;
		FileSearch::Ref search = fs->openSearch("@r:/*.*");
		if (!search)
			return;
		String path;
		while (search->nextFile(path)) {
			std::cout << path << std::endl;
		}
	}

	void FileCopyTest(const FileSystem::Ref &fs, const char *src, const char *dst) {
		
	}
}
	void FileTest() {
        Begin("File Test");
		FileSystem::Ref fs = FileSystem::create();
        DO(FileSearch(fs));
        DO(FileCopyTest(fs, "pak0.pak", "pak0.copy"));
	}
}
