// PackageCooker.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../App.h"
#include "../Engine.h"
#include "Packages.h"
#include <Runtime/File.h>
#include <Runtime/Endian/EndianStream.h>
#include <Runtime/DataCodec/LmpWriter.h>
#include <Runtime/DataCodec/ZLib.h>
#include "../Renderer/Material.h"
#include <Runtime/PushSystemMacros.h>

using namespace xtime;

namespace pkg {

namespace {

const char *ErrorString(int r)
{
	switch (r)
	{
	case file::ErrorPartial:
		return "ErrorPartial";
	case file::ErrorAborted:
		return "ErrorAborted";
	case file::ErrorAccessDenied:
		return "ErrorAccessDenied";
	case file::ErrorFileNotFound:
		return "ErrorFileNotFound";
	case file::ErrorGeneric:
		return "ErrorGeneric";
	case file::ErrorExpansionFailed:
		return "ErrorExpansionFailed";
	case file::ErrorInvalidArguments:
		return "ErrorInvalidArguments";
	case file::ErrorDriveNotReady:
		return "ErrorDriveNotRead";
	case file::ErrorTrayOpen:
		return "ErrorTrayOpen";
	case file::ErrorCompressionFailure:
		return "ErrorCompressionFailure";
	case file::ErrorOutOfMemory:
		return "ErrorOutOfMemory";
	case file::Success:
		return "Success";
	case file::Pending:
		return "Pending";
	case SR_ErrorGeneric:
		return "SR_ErrorGeneric";
	case SR_ParseError:
		return "SR_ParseError";
	case SR_BadVersion:
		return "SR_BadVersion";
	case SR_MetaError:
		return "SR_MetaError";
	case SR_MissingFile:
		return "SR_MissingFile";
	case SR_InvalidFormat:
		return "SR_InvalidFormat";
	case SR_CorruptFile:
		return "SR_CorruptFile";
	case SR_IOError:
		return "SR_IOError";
	case SR_CompilerError:
		return "SR_CompilerError";
	case SR_ScriptError:
		return "SR_ScriptError";
	}

	return "Unknown";
}

}

#if defined(RAD_OPT_PC_TOOLS)

bool PackageMan::MakeBuildDirs(int flags, std::ostream &out)
{
	if (flags&P_Clean)
	{
		out << "(CLEAN): Removing Output Directories..." << std::endl;
		file::DeleteDirectory("9:/Cooked", 0);
	}

	out << "Creating Output Directories..." << std::endl;
	
	bool done = false;
	int maxTries = 1;

	if (flags&P_Clean)
		maxTries = 2;

	for (int i = 0; i < maxTries && !done; ++i)
	{
		if (flags&P_Clean)
		{
			// wait for file system to settle.
			thread::Sleep(2000);
		}

		if (!file::CreateDirectory("9:/Cooked/Packages/Base", 0))
			continue;
		if (!file::CreateDirectory("9:/Cooked/Out/Generic", 0))
			continue;
		if (!MakePackageDirs("9:/Cooked/Out/Generic/"))
			continue;
		if (!file::CreateDirectory("9:/Cooked/Out/Packages", 0))
			continue;
		if (!file::CreateDirectory("9:/Cooked/Out/Shaders", 0))
			continue;
		if (!file::CreateDirectory("9:/Cooked/Out/Tags", 0))
			continue;
		if (!file::CreateDirectory("9:/Cooked/Out/Globals", 0))
			continue;
		if (!MakePackageDirs("9:/Cooked/Out/Tags/"))
			continue;
		if (!MakePackageDirs("9:/Cooked/Out/Globals/"))
			continue;
		
		// package output directories.
		
		if (flags&(P_TargetIPhone|P_TargetIPad))
		{
			if (flags&P_TargetIPhone)
			{
				if (!file::CreateDirectory("9:/Cooked/Out/IOS/IPhone", 0))
					continue;
				if (!MakePackageDirs("9:/Cooked/Out/IOS/IPhone/"))
					continue;
			}
			if (flags&P_TargetIPad)
			{
				if (!file::CreateDirectory("9:/Cooked/Out/IOS/IPad", 0))
					continue;
				if (!MakePackageDirs("9:/Cooked/Out/IOS/IPad/"))
					continue;
			}
		}

		if (flags&P_TargetPC)
		{
			if (!file::CreateDirectory("9:/Cooked/Out/PC", 0))
				continue;
			if (!MakePackageDirs("9:/Cooked/Out/PC/"))
				continue;
		}

		done = true;
	}

	return done;
}

int PackageMan::Cook(
	const StringVec &roots,
	int flags,
	int languages,
	int compression,
	std::ostream &out
)
{
	bool scriptsOnly = (flags&P_ScriptsOnly) ? true : false;

	if (!languages) {
		out << "ERROR: no valid languages specified." << std::endl;
		return SR_ErrorGeneric;
	}

	if (!MakeBuildDirs(flags, out)) {
		out << "ERROR: failed to create output directories!" << std::endl;
		return SR_ErrorGeneric;
	}

	{ // show languages
		bool sep = false;

		out << "Languages: ";
		for (int i = StringTable::LangId_EN; i < StringTable::LangId_MAX; ++i) {
			if (!((1<<i)&languages))
				continue;
			if (sep) {
				sep = false;
				out << ", ";
			}
			out << String(StringTable::Langs[i]).upper();
			sep = true;
		}
		out << std::endl;
	}

	xtime::TimeVal startTime = xtime::ReadMilliseconds();

	m_cookState.reset(new (ZPackages) CookState());
	m_cookState->cout = &out;
	m_cookState->languages = languages;

	int r = SR_Success;
	int ptargets = P_TARGET_FLAGS(flags);
	flags &= ~ptargets;

	if (scriptsOnly)
	{
		r = BuildPakFiles(ptargets, compression);
	}
	else
	{
		r::Material::BeginCook();

		out << "Coooking generics..." << std::endl;
		r = CookPlat(roots, flags, flags|ptargets, out);
		
		if (r == SR_Success)
		{
			for (int i = P_FirstTarget; i <= P_LastTarget; i <<= 1)
			{
				if (i&ptargets)
				{
					out << "Cooking " << PlatformNameForFlags(i) << "..." << std::endl;
					r = CookPlat(roots, flags|i, flags|ptargets, out);
					if (r != SR_Success)
					{
						out << "ERROR: cooking " << PlatformNameForFlags(i) << "!" << std::endl;
						break;
					}
				}
			}
		}
		else
		{
			out << "ERROR: cooking generics!" << std::endl;
		}

		if (r == SR_Success)
		{
			r = BuildPackageData();
			if (r == SR_Success)
				r = BuildPakFiles(ptargets, compression);
		}

		r::Material::EndCook();
	}

	xtime::TimeVal totalTime = xtime::ReadMilliseconds() - startTime;
	UReg days, hours, minutes;
	FReg seconds;

	xtime::MilliToDayHourSecond(totalTime, &days, &hours, &minutes, &seconds);

	out << "Finished in " << (days*24+hours) << " hour(s), " << minutes << " minute(s), " << seconds << " second(s)" << std::endl;

	return r;
}

int PackageMan::CookPlat(
	const StringVec &roots,
	int flags,
	int allflags,
	std::ostream &out
)
{
	std::set<int> cooked;
	int pflags = flags&P_AllTargets;

	for (StringVec::const_iterator it = roots.begin(); it != roots.end(); ++it)
	{
		if (m_cancelCook)
		{
			*m_cookState->cout << "ERROR: cook cancelled..." << std::endl;
			return SR_ErrorGeneric;
		}

		Asset::Ref asset = Resolve((*it).c_str, Z_Cooker);
		if (!asset)
		{
			out << "ERROR: resolving " << *it << "!" << std::endl;
			return SR_MissingFile;
		}

		if (cooked.find(asset->id) != cooked.end())
			continue; // already cooked

		cooked.insert(asset->id);

		Cooker::Ref cooker = CookerForAsset(asset);
		if (!cooker)
			return SR_ErrorGeneric; 
		
		CookStatus status = cooker->Status(flags, allflags);
		if (status == CS_NeedRebuild)
		{
			out << (*it) << ": Cooking..." << std::endl;

			int r;

			try
			{
				r = cooker->Cook(flags, allflags);
			}
			catch (exception &e)
			{
				out << "ERROR: exception '" << e.type() << "' msg: '" << (e.what()?e.what():"no message") << "' occured!" << std::endl;
				r = SR_IOError;
			}

			cooker->m_asset.reset();

			if (r != SR_Success)
			{
				out << "ERROR: " << ErrorString(r) << std::endl;
				return r;
			}
		}
		else if (status == CS_UpToDate)
		{
			cooker->LoadImports();
			out << (*it) << ": Up to date." << std::endl;
		}
	}

	// follow imports...
	bool imports = true;

	while (imports)
	{
		if (m_cancelCook)
		{
			*m_cookState->cout << "ERROR: cook cancelled..." << std::endl;
			return SR_ErrorGeneric;
		}

		imports = false;

		std::set<int> added;

		for (std::set<int>::const_iterator it = cooked.begin(); it != cooked.end(); ++it)
		{
			if (m_cancelCook)
			{
				*m_cookState->cout << "ERROR: cook cancelled..." << std::endl;
				return SR_ErrorGeneric;
			}

			const Cooker::Ref &cooker = m_cookState->cookers[*it];
			RAD_ASSERT(cooker);
			
			for (Cooker::ImportVec::const_iterator it2 = cooker->m_imports.begin(); it2 != cooker->m_imports.end(); ++it2)
			{
				if (m_cancelCook)
				{
					*m_cookState->cout << "ERROR: cook cancelled..." << std::endl;
					return SR_ErrorGeneric;
				}

				const Cooker::Import &imp = *it2;
				if (imp.pflags&pflags || (pflags==imp.pflags))
				{
					Asset::Ref asset = Resolve(imp.path.c_str, Z_Cooker);
					if (!asset)
					{
						out << "ERROR: resolving import " << imp.path << " referenced from " << cooker->m_assetPath << "!" << std::endl;
						return SR_MissingFile;
					}

					if (added.find(asset->id)==added.end() && cooked.find(asset->id)==cooked.end())
					{
						imports = true;

						added.insert(asset->id);
						Cooker::Ref impCooker = CookerForAsset(asset);

						if (!impCooker)
							return SR_ErrorGeneric; 

						CookStatus status = impCooker->Status(flags, allflags);
						if (status == CS_NeedRebuild)
						{
							out << imp.path << ": Cooking..." << std::endl;

							int r;
							try
							{
								r = impCooker->Cook(flags, allflags);
							}
							catch (exception &e)
							{
								out << "ERROR: exception '" << e.type() << "' msg: '" <<(e.what()?e.what():"no message") << "' occured!" << std::endl;
								r = SR_IOError;
							}

							impCooker->m_asset.reset();

							if (r != SR_Success)
							{
								out << "ERROR: " << ErrorString(r) << std::endl;
								return r;
							}
						}
						else if (status == CS_UpToDate)
						{
							impCooker->LoadImports();
							out << imp.path << ": Up to date." << std::endl;
						}
					}
				}
			}
		}

		for (std::set<int>::const_iterator it = added.begin(); it != added.end(); ++it)
			cooked.insert(*it);
	}

	if (m_cancelCook)
	{
		*m_cookState->cout << "ERROR: cook cancelled..." << std::endl;
		return SR_ErrorGeneric;
	}

	return SR_Success;
}

int PackageMan::BuildPakFiles(int flags, int compression)
{
	int r = BuildPak0(compression);
	if (r != SR_Success)
		return r;

	for (int i = P_FirstTarget; i <= P_LastTarget; i <<= 1)
	{
		if (!(i&flags))
			continue;
		r = BuildTargetPak(i, compression);
		if (r != SR_Success)
			return r;
	}

	return SR_Success;
}

int PackageMan::BuildPak0(int compression)
{
	*m_cookState->cout << "------ Packaging Generics ------" << std::endl;

	char nativePath[file::MaxFilePathLen+1];
	file::ExpandToNativePath("9:/Cooked/Packages/Base/pak0.pak", nativePath, file::MaxFilePathLen+1);
	FILE *fp = fopen(nativePath, "wb");

	if (!fp)
	{
		*m_cookState->cout << "ERROR opening '" << nativePath << "'" << std::endl;
		return SR_IOError;
	}

	file::stream::OutputBuffer ob(fp);
	stream::LittleOutputStream os(ob);
	data_codec::lmp::Writer lumpWriter;

	lumpWriter.Begin(DPakSig, DPakMagic, os);

	int r = PakDirectory("9:/Cooked/Out/Packages", "Packages/", compression, lumpWriter);
	if (r != SR_Success)
	{
		fclose(fp);
		return r;
	}

	r = PakDirectory("9:/Cooked/Out/Shaders", "Shaders/", compression, lumpWriter);
	if (r != SR_Success)
	{
		fclose(fp);
		return r;
	}

	{
		String path(CStr("9:/"));
		path += m_engine.sys->files->hddRoot;
		path += "/Scripts";
		r = PakDirectory(path.c_str, "Scripts/", compression, lumpWriter);
		if (r != SR_Success)
		{
			fclose(fp);
			return r;
		}
	}

	r = PakDirectory("9:/Cooked/Out/Generic", "Cooked/", compression, lumpWriter);
	if (r != SR_Success)
	{
		fclose(fp);
		return r;
	}

	U32 numLumps = lumpWriter.NumLumps();
	lumpWriter.SortLumps();
	lumpWriter.End();
	fclose(fp);

	*m_cookState->cout << "Wrote " << numLumps << " file(s)." << std::endl;

	if (numLumps < 1)
	{
		*m_cookState->cout << "DELETING empty pak file '" << nativePath << "'" << std::endl;
		file::DeleteFile(nativePath, file::NativePath);
	}

	return SR_Success;
}

int PackageMan::BuildTargetPak(int plat, int compression)
{
	String path("9:/Cooked/Packages/Base/");
	
	switch (plat)
	{
	case P_TargetPC:
		*m_cookState->cout << "------ Packaging PC ------" << std::endl;
		path += "pc.pak";
		break;
	case P_TargetIPhone:
		*m_cookState->cout << "------ Packaging IPhone ------" << std::endl;
		path += "iphone.pak";
		break;
	case P_TargetIPad:
		*m_cookState->cout << "------ Packaging IPad ------" << std::endl;
		path += "ipad.pak";
		break;
	case P_TargetXBox360:
		*m_cookState->cout << "------ Packaging XBox360 ------" << std::endl;
		path += "xbox360.pak";
		break;
	case P_TargetPS3:
		*m_cookState->cout << "------ Packaging PS3 ------" << std::endl;
		path += "ps3.pak";
		break;
	}

	char nativePath[file::MaxFilePathLen+1];
	file::ExpandToNativePath(path.c_str, nativePath, file::MaxFilePathLen+1);
	FILE *fp = fopen(nativePath, "wb");

	if (!fp)
	{
		*m_cookState->cout << "ERROR opening '" << nativePath << "'" << std::endl;
		return SR_IOError;
	}

	file::stream::OutputBuffer ob(fp);
	stream::LittleOutputStream os(ob);
	data_codec::lmp::Writer lumpWriter;

	lumpWriter.Begin(DPakSig, DPakMagic, os);

	int r;
	switch (plat)
	{
	case P_TargetPC:
		r = PakDirectory("9:/Cooked/Out/PC", "Cooked/", compression, lumpWriter);
		break;
	case P_TargetIPhone:
		r = PakDirectory("9:/Cooked/Out/IOS/IPhone", "Cooked/", compression, lumpWriter);
		break;
	case P_TargetIPad:
		r = PakDirectory("9:/Cooked/Out/IOS/IPad", "Cooked/", compression, lumpWriter);
		break;
	case P_TargetXBox360:
		r = PakDirectory("9:/Cooked/Out/XBox360", "Cooked/", compression, lumpWriter);
		break;
	case P_TargetPS3:
		r = PakDirectory("9:/Cooked/Out/PS3", "Cooked/", compression, lumpWriter);
		break;
	}

	if (r != SR_Success)
	{
		fclose(fp);
		return r;
	}

	U32 numLumps = lumpWriter.NumLumps();
	lumpWriter.SortLumps();
	lumpWriter.End();
	fclose(fp);

	*m_cookState->cout << "Wrote " << numLumps << " file(s)." << std::endl;

	if (numLumps < 1)
	{
//		*m_cookState->cout << "DELETING empty pak file '" << string::Shorten(nativePath) << "'" << std::endl;
		file::DeleteFile(nativePath, file::NativePath);
	}

	return SR_Success;
}

int PackageMan::PakDirectory(
	const char *path, 
	const char *prefix,
	int compression, 
	data_codec::lmp::Writer &lumpWriter
)
{
	RAD_ASSERT(path);

	file::Search s;
	file::SearchFlags flags = (file::SearchFlags)(file::FileNames|file::Recursive);
	
	if (!s.Open(path, ".*", flags))
		return SR_Success;

	char filename[file::MaxFilePathLen+1];
	while (s.NextFile(filename, file::MaxFilePathLen+1))
	{
		const String kFilename(CStr(filename));

		if (string::strstr(filename, ".svn/") || string::strstr(filename, ".cvs/"))
			continue;
		
		String fpath(CStr(path));
		if (fpath[fpath.length-1] != '\\' && fpath[fpath.length-1] != '/')
			fpath += '/';
		fpath += kFilename;

		String lumpName(prefix);
		lumpName += kFilename;

		*m_cookState->cout << lumpName << "... " << std::flush;

		char nativePath[file::MaxFilePathLen+1];
		file::ExpandToNativePath(fpath.c_str, nativePath, file::MaxFilePathLen+1);

		FILE *fp = fopen(nativePath, "rb");
		if (!fp)
		{
			*m_cookState->cout << std::endl << "ERROR failed to load '" << nativePath << "'!" << std::endl;
			return SR_IOError;
		}

		fseek(fp, 0, SEEK_END);
		size_t size = ftell(fp);
		fseek(fp, 0, SEEK_SET);

		if (size < 1)
		{
			fclose(fp);
			*m_cookState->cout << "(SKIPPING ZERO LENGTH FILE)!" << std::endl;
			continue;
		}

		void *data = safe_zone_malloc(ZPackages, size);
		if (fread(data, 1, size, fp) != size)
		{
			zone_free(data);
			fclose(fp);
			*m_cookState->cout << std::endl << "ERROR failed to read " << size << " byte(s) from '" << nativePath << "'!" << std::endl;
			return SR_IOError;
		}

		fclose(fp);

		bool compressed = false;

		if (compression)
		{
			AddrSize zipSize = data_codec::zlib::PredictEncodeSize(size);
			void *zipData = safe_zone_malloc(ZPackages, zipSize);

			if (!data_codec::zlib::Encode(data, size, compression, zipData, &zipSize))
			{
				zone_free(data);
				zone_free(zipData);
				*m_cookState->cout << "ERROR compression failure!" << std::endl;
				return SR_IOError;
			}

			if (zipSize < size)
			{
				// compression is good
				data_codec::lmp::Writer::Lump *l = 
					lumpWriter.WriteLump(lumpName.c_str, zipData, zipSize, 8);
				data_codec::lmp::LOfs *uncSize = (data_codec::lmp::LOfs*)l->AllocateTagData(sizeof(data_codec::lmp::LOfs));
				*uncSize = (data_codec::lmp::LOfs)size;

				float ratio = (1.0f - ((float)zipSize / (float)size)) * 100.0f;
				char nums[16];
				sprintf(nums, "%.1f", ratio);
				*m_cookState->cout << "(" << nums << "%)" << std::endl;

				compressed = true;
			}

			zone_free(zipData);
		}

		if (!compressed)
		{
			lumpWriter.WriteLump(lumpName.c_str, data, size, 8);
			*m_cookState->cout << "(0%)" << std::endl;
		}

		zone_free(data);
	}

	return SR_Success;
}

bool PackageMan::LoadTagFile(const Cooker::Ref &cooker, int pflags, void *&data, AddrSize &size)
{
	String path = cooker->TagPath(pflags);
	char nativePath[file::MaxFilePathLen+1];
	file::ExpandToNativePath(path.c_str, nativePath, file::MaxFilePathLen+1);
	FILE *fp = fopen(nativePath, "rb");
	if (!fp)
		return false;

	fseek(fp, 0, SEEK_END);
	size = (AddrSize)ftell(fp);
	fseek(fp, 0, SEEK_SET);
	
	if (size < 1)
	{
		fclose(fp);
		return false;
	}

	data = safe_zone_malloc(ZPackages, size);
	if (fread(data, 1, (size_t)size, fp) != (AddrSize)size)
	{
		fclose(fp);
		zone_free(data);
		return false;
	}

	fclose(fp);
	return true;
}

bool PackageMan::CreateTagData(const Cooker::Ref &cooker, TagData *&data, AddrSize &size)
{
	RAD_ASSERT(cooker);

	AddrSize ofs = sizeof(TagData);
	if (!cooker->m_imports.empty())
		ofs += sizeof(U16)*(cooker->m_imports.size()-1);
	
	TagData *tag = (TagData*)safe_zone_calloc(ZPackages, 1, ofs);
	void *file;
	AddrSize fileSize;

	tag->type = (U16)cooker->m_type;

	if (LoadTagFile(cooker, 0, file, fileSize))
	{
		tag = (TagData*)safe_zone_realloc(ZPackages, tag, ofs+fileSize);
		tag->ofs[0] = (U32)ofs;
		memcpy(((U8*)tag)+tag->ofs[0], file, fileSize);
		ofs += fileSize;
		zone_free(file);
	}

	// load target tags
	int targetNum = 1; // starts at 1 (slot for generics)
	for (int i = P_FirstTarget; i <= P_LastTarget; i <<= 1, ++targetNum)
	{
		if (LoadTagFile(cooker, i, file, fileSize))
		{
			tag = (TagData*)safe_zone_realloc(ZPackages, tag, ofs+fileSize);
			tag->ofs[targetNum] = (U32)ofs;
			memcpy(((U8*)tag)+tag->ofs[targetNum], file, fileSize);
			ofs += fileSize;
			zone_free(file);
		}
	}

	data = tag;
	size = ofs;

	return true;
}

int PackageMan::CookPackage::AddImport(const char *path)
{
	RAD_ASSERT(path);

	for (StringVec::const_iterator it = imports.begin(); it != imports.end(); ++it)
	{
		if ((*it) == path)
			return (int)(it-imports.begin());
	}

	imports.push_back(String(path));
	return (int)(imports.size()-1);
}

int PackageMan::BuildPackageData()
{
	CookPackage::Map packages;

	// gather packages.
	for (CookerMap::const_iterator it = m_cookState->cookers.begin(); it != m_cookState->cookers.end(); ++it)
	{
		if (m_cancelCook)
		{
			*m_cookState->cout << "ERROR cook cancelled..." << std::endl;
			return SR_ErrorGeneric;
		}

		const Package::Ref &pkg = it->second->m_pkg;

		CookPackage::Map::iterator mit = packages.find(pkg.get());
		if (mit == packages.end())
		{
			CookPackage cp;
			cp.pkg = pkg;
			cp.cookers.push_back(it->second);
			packages.insert(CookPackage::Map::value_type(pkg.get(), cp));
		}
		else
		{
			mit->second.cookers.push_back(it->second);
		}
	}

	// generate a LuMP file for each package which contains the
	// asset directory and tags
	for (CookPackage::Map::iterator it = packages.begin(); it != packages.end(); ++it)
	{
		if (m_cancelCook)
		{
			*m_cookState->cout << "ERROR cook cancelled..." << std::endl;
			return SR_ErrorGeneric;
		}

		CookPackage &pkg = it->second;
		data_codec::lmp::Writer lmpWriter;

		String path(CStr("9:/Cooked/Out/Packages/"));
		path += pkg.pkg->name;
		path += ".lump";

		char nativePath[file::MaxFilePathLen+1];
		file::ExpandToNativePath(path.c_str, nativePath, file::MaxFilePathLen+1);

		FILE *fp = fopen(nativePath, "wb");
		if (!fp)
		{
			*m_cookState->cout << "ERROR failed to open '" << path << "!" << std::endl;
			return SR_ErrorGeneric;
		}

		file::stream::OutputBuffer ob(fp);
		stream::LittleOutputStream os(ob);

		lmpWriter.Begin(LumpSig, LumpId, os);

		for (Cooker::Vec::const_iterator ait = pkg.cookers.begin(); ait != pkg.cookers.end(); ++ait)
		{
			if (m_cancelCook)
			{
				fclose(fp);
				*m_cookState->cout << "ERROR cook cancelled..." << std::endl;
				return SR_ErrorGeneric;
			}

			TagData *data;
			AddrSize size;
			const Cooker::Ref &cooker = *ait;

			if (!CreateTagData(cooker, data, size))
			{
				fclose(fp);
				return SR_ErrorGeneric;
			}

			// write zero length lump
			data_codec::lmp::Writer::Lump *l = lmpWriter.WriteLump(cooker->m_assetName.c_str, 0, 0, 4);
			if (!l)
			{
				*m_cookState->cout << "ERROR writing lump '" << cooker->m_assetName << "' to '" << path << "!" << std::endl;
				fclose(fp);
				return SR_ErrorGeneric;
			}

			if (size)
			{
				// map imports.
				int importNum = 0;
				data->numImports = (U16)cooker->m_imports.size();
				for (Cooker::ImportVec::const_iterator impIt = cooker->m_imports.begin(); impIt != cooker->m_imports.end(); ++impIt, ++importNum)
				{
					data->imports[importNum] = (U16)pkg.AddImport((*impIt).path.c_str);
				}

				void *tag = l->AllocateTagData((data_codec::lmp::LOfs)size);
				if (!tag)
				{
					*m_cookState->cout << "ERROR allocating " << size << " byte lump for '" << cooker->m_assetPath << "' in file '" << path << "!" << std::endl;
					fclose(fp);
					return SR_ErrorGeneric;
				}
				memcpy(tag, data, size);
				zone_free(data);
			}
		}

		// import table
		{
			AddrSize importTagSize = sizeof(U16);
			for (StringVec::const_iterator it = pkg.imports.begin(); it != pkg.imports.end(); ++it)
			{
				importTagSize += (*it).length + 1 + sizeof(U16);
			}

			data_codec::lmp::Writer::Lump *l = lmpWriter.WriteLump("@imports", 0, 0, 4);

			if (!l)
			{
				*m_cookState->cout << "ERROR writing lump '@imports' to '" << path << "!" << std::endl;
				fclose(fp);
				return SR_ErrorGeneric;
			}

			U8 *tag = (U8*)l->AllocateTagData((data_codec::lmp::LOfs)importTagSize);
			if (!tag)
			{
				*m_cookState->cout << "ERROR allocating " << importTagSize << " byte lump for '@imports' in file '" << path << "!" << std::endl;
				fclose(fp);
				return SR_ErrorGeneric;
			}

			*reinterpret_cast<U16*>(tag) = (U16)pkg.imports.size();
			tag += sizeof(U16);

			for (StringVec::const_iterator it = pkg.imports.begin(); it != pkg.imports.end(); ++it)
			{
				*reinterpret_cast<U16*>(tag) = (U16)(*it).length+1;
				tag += sizeof(U16);
				memcpy(tag, (*it).c_str, (*it).length+1);
				tag += (*it).length+1;
			}
		}

		lmpWriter.SortLumps();
		lmpWriter.End();
		fclose(fp);
	}

	return SR_Success;
}

void PackageMan::CancelCook()
{
	m_cancelCook = true;
}

void PackageMan::ResetCancelCook()
{
	m_cancelCook = false;
}

Cooker::Ref PackageMan::CookerForAsset(const Asset::Ref &asset)
{
	{
		CookerMap::iterator it = m_cookState->cookers.find(asset->id);
		if (it != m_cookState->cookers.end())
		{
			it->second->m_asset = asset; // re-attach
			return it->second;
		}
	}

	TypeCookerFactoryMap::iterator it = m_cookerFactoryMap.find(asset->type);
	if (it == m_cookerFactoryMap.end())
	{
		*m_cookState->cout << "ERROR: No cooker for asset type: " << asset::TypeString(asset->type) << std::endl;
		return Cooker::Ref();
	}

	Cooker::Ref cooker(it->second->New());
	m_cookState->cookers[asset->id] = cooker;

	// We cache alot of this information because the
	// asset may get free'd during cook.
	cooker->m_cout = m_cookState->cout;
	cooker->m_languages = m_cookState->languages;
	cooker->m_asset = asset;
	cooker->m_pkgMan = this;
	cooker->m_assetPath = asset->path.get();
	cooker->m_assetName = asset->name.get();
	cooker->m_pkg = asset->pkg;
	cooker->m_type = asset->type;
	cooker->m_cooking = true;
	cooker->LoadGlobals();

	return cooker;
}

#endif

bool PackageMan::MakeIntermediateDirs()
{
	file::CreateDirectory("9:/Temp/Out/Generic", 0);
	MakePackageDirs("9:/Temp/Out/Generic/");

	file::CreateDirectory("9:/Temp/Out/IOS/IPhone", 0);
	MakePackageDirs("9:/Temp/Out/IOS/IPhone/");
	file::CreateDirectory("9:/Temp/Out/IOS/IPad", 0);
	MakePackageDirs("9:/Temp/Out/IOS/IPad/");
	file::CreateDirectory("9:/Temp/Out/PC", 0);
	MakePackageDirs("9:/Temp/Out/PC/");

	file::CreateDirectory("9:/Temp/Out/Shaders", 0);
	file::CreateDirectory("9:/Temp/Out/Tags", 0);
	file::CreateDirectory("9:/Temp/Out/Globals", 0);
	MakePackageDirs("9:/Temp/Out/Tags/");
	MakePackageDirs("9:/Temp/Out/Globals/");

	return true;
}

bool PackageMan::MakePackageDirs(const char *prefix)
{
	const String kPrefix(CStr(prefix));

	for(Package::Map::const_iterator it = m_packages.begin(); it != m_packages.end(); ++it)
	{
		String path(kPrefix);
		path += it->second->name;
		if (!file::CreateDirectory(path.c_str, 0))
			return false;
	}

	return true;
}

Cooker::Ref PackageMan::AllocateIntermediateCooker(const Asset::Ref &_asset)
{
	// clone this asset so we can mess with it.
	Asset::Ref asset = Asset(_asset->id, Z_Unique);
	if (!asset)
		return Cooker::Ref();

	TypeCookerFactoryMap::iterator it = m_cookerFactoryMap.find(asset->type);
	if (it == m_cookerFactoryMap.end())
	{
		COut(C_Info) << "ERROR: No cooker for asset type: " << asset::TypeString(asset->type) << std::endl;
		return Cooker::Ref();
	}

	Cooker::Ref cooker(it->second->New());

	cooker->m_cout = &COut(C_Info);
	cooker->m_languages = (1<<(App::Get()->langId.get()));
	cooker->m_asset = asset;
	cooker->m_pkgMan = this;
	cooker->m_assetPath = asset->path.get();
	cooker->m_assetName = asset->name.get();
	cooker->m_pkg = asset->pkg;
	cooker->m_type = asset->type;
	cooker->m_cooking = false;
	cooker->LoadGlobals();

	return cooker;
}

int Cooker::Cook(int flags, int allflags)
{
	int r = Compile(flags, allflags);
	if (r == SR_Success)
		SaveState();
	return r;
}

void Cooker::LoadGlobals()
{
	String spath;
	if (m_cooking)
		spath = CStr("9:/Cooked/Out/Globals/");
	else
		spath = CStr("9:/Temp/Out/Globals/");

	spath += m_assetPath;

	char path[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, path, file::MaxFilePathLen+1))
	{
		m_globals = Persistence::Load(0);
		return;
	}

