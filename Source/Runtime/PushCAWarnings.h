// PushCAWarnings.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#if defined(PRIVATE_RAD_OPT_PUSH_CAWARNINGS)
	#error "umatched/nested 'Runtime/PushCAWarnings.h' detected"
#endif

#if defined(RAD_OPT_CA_ENABLED)
#if defined(RAD_OPT_WINX)
	#if !defined(ALL_CODE_ANALYSIS_WARNINGS)
		#include <CodeAnalysis/Warnings.h>
	#endif

	#if !defined(CAWARN_DISABLE)
		#define CAWARN_DISABLE ALL_CODE_ANALYSIS_WARNINGS
	#endif

	#pragma warning (push)
	#pragma warning (disable:CAWARN_DISABLE)
#endif
#endif

#undef CAWARN_DISABLE

#define PRIVATE_RAD_OPT_PUSH_CAWARNINGS
