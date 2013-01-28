/*! \file UIWidgetDef.h
	\copyright Copyright (c) 2012 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\author Joe Riedel
	\ingroup ui
*/

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
