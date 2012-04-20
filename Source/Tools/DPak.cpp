// DPak.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

//
// Dpak is a file format built on top of the LuMP file system. Dpak files are
// compressed using zlib (unless no compression is specified).
//

#include <Runtime/Base.h>
#include <Runtime/DataCodec/LmpReader.h>
#include <Runtime/DataCodec/LmpWriter.h>
#include <Runtime/DataCodec/ZLib.h>
#include <Runtime/Stream.h>
#include <Runtime/String.h>
#include <Runtime/EndianStream.h>
#include <Runtime/Utils.h>
#include <Runtime/File.h>

#include <string>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <algorithm>

enum
{
	ZPakSig = RAD_FOURCC('Z', 'P', 'A', 'K'),
	ZPakMagic = 0xAD432C64,
	DPakSig = RAD_FOURCC('D', 'P', 'A', 'K'),
	DPakMagic = 0xA3054028
};

using namespace std;

using namespace stream;
using namespace data_codec::lmp;
using namespace data_codec::zlib;
using namespace string;
using namespace file;

AddrSize MyGetFileSize(const char *file)
{
	AddrSize size = 0;

	FILE *fp = fopen(file, "rb");
	if (fp)
	{
		fseek(fp, 0, 2);
		size = (AddrSize)ftell(fp);
		fclose(fp);
	}

	return size;
}

//////////////////////////////////////////////////////////////////////////////////////////
// FileBuffer
//////////////////////////////////////////////////////////////////////////////////////////

class FileBuffer : public IOutputBuffer, public IInputBuffer
{
public:

	FileBuffer(FILE* fp)
	{
		m_file = fp;
	}

	// write
	SPos Write(const void* buff, SPos numBytes, UReg* errorCode)
	{
		SPos b = (SPos)fwrite(buff, 1, numBytes, m_file);
		SetErrorCode(errorCode, (b < numBytes) ? ErrorOverflow : ::stream::Success);
		return b;
	}

	// note: if SEEK_END is specified, offset is interpreted as a negative number!
	bool SeekOut(Seek seekType, SPos ofs, UReg* errorCode)
	{
		RAD_FAIL("Seek is not implemented!");
		return false;
	}

	SPos OutPos() const
	{
		return (SPos)ftell(m_file);
	}

	void Flush()
	{
		fflush(m_file);
	}

	UReg OutCaps() const
	{
		return 0;
	}

	UReg OutStatus() const
	{
		return StatusOutputOpen;
	}

	// read
	SPos Read(void* buff, SPos numBytes, UReg* errorCode)
	{
		SPos b = (SPos)fread(buff, 1, numBytes, m_file);
		SetErrorCode(errorCode, (b < numBytes) ? ErrorUnderflow : ::stream::Success);
		return b;
	}

	// note: if SEEK_END is specified, offset is interpreted as a negative number!
	bool SeekIn(Seek seekType, SPos ofs, UReg* errorCode)
	{
		int origin;
		long lofs;

		switch (seekType)
		{
		case StreamBegin:
			origin = 0;
			lofs = (long)ofs;
		break;
		case StreamCur:
			origin = 1;
			lofs = (long)ofs;
		break;
		case StreamEnd:
			origin = 2;
			lofs = -(long)ofs;
		break;
		}

		int r = fseek(m_file, lofs, origin);
		SetErrorCode(errorCode, (r==0) ? ErrorBadSeekPos : ::stream::Success);
		return r==0;
	}

	SPos InPos() const
	{
		return (SPos)ftell(m_file);
	}

	SPos Size()  const
	{
		size_t ofs = ftell(m_file);
		fseek(m_file, 0, 2);
		size_t size = ftell(m_file);
		fseek(m_file, (long)ofs, 0);
		return (SPos)size;
	}

	UReg InCaps() const
	{
		return CapSeekInput | CapSizeInput;
	}

	UReg InStatus() const
	{
		return StatusInputOpen;
	}

private:

	FILE* m_file;
};

//////////////////////////////////////////////////////////////////////////////////////////
// program
//////////////////////////////////////////////////////////////////////////////////////////

