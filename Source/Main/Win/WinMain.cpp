/*! \file WinMain.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup Main
*/

#include RADPCH
#if !defined(RAD_OPT_SHIP)
#include <VLD/vld.h>
#endif

#include <string>
#include <vector>

#if defined(RAD_OPT_PC_TOOLS)
#include "../QtAppMain.h"
#include <process.h>

namespace {

std::string ExeName() {
	char name[256];
	int x = ::GetModuleFileNameA(0, name, 255);
	name[x] = 0; // terminate.
	return name;
}

bool IsDotCom() {
	std::string x = ExeName();
	return x.length() > 4 && !::stricmp(&x[x.length()-4], ".com");
}

void SpawnSelf() {
	char str[256];
	::strcpy(str, ExeName().c_str());
	str[::strlen(str)-4] = 0;
	::strcat(str, ".exe");
	::_spawnl(_P_DETACH, str, str, NULL);
}
}

#endif

namespace {

class Argcv {
public:

	Argcv(LPSTR lpCmdLine) {

		// splits strings by spaces, unless the are quoted
		std::string s;
		char quoteStyle;
		bool inQuote = false;

		for (int i = 0; lpCmdLine[i]; ++i) {
			if (!inQuote && lpCmdLine[i] == ' ') {
				if (!s.empty()) {
					m_storage.push_back(s);
					s.clear();
				}
			} else if (!inQuote && lpCmdLine[i] == '\'' || lpCmdLine[i] == '"') {
				quoteStyle = lpCmdLine[i];
				inQuote = true;
			} else if (inQuote && lpCmdLine[i] == quoteStyle) {
				inQuote = false;
				// quotes always go in even if empty
				m_storage.push_back(s);
				s.clear();
			} else {
				s += lpCmdLine[i];
			}
		}

		// unterminated empty quotes don't go in.
		if (!s.empty()) {
			m_storage.push_back(s);
			s.clear();
		}

		argc = (int)m_storage.size();
		argv = 0;
		if (argc > 0)
			argv = new const char *[argc];

		for (int i = 0; i < argc; ++i)
			argv[i] = m_storage[i].c_str();
	}

	~Argcv() {
		if (argv)
			delete[] argv;
	}

	int argc;
	const char **argv;

private:

	std::vector<std::string> m_storage;
};
}

int APIENTRY WinMain(
	HINSTANCE hInstance,
	HINSTANCE hPrevInstance,
	LPSTR     lpCmdLine,
	int       nCmdShow)
{
#if defined(RAD_OPT_PC_TOOLS)
	if (IsDotCom()) {
		SpawnSelf();
		return 0;
	}
#endif

	Argcv args(lpCmdLine);

#if defined(RAD_OPT_PC_TOOLS)
	return QtAppMain(args.argc, args.argv);
#endif
}
