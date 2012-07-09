// RBackend.cpp
// Copyright (c) 2010 Sunside Inc., All Rights Reserved
// PC Rendering Backend
// Author: Joe Riedel
// See Radiance/LICENSE for licensing terms.

#include RADPCH
#include "RBDetails.h"
#include "../../COut.h"
#include <SDL/SDL.h>
#include "../GL/GLTable.h"
#include "../GL/GLState.h"

#if defined(RAD_OPT_PC_TOOLS)
#include <QtOpenGL/QGLFormat>
#endif

#undef min
#undef max

using namespace string;

namespace r {

#if defined(RAD_OPT_PC_TOOLS)

RADENG_API void RADENG_CALL SetQGLFormat(QGLFormat &fmt)
{
	fmt.setAccum(false);
	fmt.setDepth(true);
	fmt.setDepthBufferSize(24);
	fmt.setDirectRendering(true);
	fmt.setDoubleBuffer(true);
	fmt.setSampleBuffers(false);
	fmt.setStencil(true);
	fmt.setStencilBufferSize(8);
	fmt.setStereo(false);
	fmt.setRgba(true);
	fmt.setPlane(0);
	fmt.setAlpha(true);
	fmt.setAlphaBufferSize(8);
	fmt.setRedBufferSize(8);
	fmt.setGreenBufferSize(8);
	fmt.setBlueBufferSize(8);
	fmt.setSwapInterval(0);
}

#endif

namespace {

void SetSDLGLAttributes(
		int redBits,
		int greenBits,
		int blueBits,
		int alphaBits,
		bool doubleBuffer,
		int depthBits,
		int stencilBits,
		int numMultiSampleSamples,
		int vsync,
		bool forceHardware
	)
{
	SDL_GL_SetAttribute(SDL_GL_RED_SIZE, redBits);
	SDL_GL_SetAttribute(SDL_GL_GREEN_SIZE, greenBits);
	SDL_GL_SetAttribute(SDL_GL_BLUE_SIZE, blueBits);
	SDL_GL_SetAttribute(SDL_GL_ALPHA_SIZE, alphaBits);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, doubleBuffer ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, depthBits);
	SDL_GL_SetAttribute(SDL_GL_STENCIL_SIZE, stencilBits);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLEBUFFERS, numMultiSampleSamples ? 1 : 0);
	SDL_GL_SetAttribute(SDL_GL_MULTISAMPLESAMPLES, numMultiSampleSamples);
	SDL_GL_SetAttribute(SDL_GL_SWAP_CONTROL, vsync);
	SDL_GL_SetAttribute(SDL_GL_ACCELERATED_VISUAL, forceHardware ? 1 : 0);
}

void SetSDLGLAttributes(
	bool doubleBuffer,
	int numMultiSampleSamples,
	int vsync,
	bool forceHardware
)
{
	SetSDLGLAttributes(
		8,
		8,
		8,
		8,
		doubleBuffer,
		24,
		8,
		numMultiSampleSamples,
		vsync,
		forceHardware
	);
}

// these came from: http://en.wikipedia.org/wiki/List_of_common_resolutions

int stdVidModes[] =
{
	640, 480, // 4:3
	768, 480, // 16:10
	800, 600, // 4:3
	856, 480, // 16:9
	960, 540, // 16:9
	960, 720, // 4:3
	1024, 576, // 16:9
	1024, 640, // 16:10
	1024, 768, // 4:3
	1152, 720, // 16:10
	1152, 864, // 4:3
	1280, 720, // 16:9
	1280, 768, // 16:10
	1280, 800, // 16:10
	1280, 960, // 4:3
	1280, 1024, // 4:3
	1366, 768, // 16:9
	1400, 1050, // 4:3
	1440, 900, // 16:10
	1440, 1080, // 4:3
	1600, 900, // 16:9
	1600, 1200, // 4:3
	1680, 1050, // 16:10
	1792, 1344, // 4:3
	1856, 1392, // 4:3
	1920, 1080, // 16:9
	1920, 1200, // 16:10
	1920, 1440, // 4:3
	2048, 1152, // 16:9
	2048, 1536, // 4:3
	2560, 1440, // 16:9
	2560, 1600, // 16:10
	2560, 1920, // 4:3
	2800, 2100, // 4:3
	3200, 2400, // 4:3
	3840, 2160, // 16:9
	3840, 2400, // 16:10
	4096, 2304, // 16:9
	4096, 3072, // 4:3
	5120, 3200, // 16:10
	6400, 4800, // 4:3
	7680, 4320, // 16:9
	7680, 4800, // 16:10
	-1
};

