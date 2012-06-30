// StringTest.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../UTCommon.h"
#include <Runtime/File.h>

using namespace file;
using namespace xtime;
using namespace string;
using namespace stream;

namespace ut
{
namespace
{
	enum {
		kMaxSearchFiles = Kilo
	};

	void FileSearch(const FileSystem::Ref &fs) {
		std::cout << "Searchin @r:/*.*" << std::endl;
		FileSearch::Ref search = fs->openSearch("@r:/*.*");
		if (!search)
			return;
		int c = 0;
		String path;
		while (search->nextFile(path) && (c++ < kMaxSearchFiles)) {
			std::cout << path << std::endl;
		}
	}

	void FileCopyTest(const FileSystem::Ref &fs, const char *szSrc, const char *szDst) {
		MMFile::Ref src = fs->openFile(szSrc, kFileOption_MapEntireFile);
		if (!src)
			return;
		FILE *dst = fs->fopen(szDst, "wb");

		MMFileInputBuffer ib(src);
		FILEOutputBuffer ob(dst);

		InputStream is(ib);
		OutputStream os(ob);

		is.PipeToStream(
			os,
			0,
			0,
			PipeAll
		);
		
		fclose(dst);
	}

	void PakFileTest(const FileSystem::Ref &fs, const char *path) {
		PakFile::Ref pak = fs->openPakFile(path);
		if (!pak)
			return;
		const int kNumLumps = pak->numLumps;
		std::cout << "Num Lumps: " << kNumLumps << std::endl;
		for (int i = 0; i < kNumLumps; ++i) {
			std::cout << pak->lumpForIndex(i)->Name() << std::endl;
		}
	}
}
	void FileTest() {
        Begin("File Test");
		FileSystem::Ref fs = FileSystem::create();
        //DO(FileSearch(fs));
        DO(FileCopyTest(fs, "@r:/pak0.pak", "@r:/pak0.copy"));
		DO(PakFileTest(fs, "@r:/pak0.pak"));
	}
}
