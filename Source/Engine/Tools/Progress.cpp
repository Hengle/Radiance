// Progress.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH

#if defined(RAD_OPT_TOOLS)

#include "Progress.h"

namespace tools {

RADENG_API NullUIProgress_t NullUIProgress;

boost::thread_specific_ptr<UIProgress::UIStringBuf> UIProgress::s_uiStringBufPtr[S_Max];
boost::thread_specific_ptr<std::ostream> UIProgress::s_ostreamPtr[S_Max];

std::ostream &UIProgress::Out(Severity s)
{
	RAD_ASSERT(s >= 0 && s < S_Max);

	if (s_ostreamPtr[s].get() == 0)
	{
		RAD_ASSERT(s_uiStringBufPtr[s].get() == 0);
		s_uiStringBufPtr[s].reset(new UIStringBuf(*this, s));
		s_ostreamPtr[s].reset(new std::ostream(s_uiStringBufPtr[s].get()));
	}
	return *s_ostreamPtr[s].get();
}

} // tools

#endif // RAD_OPT_TOOLS
