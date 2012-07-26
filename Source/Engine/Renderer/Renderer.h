/*! \file Renderer.h
	\copyright Copyright (c) 2010 Sunside Inc., All Rights Reserved.
	\copyright See Radiance/LICENSE for licensing terms.
	\ingroup renderer
*/

/*! \defgroup renderer Rendering System
*/

#pragma once

#include "RendererDef.h"
#include <Runtime/Container/ZoneVector.h>

namespace r {

class VidMode {
public:
	
	VidMode() : w(0), h(0), bpp(0), hz(0), fullscreen(false) {}
	VidMode(const VidMode &v) : w(v.w), h(v.h), bpp(v.bpp), hz(v.hz), fullscreen(v.fullscreen) {}
	VidMode(int _w, int _h, const VidMode &v) : w(_w), h(_h), bpp(v.bpp), hz(v.hz), fullscreen(v.fullscreen) {}
	VidMode(
		int _w,
		int _h,
		int _bpp,
		int _hz,
		bool _fullscreen) : w(_w), h(_h), bpp(_bpp), hz(_hz), fullscreen(_fullscreen)
	{}

	int w, h, bpp, hz;
	bool fullscreen;

	bool Standard() const { 
		return Is4x3(); 
	}

	bool Wide() const { 
		return !Standard(); 
	}

	bool Is4x3() const { 
		return !(Is16x9() || Is16x10()); 
	}

	bool Is16x9() const { 
		return Div(16, 9); 
	}

	bool Is16x10() const { 
		return Div(16, 10); 
	}

	bool SameAspect(const r::VidMode &v) const {
		if (Is4x3())
			return v.Is4x3();
		if (Is16x9())
			return v.Is16x9();
		if (Is16x10())
			return v.Is16x10();
		return true; // unrecognizable.
	}
	
	bool SameSize(const r::VidMode &v) const {
		return (w == v.w) && (h == v.h);
	}

private:

	bool Div(int x, int y) const;
};

typedef zone_vector<VidMode, ZRenderT>::type VidModeVec;
typedef boost::shared_ptr<VidModeVec> VidModeVecRef;

//////////////////////////////////////////////////////////////////////////////////////////

RAD_REFLECTED_INTERFACE_BEGIN(IRenderer, IInterface, r.IRenderer)

	virtual void Initialize(int version = Version) = 0;

	virtual void ClearBackBuffer() = 0;
	virtual void ClearDepthBuffer() = 0;
	virtual void CommitStates() = 0;
	virtual void BindFramebuffer() = 0;
	virtual void UnbindStates() = 0;

#if !defined(RAD_OPT_PC_TOOLS)
	// handled by Qt in tools builds
	virtual void SwapBuffers() = 0;
#endif

	RAD_DECLARE_PROPERTY(IRenderer, ctx, const HContext&, const HContext&);

protected:

	virtual RAD_DECLARE_GET(ctx, const HContext&) = 0;
	virtual RAD_DECLARE_SET(ctx, const HContext&) = 0;
	
RAD_INTERFACE_END

RAD_REFLECTED_INTERFACE_BEGIN(IContext, IInterface, r.IContext)

RAD_INTERFACE_END

} // r
