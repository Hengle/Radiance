/* inffast.h -- header to use inffast.c
 * Copyright (C) 1995-2002 Mark Adler
 * For conditions of distribution and use, see copyright notice in zlib.h
 */

/* WARNING: this file should *not* be used by applications. It is
   part of the implementation of the compression library and is
   subject to change. Applications should only use zlib.h.
 */

//////////////////////////////////////////////////////////////////////////////////////////
// Begin namespace data_codec::zlib
//////////////////////////////////////////////////////////////////////////////////////////


namespace data_codec {
namespace zlib {

void inflate_fast OF((z_streamp strm, unsigned start));

//////////////////////////////////////////////////////////////////////////////////////////
// End namespace data_codec::zlib
//////////////////////////////////////////////////////////////////////////////////////////

} // namespace zlib
} // namespace data_codec