	FILE *fp = fopen(path, "rb");
	if (fp)
	{
		file::stream::InputBuffer ib(fp);
		stream::InputStream is(ib);
		m_globals = Persistence::Load(is);
		fclose(fp);
	}
	else
	{
		m_globals = Persistence::Load(0);
	}
}

void Cooker::SaveGlobals()
{
	String spath;
	if (m_cooking)
		spath = CStr("9:/Cooked/Out/Globals/");
	else
		spath = CStr("9:/Temp/Out/Globals/");

	spath += m_assetPath;

	char path[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, path, file::MaxFilePathLen+1))
		return;

	FILE *fp = fopen(path, "wb");
	if (fp)
	{
		file::stream::OutputBuffer ob(fp);
		stream::OutputStream os(ob);
		m_globals->Save(os);
		fclose(fp);
	}
}

void Cooker::LoadImports()
{
	if (!m_cooking)
		return;

	m_imports.clear();

	String spath(CStr("9:/Cooked/Out/Globals/"));
	spath += m_assetPath;
	spath += ".imports";

	char path[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, path, file::MaxFilePathLen+1))
	{
		return;
	}

	FILE *fp = fopen(path, "rb");
	if (fp)
	{
		file::stream::InputBuffer ib(fp);
		stream::InputStream is(ib);
		
		U32 tag;
		if (!is.Read(&tag))
			return;
		if (tag != ImportsTag)
			return;
		U32 numStrings;
		if (!is.Read(&numStrings))
			return;

		for (U32 i = 0; i < numStrings; ++i)
		{
			char buf[256];
			U32 size;
			int pflags;

			if (!is.Read(&pflags))
				return;
			if (!is.Read(&size))
				return;
			if (is.Read(buf, (stream::SPos)size, 0) != (stream::SPos)size)
				return;
			buf[size] = 0;

			Import imp;
			imp.pflags = pflags;
			imp.path = buf;

			m_imports.push_back(imp);
		}

		fclose(fp);
	}
}

