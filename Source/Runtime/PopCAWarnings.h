// PopCAWarnings.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if !defined(PRIVATE_RAD_OPT_PUSH_CAWARNINGS)
	#error "no matching 'Runtime/PushCAWarnings.h' was included"
#endif

#if defined(RAD_OPT_CA_ENABLED)
#if defined(RAD_OPT_WINX)
	#pragma warning (pop)
#endif
#endif

#undef PRIVATE_RAD_OPT_PUSH_CAWARNINGS
