// Tokenizer.h
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#pragma once

#include <string>

class Tokenizer
{
public:
 
	enum { DefaultTokenizerSize = 256, WholeFile = 0xffffffff };

	Tokenizer();
	Tokenizer( const Tokenizer& t );
	virtual ~Tokenizer();

	bool InitParsing(const char *filename, int nNumBytes);
	void FreeScript();
	void Skip(int nNum=1);	// Skips n tokens.
	bool GetToken(std::string& sToken);
	bool GetFloat(float* f);
	bool GetInt(int* i);
	int GetNumBytes() { return m_nNumBytes; }
	int GetByteOffset() { return (int)(m_pOffset-m_pBuffer); }
	int  GetLine() { return m_nLine+1; }
	void UngetToken() {m_bUnget = true; }
	bool FindToken( const char* token );
	bool FindTokeni( const char* token );
	bool IsNextToken( const char* token );
	bool IsNextTokeni( const char* token );
	void RestartParsing();
	void SaveState();
	void RestoreState();

	const char* GetTokenStart();
	const char* GetTokenEnd();

private:

	char m_default_buff[DefaultTokenizerSize];
	
	bool m_bUnget;
	char* m_pBuffer;
	char* m_pOffset;
	std::string m_sToken;
	int m_nLine;
	int m_nNumBytes;
	const char* m_pTokenStart;
	const char* m_pTokenEnd;

	struct m_sr_t
	{
		m_sr_t() {}
		bool m_bUnget;
		char* m_pBuffer;
		char* m_pOffset;
		std::string m_sToken;
		int m_nLine;
		int m_nNumBytes;
		const char* m_pTokenStart;
		const char* m_pTokenEnd;
	} m_sr;
};