void Cooker::SaveImports()
{
	if (!m_cooking)
		return;

	String spath(CStr("9:/Cooked/Out/Globals/"));
	spath += m_assetPath;
	spath += CStr(".imports");

	char path[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, path, file::MaxFilePathLen+1))
		return;

	FILE *fp = fopen(path, "wb");
	if (fp)
	{
		file::stream::OutputBuffer ob(fp);
		stream::OutputStream os(ob);
		
		if (!os.Write((U32)ImportsTag))
			return;
		if (!os.Write((U32)m_imports.size()))
			return;

		for (ImportVec::const_iterator it = m_imports.begin(); it != m_imports.end(); ++it)
		{
			const Import &i = *it;
			if (!os.Write(i.pflags))
				return;
			if (!os.Write((U32)i.path.length.get()))
				return;
			if (os.Write(i.path.c_str, (stream::SPos)i.path.length.get(), 0) != (stream::SPos)i.path.length.get())
				return;
		}

		fclose(fp);
	}
}

void Cooker::SaveState()
{
	SaveGlobals();
	SaveImports();
}

String Cooker::FilePath(const char *path, int pflags)
{
	RAD_ASSERT(path);
	String spath(CStr(path));
	String sbase;

	if (pflags&P_TargetIPhone)
	{
		if (m_cooking)
			sbase = CStr("9:/Cooked/Out/IOS/IPhone/");
		else
			sbase = CStr("9:/Temp/Out/IOS/IPhone/");
	}
	else if (pflags&P_TargetIPad)
	{
		if (m_cooking)
			sbase = CStr("9:/Cooked/Out/IOS/IPad/");
		else
			sbase = CStr("9:/Temp/Out/IOS/IPad/");
	}
	else if (pflags&P_TargetPC)
	{
		if (m_cooking)
			sbase = CStr("9:/Cooked/Out/PC/");
		else
			sbase = CStr("9:/Temp/Out/PC/");
	}
	else
	{
		if (m_cooking)
			sbase = CStr("9:/Cooked/Out/Generic/");
		else
			sbase = CStr("9:/Temp/Out/Generic/");
	}

	return sbase + spath;
}

