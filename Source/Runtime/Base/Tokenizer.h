/*! \file Tokenizer.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#pragma once

#include "Base.h"
#include "../String.h"
#include "../Stream.h"
#include "../FileDef.h"
#include "../PushPack.h"

class RADRT_CLASS Tokenizer {
public:

	enum Result {
		kResult_Success,
		kResult_UnterminatedQuote,
		kResult_UnterminatedComment,
		kResult_EOL,
		kResult_EOF
	};

	enum TokenMode {
		kTokenMode_SameLine,
		kTokenMode_CrossLine
	};

	Tokenizer();
	explicit Tokenizer(stream::InputStream &is);
	explicit Tokenizer(const file::MMFileInputBufferRef &ib);
	~Tokenizer();

	void Bind(stream::InputStream &is);
	void Bind(const file::MMFileInputBufferRef &ib);

	bool Skip(int numTokens, TokenMode mode);
	bool GetToken(String &token, TokenMode mode);

	enum FetchMode {
		kFetchMode_RestOfLine,
		kFetchMode_RestOfFile
	};

	//! Gets the remaining text in the token stream.
	bool GetRemaining(String &token, FetchMode mode);

	bool GetFloat(float &f, TokenMode mode);
	bool GetInt(int &i, TokenMode mode);

	void UngetToken();

	bool FindToken(const char *token, TokenMode mode);
	bool FindTokeni(const char *token, TokenMode mode);

	bool IsNextToken(const char *token, TokenMode mode);
	bool IsNextTokeni(const char *token, TokenMode mode);

	void Restart();
	void Reset();
	String ErrorMessage() const;

	RAD_DECLARE_READONLY_PROPERTY(Tokenizer, line, int);
	RAD_DECLARE_READONLY_PROPERTY(Tokenizer, result, Result);

private:

	String m_token;
	stream::InputStream m_iis;
	file::MMFileInputBufferRef m_ib;
	stream::InputStream *m_is;
	int m_line;
	int m_errorLine;
	int m_peek;
	int m_char;
	Result m_result;
	bool m_unget;
	bool m_unfetch;
	bool m_inToken;

	RAD_DECLARE_GET(line, int) {
		return m_line;
	}

	RAD_DECLARE_GET(result, Result) {
		return m_result;
	}

	enum InternalFetchMode {
		kInternalFetchMode_Token,
		kInternalFetchMode_AllowWhitespace,
		kInternalFetchMode_DiscardWhitespace
	};

	enum NewLineMode {
		kNewLineMode_StopAtEndOfLine,
		kNewLineMode_CrossLines
	};

	void ParseQuote();
	void SkipLineComment();
	void SkipBlockComment();

	int Fetch(InternalFetchMode fetchMode, NewLineMode newLineMode);
	int Peek(InternalFetchMode fetchMode, NewLineMode newLineMode);
	int PeekFetch(InternalFetchMode fetchMode, NewLineMode newLineMode);
	void FetchEndOfLine();
	void FlushPeek();
	int FetchChar();
	void UnfetchChar();
	void ResetFetchState();
};

#include "../PopPack.h"
