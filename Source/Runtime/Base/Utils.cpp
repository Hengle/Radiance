// Utils.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "Base.h"
#include <string.h>
#include "../String.h"
#include "../PushSystemMacros.h"

RADRT_API void RADRT_CALL FormatSize(SizeBuffer& buffer, AddrSize size)
{
	if (size < Kilo)
	{
		sprintf(buffer, "%i byte(s)", (U32)size);
	}
	else if (size < Meg)
	{
		sprintf(buffer, "%.3f KB(s)", (F32)size / Constants<float>::Kilo());
	}
	else if (size < Gig)
	{
		sprintf(buffer, "%.3f MB(s)", (F32)size / Constants<float>::Meg());
	}
	else
	{
		sprintf(buffer, "%.3f GB(s)", (F32)((F64)size / Constants<double>::Gig()));
	}
}

#if defined(RAD_OPT_PC)
#if defined(RAD_OPT_WIN)
#include <windows.h>
#undef MessageBox
RADRT_API int RADRT_CALL MessageBox(const char *title, const char *message, MessageBoxStyle style)
{
	UINT z = MB_OK;
	if (style == MBStyleOkCancel)
		z = MB_OKCANCEL;
	if (style == MBStyleYesNo)
		z = MB_YESNO;
	int x = MessageBoxA(0, message, title, z);
	if (x == IDOK)
		x = MBOk;
	if (x == IDCANCEL)
		x = MBCancel;
	if (x == IDYES)
		x = MBYes;
	if (x == IDNO)
		x = MBNo;
	return x;
}
#else
#include <stdlib.h>
RADRT_API int RADRT_CALL MessageBox(const char *title, const char *message, MessageBoxStyle style)
{
	std::string buttons;
	// Note values match (MBOk, MBCancel, MBYes, MBNo) + 2
	if (style == MBStyleOk)
		buttons = "-buttons OK:2 -default OK";
	if (style == MBStyleOkCancel)
		buttons = "-buttons OK:2, Cancel:3 -default OK";
	if (style == MBStyleYesNo)
		buttons = "-buttons Yes:4, No:5 -default Yes";
	std::string cmd("xmessage ");
	cmd += buttons;
	cmd += " -center ";
	cmd += message;

	int r = system(cmd.c_str());
	RAD_ASSERT(r>=2);
	return r-2;
}
#endif
#endif