bool Cooker::FileTime(const char *path, int pflags, xtime::TimeDate &time)
{
	String s = FilePath(path, pflags);
	return file::FileTime(s.c_str, &time, 0);
}

BinFile::Ref Cooker::OpenWrite(const char *path, int pflags)
{
	String spath = FilePath(path, pflags);
	char wpath[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, wpath, file::MaxFilePathLen+1))
		return BinFile::Ref();
	FILE *fp = fopen(wpath, "wb");
	if (!fp)
		return BinFile::Ref();
	return BinFile::Ref(new (ZPackages) BinFile(fp));
}

BinFile::Ref Cooker::OpenRead(const char *path, int pflags)
{
	String spath = FilePath(path, pflags);
	char wpath[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, wpath, file::MaxFilePathLen+1))
		return BinFile::Ref();
	FILE *fp = fopen(wpath, "rb");
	if (!fp)
		return BinFile::Ref();
	return BinFile::Ref(new (ZPackages) BinFile(fp));
}

int Cooker::LoadFile(
	const char *path, 
	int pflags,
	int &media,
	file::HBufferedAsyncIO &buf,
	const file::HIONotify &notify,
	file::FPos alignment,
	::Zone &zone
)
{
	String spath = FilePath(path, pflags);
	return engine->sys->files->LoadFile(
		spath.c_str,
		media,
		buf,
		notify,
		alignment,
		zone,
		false
	);
}

