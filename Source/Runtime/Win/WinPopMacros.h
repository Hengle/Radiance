// PopSystemMacros.h                                                                          
// Restore Win32 macro pollution.                                                           
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Sanity Check                                                                        
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(RAD_OPT_WINX)
	#error "Include error, this file only works under windows!"
#endif

#pragma pop_macro("BitScanForward")
#pragma pop_macro("BitScanReverse")
#pragma pop_macro("Yield")
#pragma pop_macro("MessageBox")
#pragma pop_macro("CreateDirectory")
#pragma pop_macro("DeleteFile")
#pragma pop_macro("max")
#pragma pop_macro("min")