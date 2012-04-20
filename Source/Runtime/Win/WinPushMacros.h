// PushSystemMacros.h                                                                          
// Cleanup Win32 macro pollution.                                                           
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// See Radiance/LICENSE for licensing terms.

//////////////////////////////////////////////////////////////////////////////////////////
// Sanity Check                                                                        
//////////////////////////////////////////////////////////////////////////////////////////

#if !defined(RAD_OPT_WINX)
	#error "Include error, this file only works under windows!"
#endif

#pragma push_macro("BitScanForward")
#pragma push_macro("BitScanReverse")
#pragma push_macro("Yield")
#pragma push_macro("MessageBox")
#pragma push_macro("CreateDirectory")
#pragma push_macro("DeleteFile")
#pragma push_macro("max")
#pragma push_macro("min")
#undef BitScanForward
#undef BitScanReverse
#undef Yield
#undef MessageBox
#undef CreateDirectory
#undef DeleteFile
#undef min
#undef max
