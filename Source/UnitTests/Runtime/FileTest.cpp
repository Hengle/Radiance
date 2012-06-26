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
#if defined(RAD_OPT_WINX)
        const char *s_root = "c:";
        const char *s_data = "../../Source/UnitTests/Data";
#else
        const char *s_root = "/";
        const char *s_data = "../../Source/UnitTests/Data";
#endif

	void FileSearch()
	{
		Search fs;
		if (fs.Open(s_root, ".*", SearchFlags(Recursive|FileNames|NativePath)))
		{
			enum { MAXFILES = 100 };
			int c = 0;
			std::cout << "Searching '" << s_root << "' (*.*) (first " << MAXFILES << ")..." << std::endl;

			char name[MaxFilePathLen+1];
			while (++c <= MAXFILES && fs.NextFile(name, MaxFilePathLen+1, 0, 0))
			{
				std::wcout << name << std::endl;
			}
		}
		else
		{
			FAIL(-1, "Failed to open file search!");
		}
	}

	void AsyncTest(const char *filename, const char *save)
	{
		char path[MaxFilePathLen+1];
		cpy(path, s_data);
		cat(path, "/");
		cat(path, filename);

		File file;
		FPos ioBytes;

		if (file.Open(path, OpenExisting, AccessRead, ShareRead, FileOptions(Async|NativePath), 0) != Success)
		{
			FAIL(-1, "Error opening %ls for read!", path);
		}

		AsyncIO io;
		FPos size = file.Size();
		FPos alignSize;

		void *data = file.SafeIOMalloc(size, &alignSize);

		std::cout << "Streaming " << size << " byte(s) from file '" << path << "'..." << std::endl;
		SecondsTimer<> t;
		t.Start();

		Result r = file.Read(data, alignSize, &ioBytes, 0, 0);
		if (r != Success && r != ErrorPartial && r != Pending)
		{
			FAIL(-1, "Failed with error code %d", r);
		}
		t.Stop();
		std::wcout << "Streamed " << ioBytes << " byte(s) in " << t.Elapsed() << " second(s)." << std::endl;
		file.Close();

		if (ioBytes < size)
		{
			FAIL(-1, "ErrorPartial!");
		}

		if (save)
		{
			cpy(path, s_data);
			cat(path, "/");
			cat(path, save);

			if (file.Open(path, CreateAlways, AccessWrite, ShareMode(0), FileOptions(Async|NativePath), 0) != Success)
			{
				FAIL(-1, "Error opening %ls for write!", path);
			}

			std::wcout << "Streaming " << size << " byte(s) to file '" << path << "'..." << std::endl;
			t.Start();

			r = file.Write(data, size, &ioBytes, 0, 0);
			if (r != Success)
			{
				FAIL(-1, "Write failed, error code %d", r);
			}
			t.Stop();
			std::wcout << "Streamed " << ioBytes << " byte(s) in " << t.Elapsed() << " second(s)." << std::endl;

			file.Close();

			if (ioBytes < size)
			{
				FAIL(-1, "ErrorPartial!");
			}
		}

		File::IOFree(data);
	}
}
	void FileTest()
	{
        Begin("File Test");
#if defined(RAD_OPT_DEBUG)
        EnforcePortablePaths(false);
#endif
        DO(FileSearch());
        DO(AsyncTest("big.zip", "out.zip"));
	}
}
