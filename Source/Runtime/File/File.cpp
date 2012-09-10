// File.cpp
// Platform Agnostic File System
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "File.h"
#include "PrivateFile.h"
#include "../String.h"
#include <stdio.h>
#include "../PushSystemMacros.h"

using namespace string;

namespace file {

RAD_ZONE_DEF(RADRT_API, ZFile, "File", ZRuntime);

namespace {

char s_aliasTable[MaxAliases][MaxAliasLen+1];

static bool CheckPathCharacters(
	const char *string,
	UReg ofs
)
{
	RAD_ASSERT(string);
	RAD_ASSERT(len(string) > (int)ofs);

	string = &string[ofs];
	while (string[0] != 0)
	{
		if (!((string[0] >= L'0' && string[0] <= L'9') ||
			(string[0] >= L'a' && string[0] <= L'z') || (string[0] >= L'A' && string[0] <= L'Z') ||
			(string[0] == L'_') || (string[0] == L'.') || string[0] == L'!' || string[0] == L'/'))
		{
			return false;
		}

		string++;
	}

	return true;
}

static bool CheckExtensionCharacters(
	const char *string,
	UReg ofs
)
{
	RAD_ASSERT(string);
	RAD_ASSERT(len(string) > (int)ofs);

	string = &string[ofs];
	while (string[0] != 0)
	{
		if (!((string[0] >= L'0' && string[0] <= L'9') ||
			(string[0] >= L'a' && string[0] <= L'z') || (string[0] >= L'A' && string[0] <= L'Z')
			|| (string[0] == L'_') ||
			(string[0] == L'.') || string[0] == L'!' || string[0] == L'*'))
		{
			return false;
		}

		string++;
	}

	return true;
}

static bool CheckDirectorySeparators(
	const char *string
)
{
	RAD_ASSERT(string);

	UReg c = 0;
	while (string[0] != 0)
	{
		if (string[0] == L'\\') return false;
		if (string[0] != L'/') c = 0;
		if (string[0] == L'/') c++;

		if (c > 1) return false; // more than one / in a row (i.e. blah//blah//blah)
		string++;
	}

	return true;
}

// enforce 8.3
static bool CheckFilename(
	const char *string
)
{
	RAD_ASSERT(string);
	UReg name, ext, l;

	name = ext = 0;
	l = (UReg)len(string);
	for (; l > 0 && (string[l-1] != L'/'); --l)
	{
		if (string[l-1] == L'.')
		{
			if (l-1 > 0)
			{
				for (l = l-1; l > 0 && (string[l-1] != L'/'); --l)
				{
					name++;
				}

				// special case, because they had an extension.
				//if (name > 0 || ext > 0)
				{
					if (name == 0 || name > 8) return false; // illegal
					if (ext == 0 || ext > 3) return false;
					return true;
				}

				//continue; // was just a directory like '9:/test/../../something'
			}

			return false;
		}
		else
		{
			ext++;
		}
	}

	if ((l>0) && (string[l-1] == L'/'))
	{
		// note that 'ext' is the name length in this case because we had no period '.'
		if (ext == 0 || ext > 8) return false; // illegal
		return true;
	}

	return false;
}

// enforce 8.3
static bool CheckDirectories(
	const char *string,
	UReg ofs
)
{
	RAD_ASSERT(string);
	RAD_ASSERT(len(string) > (int)ofs);

	static const UReg MAXDIRDEPTH = 32;
	UReg c = 0;
	UReg l = 0;
	bool n = false;
	string = &string[ofs];

	while (string[0] != 0)
	{
		if (string[0] == L'/')
		{
			c++;
			n = false;
			if (l > 8) return false;
			l = 0;
		}
		else
		{
			n = true;
			l++;
		}

		string++;
	}

	c += n ? 1 : 0;
	return (c <= MAXDIRDEPTH) && (l <= 12);
}

}

#if defined(RAD_OPT_DEBUG)

static bool s_enforcePortablePaths = true;

RADRT_API bool RADRT_CALL EnforcePortablePaths(bool enforce)
{
	bool x = s_enforcePortablePaths;
	s_enforcePortablePaths = enforce;
	return x;
}

RADRT_API bool RADRT_CALL EnforcePortablePathsEnabled()
{
	return s_enforcePortablePaths;
}

namespace details {

void AssertCreationType(CreationType ct)
{
	RAD_ASSERT_MSG( ct >= CreateAlways && ct <= TruncateExisting, "Invalid Creation Type!" );
}

void AssertFilePath(
	const char *string,
	bool requireAlias
)
{
	UReg p, ofs;
	bool hasAlias;

	RAD_ASSERT_MSG(string[0] != 0, "Null file path!");

	hasAlias = details::ExtractAlias(string, &p, &ofs);
	if (!hasAlias)
	{
		ofs = 0;
	}

	if (requireAlias)
	{
		RAD_ASSERT_MSG(hasAlias, "File path must have an alias!");
		RAD_ASSERT_MSG(string[ofs] == L'/' || string[ofs] == 0, "File path must have a '/' separating the alias from the path (i.e. 9:/dir/filename.ext)");
	}

	RAD_ASSERT_MSG(string[len(string)-1] != L'/', "File path cannot end in a '/'");
	RAD_ASSERT_MSG(CheckDirectorySeparators(string), "Only a single '/' can be used to seperate directories!");

	if (EnforcePortablePathsEnabled())
	{
		if (hasAlias) ++ofs;

		if (string[ofs] != 0)
		{
			// can have nothing other than lower-case alpha numeric characters, in 8.3 format.
			RAD_ASSERT_MSG(CheckPathCharacters(string, ofs), "Path and filenames can only contain characters a-z, digits 0-9, and '_' '.' '!'");
			RAD_ASSERT_MSG(CheckFilename(string) && CheckDirectories(string, ofs), "Path and filenames must be in 8.3 format, and cannot be nested deeper than 32 levels!");
		}
	}

	if (hasAlias) details::AssertAliasOK(string);
}

void AssertExtension(const char *ext)
{
	RAD_ASSERT(ext);
	RAD_ASSERT_MSG(ext[0] == L'.', "Extensions must start with a period!");
	RAD_ASSERT_MSG(CheckExtensionCharacters(ext,0), "Extensions can only can only contain characters a-z, digits 0-9, and '_' '.' '*'");
	RAD_ASSERT_MSG(len(ext) <= MaxExtLen, "Extension length exceeds MaxExtLen!");

	if (EnforcePortablePathsEnabled())
	{
		// check for four because of the '.'
		RAD_ASSERT_MSG(len(ext) <= 4, "An extension cannot be > 3 characters!");
	}
}

} // details

#endif

namespace details {

bool ExtractAlias(
	const char *path,
	UReg *aliasNumber,
	UReg *strOfs
)
{
	RAD_ASSERT(path);
	RAD_ASSERT(aliasNumber&&strOfs);

	// if the path is all digits, followed by a colon, then parse it out.
	int i;
	for (i = 0; path[i] >= L'0' && path[i] <= L'9'; i++) {}

	if (path[i] == L':')
	{
		*aliasNumber = 0;
		*strOfs = i+1;
		sscanf(path, "%d:", (int*)aliasNumber);
		return true;
	}
	else
	{
		return false;
	}
}

} // details

// returned with a '.'

RADRT_API void RADRT_CALL FileExt(
	const char *path,
	char *ext,
	UReg extBufferSize
)
{
	RAD_ASSERT(path);
	RAD_ASSERT(ext);
	RAD_ASSERT(extBufferSize > 0);

	ext[0] = 0;

	UReg l = (UReg)len(path);
	if (l > 0)
	{
		// find the '.'
		for (UReg i = l; i > 0; --i)
		{
			if (path[i-1] == L'.')
			{
				ncpy(ext, &path[i-1], (int)extBufferSize);
				break;
			}
		}
	}
}

// returned with a '.'

RADRT_API UReg RADRT_CALL FileExtLength(
	const char *path
)
{
	RAD_ASSERT(path);

	UReg c = 0;

	UReg l = (UReg)len(path);
	if (l > 0)
	{
		// find the '.'
		// note odd test i < l because of unsigned wrap.
		for (UReg i = l-1; i < l; i--)
		{
			if (path[i] == L'.')
			{
				c = l-i;
				break;
			}
		}
	}

	return c;
}

RADRT_API void RADRT_CALL SetFileExt(
	const char *path,
	const char *ext,
	char *newPath,
	UReg newPathBufferSize
)
{
	RAD_ASSERT(path);
	RAD_ASSERT(newPath);
	RAD_ASSERT(newPathBufferSize);
#if defined(RAD_OPT_DEBUG)
	if (ext && ext[0])
	{
		RAD_ASSERT_MSG(ext[0] == '.', "Extensions must start with a period!");
	}
#endif

	newPathBufferSize--; // for null.
	UReg ofs = 0;
	while (path[ofs] != L'.' && path[ofs] != 0 && ofs < newPathBufferSize)
	{
		newPath[ofs] = path[ofs];
		++ofs;
	}

	if (ext && ext[0])
	{
		UReg extOfs = 0;
		while (ofs < newPathBufferSize && ext[extOfs] != 0)
		{
			newPath[ofs++] = ext[extOfs++];
		}
	}

	newPath[ofs] = 0;
}

RADRT_API UReg RADRT_CALL SetFileExtLength(
	const char *path,
	const char *ext
)
{
	RAD_ASSERT(path);

#if defined(RAD_OPT_DEBUG)
	if (ext && ext[0])
	{
		RAD_ASSERT_MSG(ext[0] == '.', "Extensions must start with a period!");
	}
#endif

	UReg ofs = 0;
	while (path[ofs] != L'.' && path[ofs] != 0)
	{
		ofs++;
	}

	if (ext && ext[0])
	{
		UReg extOfs = 0;
		while (ext[extOfs] != 0)
		{
			ofs++; extOfs++;
		}
	}

	return ofs;
}

RADRT_API UReg RADRT_CALL FileBaseNameLength(
	const char *path
)
{
	// work our way backwards until we hit a slash
	size_t len = ::string::len(path);
	const char *start = path + len;
	const char *end = start;
	bool ext = false;

	for (--start; (end-start) < (int)len; --start)
	{
		if (*start == L'.' && !ext)
		{
			end = start;
			ext = true;
		}
		else if (*start == L'/' || *start == L'\\')
		{
			++start;
			break;
		}
	}

	return (UReg)(end-start);
}

RADRT_API void FileBaseName(
	const char *path,
	char *basePath,
	UReg basePathBufferSize
)
{
	// work our way backwards until we hit a slash
	size_t len = ::string::len(path);
	const char *start = path + len;
	const char *end = start;
	bool ext = false;

	for (--start; (end-start) < (int)len; --start)
	{
		if (*start == L'.' && !ext)
		{
			end = start;
			ext = true;
		}
		else if (*start == L'/' || *start == L'\\')
		{
			++start;
			break;
		}
	}

	// +1 ncpy null
	len = std::min<UReg>((UReg)(end-start)+1, basePathBufferSize);
	::string::ncpy(basePath, start, (int)len);
}

RADRT_API UReg RADRT_CALL FilePathNameLength(
	const char *path
)
{
	// work our way backwards until we hit a slash
	size_t len = ::string::len(path);
	const char *end = path + len;

	for (--end; end > path; --end)
	{
		if (*end == L'/' || *end == L'\\')
		{
			--end;
			break;
		}
	}

	return (UReg)(end-path)+1;
}

RADRT_API void RADRT_CALL FilePathName(
	const char *path,
	char *pathName,
	UReg pathNameBufferSize
)
{
	// work our way backwards until we hit a slash
	size_t len = ::string::len(path);
	const char *end = path + len;

	for (--end; end > path; --end)
	{
		if (*end == L'/' || *end == L'\\')
		{
			--end;
			break;
		}
	}

	// +1 ncpy null
	len = std::min<UReg>((UReg)(end-path)+2, pathNameBufferSize);
	::string::ncpy(pathName, path, (int)len);
}

RADRT_API bool RADRT_CALL FilePathIsValid(
	const char *string
)
{
	RAD_ASSERT(string);
	UReg p, ofs;

	if (string[0] == 0) 
		return false;
	if (!details::ExtractAlias(string, &p, &ofs)) 
		return false;
	if (string[ofs] != L'/') 
		return false;
	if (string[len(string)-1] == L'/') 
		return false;

	bool s = CheckDirectorySeparators(string);

#if defined(RAD_OPT_DEBUG)
	if (EnforcePortablePathsEnabled())
	{
		s = s && CheckPathCharacters(string, ofs+1) && CheckFilename(string) && CheckDirectories(string, ofs+1);
	}
#endif

	bool aliasVisitTable[MaxAliases];

	for (int i = 0; i < MaxAliases; i++)
	{
		aliasVisitTable[i] = false;
	}

	char src[MaxFilePathLen+1], dst[MaxFilePathLen+1];

	cpy(src, string);

	while (details::ExtractAlias(src, &p, &ofs))
	{
		s = s && (p < MaxAliases);
		s = s && (s_aliasTable[p] != 0);
		s = s && ((len(s_aliasTable[p]) + len(src)-ofs) <= MaxFilePathLen);
		s = s && (aliasVisitTable[p] == false);

		s = s && (src[ofs] == 0 || (src[ofs] == L'/' && src[ofs+1] != 0));
		s = s && ((len(s_aliasTable[p]) + len(src)-ofs) <= MaxFilePathLen);

		if (!s) break;

		cpy(dst, s_aliasTable[p]);
		cat(dst, &src[ofs]);
		cpy(src, dst);

		aliasVisitTable[p] = true;
	}

#if defined(RAD_OPT_DEBUG)
	if (s && EnforcePortablePathsEnabled())
	{
		s = (aliasVisitTable[0]||aliasVisitTable[1]||aliasVisitTable[9]);
	}
#endif

	return s;
}

#if defined(RAD_OPT_DEBUG)
namespace details {
void UncheckedSetAlias(
	UReg aliasNumber,
	const char *string
)
#else
RADRT_API void RADRT_CALL SetAlias(
	UReg aliasNumber,
	const char *string
)
#endif
{
	RAD_ASSERT(aliasNumber < MaxAliases);
	if (string && string[0])
	{
		RAD_ASSERT_MSG(len(string) <= MaxAliasLen, "Alias length exceeds maximum!");
		RAD_ASSERT_MSG(string[len(string)-1] != '/', "Alias cannot end with a directory!");
		RAD_ASSERT_MSG(aliasNumber < MaxAliases, "Alias out of bounds!");
		cpy(s_aliasTable[aliasNumber], string);
	}
	else
	{
		s_aliasTable[aliasNumber][0] = 0;
	}
}
#if defined (RAD_OPT_DEBUG)
} // details
#endif

RADRT_API bool RADRT_CALL ExpandAliases(
	const char *portablePath,
	char *expandedPath,
	UReg expandedPathBufferSize
)
{
	RAD_ASSERT(portablePath);
	RAD_ASSERT(len(portablePath) <= MaxFilePathLen);
	char src[MaxFilePathLen+1], dst[MaxFilePathLen+1];
	UReg num, ofs;
	bool valid = false;

	expandedPath[0] = 0;
	cpy(src, portablePath);

	while (details::ExtractAlias(src, &num, &ofs))
	{
		RAD_ASSERT_MSG(num < MaxAliases, "Alias out of bounds!");
		RAD_ASSERT_MSG(s_aliasTable[num] != 0, "Invalid alias (an alias was referenced that has not been set)!");
		RAD_ASSERT_MSG((len(s_aliasTable[num]) + len(src)-ofs) <= MaxFilePathLen, "Expanded alias exceeds MaxFilePathLen!");

		cpy(dst, s_aliasTable[num]);
		cat(dst, &src[ofs]);
		cpy(src, dst);

		valid = true;
	}

	ncpy(expandedPath, src, (int)expandedPathBufferSize);

	return valid;
}

RADRT_API UReg RADRT_CALL ExpandAliasesLength(
	const char *portablePath
)
{
	RAD_ASSERT(portablePath);
	RAD_ASSERT(len(portablePath) <= MaxFilePathLen);
	char src[MaxFilePathLen+1], dst[MaxFilePathLen+1];
	UReg num, ofs;
	bool valid = false;

	cpy(src, portablePath);

	while (details::ExtractAlias(src, &num, &ofs))
	{
		RAD_ASSERT_MSG(num < MaxAliases, "Alias out of bounds!");
		RAD_ASSERT_MSG(s_aliasTable[num][0] != 0, "Invalid alias (an alias was referenced that has not been set)!");
		RAD_ASSERT_MSG((len(s_aliasTable[num]) + len(src)-ofs) <= MaxFilePathLen, "Expanded alias exceeds MaxFilePathLen!");

		cpy(dst, s_aliasTable[num]);
		cat(dst, &src[ofs]);
		cpy(src, dst);

		valid = true;
	}

	if (valid)
	{
		return (UReg)len(src);
	}
	else
	{
		return 0;
	}
}

#if defined(RAD_OPT_DEBUG)

namespace details {

void AssertAliasOK(
	const char *alias
)
{
	RAD_ASSERT(alias);
	RAD_ASSERT(len(alias) <= MaxFilePathLen);
	bool aliasVisitTable[MaxAliases];

	for (int i = 0; i < MaxAliases; i++)
	{
		aliasVisitTable[i] = false;
	}

	char src[MaxFilePathLen+1], dst[MaxFilePathLen+1];
	UReg num, ofs;

	cpy(src, alias);

	while (details::ExtractAlias(src, &num, &ofs))
	{
		RAD_ASSERT_MSG(num < MaxAliases, "Alias out of bounds!");
		RAD_ASSERT_MSG(s_aliasTable[num] != 0, "Invalid alias (an alias was referenced that has not been set)!");
		RAD_ASSERT_MSG((len(s_aliasTable[num]) + len(src)-ofs) <= MaxFilePathLen, "Expanded alias exceeds MaxFilePathLen!");
		RAD_ASSERT_MSG(aliasVisitTable[num] == false, "Recursive alias!");

		RAD_ASSERT_MSG((src[ofs] == '/' && src[ofs+1] != 0) || src[ofs] == 0 , "A path must start with a '/' immediately after the alias (i.e. 9:/path/filename.ext) if there is a filename, and cannot end with a '/'!");

		cpy(dst, s_aliasTable[num]);
		cat(dst, &src[ofs]);
		cpy(src, dst);

		aliasVisitTable[num] = true;
	}

	if (EnforcePortablePathsEnabled())
	{
		RAD_ASSERT_MSG((aliasVisitTable[0]||aliasVisitTable[1]||aliasVisitTable[9]), "File System Alias portability error: all user defined aliases/paths MUST referece alias 0, 1 or 9 directly or by reduction!");
	}
}

} // details

RADRT_API void RADRT_CALL SetAlias(
	UReg aliasNumber,
	const char *string
)
{
	if (EnforcePortablePathsEnabled())
	{
		RAD_ASSERT_MSG((aliasNumber != 0 && aliasNumber != 1 && aliasNumber != 9), "File System Alias portability error: you cannot set aliases 0, 1, or 9 in portability mode!");
	}
	if (string)
	{
		details::AssertAliasOK(string);
		details::UncheckedSetAlias(aliasNumber, string);
		details::AssertAliasOK(string);
	}
	else
	{
		details::UncheckedSetAlias(aliasNumber, 0);
	}
}

#endif

namespace details {

void InitializeAliasTable()
{
	// clear the alias table.
	for (int i = 0; i < MaxAliases; i++)
	{
		s_aliasTable[i][0] = 0;
	}
}

} // details
} // file