String Cooker::TagPath(int pflags)
{
	String spath;
	if (m_cooking)
		spath = CStr("9:/Cooked/Out/Tags/");
	else
		spath = CStr("9:/Temp/Out/Tags/");

	spath += m_assetPath;

	if (pflags)
	{
		if (pflags&P_TargetPC)
			spath += CStr(".pc");
		else if (pflags&P_TargetIPhone)
			spath += CStr(".iphone");
		else if (pflags&P_TargetIPad)
			spath += CStr(".ipad");
		else if (pflags&P_TargetXBox360)
			spath += CStr(".xbox360");
		else if (pflags&P_TargetPS3)
			spath += CStr(".ps3");
	}

	spath += CStr(".tag");
	return spath;
}

BinFile::Ref Cooker::OpenTagWrite(int pflags)
{
	String spath = TagPath(pflags);

	char path[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, path, file::MaxFilePathLen+1))
		return BinFile::Ref();
	FILE *fp = fopen(path, "wb");
	if (!fp)
		return BinFile::Ref();
	return BinFile::Ref(new (ZPackages) BinFile(fp));
}

BinFile::Ref Cooker::OpenTagRead(int pflags)
{
	String spath = TagPath(pflags);

	char path[file::MaxFilePathLen+1];
	if (!file::ExpandToNativePath(spath.c_str, path, file::MaxFilePathLen+1))
		return BinFile::Ref();
	FILE *fp = fopen(path, "rb");
	if (!fp)
		return BinFile::Ref();
	return BinFile::Ref(new (ZPackages) BinFile(fp));
}