static bool DecompressFile(const char* filenameIn, const char* filenameOut, int endian)
{

	FILE* fp = fopen(filenameIn, "rb");
	if (fp)
	{
		FileBuffer fb(fp);
		InputStream* is;
		LittleInputStream lis(fb);
		BigInputStream bis(fb);

		if (endian) is = &lis;
		else is = &bis;

		U32 sig, magic;

		if (!is->Read(&sig, 0) ||
			!is->Read(&magic, 0))
		{
			cout << "Error: unable to read header from '" << filenameIn << "'!" << endl;
			fclose(fp);
			return false;
		}

		if (sig != ZPakSig ||
			magic != ZPakMagic)
		{
			cout << "Error: '" << filenameIn << "' is not a ZPAK file!" << endl;
			fclose(fp);
			return false;
		}

		FILE* outfp = fopen(filenameOut, "wb");
		if (outfp)
		{
			FileBuffer outfb(outfp);
			OutputStream os(outfb);

			if (!data_codec::zlib::StreamDecode(*is, os))
			{
				cout << "Error: zlib decompression error!" << endl;
				fclose(fp);
				fclose(outfp);
				return false;
			}

			fclose(fp);
			fclose(outfp);
		}
		else
		{
			cout << "Error: unable to open '" << filenameOut << "' for writing!" << endl;
			fclose(fp);
			return false;
		}
	}
	else
	{
		cout << "Error: unable to open '" << filenameIn << "' for input!" << endl;
		return false;
	}

	return true;
}

static bool CompressFile(const char* filenameIn, const char* filenameOut, int compression, int endian)
{

	FILE* fp = fopen(filenameIn, "rb");
	if (fp)
	{
		FileBuffer fb(fp);
		InputStream is(fb);

		FILE* outfp = fopen(filenameOut, "wb");
		if (outfp)
		{
			FileBuffer outfb(outfp);
			LittleOutputStream los(outfb);
			BigOutputStream bos(outfb);

			OutputStream*     os;
			if (endian) os = &los;
			else os = &bos;

			if (!os->Write(ZPakSig, 0) ||
				!os->Write(ZPakMagic, 0))
			{
				cout << "Error: unable to write header for '" << filenameOut << "'!" << endl;
				fclose(fp);
				return false;
			}

			if (!data_codec::zlib::StreamEncode(is, *os, compression))
			{
				cout << "Error: zlib compression error!" << endl;
				fclose(fp);
				fclose(outfp);
				return false;
			}

			fclose(fp);
			fclose(outfp);
		}
		else
		{
			cout << "Error: unable to open '" << filenameOut << "' for writing!" << endl;
			fclose(fp);
			return false;
		}
	}
	else
	{
		cout << "Error: unable to open '" << filenameIn << "' for input!" << endl;
		return false;
	}

	return true;
}

struct FileHeader
{
	LOfs fileSize;
};

static bool LoadFile(const char* file, void** data, AddrSize* size)
{
	FILE *fp = fopen(file, "rb");
	if (!fp) return false;
	fseek(fp, 0, 2);
	*size = (AddrSize)ftell(fp);
	fseek(fp, 0, 0);

	if (*size)
	{
		*data = safe_malloc(*size);
		if (fread(*data, 1, (size_t)*size, fp) != *size)
		{
			free(*data);
			*data = 0;
			return false;
		}

		return true;
	}

	return false;
}

static bool ListFiles(const char* dpak, int endian)
{
	StreamReader dpkr;

	FILE* fp = fopen(dpak, "rb");
	if (fp)
	{
		FileBuffer fb(fp);
		InputStream is(fb);

		U32 errorCount = 0;

		if (dpkr.LoadLumpInfo(DPakSig, DPakMagic, is, endian ? LittleEndian : BigEndian))
		{
			U32 lumpCount = dpkr.NumLumps();
			for (U32 i = 0; i < lumpCount; ++i)
			{
				const StreamReader::Lump* l = dpkr.GetByIndex(i);
				cout << l->Name() << ": offset (" << l->Ofs() << "), ";

				FileHeader* h = (FileHeader*)l->TagData();
				if (h) // compressed
				{
					cout << "compressed (" << l->Size() << "), uncompressed (" << h->fileSize << ")." << endl;
				}
				else
				{
					cout << "uncompressed (" << l->Size() << ")." << endl;
				}
			}

			cout << lumpCount << " file(s)." << endl;
		}
		else
		{
			fclose(fp);
			cout << "Error: unable to load lump directory from '" << dpak << "'!" << endl;
			return false;
		}
	}
	else
	{
		cout << "Error: unable to open '" << dpak << "'!" << endl;
		return false;
	}

	return true;
}

