/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: platform\winapi #]
* [# platform: winapi #]
***/

#include "build.h"
#include "gl4ThreadWinApi.h"
#include "gl4SwapchainWinApi.h"
#include "gl4BackgroundQueueWinApi.h"

#include "gpu/api_common/include/apiWindow.h"
#include "core/app/include/commandline.h"

#ifndef DPI_ENUMS_DECLARED
typedef enum PROCESS_DPI_AWARENESS
{
    PROCESS_DPI_UNAWARE = 0,
    PROCESS_SYSTEM_DPI_AWARE = 1,
    PROCESS_PER_MONITOR_DPI_AWARE = 2
} PROCESS_DPI_AWARENESS;
#endif /*DPI_ENUMS_DECLARED*/

#ifndef DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2
#define DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2 ((DPI_AWARENESS_CONTEXT)-4)
#endif

BEGIN_BOOMER_NAMESPACE_EX(gpu::api::gl4)

//--

// setup format
const int PixelFormatAttributes[] =
{
	WGL_DRAW_TO_WINDOW_ARB, GL_TRUE,
	WGL_SUPPORT_OPENGL_ARB, GL_TRUE,
	WGL_DOUBLE_BUFFER_ARB, GL_TRUE,
	WGL_PIXEL_TYPE_ARB, WGL_TYPE_RGBA_ARB,
	WGL_COLOR_BITS_ARB, 32,
	0, // End
};

// context attributes
const int ContextAttributes[] =
{
	WGL_CONTEXT_MAJOR_VERSION_ARB, 4,
	WGL_CONTEXT_MINOR_VERSION_ARB, 6,
	WGL_CONTEXT_FLAGS_ARB, WGL_CONTEXT_DEBUG_BIT_ARB,
	WGL_CONTEXT_PROFILE_MASK_ARB, WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
	0, // End
};

// old pixel format descriptor
PIXELFORMATDESCRIPTOR pfd =
{
	sizeof(PIXELFORMATDESCRIPTOR),
	1,
	PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER,    //Flags
	PFD_TYPE_RGBA,        // The kind of framebuffer. RGBA or palette.
	32,                   // Colordepth of the framebuffer.
	0, 0, 0, 0, 0, 0,
	0,
	0,
	0,
	0, 0, 0, 0,
	24,                   // Number of bits for the depthbuffer
	8,                    // Number of bits for the stencilbuffer
	0,                    // Number of Aux buffers in the framebuffer.
	PFD_MAIN_PLANE,
	0,
	0, 0, 0
};

//--

ThreadWinApi::ThreadWinApi(Device* drv, WindowManager* windows)
	: Thread(drv, windows)
{
}

ThreadWinApi::~ThreadWinApi()
{
}

bool ThreadWinApi::threadStartup(const CommandLine& cmdLine, DeviceCaps& outCaps)
{
	// get the handle to dummy window
	m_hFakeHWND = (HWND)m_windows->offscreenWindow();
	if (!m_hFakeHWND)
	{
		TRACE_ERROR("Offscreen window is required for OpenGL rendering");
		return false;
	}

	// get the global DC
	HDC hDC = GetDC(m_hFakeHWND);
	if (NULL == hDC)
	{
		TRACE_ERROR("Failed to create a dummy window class");
		return false;
	}

	// find the pixel format
	auto pixelFormat = ChoosePixelFormat(hDC, &pfd);
	if (pixelFormat == 0)
	{
		ReleaseDC(m_hFakeHWND, hDC);
		TRACE_ERROR("Failed to find compatible pixel format for the rendering");
		return false;
	}

	// select the pixel format
	if (!SetPixelFormat(hDC, pixelFormat, &pfd))
	{
		ReleaseDC(m_hFakeHWND, hDC);
		TRACE_ERROR("Failed to change pixel format of dummy window");
		return false;
	}

	// create a temporary content (we cant use it since its OpenGL1.1 or sth)
	auto tempRC = wglCreateContext(hDC);
	if (NULL == tempRC)
	{
		ReleaseDC(m_hFakeHWND, hDC);
		TRACE_ERROR("Failed to create OpenGL context");
		return false;
	}

	// initialize extensions
	wglMakeCurrent(hDC, tempRC);
	wglewInit();

	// query the function
	auto wglCreateContextAttribsARB = (PFNWGLCREATECONTEXTATTRIBSARBPROC)wglGetProcAddress("wglCreateContextAttribsARB");
	if (!wglCreateContextAttribsARB)
	{
		wglDeleteContext(tempRC);
		ReleaseDC(m_hFakeHWND, hDC);
		TRACE_ERROR("ARB_create_context not supported, very old drivers :(");
		return false;
	}

	// destroy the temp context
	wglMakeCurrent(hDC, NULL);
	wglDeleteContext(tempRC);

	// create the proper context
	HGLRC hRC = wglCreateContextAttribsARB(hDC, NULL, ContextAttributes);
	if (NULL == hRC)
	{
		ReleaseDC(m_hFakeHWND, hDC);
		TRACE_ERROR("Failed to create context via wglCreateContextAttribsARB");
		return false;
	}

	// activate context
	if (!wglMakeCurrent(hDC, hRC))
	{
		wglDeleteContext(hRC);
		ReleaseDC(m_hFakeHWND, hDC);
		TRACE_ERROR("Failed to make the context active");
		return false;
	}

	// setup
	m_hFakeDC = hDC;
	m_hRC = hRC;
	TRACE_INFO("OpenGL context initialized");

	// pass to lower-level initialization
	return Thread::threadStartup(cmdLine, outCaps);
}

void ThreadWinApi::threadFinish()
{
	Thread::threadFinish();

	if (NULL != m_hRC)
	{
		TRACE_INFO("Destroying context");
		wglMakeCurrent(m_hFakeDC, NULL);
		wglDeleteContext(m_hRC);
		m_hRC = NULL;

		if (NULL != m_hFakeDC)
		{
			TRACE_INFO("Destroying DC");
			ReleaseDC(m_hFakeHWND, m_hFakeDC);
			m_hFakeDC = NULL;
		}
	}
}

void ThreadWinApi::acquireSwapchain(void* windowHandle, void* deviceHandle)
{
	DEBUG_CHECK_EX(GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

	if (m_hActiveDC != (HDC)deviceHandle)
	{
		PC_SCOPE_LVL1(AcquireGLSwapChain);
		m_hActiveDC = (HDC)deviceHandle;
		wglMakeCurrent(m_hActiveDC, m_hRC);
	}
}

void ThreadWinApi::presentSwapchain(void* windowHandle, void* deviceHandle)
{
	DEBUG_CHECK_EX(GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

	if (m_hActiveDC == (HDC)deviceHandle)
	{
		wglSwapIntervalEXT(0);
		PC_SCOPE_LVL1(PresentGLSwapChain);
		SwapBuffers((HDC)deviceHandle);
	}
}

void ThreadWinApi::releaseSwapchain(void* windowHandle, void* deviceHandle)
{
	DEBUG_CHECK_EX(GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

	if (m_hActiveDC == (HDC)deviceHandle)
	{
		PC_SCOPE_LVL1(ReleaseGLSwapChain);
		wglMakeCurrent(m_hFakeDC, m_hRC);
	}

	ReleaseDC((HWND)windowHandle, (HDC)deviceHandle);
}
		
IBaseSwapchain* ThreadWinApi::createOptimalSwapchain(const OutputInitInfo& info)
{
	if (info.m_class == OutputClass::Window)
	{
		auto window = m_windows->createWindow(info);
		DEBUG_CHECK_RETURN_EX_V(window, "Window not created", nullptr);

		// get device context for the window we just created - it's needed if we want to render to it
		HDC hDC = GetDC((HWND)window);
		if (NULL == hDC)
		{
			TRACE_ERROR("Window has no valid device context");
			m_windows->closeWindow(window);
			return nullptr;
		}

		// TODO: resolution setup for full screen windows

		// find the pixel format
		auto pixelFormat = ChoosePixelFormat(hDC, &pfd);
		if (pixelFormat == 0)
		{
			TRACE_ERROR("Failed to find compatible pixel format for the rendering");
			ReleaseDC((HWND)window, hDC);
			m_windows->closeWindow(window);
			return nullptr;
		}

		// select the pixel format
		TRACE_INFO("Selected pixel format {} for window", pixelFormat);
		if (!SetPixelFormat(hDC, pixelFormat, &pfd))
		{
			TRACE_ERROR("Failed to select compatible pixel format for the rendering");
			ReleaseDC((HWND)window, hDC);
			m_windows->closeWindow(window);
			return nullptr;
		}

		// configure
		// TODO: get ACTUAL format
		IBaseWindowedSwapchain::WindowSetup setup;
		setup.colorFormat = ImageFormat::RGBA8_UNORM;
		setup.depthFormat = ImageFormat::D24S8;
		setup.samples = 1;
		setup.flipped = true; // oh yeah, oh fuckin yeah
		setup.deviceHandle = 0;
		setup.windowHandle = window;
		setup.windowManager = m_windows;
		setup.windowInterface = m_windows->windowInterface(window);

		// create the swapchain
		return new SwapchainWinApi(info.m_class, setup, (void*)window, (void*)hDC, this);
	}

	// not possible to create other swapchains
	return nullptr;
}

ThreadSharedContextWinApi::ThreadSharedContextWinApi(HDC dc, HGLRC rc)
	: hDC(dc)
	, hRC(rc)
{}

ThreadSharedContextWinApi::~ThreadSharedContextWinApi()
{
	wglMakeCurrent(hDC, NULL);
	wglDeleteContext(hRC);
}

void ThreadSharedContextWinApi::activate()
{
	wglMakeCurrent(hDC, hRC);
}

void ThreadSharedContextWinApi::deactivate()
{
	wglMakeCurrent(hDC, NULL);
}

ThreadSharedContextWinApi* ThreadWinApi::createSharedContext()
{
	if (HGLRC hRC = wglCreateContextAttribsARB(m_hFakeDC, m_hRC, ContextAttributes))
	{
		TRACE_INFO("Created shared background context");
		return new ThreadSharedContextWinApi(m_hFakeDC, hRC);
	}

	TRACE_ERROR("Failed to create shared context");
	return nullptr;
}
					
IBaseBackgroundQueue* ThreadWinApi::createOptimalBackgroundQueue(const CommandLine& cmdLine)
{
	return new BackgroundQueueWinApi(this);
}

//--

END_BOOMER_NAMESPACE_EX(gpu::api::gl4)