int Cooker::LoadTag(
	int pflags,
	int &media,
	file::HBufferedAsyncIO &buf,
	const file::HIONotify &notify
)
{
	String spath = TagPath(pflags);
	return engine->sys->files->LoadFile(
		spath.c_str,
		media,
		buf,
		notify,
		8,
		file::ZFile,
		false
	);
}

bool Cooker::HasTag(int pflags)
{
	String spath = TagPath(pflags);
	return file::FileExists(spath.c_str, 0);
}

bool Cooker::NeedsRebuild(const AssetRef &asset)
{
	Cooker::Ref cooker = m_pkgMan->CookerForAsset(asset);
	return cooker && cooker->NeedsRebuild(asset);
}

int Cooker::AddImport(const char *path, int pflags)
{
	if (!m_cooking)
		return 0;

	pflags &= P_AllTargets;

	RAD_ASSERT(path);
	for (ImportVec::iterator it = m_imports.begin(); it != m_imports.end(); ++it)
	{
		if (!string::cmp((*it).path.c_str.get(), path))
		{
			(*it).pflags |= pflags;
			return (int)(it-m_imports.begin());
		}
	}

	Import imp;
	imp.path = path;
	imp.pflags = pflags;
	m_imports.push_back(imp);
	return (int)(m_imports.size()-1);
}