static bool ExtractFileFromDpak(const char* dpak, const char *filename, const char* extractTo, int endian, bool ignoreErrors)
{
	char path[MaxFilePathLen+1], dir[MaxFilePathLen+1];
	StreamReader dpkr;

	size_t etl = len(extractTo);

	FILE* fp = fopen(dpak, "rb");
	if (fp)
	{
		FileBuffer fb(fp);
		InputStream is(fb);

		U32 errorCount = 0;

		if (dpkr.LoadLumpInfo(DPakSig, DPakMagic, is, endian ? LittleEndian : BigEndian))
		{
			const StreamReader::Lump* l = dpkr.GetByName(filename);
			if (!l)
			{
				cout << "Error: Unable to find lump named '" << filename << "'!"  << endl;
				fclose(fp);
				return false;
			}
			//cout << "Extracting '" << l->Name() << "' ..." << endl;
			cpy(path, extractTo);
			if (extractTo[etl-1] != 0 && extractTo[etl-1] != '\\' && extractTo[etl-1] != '/') cat(path, "/");
			cat(path, l->Name());

			for (char* c = path; *c; ++c) { if (*c == '\\') *c = '/'; }

			// find the last '/'
			{
				char* lastbs = path;
				for (char* c = path; *c; ++c) { if (*c == '/') lastbs = c; }
				if (lastbs != path)
				{
					size_t ln = std::min<size_t>(MaxFilePathLen, lastbs-path);
					ncpy(dir, path, ln+1);
#undef CreateDirectory
					CreateDirectory(WString(String(dir)).c_str(), NativePath);
				}
			}

			FILE* outfp = fopen(path, "wb");
			if (outfp)
			{
				if (fseek(fp, (long)l->Ofs(), 0) == 0)
				{
					void* data = safe_malloc(l->Size());

					if (fread(data, 1, l->Size(), fp) != l->Size())
					{
						cout << "Error: unable to read lump '" << l->Name() << "'!" << endl;
						errorCount++;
						if (!ignoreErrors)
						{
							free(data);
							fclose(fp);
							fclose(outfp);
							return false;
						}
					}
					else
					{
						FileHeader* h = (FileHeader*)l->TagData();
						if (h) // compressed
						{
							void* uncData = safe_malloc(h->fileSize);
							if (!Decode(data, l->Size(), uncData, h->fileSize))
							{
								cout << "Error: unable to decompress lump '" << l->Name() << "'!" << endl;
								errorCount++;
								if (!ignoreErrors)
								{
									free(data);
									free(uncData);
									fclose(fp);
									fclose(outfp);
									return false;
								}
							}
							if (fwrite(uncData, 1, h->fileSize, outfp) != h->fileSize)
							{
								cout << "Error: unable to write lump to file '" << path << "'!" << endl;
								errorCount++;
								if (!ignoreErrors)
								{
									free(data);
									free(uncData);
									fclose(fp);
									fclose(outfp);
									return false;
								}
							}
							free(uncData);
						}
						else
						{
							// just write.
							if (fwrite(data, 1, l->Size(), outfp) != l->Size())
							{
								cout << "Error: unable to write lump to file '" << path << "'!" << endl;
								errorCount++;
								if (!ignoreErrors)
								{
									free(data);
									fclose(fp);
									fclose(outfp);
									return false;
								}
							}
						}
					}

					free(data);
				}
				else
				{
					cout << "Error: unable to seek to '" << l->Name() << "'!" << endl;
					errorCount++;
					if (!ignoreErrors)
					{
						fclose(fp);
						fclose(outfp);
						return false;
					}
				}
				fclose(outfp);
			}
			else
			{
				cout << "Error: unable to open '" << path << "' for writing!" << endl;
				errorCount++;
				if (!ignoreErrors)
				{
					fclose(fp);
					return false;
				}
			}
		}
		else
		{
			fclose(fp);
			cout << "Error: unable to load lump directory from '" << dpak << "'!" << endl;
			return false;
		}
	}
	else
	{
		cout << "Error: unable to open '" << dpak << "'!" << endl;
		return false;
	}

	return true;
}

