// SoundDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#pragma once

#include "Types.h"

///////////////////////////////////////////////////////////////////////////////

enum SoundChannel
{
	SC_First,
	SC_UI = SC_First,
	SC_Ambient,
	SC_FX,
	SC_Music,
	SC_Max
};

class Sound;
typedef boost::shared_ptr<Sound> SoundRef;
typedef boost::weak_ptr<Sound> SoundWRef;
class SoundContext;
typedef boost::shared_ptr<SoundContext> SoundContextRef;
typedef boost::weak_ptr<SoundContext> SoundContextWRef;
class SoundDevice;
typedef boost::shared_ptr<SoundDevice> SoundDeviceRef;
typedef boost::weak_ptr<SoundDevice> SoundDeviceWRef;
