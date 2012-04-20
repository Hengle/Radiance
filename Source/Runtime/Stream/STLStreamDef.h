// StreamDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "IntStream.h"
#include "../PushPack.h"


namespace stream {

//////////////////////////////////////////////////////////////////////////////////////////
// stream forward declarations
//////////////////////////////////////////////////////////////////////////////////////////

template<typename _Elem, typename _Traits, typename _Alloc> class basic_stringbuf;
template<typename _Elem, typename _Traits> class basic_streambuf;
template<typename _Elem, typename _Traits> class basic_streambuf_adapter;
template<typename _Elem, typename _Traits> class basic_istream_adapter;
template<typename _Elem, typename _Traits> class basic_ostream_adapter;
template<typename _Elem, typename _Traits> class basic_iostream_adapter;

//////////////////////////////////////////////////////////////////////////////////////////
// stream type definitions
//////////////////////////////////////////////////////////////////////////////////////////

typedef basic_streambuf<char, std::char_traits<char> >               streambuf;

typedef basic_streambuf_adapter<char, std::char_traits<char> >       streambuf_adapter;
typedef basic_streambuf_adapter<wchar_t, std::char_traits<wchar_t> > wstreambuf_adapter;

typedef basic_istream_adapter<char, std::char_traits<char> >       istream_adapter;
typedef basic_istream_adapter<wchar_t, std::char_traits<wchar_t> > wistream_adapter;

typedef basic_ostream_adapter<char, std::char_traits<char> >       ostream_adapter;
typedef basic_ostream_adapter<wchar_t, std::char_traits<wchar_t> > wostream_adapter;

typedef basic_iostream_adapter<char, std::char_traits<char> >       iostream_adapter;
typedef basic_iostream_adapter<wchar_t, std::char_traits<wchar_t> > wiostream_adapter;

} // stream


#include "../PopPack.h"