static bool ExtractDpak(const char* dpak, const char* extractTo, int endian, bool ignoreErrors)
{
	char path[MaxFilePathLen+1], dir[MaxFilePathLen+1];
	StreamReader dpkr;

	size_t etl = len(extractTo);

	FILE* fp = fopen(dpak, "rb");
	if (fp)
	{
		FileBuffer fb(fp);
		InputStream is(fb);

		U32 errorCount = 0;

		if (dpkr.LoadLumpInfo(DPakSig, DPakMagic, is, endian ? LittleEndian : BigEndian))
		{
			U32 lumpCount = dpkr.NumLumps();
			for (U32 i = 0; i < lumpCount; ++i)
			{
				const StreamReader::Lump* l = dpkr.GetByIndex(i);
				cout << l->Name() << "..." << endl;
				cpy(path, extractTo);
				if (extractTo[etl-1] != 0 && extractTo[etl-1] != '\\' && extractTo[etl-1] != '/') cat(path, "/");
				cat(path, l->Name());

				for (char* c = path; *c; ++c) { if (*c == '\\') *c = '/'; }

				// find the last '\'
				{
					char* lastbs = path;
					for (char* c = path; *c; ++c) { if (*c == '/') lastbs = c; }
					if (lastbs != path)
					{
						size_t ln = std::min<size_t>(MaxFilePathLen, lastbs-path);
						ncpy(dir, path, ln+1);
#undef CreateDirectory
						CreateDirectory(WString(String(dir)).c_str(), NativePath);
					}
				}

				FILE* outfp = fopen(path, "wb");
				if (outfp)
				{
					if (fseek(fp, (long)l->Ofs(), 0) == 0)
					{
						void* data = safe_malloc(l->Size());

						if (fread(data, 1, l->Size(), fp) != l->Size())
						{
							cout << "Error: unable to read lump '" << l->Name() << "'!" << endl;
							errorCount++;
							if (!ignoreErrors)
							{
								free(data);
								fclose(fp);
								fclose(outfp);
								return false;
							}
						}
						else
						{
							FileHeader* h = (FileHeader*)l->TagData();
							if (h) // compressed
							{
								void* uncData = safe_malloc(h->fileSize);
								if (!Decode(data, l->Size(), uncData, h->fileSize))
								{
									cout << "Error: unable to decompress lump '" << l->Name() << "'!" << endl;
									errorCount++;
									if (!ignoreErrors)
									{
										free(data);
										free(uncData);
										fclose(fp);
										fclose(outfp);
										return false;
									}
								}
								if (fwrite(uncData, 1, h->fileSize, outfp) != h->fileSize)
								{
									cout << "Error: unable to write lump to file '" << path << "'!" << endl;
									errorCount++;
									if (!ignoreErrors)
									{
										free(data);
										free(uncData);
										fclose(fp);
										fclose(outfp);
										return false;
									}
								}
								free(uncData);
							}
							else
							{
								// just write.
								if (fwrite(data, 1, l->Size(), outfp) != l->Size())
								{
									cout << "Error: unable to write lump to file '" << path << "'!" << endl;
									errorCount++;
									if (!ignoreErrors)
									{
										free(data);
										fclose(fp);
										fclose(outfp);
										return false;
									}
								}
							}
						}

						free(data);
					}
					else
					{
						cout << "Error: unable to seek to '" << l->Name() << "'!" << endl;
						errorCount++;
						if (!ignoreErrors)
						{
							fclose(fp);
							fclose(outfp);
							return false;
						}
					}
					fclose(outfp);
				}
				else
				{
					cout << "Error: unable to open '" << path << "' for writing!" << endl;
					errorCount++;
					if (!ignoreErrors)
					{
						fclose(fp);
						return false;
					}
				}
			}

			cout << "Extracted " << lumpCount << " file(s), with " << errorCount << " error(s)." << endl;
		}
		else
		{
			fclose(fp);
			cout << "Error: unable to load lump directory from '" << dpak << "'!" << endl;
			return false;
		}
	}
	else
	{
		cout << "Error: unable to open '" << dpak << "'!" << endl;
		return false;
	}

	return true;
}

