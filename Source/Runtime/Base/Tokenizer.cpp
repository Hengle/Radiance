/*! \file Tokenizer.cpp
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup runtime
*/

#include RADPCH
#include "Tokenizer.h"
#include "../Stream.h"

namespace {
enum {
	InvalidChar = -1
};
}

Tokenizer::Tokenizer() : 
m_is(0) {
	Restart();
}

Tokenizer::Tokenizer(stream::InputStream &is) {
	Bind(is);
}

Tokenizer::Tokenizer(const file::MMFileInputBufferRef &ib) {
	Bind(ib);
}

Tokenizer::~Tokenizer() {
}

void Tokenizer::Bind(stream::InputStream &is) {
	Reset();
	m_is = &is;
}

void Tokenizer::Bind(const file::MMFileInputBufferRef &ib) {
	Reset();
	m_ib = ib;
	m_iis.SetBuffer(*ib);
	m_is = &m_iis;
}

bool Tokenizer::Skip(int numTokens, TokenMode mode) {

	String s;
	while (numTokens-- > 0) {
		if (!GetToken(s, mode))
			return false;
	}

	return true;
}

bool Tokenizer::GetToken(String &token, TokenMode mode) {
	token.Clear();

	if ((m_result != kResult_Success) && (m_result != kResult_EOL))
		return false;

	if (m_unget) {
		token = m_token;
		m_unget = false;
		return m_result != kResult_EOL;
	}

	m_token.Clear();

	bool firstChar = true;

	ResetFetchState();

	for (;;) {
		int r = Fetch(
			kInternalFetchMode_Token,
			(mode == kTokenMode_SameLine) ? kNewLineMode_StopAtEndOfLine : kNewLineMode_CrossLines
		);
		if (r == InvalidChar) // end of file or line.
			break;

		if (firstChar && (r == '"')) { // starting a quoted line
			ParseQuote();
			break;
		} else if (r == '/') {
			int p = Peek(
				kInternalFetchMode_Token,
				kNewLineMode_StopAtEndOfLine
			);
			if (p == '/') {
				FlushPeek();
				SkipLineComment();
				if (m_result != kResult_Success)
					break;
				if (firstChar)
					continue;
				break;
			} else if (p == '*') {
				FlushPeek();
				SkipBlockComment();
				if (m_result != kResult_Success)
					break;
				if (firstChar)
					continue;
				break;
			}
		}

		m_token += (char)r;
		firstChar = false;
	}

	token = m_token;

	if (m_result == kResult_EOF) {
		if (!token.empty)
			return true;
	}

	return m_result == kResult_Success;
}

bool Tokenizer::GetRemaining(String &token, FetchMode mode) {
	token.Clear();
	
	NewLineMode newLineMode = 
		(mode == kFetchMode_RestOfLine) ? kNewLineMode_StopAtEndOfLine : kNewLineMode_CrossLines;

	int c;
	while ((c=Fetch(kInternalFetchMode_AllowWhitespace, newLineMode)) != InvalidChar) {
		token += (char)c;
	}

	if (m_result == kResult_EOF) {
		if (!token.empty)
			return true;
	}

	return m_result == kResult_Success;
}

bool Tokenizer::GetFloat(float &f, TokenMode mode) {
	String s;
	bool r;
	if ((r=GetToken(s, mode)))
		sscanf(s.c_str.get(), "%f", &f);
	return r;
}

bool Tokenizer::GetInt(int &i, TokenMode mode) {
	String s;
	bool r;
	if ((r=GetToken(s, mode)))
		sscanf(s.c_str.get(), "%d", &i);
	return r;
}

void Tokenizer::UngetToken() {
	m_unget = true;
}

bool Tokenizer::FindToken(const char *token, TokenMode mode) {
	RAD_ASSERT(token);

	String s(CStr(token));
	String t;
	bool r;

	while ((r=GetToken(t, mode))) {
		if (s == t)
			return true;
	}

	return r;
}

bool Tokenizer::FindTokeni(const char *token, TokenMode mode) {
	RAD_ASSERT(token);

	String s(CStr(token));
	String t;
	bool r;

	while ((r=GetToken(t, mode))) {
		if (s.Comparei(t) == 0)
			return true;
	}

	return r;
}

bool Tokenizer::IsNextToken(const char *token, TokenMode mode) {
	RAD_ASSERT(token);

	String t;
	bool r;

	if ((r = GetToken(t, mode))) {
		if (t != CStr(token))
			return false;
	}

	return r;
}

bool Tokenizer::IsNextTokeni(const char *token, TokenMode mode) {
	RAD_ASSERT(token);

	String t;
	bool r;

	if ((r = GetToken(t, mode))) {
		if (t.Comparei(token))
			return false;
	}

	return r;
}

