/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
* [# platform: winapi #]
***/

#include "build.h"
#include "glDevice.h"
#include "glDeviceThreadWinApi.h"
#include "glOutput.h"

#include "rendering/api_common/include/renderingWindow.h"
#include "base/app/include/commandline.h"

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

namespace rendering
{
    namespace gl4
    {
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
            WGL_CONTEXT_MINOR_VERSION_ARB, 4,
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

        DeviceThreadWinApi::DeviceThreadWinApi(Device* drv, WindowManager* windows)
            : DeviceThread(drv, windows)
            , m_hRC(NULL)
            , m_hFakeDC(NULL)
        {

        }

        DeviceThreadWinApi::~DeviceThreadWinApi()
        {

        }

        bool DeviceThreadWinApi::initialize(const base::app::CommandLine& cmdLine)
        {
            bool result = false;
            run([&result, this, cmdLine]()
                {
                    result = initializeContext_Thread(cmdLine);
                });

            if (!result)
                return false;

            return true;
        }

        void DeviceThreadWinApi::shutdownContext_Thread()
        {
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

        void DeviceThreadWinApi::prepareWindowForDeletion_Thread(uint64_t windowHandle, uint64_t deviceHandle)
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            if (m_hActiveDC == (HDC)deviceHandle)
            {
                PC_SCOPE_LVL0(wglMakeCurrent);
                wglMakeCurrent(m_hFakeDC, m_hRC);
            }

            ReleaseDC((HWND)windowHandle, (HDC)deviceHandle);
        }

        void DeviceThreadWinApi::postWindowForDeletion_Thread(uint64_t windowHandle)
        {
            DEBUG_CHECK_EX(base::GetCurrentThreadID() == m_threadId, "This function should be called on rendering thread");

            if (windowHandle)
            {
                m_windows->disconnectWindow(windowHandle);

                auto lock = CreateLock(m_windowsToCloseLock);
                m_windowsToClose.pushBack(windowHandle);
            }
        }

        bool DeviceThreadWinApi::initializeContext_Thread(const base::app::CommandLine& cmdLine)
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

            // initialize the extension library
            auto ret = glewInit();
            if (GLEW_OK != ret)
            {
                TRACE_ERROR("Failed to initialize GLEW, error: {}", glewGetErrorString(ret));
                return false;
            }

            // print OpenGL information
            TRACE_INFO("Vendor '{}'", (const char*)glGetString(GL_VENDOR));
            TRACE_INFO("Renderer '{}'", (const char*)glGetString(GL_RENDERER));
            TRACE_INFO("Version '{}'", (const char*)glGetString(GL_VERSION));
            TRACE_INFO("GLEW version: {}", glewGetString(GLEW_VERSION));
            TRACE_INFO("GLSL version: '{}'", (const char*)glGetString(GL_SHADING_LANGUAGE_VERSION));
            //TRACE_INFO("GLSL extensions '{}'", glGetEx(GL_EXTENENSIONS));

            // get version
            GLint majorVersion = 0;
            GLint minorVersion = 0;
            glGetIntegerv(GL_MAJOR_VERSION, &majorVersion);
            glGetIntegerv(GL_MINOR_VERSION, &minorVersion);
            TRACE_INFO("GL version: {}.{}", majorVersion, minorVersion);

            // enable debug output
#ifdef BUILD_RELEASE
            auto enableDebugging = cmdLine.singleValueBool("glDebug", false);
#else
            auto enableDebugging = cmdLine.singleValueBool("glDebug", true);
#endif

            // setup
            m_hFakeDC = hDC;
            m_hRC = hRC;
            TRACE_INFO("OpenGL context initialized");

            // initialize debugging
            if (enableDebugging)
                initializeDebug_Thread();

            return true;
        }

        ObjectID DeviceThreadWinApi::createOutput(const OutputInitInfo& info)
        {
            ObjectID outputHandle;

            // different wrappers require different stuff
            if (info.m_class == DriverOutputClass::NativeWindow || info.m_class == DriverOutputClass::Fullscreen)
            {
                auto* output = new Output(m_device, this, m_windows, info.m_class, true);
                outputHandle = output->handle();

                // create window
                auto window = m_windows->createWindow(outputHandle, info);
                if (!window)
                {
                    delete output;
                    return ObjectID();
                }

                // get device context for the window we just created - it's needed if we want to render to it
                HDC hDC = GetDC((HWND)window);
                if (NULL == hDC)
                {
                    TRACE_ERROR("Window has no valid device context");
                    m_windows->closeWindow(window);
                    delete output;
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
                    delete output;
                    return nullptr;
                }

                // select the pixel format
                TRACE_INFO("Selected pixel format {} for window", pixelFormat);
                if (!SetPixelFormat(hDC, pixelFormat, &pfd))
                {
                    TRACE_ERROR("Failed to select compatible pixel format for the rendering");
                    ReleaseDC((HWND)window, hDC);
                    m_windows->closeWindow(window);
                    delete output;
                    return nullptr;
                }

                // configure
                const auto colorFormat = ImageFormat::RGBA8_UNORM;
                const auto depthFormat = ImageFormat::D24S8;
                auto windowInterface = m_windows->windowInterface(window);
                output->bind(window, (uint64_t)hDC, colorFormat, depthFormat, windowInterface);
            }
            else
            {
                TRACE_ERROR("Unsupported output setup");
            }
            
            return outputHandle;
        }

        void DeviceThreadWinApi::bindOutput(ObjectID id)
        {
            if (auto* output = m_device->objectRegistry().resolveStatic<Output>(id))
            {
                DEBUG_CHECK_RETURN(output->deviceHandle());
                if (m_hActiveDC != (HDC)output->deviceHandle())
                {
                    PC_SCOPE_LVL0(wglMakeCurrent);
                    m_hActiveDC = (HDC)output->deviceHandle();
                    wglMakeCurrent(m_hActiveDC, m_hRC);
                }
            }
        }

        void DeviceThreadWinApi::swapOutput(ObjectID id)
        {
            if (auto* output = m_device->objectRegistry().resolveStatic<Output>(id))
            {
                DEBUG_CHECK_RETURN(output->deviceHandle());
                if (m_hActiveDC == (HDC)output->deviceHandle())
                {
                    PC_SCOPE_LVL0(SwapBuffers);
                    SwapBuffers((HDC)output->deviceHandle());

                    if (output->windowHandle())
                        m_windows->finishWindowRendering(output->windowHandle());

                    /*PC_SCOPE_LVL0(wglMakeCurrent);
                    wglMakeCurrent(m_hFakeDC, m_hRC);
                    m_hActiveDC = m_hFakeDC;*/
                }
            }
        }

        //--

    } // gl4
} // device