static bool MakeDpak(const char* dirtopak, const char* dpakfile, UReg compression, int endian, int align, bool ignoreErrors)
{
	Search s;

	RAD_ASSERT(sizeof(FileHeader) == 4);

	size_t dtpl = len(dirtopak);

	if (s.Open(WString(String(dirtopak)).c_str(), L".*", SearchFlags(NativePath|FileNames|Recursive)))
	{
		wchar_t wfile[MaxFilePathLen+1];
		char file[MaxFilePathLen+1], path[MaxFilePathLen+1], lumpName[MaxLumpNameLen+1];

		// open the output file.
		FILE* fp;

		fp = fopen(dpakfile, "wb");

		if (!fp)
		{
			cout << "Error: unable to open output file: '" << dpakfile << "'!" << endl;
			return false;
		}

		FileBuffer fb(fp);
		LittleOutputStream los(fb);
		BigOutputStream bos(fb);

		OutputStream*     os;
		if (endian) os = &los;
		else os = &bos;

		Writer dpkw;

		if (!dpkw.Begin(DPakSig, DPakMagic, *os))
		{
			fclose(fp);
			cout << "Error: unable to write dpak header!" << endl;
			return false;
		}

		UReg errorCount = 0;

		while (s.NextFile(wfile, MaxFilePathLen+1))
		{
			cpy(file, String(WString(wfile)).c_str());
			cpy(path, dirtopak);
			if (path[dtpl-1] != '\\' && path[dtpl-1] != '/') cat(path, "/");
			cat(path, file);

			cpy(lumpName, file);
			for (char* c = lumpName; *c; ++c) { if (*c == '\\') *c = '/'; }

			void* data;
			AddrSize size;

			cout << lumpName << "... " << flush;

			if (LoadFile(path, &data, &size))
			{
				Writer::Lump* lump;
				void *zptr = 0;

				if (compression)
				{
					AddrSize zsize = PredictEncodeSize(size);
					zptr = safe_malloc(zsize);

					if (Encode(data, size, compression, zptr, &zsize))
					{
						RAD_ASSERT(zsize);
						if (zsize >= size) goto skip_compress; // don't compress it.
						lump = dpkw.WriteLump(lumpName, zptr, zsize, align);
						FileHeader* h = (FileHeader*)lump->AllocateTagData(sizeof(FileHeader));
						RAD_VERIFY_MSG(h, "Failed to allocate lump tag!");
						h->fileSize = (LOfs)size;
						float ratio = (1.0f - ((float)zsize / (float)size)) * 100.0f;
						char nums[16];
						sprintf(nums, "%.1f", ratio);
						cout << "(" << nums << "%)" << endl;
					}
					else
					{
						cout << "Error: unable to compress '" << path << "'!" << endl;
						errorCount++;
						if (!ignoreErrors)
						{
							fclose(fp);
							free(data);
							free(zptr);
							return false;
						}
					}
				}
				else
				{
skip_compress:
					lump = dpkw.WriteLump(lumpName, data, size, align);
					cout << "(0%)" << endl;
				}

				if (zptr) { free(zptr); }
				free(data);
			}
			else
			{
				if (FileExists(WString(String(path)).c_str(), NativePath))
				{
					cout << "Error: unable to load '" << path << "', file may be in use!" << endl;
				}
				else
				{
					cout << "Error: unable to load '" << path << "', file does not exist!" << endl;
				}
				errorCount++;
				if (!ignoreErrors)
				{
					fclose(fp);
					return false;
				}
			}
		}

		dpkw.SortLumps();
		U32 numLumps = dpkw.NumLumps();

		if (!dpkw.End())
		{
			cout << "Error: failed writing dpak directory!" << endl;
			fclose(fp);
			return false;
		}
		else
		{
			cout << "Wrote " << numLumps << " file(s) to '" << dpakfile << "' with " << errorCount << " error(s)." << endl;
		}

		fclose(fp);
	}
	else
	{
		cout << "Error: unable to perform file search!" << endl;
		return false;
	}

	return true;
}

