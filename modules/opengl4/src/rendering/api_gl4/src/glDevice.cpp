/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: api #]
***/

#include "build.h"

#include "glDevice.h"
#include "glSampler.h"
#include "glImage.h"
#include "glShaders.h"
#include "glObjectCache.h"
#include "glDeviceThread.h"

#include "base/app/include/commandline.h"
#include "base/containers/include/stringBuilder.h"
#include "base/system/include/thread.h"

#include "rendering/device/include/renderingCommandBuffer.h"

#ifdef PLATFORM_WINDOWS
    #pragma comment(lib, "opengl32.lib")
#endif

#if defined(PLATFORM_WINAPI)
    #include "glDeviceThreadWinApi.h"
    typedef rendering::gl4::DeviceThreadWinApi DriverThreadClass;
#elif defined(PLATFORM_LINUX)
    #include "glDeviceThreadX11.h"
    typedef rendering::gl4::DriverThreadX11 DriverThreadClass;
#endif
#include "rendering/device/include/renderingOutput.h"
#include "glObjectRegistry.h"
#include "glTempBuffer.h"

namespace rendering
{
    namespace gl4
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(Device);
            RTTI_METADATA(DeviceNameMetadata).name("GL4");
        RTTI_END_TYPE();

        Device::Device()
        {
            memzero(m_predefinedImages, sizeof(m_predefinedImages));
            memzero(m_predefinedSamplers, sizeof(m_predefinedSamplers));
        }

        Device::~Device()
        {
            ASSERT(m_thread == nullptr);
            ASSERT(m_windows == nullptr);
        }
        
        base::StringBuf Device::name() const
        {
            return m_desc;
        }

        base::Point Device::maxRenderTargetSize() const
        {
            base::InplaceArray<base::Rect, 8> displays;
            m_windows->enumMonitorAreas(displays);

            base::Point ret;
            for (auto& info : displays)
            {
                ret.x = std::max<int>(ret.x, info.width());
                ret.y = std::max<int>(ret.y, info.height());
            }

            ret.x = std::max<int>(ret.x, 1920);
            ret.y = std::max<int>(ret.y, 1080);

            return ret;
        }

        bool Device::initialize(const base::app::CommandLine& cmdLine)
        {
            // create window manager
            m_windows = WindowManager::Create(true);
            if (!m_windows)
                return false;

            // create the runtime thread
            auto thread = new DriverThreadClass(this, m_windows);
            if (!thread->initialize(cmdLine))
            {
                delete m_windows;
                delete thread;
                return false;
            }

            // create the transient allocator
            m_thread = thread;
            m_objectRegistry = new ObjectRegistry(this, m_thread);
            m_objectCache = new ObjectCache(this);

            // staging pools
            m_bufferPoolConstants = new TempBufferPool(this, TempBufferType::Constants);
            m_bufferPoolStaging = new TempBufferPool(this, TempBufferType::Staging);

            // create default objects - NOTE this requires running on device thread
            m_thread->run([this]()
                {
                    createPredefinedImages();
                    createPredefinedSamplers();
                });

            // we are initialized now
            TRACE_INFO("OpenGL initialized");
            return true;
        }

        void Device::shutdown()
        {
            // finish any high level rendering
            sync();

            // delete object registry on main thread
            delete m_objectRegistry;
            m_objectRegistry = nullptr;

            // predefined objects are also freed since they were allocated from pool
            memset(m_predefinedImages, 0, sizeof(m_predefinedImages));
            memset(m_predefinedSamplers, 0, sizeof(m_predefinedSamplers));

            // delete object cache
            // NOTE: deletions of cached objects must be run from device thread
            m_thread->run([this]()
                {
                    delete m_objectCache;
                    m_objectCache = nullptr;

                    delete m_bufferPoolConstants;
                    m_bufferPoolConstants = nullptr;

                    delete m_bufferPoolStaging;
                    m_bufferPoolStaging = nullptr;
                });

            // sync to let the objects delete properly
            sync();

            // close the runtime thread
            delete m_thread;
            m_thread = nullptr;

            // close the window manager
            delete m_windows;
            m_windows = nullptr;
        }

        void Device::advanceFrame()
        {
            PC_SCOPE_LVL1(AdvanceFrame);
            m_windows->updateWindows();
            m_thread->advanceFrame();
        }

        void Device::sync()
        {
            PC_SCOPE_LVL1(DriverSync);
            m_thread->sync();
        }

        //---

        const char* GetGLErrorDesc(GLuint result)
        {
            switch (result)
            {
                case GL_NO_ERROR: return "GL_NO_ERROR";
                case GL_INVALID_ENUM: return "GL_INVALID_ENUM";
                case GL_INVALID_VALUE: return "GL_INVALID_VALUE";
                case GL_INVALID_OPERATION: return "GL_INVALID_OPERATION";
                case GL_INVALID_FRAMEBUFFER_OPERATION: return "GL_INVALID_FRAMEBUFFER_OPERATION";
                case GL_OUT_OF_MEMORY: return "GL_OUT_OF_MEMORY";
                case GL_STACK_UNDERFLOW: return "GL_STACK_UNDERFLOW";
                case GL_STACK_OVERFLOW: return "GL_STACK_OVERFLOW";
            }

            return "Unknown";
        }

        void ValidateResult(const char* testExpr, uint32_t line, const char* file)
        {
#ifndef BUILD_RELEASE
            auto result = glGetError();
            if (result != GL_NO_ERROR)
            {
                TRACE_ERROR("Runtime API error: {}", GetGLErrorDesc(result));
                TRACE_ERROR("{}({}): GL: Failed expression: {}", file, line, testExpr);
            }

#ifdef BUILD_DEBUG
            DEBUG_CHECK_EX(result == GL_NO_ERROR, base::TempString("Runtime API error: {} at {}", GetGLErrorDesc(result), testExpr));
#endif
#endif
        }

        //---

        void Device::submitWork(command::CommandBuffer* masterCommandBuffer, bool background)
        {
            (void)background; // no background rendering jobs on OpenGL :(
            m_thread->submit(masterCommandBuffer);
        }

        //---

    } // gl4
} // rendering
