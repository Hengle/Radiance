// GCCOpts.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#define RADENG_API   __attribute__ ((visibility("default")))
#define RADENG_CALL
#define RADENG_CLASS RADENG_API
#define RADENG_TEMPLATE_EXTERN
#define RADENG_TEMPLATE_EXPLICIT(_c_) RADENG_TEMPLATE_EXTERN template class RADENG_CLASS _c_