static bool MakeDpakFromListFile(const char* listFile, const char* dpakfile, const char* wrkDir, int endian, bool ignoreErrors)
{
	fstream listStream;
	char file[MaxFilePathLen+1], path[MaxFilePathLen+1], lumpName[MaxLumpNameLen+1];

	RAD_ASSERT(sizeof(FileHeader) == 4);

	size_t wdpl = len(wrkDir);

	listStream.open(listFile, ios_base::in);
	if (listStream.is_open())
	{
		// open the output file.
		FILE* fp;

		fp = fopen(dpakfile, "wb");

		if (!fp)
		{
			cout << "Error: unable to open output file: '" << dpakfile << "'!" << endl;
			return false;
		}

		FileBuffer fb(fp);
		LittleOutputStream los(fb);
		BigOutputStream bos(fb);

		OutputStream*     os;
		if (endian) os = &los;
		else os = &bos;

		Writer dpkw;

		if (!dpkw.Begin(DPakSig, DPakMagic, *os))
		{
			fclose(fp);
			cout << "Error: unable to write dpak header!" << endl;
			return false;
		}

		UReg errorCount = 0;

		for (listStream.getline(file, MaxFilePathLen); !listStream.fail(); listStream.getline(file, MaxFilePathLen))
		{
			char cnums[256];
			listStream.getline(cnums, 255);
			if (listStream.fail()) break;
			int alignment;
			sscanf(cnums, "%d", &alignment);
			listStream.getline(cnums, 255);
			if (listStream.fail()) break;
			int compress;
			sscanf(cnums, "%d", &compress);

			// make sure this isn't garbage...
			{
				int i = 0;
				while (file[i] != 0)
				{
					if (isgraph(file[i])) break;
					++i;
				}
				if (file[i] == 0) continue; // this has no alpha characters, it can't be a filename.
			}

			cpy(path, wrkDir);
			if (path[wdpl-1] != '\\' && path[wdpl-1] != '/') cat(path, "/");
			cat(path, file);

			cpy(lumpName, file);
			for (char* c = lumpName; *c; ++c) { if (*c == '\\') *c = '/'; }

			void* data;
			AddrSize size;

			if (compress < 0 || compress > 9)
			{
				cout << "Error: illegal compression level specified for :'" << lumpName << "', skipping." << endl;
				errorCount++;
				if (!ignoreErrors)
				{
					return false;
				}
			}

			cout << lumpName << "... " << flush;

			if (LoadFile(path, &data, &size))
			{
				void *zptr = 0;
				Writer::Lump* lump;

				if (compress)
				{
					AddrSize zsize = PredictEncodeSize(size);
					zptr = safe_malloc(zsize);

					if (Encode(data, size, compress, zptr, &zsize))
					{
						RAD_ASSERT(zsize);
						if (zsize >= size) goto skip_compress;
						lump = dpkw.WriteLump(lumpName, zptr, zsize, alignment);
						FileHeader* h = (FileHeader*)lump->AllocateTagData(sizeof(FileHeader));
						RAD_VERIFY_MSG(h, "Failed to allocate lump tag!");
						h->fileSize = (LOfs)size;
						float ratio = (1.0f - ((float)zsize / (float)size)) * 100.0f;
						char nums[16];
						sprintf(nums, "%.1f", ratio);
						cout << "(" << nums << "%)" << endl;
					}
					else
					{
						cout << "Error: unable to compress '" << path << "'!" << endl;
						errorCount++;
						if (!ignoreErrors)
						{
							free(data);
							free(zptr);
							return false;
						}
					}
				}
				else
				{
skip_compress:
					if (zptr) { free(zptr); }
					lump = dpkw.WriteLump(lumpName, data, size, alignment);
					cout << "(STORE)" << endl;
				}

				free(data);
			}
			else
			{
				if (FileExists(WString(String(path)).c_str(), NativePath))
				{
					cout << "Error: unable to load '" << path << "', file may be in use!" << endl;
				}
				else
				{
					cout << "Error: unable to load '" << path << "', file does not exist!" << endl;
				}
				errorCount++;
				if (!ignoreErrors) return false;
			}
		}

		dpkw.SortLumps();
		U32 numLumps = dpkw.NumLumps();

		if (!dpkw.End())
		{
			cout << "Error: failed writing dpak directory!" << endl;
			return false;
		}
		else
		{
			SizeBuffer buf;
			FormatSize(buf, MyGetFileSize(dpakfile));

			cout << "Wrote " << numLumps << " file(s) to '" << dpakfile << "' with " << errorCount << " error(s)." << endl;
			cout << "DPK Size: " << buf << "." << endl;
		}
	}
	else
	{
		cout << "Error: unable to open list file!" << endl;
		return false;
	}

	return true;
}

