// ImageCodecDef.inl
// Image Codec Inlines.
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

namespace image_codec {

//////////////////////////////////////////////////////////////////////////////////////////
// image_codec::IsPalettedFormat()
//////////////////////////////////////////////////////////////////////////////////////////

inline bool IsPalettedFormat(UReg format)
{
	return (format == Format_PAL8_RGB555) || 
		(format == Format_PAL8_RGB565) ||
		(format == Format_PAL8_RGB888) ||
		(format == Format_PAL8_RGBA8888);
}

} // image_codec
