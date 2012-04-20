// Tokenizer.cpp
// Copyright (c) 2010 Pyramind Labs LLC, All Rights Reserved
// Author: Joe Riedel (joeriedel@hotmail.com)
// See Radiance/LICENSE for licensing terms.

#include "Tokenizer.h"
#include "../Runtime/Base.h"

Tokenizer::Tokenizer()
{
	m_bUnget = false;
	m_pBuffer = 0;
	m_pOffset = 0;
	m_sToken  = "";
	m_pTokenStart = 0;
	m_pTokenEnd = 0;
}

Tokenizer::Tokenizer( const Tokenizer& t )
{
	this->m_nNumBytes = t.m_nNumBytes;
	this->m_nLine = t.m_nLine;
	this->m_bUnget = t.m_bUnget;
	this->m_sToken = t.m_sToken;
	
	if( t.m_nNumBytes > 0 )
	{
		this->m_pBuffer = (char*)zone_malloc(ZRuntime, t.m_nNumBytes, 0);
		RAD_ASSERT( this->m_pBuffer );
		this->m_pOffset = this->m_pBuffer + (int)(t.m_pOffset-t.m_pBuffer);
	}
	else
	{
		this->m_pBuffer = this->m_pOffset = 0;
	}	
}

Tokenizer::~Tokenizer()
{
	FreeScript();
}

void Tokenizer::SaveState()
{
	m_sr.m_bUnget = m_bUnget;
	m_sr.m_pBuffer = m_pBuffer;
	m_sr.m_pOffset = m_pOffset;
	m_sr.m_sToken = m_sToken;
	m_sr.m_nLine = m_nLine;
	m_sr.m_nNumBytes = m_nNumBytes;
	m_sr.m_pTokenStart = m_pTokenStart;
	m_sr.m_pTokenEnd = m_pTokenEnd;
}

void Tokenizer::RestoreState()
{
	m_bUnget = m_sr.m_bUnget;
	m_pBuffer = m_sr.m_pBuffer;
	m_pOffset = m_sr.m_pOffset;
	m_sToken = m_sr.m_sToken;
	m_nLine = m_sr.m_nLine;
	m_nNumBytes = m_sr.m_nNumBytes;
	m_pTokenStart = m_sr.m_pTokenStart;
	m_pTokenEnd = m_sr.m_pTokenEnd;
}

const char* Tokenizer::GetTokenStart()
{
	return m_pTokenStart;
}

const char* Tokenizer::GetTokenEnd()
{
	return m_pTokenEnd;
}

bool Tokenizer::FindToken( const char* token )
{
	std::string t;
	while( GetToken(t) )
	{
		if(!strcmp( t.c_str(), token ) )
			return true;
	}
	return false;
}

bool Tokenizer::FindTokeni( const char* token )
{
	std::string t;
	while( GetToken(t) )
	{
		if(!stricmp( t.c_str(), token ) )
			return true;
	}
	return false;
}

bool Tokenizer::IsNextToken( const char* token )
{
	std::string t;
	return GetToken(t) && !strcmp(t.c_str(), token);
}

bool Tokenizer::IsNextTokeni( const char* token )
{
	std::string t;
	return GetToken(t) && !stricmp(t.c_str(), token);
}
	
void Tokenizer::FreeScript()
{
	if(m_pBuffer != 0 && m_pBuffer != m_default_buff )
		zone_free(m_pBuffer);
	m_pBuffer = 0;
}

void Tokenizer::Skip(int nNum)
{
	int i;
	std::string s;

	for(i = 0; i < nNum; i++)
		GetToken(s);
}