static void Syntax()
{
	cout << "Syntax: dpak {-?/-help} {-p \"output dpak filename\" \"input directory\" compressionLevel (0-9)} {-pl \"output dpak filename\" \"input list file\" \"working directory\" compressionLevel (0-9)} {-e little/big} {-a alignment} {-ignore} {-l \"dpak file\"} {-x \"dpak file\" \"extract location\"} {-xf \"dpak file\" \"file to extract\"} {-z \"file to compress\" \"output file\" compression_level (0-9) } {-zx \"file to decompress\" \"output file\"}" << endl;
	cout << "\t-p      : pak the specified directory into the specified pak file." << endl;
	cout << "\t-a      : sets the alignment of a directory packing operation. The default is 8 bytes." << endl;
	cout << "\t-pl     : pak the files in the specified list file into the specified pak file." << endl;
	cout << "\t          files in the list file are relative to the working directory." << endl;
	cout << "\t-l      : list the files in the specified dpak file." << endl;
	cout << "\t-x      : extract the files in a dpak file to the specified directory." << endl;
	cout << "\t-xf     : extract the specified file in a dpak file." << endl;
	cout << "\t-z      : compresses the specified file." << endl;
	cout << "\t-zx     : decompresses the specified file." << endl;
	cout << "\t-e      : sets the endian mode of the operation. (default little endian)" << endl;
	cout << "\t-ignore : ignore errors (on batch operations)." << endl;
	cout << "\t-?/-help : displays this message." << endl;
}