int Cooker::FirstTarget(int allflags)
{
	for (int i = P_FirstTarget; i <= P_LastTarget; i <<= 1)
	{
		if (i&allflags)
			return i;
	}

	return 0;
}

Engine *Cooker::RAD_IMPLEMENT_GET(engine)
{
	return App::Get()->engine;
}

void Cooker::UpdateModifiedTime(int target)
{
	String key(TargetPath(target)+"__cookerModifiedTime");
	TimeDate td = TimeDate::Now(TimeDate::universal_time_tag());

	world::Keys::Pairs::iterator it = globals->pairs.find(key);
	if (it == globals->pairs.end())
	{
		globals->pairs[key] = asset->entry->modifiedTime->ToString();
	}
	else
	{
		it->second = asset->entry->modifiedTime->ToString();
	}
}

int Cooker::CompareVersion(int target, bool updateIfNewer)
{
	String key(TargetPath(target)+"__cookerVersion");

	world::Keys::Pairs::iterator it = globals->pairs.find(key);
	if (it == globals->pairs.end())
	{
		char sz[64];
		string::itoa(version, sz);
		globals->pairs[key] = String(sz);

		return -1; // we have no version on disk, so the source data has to be newer
	}

	int v = string::atoi(it->second.c_str.get());

	// this logic may appear backwards but what we are saying here is that
	// if the cached version is less than our current version then the source
	// data on disk must be newer and should be recooked.

	int r = 0;
	if (version > -1)
		r = (v < version) ? -1 : (v > version) ? 1 : 0;
	else // version of -1 means always rebuild.
		r = -1;
	
	if (r < 0 && updateIfNewer)
	{
		char sz[64];
		string::itoa(version, sz);
		globals->pairs[key] = String(sz);
	}

	return r;
}

int Cooker::CompareModifiedTime(int target, bool updateIfNewer)
{
	String key(TargetPath(target)+"__cookerModifiedTime");

	world::Keys::Pairs::iterator it = globals->pairs.find(key);
	if (it == globals->pairs.end())
	{
		globals->pairs[key] = asset->entry->modifiedTime->ToString();
		return -1; // always older
	}

	TimeDate td = TimeDate::FromString(it->second.c_str);
	int r = td.Compare(*asset->entry->modifiedTime.get());
	if (r < 0 && updateIfNewer)
	{
		globals->pairs[key] = asset->entry->modifiedTime->ToString();
	}

	return r;
}