void Tokenizer::Restart() {
	m_line = 1;
	m_errorLine = 1;
	m_result = kResult_Success;
	m_peek = InvalidChar;
	m_char = InvalidChar;
	m_unget = false;
	m_unfetch = false;
	m_inToken = false;
	m_token.Clear();

	if (m_is)
		m_is->SeekIn(stream::StreamBegin, 0, 0);
}

void Tokenizer::Reset() {
	m_ib.reset();
	m_is = 0;
	Restart();
}

String Tokenizer::ErrorMessage() const {

	String s;

	switch (m_result) {
	case kResult_UnterminatedQuote:
		s.PrintfASCII("Unterminated quote on line %d.", m_errorLine);
		return s;
	case kResult_UnterminatedComment:
		s.PrintfASCII("Unterminated block comment on line %d.", m_errorLine);
		return s;
	case kResult_EOL:
		s.PrintfASCII("Expected more tokens on line %d.", m_errorLine);
		return s;
	case kResult_EOF:
		return CStr("End of file.");
	}

	return CStr("Success");
}

void Tokenizer::ParseQuote() {
	m_errorLine = m_line; // in-case of error.

	for (;;) {
		int r = Fetch(
			kInternalFetchMode_AllowWhitespace,
			kNewLineMode_StopAtEndOfLine
		);

		if (r == InvalidChar) {
			m_result = kResult_UnterminatedQuote;
			break;
		}

		if (r == '"') {
			// terminating quote.
			m_result = kResult_Success;
			break;
		}

		m_token += (char)r;
	}
}

void Tokenizer::SkipLineComment() {
	m_errorLine = m_line; // in-case of error.
	FetchEndOfLine();
}

void Tokenizer::SkipBlockComment() {
	m_errorLine = m_line; // in-case of error.
	m_inToken = false;

	for (;;) {
		int r = Fetch(
			kInternalFetchMode_AllowWhitespace,
			kNewLineMode_CrossLines
		);

		if (r == InvalidChar) {
			m_result = kResult_UnterminatedComment;
			break;
		}

		if (r == '*') {
			int p = Peek(
				kInternalFetchMode_AllowWhitespace,
				kNewLineMode_CrossLines
			);
			if (p == '/') {
				FlushPeek();
				m_result = kResult_Success;
				break;
			}
		}
	}
}

int Tokenizer::Fetch(InternalFetchMode fetchMode, NewLineMode newLineMode) {
	int p = Peek(fetchMode, newLineMode);
	FlushPeek();
	return p;
}

int Tokenizer::Peek(InternalFetchMode fetchMode, NewLineMode newLineMode) {
	if (m_peek != InvalidChar)
		return m_peek;
	return PeekFetch(fetchMode, newLineMode);
}

int Tokenizer::PeekFetch(InternalFetchMode fetchMode, NewLineMode newLineMode) {

	m_peek = InvalidChar;
	
	if ((fetchMode == kInternalFetchMode_DiscardWhitespace) || 
		((fetchMode == kInternalFetchMode_Token) && !m_inToken)) {
		int c;
		while ((c=FetchChar()) != InvalidChar) {
			if (c > 32)
				break;
			if (c == '\n') {
				++m_line;
				if (newLineMode == kNewLineMode_StopAtEndOfLine) {
					if (fetchMode == kInternalFetchMode_Token)
						m_result = kResult_EOL;
					return InvalidChar;
				}
			}
		}

		if (c == InvalidChar)
			return InvalidChar;
		UnfetchChar();

		if (fetchMode == kInternalFetchMode_Token)
			m_inToken = true;
	}

	int c = FetchChar();
	if (c <= 32) {
		if (c == '\n') {
			++m_line;
			if ((fetchMode != kInternalFetchMode_Token) &&
				(newLineMode == kNewLineMode_StopAtEndOfLine)) {
				return InvalidChar;
			}
		}
		if (fetchMode == kInternalFetchMode_Token) {
			m_inToken = false;
			return InvalidChar;
		}
	}

	m_peek = c;
	return c;
}

void Tokenizer::FetchEndOfLine() {
	m_inToken = false;
	while (Fetch(kInternalFetchMode_AllowWhitespace, kNewLineMode_StopAtEndOfLine) != InvalidChar) {
	}
}

void Tokenizer::FlushPeek() {
	m_peek = InvalidChar;
}

int Tokenizer::FetchChar() {
	if (!m_is) {
		m_result = kResult_EOF;
		return InvalidChar;
	}

	m_result = kResult_Success;

	if (m_unfetch) {
		m_unfetch = false;
		return m_char;
	}

	while (m_is->Read(&m_char, 1, 0) == 1) {
		m_char = m_char&0xff;
		if (m_char != '\r')
			return m_char;
	}

	m_result = kResult_EOF;
	m_char = InvalidChar;
	return InvalidChar;
}

void Tokenizer::UnfetchChar() {
	if (m_char != InvalidChar)
		m_unfetch = true;
}

void Tokenizer::ResetFetchState() {
	m_inToken = false;
}