bool Tokenizer::InitParsing(const char* szScript, int nNumBytes)
{
	FreeScript();

	if(nNumBytes == WholeFile)
		nNumBytes = (int)strlen(szScript);

	if( nNumBytes > DefaultTokenizerSize )
	{
		m_pBuffer = (char*)zone_malloc(ZRuntime, nNumBytes+1, 0);
		if(m_pBuffer == 0)
			return false;
	}
	else
	{
		m_pBuffer = m_default_buff;
	}
	
	memcpy(m_pBuffer, szScript, nNumBytes);
	m_nNumBytes = nNumBytes;
	m_nLine = 0;
	m_pOffset = m_pBuffer;
	m_pBuffer[nNumBytes] = 0;
	m_pTokenStart = m_pOffset;
	m_pTokenEnd = m_pOffset;

	return true;
}

void Tokenizer::RestartParsing()
{
	m_sToken = "";
	m_pOffset = m_pBuffer;
	m_nLine = 0;
}

bool Tokenizer::GetFloat( float* f )
{
	std::string t;
	if( !GetToken(t) )
		return false;
	*f = (float)atof(t.c_str());
	return true;
}

bool Tokenizer::GetInt( int* i )
{
	std::string t;
	if( !GetToken(t) )
		return false;
	*i = (int)atoi(t.c_str());
	return true;
}

// Get's the next token out of the input stream.
bool Tokenizer::GetToken(std::string& sToken)
{
	if(m_bUnget)
	{
		m_bUnget = false;
		sToken = m_sToken;
		return true;
	}

	sToken = "";
	m_sToken = "";

	if(m_pOffset == NULL || *m_pOffset == '\0')
		return false;

CTokenizer_GetToken_Start:

	// Skip to the next.
	if(*m_pOffset <= 32 && *m_pOffset != '\0')
	{
		while(*m_pOffset <= 32 && *m_pOffset != '\0')
		{
			if(*m_pOffset == '\n')
				m_nLine++;

			m_pOffset++;
		}
	}

	if(*m_pOffset == '\0')
		return false;

	// Skip any comments.
	if(*m_pOffset == '/')
	{
		switch(m_pOffset[1])
		{

		case '/':

			while(*m_pOffset != '\n' && *m_pOffset != '\0')
				m_pOffset++;

			if(*m_pOffset == '\0')	// Bad
				return false;
			else
				m_pOffset++;

			m_nLine++;
			goto CTokenizer_GetToken_Start;

			break;

		case '*':

			// Skip till the next */.
			{
				char *TempPtr;
				TempPtr = m_pOffset;
				while(*TempPtr != '\0' && (*TempPtr != '*' || TempPtr[1] != '/'))
				{
					if(*TempPtr == '\n')
						m_nLine++;

					TempPtr++;
				}
	
				if(*TempPtr == '\0') {
					m_pOffset = TempPtr;
					return false;
				} else
					TempPtr+=2;
				m_pOffset = TempPtr;
				goto CTokenizer_GetToken_Start;
			}

			break;
		}
	}
	else if(*m_pOffset == '"')	// Quoted string?
	{
		m_pTokenStart = m_pOffset;

		// Read till closing ".
		m_pOffset++;
		while(*m_pOffset != '\0' && *m_pOffset != '"')
		{
			// Allow \" for a quote character in a string and a \\ to be a single \*
			if(*m_pOffset == '\\')
			{
				if(m_pOffset[1] == '"' || m_pOffset[1] == '\\')
					m_pOffset++;
			}

			if(*m_pOffset == '\n')
				m_nLine++;
			else
				m_sToken += *m_pOffset;
			m_pOffset++;
		}

		if(*m_pOffset != '\0')
			m_pOffset++;
		
		m_pTokenEnd = m_pOffset;

		sToken = m_sToken;
		return true;
	}

	m_pTokenStart = m_pOffset;

	// Load the next white space delim token.
	while(*m_pOffset > 32 && *m_pOffset != '\0')
	{
		m_sToken += *m_pOffset;
		m_pOffset++;
	}
	
	m_pTokenEnd = m_pOffset;

	// Copy the token.
	sToken = m_sToken;

	return true;
}