void MakeWindowedVidModeList(const VidMode &curMode, const int *modes, VidModeVec &out)
{
	RAD_ASSERT(modes);
	for (int i = 0;modes[i] != -1;i += 2)
	{
		if (modes[i] <= curMode.w && modes[i+1] <= curMode.h)
		{
			out.push_back(VidMode(modes[i], modes[i+1], curMode));
		}
	}
}

} // namespace

//////////////////////////////////////////////////////////////////////////////////////////

RAD_IMPLEMENT_COMPONENT(RBackend, r.RBackend);
RAD_IMPLEMENT_COMPONENT(RBContext, r.RBContext);

boost::thread_specific_ptr<HContext> RBackend::s_ctx;

RBackend::RBackend()
{
}

RBackend::~RBackend()
{
	VidReset();
}

void RBackend::Initialize(int version)
{
	if (version != Version) 
		throw BadInterfaceVersionException();
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(0, 0);
	GetCurMode();
	EnumVidModes();
}

void RBackend::ClearBackBuffer()
{
	glClearColor(0.f, 0.f, 0.f, 0.f);
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT|GL_STENCIL_BUFFER_BIT);
}

void RBackend::ClearDepthBuffer()
{
	glClear(GL_DEPTH_BUFFER_BIT);
}

void RBackend::SwapBuffers()
{
	SDL_GL_SwapBuffers();
}

void RBackend::CommitStates()
{
	gls.Commit();
}

void RBackend::BindFramebuffer()
{
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, 0);
	gls.BindBuffer(GL_RENDERBUFFER_EXT, 0);
}

void RBackend::UnbindStates()
{
	gls.DisableTextures();
	gls.DisableVertexAttribArrays(true);
	gls.DisableAllMGSources();
	gls.DisableAllMTSources();
	gls.UseProgram(0, true);
	gls.BindBuffer(GL_ELEMENT_ARRAY_BUFFER_ARB, GLVertexBufferRef());
	gls.BindBuffer(GL_ARRAY_BUFFER_ARB, GLVertexBufferRef());
	if (gl.vbos&&gl.vaos)
		gls.BindVertexArray(GLVertexArrayRef());
	gls.BindBuffer(GL_FRAMEBUFFER_EXT, 0);
	gls.BindBuffer(GL_RENDERBUFFER_EXT, 0);
}

VidModeVecRef RBackend::VidModeList(
	int width,
	int height,
	int bpp,
	int hz,
	int filter
)
{
	VidModeVecRef modes(new VidModeVec());

	for (VidModeVec::const_iterator it = m_modes.begin(); it != m_modes.end(); ++it)
	{
		const VidMode &m = *it;
		if ((width == 0 || (filter&ForceWidth) || m.w >= width) &&
			(width == 0 || !(filter&ForceWidth) || m.w==width) &&
			(height == 0 || (filter&ForceHeight) || m.h >= height) &&
			(height == 0 || !(filter&ForceHeight) || m.h == height) &&
			(bpp == 0 || (filter&MinBPP) || m.bpp >= bpp) &&
			(bpp == 0 || !(filter&MinBPP) || m.bpp == bpp) &&
			(hz == 0 || (filter&ForceHz) || m.hz >= hz) &&
			(hz == 0 || !(filter&ForceHz) || m.hz == hz) &&
			(!m.fullscreen || (filter&Fullscreen)) &&
			(m.fullscreen || (filter&Windowed)))
		{
			int z = 0;
			if (m.Is4x3()) z |= _4x3;
			if (m.Is16x9()) z |= _16x9;
			if (m.Is16x10()) z |= _16x10;

			if (!(filter&(_4x3|_16x9|_16x10)) || (z&filter))
			{
				if (CheckVidMode(m))
				{
					modes->push_back(m);
				}
			}
		}
	}

	return modes;
}

bool RBackend::CheckVidMode(const VidMode &mode)
{
	return SDL_VideoModeOK(mode.w, mode.h, mode.bpp, SDL_OPENGL|((mode.fullscreen)?SDL_FULLSCREEN:0))==mode.bpp;
}

bool RBackend::SetVidMode(const VidMode &mode)
{
	if (!CheckVidMode(mode)) 
		return false;
	VidReset();
	COut(C_Info) << "SetVidMode: " << mode.w << "x" << mode.h << "x" << mode.bpp << "x" << mode.hz << "x" << (mode.fullscreen ? "fullscreen" : "windowed") << std::endl;
	bool r = SDL_SetVideoMode(mode.w, mode.h, mode.bpp, SDL_OPENGL|(mode.fullscreen ? SDL_FULLSCREEN : 0)) != 0;
	if (r)
		m_curMode = mode;
	return r;
}

