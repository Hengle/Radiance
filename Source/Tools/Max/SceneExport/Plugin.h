// Plugin.h
// Copyright (c) 2009 Pyramind Labs LLC, All Rights Reserved

#pragma once

#include "max.h"

class Plugin : public SceneExport
{
public:

	virtual int ExtCount() { return 1; }
	virtual const TCHAR *Ext(int n) { return _T("RSCN"); }

	virtual const TCHAR *LongDesc() { return _T("Radiance Scene"); }
	virtual const TCHAR *ShortDesc() { return _T("Radiance Scene"); }
	virtual const TCHAR *AuthorName() { return _T("Pyramind Labs, LLC"); }
	virtual const TCHAR *CopyrightMessage() { return _T("Copyright (c) 2009 Pyramind Labs, LLC"); }
	virtual const TCHAR *OtherMessage1()	{ return _T(""); }
	virtual const TCHAR *OtherMessage2()	{ return _T(""); }
	virtual unsigned int Version() { return 1; }
	virtual void ShowAbout(HWND hWnd);
	virtual int	DoExport(const TCHAR *name, ExpInterface *ei,Interface *ip, BOOL suppressPrompts, DWORD options);

	Interface *MaxInterface() { return m_i; }
	ExpInterface *MaxExportInterface() { return m_ei; }

	static bool QueryUnload() { return s_busy == 0; }
private:

	static int s_busy;

	Interface *m_i;
	ExpInterface *m_ei;
};