// UIWidgetDef.h
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include "../Types.h"
#include "../World/Lua/D_Asset.h"

namespace ui {

class RBDraw;
typedef boost::weak_ptr<RBDraw> RBDrawWRef;
typedef boost::shared_ptr<RBDraw> RBDrawRef;
class Root;
typedef boost::weak_ptr<Root> RootWRef;
typedef boost::shared_ptr<Root> RootRef;
class Widget;
typedef boost::weak_ptr<Widget> WidgetWRef;
typedef boost::shared_ptr<Widget> WidgetRef;

} // ui