int main(int argc, const char** argv)
{
	bool ok = false;
	cout << "DPak LuMP file builder 1.0" << endl;
	cout << "Copyright (c) 2010 Pyramind Labs, LLC. All Rights Reserved." << endl;
	cout << __DATE__ << " @ " << __TIME__ << endl;

	UReg compression = NoCompression;
	char dpakname[256], dirtopak[256], wrkDir[256];

	cpy(dirtopak, "base");
	cpy(dpakname, "default.dpk");

	bool extract = false;
	bool extractFile = false;
	bool listFiles = false;
	bool zpak = false;
	bool fileList = false;
	bool ignoreErrors = false;
	int endian = 1; // little
	int alignment = 8; // defaults to 8 byte alignment.

	if (argc > 1)
	{
		for (int i = 1; i < argc; i++)
		{
			if (!icmp(argv[i], "-?") ||
				!icmp(argv[i], "-help"))
			{
				Syntax();
				goto shutdown;
			}

		}

		for (int i = 1; i < argc; i++)
		{
			if (!icmp(argv[i], "-p"))
			{
				if ((i+3) < argc)
				{
					cpy(dpakname, argv[i+1]);
					cpy(dirtopak, argv[i+2]);
					sscanf(argv[i+3], "%d", &compression);
					if (compression > BestCompression)
					{
						cout << "Syntax error: '-p' compression level cannot exceed 9!" << endl;
						Syntax();
						goto shutdown;
					}
					i+=3;
				}
				else
				{
					cout << "ntax error: '-p' requires 3 arguments!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-pl"))
			{
				if ((i+3) < argc)
				{
					cpy(dpakname, argv[i+1]);
					cpy(dirtopak, argv[i+2]);
					cpy(wrkDir, argv[i+3]);
					i+=3;
					fileList = true;
				}
				else
				{
					cout << "Syntax error: '-pl' requires 3 arguments!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-x"))
			{
				if ((i+1) < argc)
				{
					cpy(dpakname, argv[i+1]);
					cpy(dirtopak, argv[i+2]);
					i+=2;
					extract = true;
				}
				else
				{
					cout << "Syntax error: '-x' requires string argument!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-xf"))
			{
				if ((i+2) < argc)
				{
					cpy(dpakname, argv[i+1]);
					cpy(dirtopak, argv[i+2]);
					i+=2;
					extractFile = true;
				}
				else
				{
					cout << "Syntax error: '-xf' requires 2 string arguments!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-z"))
			{
				if ((i+3) < argc)
				{
					cpy(dpakname, argv[i+1]);
					cpy(dirtopak, argv[i+2]);
					sscanf(argv[i+3], "%d", &compression);
					if (compression > BestCompression)
					{
						cout << "Syntax error: '-z' compression level cannot exceed 9!" << endl;
						Syntax();
						goto shutdown;
					}
					i+=3;
					zpak = true;
					extract = false;
				}
				else
				{
					cout << "Syntax error: '-x' requires string argument!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-zx"))
			{
				if ((i+2) < argc)
				{
					cpy(dpakname, argv[i+1]);
					cpy(dirtopak, argv[i+2]);
					i+=2;
					extract = true;
					zpak = true;
				}
				else
				{
					cout << "Syntax error: '-zx' requires 2 string arguments!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-l"))
			{
				if ((i+1) < argc)
				{
					cpy(dpakname, argv[i+1]);
					i++;
					listFiles = true;
				}
				else
				{
					cout << "Syntax error: '-l' requires 1 string arguments!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-e"))
			{
				if ((i+1) < argc)
				{
					endian = (!icmp(argv[i+1], "little")) ? 1 : 0;
					++i;
				}
				else
				{
					cout << "Syntax error: '-e' requires string argument!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-a"))
			{
				if ((i+1) < argc)
				{
					sscanf(argv[i+1], "%d", &alignment);
					++i;
				}
				else
				{
					cout << "Syntax error: '-a' requires number argument!" << endl;
					Syntax();
					goto shutdown;
				}
			}
			else if (!icmp(argv[i], "-ignore"))
			{
				ignoreErrors = true;
			}
			else
			{
				cout << "Unrecognized argument '" << argv[i] << "'" << endl;
			}
		}
	}
	else
	{
		cout << "Error: expected arguments!" << endl;
		Syntax();
		goto shutdown;
	}

	if (zpak)
	{
		if (extract)
		{
			cout << "decompressing '" << dpakname << "' to '" << dirtopak << "'..." << endl;
			ok = DecompressFile(dpakname, dirtopak, endian);
		}
		else
		{
			cout << "compressing '" << dpakname << "' to '" << dirtopak << "', compression level: ";
			if (compression == NoCompression)
			{
				cout << "NO COMPRESSION..." << endl;
			}
			else
			{
				cout << compression << "..." << endl;
			}
			ok = CompressFile(dpakname, dirtopak, compression, endian);
		}
	}
	else
	{
		if (listFiles)
		{
			cout << "listing files in '" << dpakname << "'." << endl;
			ok = ListFiles(dpakname, endian);
		}
		else if (extract)
		{
			cout << "extracting '" << dpakname << "' to '" << dirtopak << "'..." << endl;
			ok = ExtractDpak(dpakname, dirtopak, endian, ignoreErrors);
		}
		else if(extractFile)
		{
			cout << "extracting '" << dirtopak << "' from '" << dpakname << "'..." << endl;
			ok = ExtractFileFromDpak(dpakname, dirtopak, "", endian, ignoreErrors);
		}
		else
		{
			if (fileList)
			{
				cout << "paking files listed in '" << dirtopak << "' into '" << dpakname << "..." << endl;
				ok = MakeDpakFromListFile(dirtopak, dpakname, wrkDir, endian, ignoreErrors);
				if (!ok) cout << "FATAL ERROR: operation aborted." << endl;
			}
			else
			{
				cout << "paking '" << dirtopak << "' into '" << dpakname << "', compression level: ";
				if (compression == NoCompression)
				{
					cout << "NO COMPRESSION..." << endl;
				}
				else
				{
					cout << compression << "..." << endl;
				}
				ok = MakeDpak(dirtopak, dpakname, compression, endian, alignment, ignoreErrors);
				if (!ok) cout << "FATAL ERROR: operation aborted." << endl;
			}
		}
	}

shutdown:

	return (ok) ? 0 : -1;
}
