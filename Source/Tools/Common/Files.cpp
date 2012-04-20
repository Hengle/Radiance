// Files.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "Files.h"

void FreeFileData(void *data)
{
	if (data)
	{
		delete[] ((U8*)data);
	}
}

void *LoadFile(const char *filename, int &len)
{
	FILE *fp = fopen(filename, "rb");
	if (fp)
	{
		fseek(fp, 0, SEEK_END);
		len = (int)ftell(fp);
		fseek(fp, 0, SEEK_SET);
		U8 *data = new U8[len];
		if (fread(data, 1, len, fp) != len)
		{
			delete[] data;
			data = 0;
		}
		fclose(fp);
		return data;
	}
	
	return 0;
}

void *SafeLoadFile(const char *filename, int &len)
{
	void *d = LoadFile(filename, len);
	if (!d)
	{
		printf("ERROR: loading %s\n", filename);
		exit(-1);
	}
	return d;
}