int Cooker::CompareCachedFileTime(int target, const char *key, const char *path, bool updateIfNewer)
{
	RAD_ASSERT(key);
	RAD_ASSERT(path);
	
	String skey(TargetPath(target)+key);
	String skeyFile(skey+"_file");
	String spath(path);
	bool force = false;

	world::Keys::Pairs::const_iterator it = globals->pairs.find(skeyFile);
	if (it != globals->pairs.end())
	{
		if (it->second != spath)
			force = true; // filename changed!
	}

	globals->pairs[skeyFile] = spath;

	TimeDate selfTime;
	bool m = TimeForKey(target, key, selfTime);

	if (!m && !updateIfNewer)
		return -1; // always older

	TimeDate fileTime;

	spath = CStr("9:/");
	spath += engine->sys->files->hddRoot.get();
	spath += "/";
	spath += path;

	if (!file::FileTime(spath.c_str, &fileTime, 0))
		return -1; // always older

	int c = m ? selfTime.Compare(fileTime) : -1;
	if (force)
		c = -1;

	if (c < 0 && updateIfNewer)
		globals->pairs[skey] = fileTime.ToString();

	return c;
}

int Cooker::CompareCachedFileTimeKey(int target, const char *key, const char *localized, bool updateIfNewer)
{
	RAD_ASSERT(key);

	const String *s = asset->entry->KeyValue<String>(key, target);
	if (!s) {
		cout.get() << "WARNING: Cooker(" << asset->path.get() << ") is asking for key '" << key << "' which does not exist or is not a string in target '" << target << "'" << std::endl;
		return -1; // always older (error condition)
	}

	if (!localized)
		return CompareCachedFileTime(target, key, s->c_str, updateIfNewer);

	String sKey(key);
	String sPath(*s);
	
	// create localized file path.
	char ext[file::MaxFilePathLen+1];
	char path[file::MaxFilePathLen+1];

	file::FileExt(sPath.c_str, ext, file::MaxFilePathLen+1);
	file::SetFileExt(sPath.c_str, 0, path, file::MaxFilePathLen+1);

	String sExt(CStr(ext));
	String sBasePath(CStr(path));

	int r = 0;

	for (int i = StringTable::LangId_EN; i < StringTable::LangId_MAX; ++i) {
		if (!((1<<i)&languages))
			continue;

		String x, k;

		if (i != StringTable::LangId_EN) {
			x = sBasePath;
			x += "_";
			x += StringTable::Langs[i];
			x += sExt;
			k = sKey;
			k += "_cookerLang_";
			k += StringTable::Langs[i];
		} else {
			x = sPath;
			k = sKey;
		}

		// don't early out here just record the newest value. we need to cache all file times for all versions.
		int z = CompareCachedFileTime(target, k.c_str, x.c_str, updateIfNewer);
		r = ((r<z)&&(r!=0)) ? r : z;
	}

	int lr = CompareCachedLocalizeKey(target, localized);
	return ((r<lr)&&(r!=0)) ? r : lr;
}

int Cooker::CompareCachedStringKey(int target, const char *key) {
	RAD_ASSERT(key);

	const String *s = asset->entry->KeyValue<String>(key, target);
	if (!s) {
		cout.get() << "WARNING: Cooker(" << asset->path.get() << ") is asking for key '" << key << "' which does not exist or is not a string in target '" << target << "'" << std::endl;
		return -1; // key is missing!
	}
	
	String skey(TargetPath(target)+key);

	world::Keys::Pairs::const_iterator it = globals->pairs.find(skey);
	if (it != globals->pairs.end()) {
		if (it->second != *s) {
			globals->pairs[skey] = *s;
			return -1; // changed!
		}
	} else {
		globals->pairs[skey] = *s;
		return -1; // doesn't exist!
	}

	return 0;
}

int Cooker::CompareCachedLocalizeKey(int target, const char *key) {
	RAD_ASSERT(key);

	const bool *b = asset->entry->KeyValue<bool>(key, target);
	if (!b) {
		cout.get() << "WARNING: Cooker(" << asset->path.get() << ") is asking for key '" << key << "' which does not exist or is not a bool in target '" << target << "'" << std::endl;
		return -1; // key is missing!
	}

	if (!(*b))
		return 0; // not localized ignore it.

	String localizedString = LocalizedString(languages);
	String skey(TargetPath(target)+CStr("__cookerLocalizedVersion"));
	world::Keys::Pairs::const_iterator it = globals->pairs.find(skey);

	if (it != globals->pairs.end()) {
		if (it->second != localizedString) {
			globals->pairs[skey] = localizedString;
			return -1; // changed!
		}
	} else {
		globals->pairs[skey] = localizedString;
		return -1; // doesn't exist!
	}

	return 0; // same languages
}

bool Cooker::ModifiedTime(int target, xtime::TimeDate &td) const
{
	return TimeForKey(target, "__cookerModifiedTime", td);
}

bool Cooker::TimeForKey(int target, const char *key, xtime::TimeDate &td) const
{
	RAD_ASSERT(key);
	String skey(TargetPath(target)+key);
	world::Keys::Pairs::const_iterator it = globals->pairs.find(skey);
	if (it == globals->pairs.end())
		return false;
	td = TimeDate::FromString(it->second.c_str);
	return true;
}

String Cooker::TargetPath(int target)
{
	const char *sz = pkg::PlatformNameForFlags(target);
	if (!sz)
		return CStr("Generic/");
	return CStr(sz)+"/";
}

String Cooker::LocalizedString(int languages) {
	String s;
	bool sep = false;

	for (int i = StringTable::LangId_EN; i < StringTable::LangId_MAX; ++i) {
		if (!((1<<i)&languages))
			continue;
		if (sep) {
			sep = false;
			s += ";";
		}
		s += String(StringTable::Langs[i]).upper();
		sep = true;
	}

	return s;
}

} // pkg
