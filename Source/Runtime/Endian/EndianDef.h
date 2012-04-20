// EndianDef.h
// Endian Conversion.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once


namespace endian {

//////////////////////////////////////////////////////////////////////////////////////////
// endian::BYTESWAPFIELDOFS
//////////////////////////////////////////////////////////////////////////////////////////

typedef U16 BYTESWAPFIELDOFS;

//////////////////////////////////////////////////////////////////////////////////////////
// endian::ByteSwapCodes
//////////////////////////////////////////////////////////////////////////////////////////

struct ByteSwapCodes
{
	U16 code;
	BYTESWAPFIELDOFS field_ofs;
	BYTESWAPFIELDOFS array_ofs;
	U16 array_stride;
	U32 field_array_code_or_count;
	struct ByteSwapCodes* struct_swap_codes;
};

} // endian