void RBackend::VidReset()
{
	COut(C_Info) << "VidReset..." << std::endl;
	gl.Reset();
	SDL_QuitSubSystem(SDL_INIT_VIDEO);
	SDL_InitSubSystem(SDL_INIT_VIDEO);
	SDL_EnableKeyRepeat(0, 0);
}

bool RBackend::VidBind()
{
	COut(C_Info) << "VidBind()..." << std::endl;
	COut(C_Info) << "Vendor: " << glGetString(GL_VENDOR) << std::endl <<
		"Renderer: " << glGetString(GL_RENDERER) << std::endl <<
		"Version:" << glGetString(GL_VERSION) << std::endl;
	COut(C_Info) << "Extensions: " << glGetString(GL_EXTENSIONS) << std::endl;

	gl.Load();

	COut(C_Info) << "MaxTextures: " << gl.maxTextures << std::endl;
	COut(C_Info) << "MaxVertexAttribs: " << gl.maxVertexAttribs << std::endl;

	return true;
}

bool RBackend::CheckCaps()
{
	return gl.v1_2 && 
		gl.ARB_vertex_buffer_object &&
#if !defined(RAD_OPT_OSX)
		gl.ARB_vertex_array_object && // OSX does not support this.
#endif
		gl.ARB_fragment_program &&
		gl.ARB_vertex_program &&
		gl.ARB_texture_compression &&
		gl.EXT_texture_compression_s3tc &&
		gl.ARB_multitexture &&
		gl.SGIS_generate_mipmap &&
		gl.ARB_texture_non_power_of_two;
}

// selects the closest valid video mode.
void RBackend::SelectVidMode(VidMode &mode)
{
	int bestDist = std::numeric_limits<int>::max();
	VidMode bestMode(mode);

	for (VidModeVec::const_iterator it = m_modes.begin(); it != m_modes.end(); ++it)
	{
		const VidMode &x = *it;
		if (x.fullscreen == mode.fullscreen)
		{
			int dx = mode.w-x.w;
			int dy = mode.h-x.h;
			int d = dx*dx+dy*dy;
			if (d < bestDist && dx >= 0 && dy >= 0)
			{
				bestMode = x;
				bestDist = d;
			}
		}
	}

	mode = bestMode;
}

const VidMode &RBackend::RAD_IMPLEMENT_GET(curVidMode)
{
	return m_curMode;
}

HContext RBackend::BindContext()
{
	return HContext(new (ZRender) RBContext());
}

void RBackend::GetCurMode()
{
	const SDL_VideoInfo *vi = SDL_GetVideoInfo();
	m_curMode.w = vi->current_w;
	m_curMode.h = vi->current_h;
	m_curMode.bpp = vi->vfmt->BitsPerPixel;
	m_curMode.hz = 60; // SDL 1.2 doesn't support this
	m_curMode.fullscreen = false;
}

void RBackend::EnumVidModes()
{
	COut(C_Info) << "EnumVidModes()" << std::endl;

	m_modes.clear();
	// get all possible windowed modes.
	MakeWindowedVidModeList(m_curMode, stdVidModes, m_modes);

	// ask SDL for fullscreen modes.
	SetSDLGLAttributes(
		true, // double buffer
		0, // no multisample
		0, // no vsync
		true // force hardware
	);
	
	// SDL_ListModes crashes on OSX so I'm doing it this way.
	for (int i = 0;stdVidModes[i] != -1;i += 2)
	{
		VidMode mode(stdVidModes[i], stdVidModes[i+1], m_curMode);
		mode.fullscreen = true;
		if (CheckVidMode(mode))
		{
			COut(C_Info) << "\t" <<
			mode.w << "x" << mode.h << "x" << mode.bpp << " " <<
			mode.hz << "hz" << std::endl;
			m_modes.push_back(mode);
		}
	}
}

const HContext &RBackend::RAD_IMPLEMENT_GET(ctx)
{
	if (!s_ctx.get())
	{
		s_ctx.reset(new HContext());
	}
	return *s_ctx.get();
}

void RBackend::RAD_IMPLEMENT_SET(ctx) (const HContext & ctx)
{
	if (!s_ctx.get())
	{
		s_ctx.reset(new HContext());
	}

	*s_ctx.get() = ctx;

	if (ctx)
	{
		gls.Bind(InterfaceComponent<RBContext>(ctx)->m_s);
	}
	else
	{
		gls.Bind(GLState::Ref());
	}

	CLEAR_GL_ERRORS();
}

RAD_BEGIN_EXPORT_COMPONENTS_FN(RADENG_API, ExportRBackendComponents)
	RAD_EXPORT_COMPONENT_NAMESPACE(r, RBackend)
RAD_END_EXPORT_COMPONENTS

} // r
