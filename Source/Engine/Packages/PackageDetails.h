// PackageDetails.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "PackagesDef.h"
#include <Runtime/Thread/Interlocked.h>
#include <Runtime/Thread/Locks.h>

///////////////////////////////////////////////////////////////////////////////
namespace pkg {
namespace details {

typedef boost::shared_mutex SharedMutex;
typedef boost::lock_guard<SharedMutex> WriteLock;
typedef boost::shared_lock<SharedMutex> ReadLock;
typedef thread::unsafe_upgrade_to_exclusive_lock<SharedMutex> UpgradeToWriteLock;
typedef thread::Interlocked<UReg> UInterlocked;

struct SinkFactoryBase {
	typedef boost::shared_ptr<SinkFactoryBase> Ref;
	virtual SinkBase *New() = 0;
	virtual int Stage() const = 0;

	SinkBase *Cast(const AssetRef &asset);
	AssetIdWMap assets[Z_Max];
};

#if defined(RAD_OPT_TOOLS)

struct CookerFactoryBase {
	typedef boost::shared_ptr<CookerFactoryBase> Ref;
	virtual CookerRef New() = 0;
};

#endif

} // details
} // pkg
