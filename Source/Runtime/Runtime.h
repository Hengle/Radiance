/*! \file Runtime.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup packages
*/

/*! \defgroup runtime Radiance Runtime
	The Radiance Runtime is a utility library that provides basic application libraries. Things
	like Strings, Files, Memory, Image Manipulation, Meta Reflection, Math, etc are included
	in the runtime systems.
*/

#pragma once

#include "Base.h"
#include "String.h"
#include "Thread.h"
#include "Time.h"
#include "Endian.h"
#include "Stream.h"
#include "PushPack.h"


namespace rt {

enum RuntimeFlags
{
	RAD_FLAG(RFNoDefaultThreads)
};

RADRT_API void RADRT_CALL Initialize(
	RuntimeFlags flags = RuntimeFlags(0),
	thread::IThreadContext *context = 0 // context used for task pump
);

RADRT_API void RADRT_CALL Finalize();
RADRT_API void RADRT_CALL ThreadInitialize();
RADRT_API void RADRT_CALL ThreadFinalize();
RADRT_API void RADRT_CALL ProcessTasks();

} // rt


#include "PopPack.h"
