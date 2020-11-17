/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: driver #]
***/

#include "build.h"

#include "glDriver.h"
#include "glSampler.h"
#include "glImage.h"
#include "glShaderLibraryAdapter.h"
#include "glTransientAllocator.h"
#include "glObjectCache.h"
#include "glDriverThread.h"

#include "base/app/include/commandline.h"
#include "base/containers/include/stringBuilder.h"
#include "base/system/include/thread.h"

#include "rendering/driver/include/renderingCommandBuffer.h"

#ifdef PLATFORM_WINDOWS
    #pragma comment(lib, "opengl32.lib")
#endif

#if defined(PLATFORM_WINAPI)
    #include "glDriverThreadWinApi.h"
    typedef rendering::gl4::DriverThreadWinApi DriverThreadClass;
#elif defined(PLATFORM_LINUX)
    #include "glDriverThreadX11.h"
    typedef rendering::gl4::DriverThreadX11 DriverThreadClass;
#endif
#include "rendering/driver/include/renderingOutput.h"

namespace rendering
{
    namespace gl4
    {
        //--

        RTTI_BEGIN_TYPE_CLASS(Driver);
            RTTI_METADATA(DriverNameMetadata).name("GL4");
        RTTI_END_TYPE();

        Driver::Driver()
            : m_transientAllocator(nullptr)
            , m_thread(nullptr)
        {
            // reset table of predefined objects
            memzero(m_predefinedImages, sizeof(m_predefinedImages));
            memzero(m_predefinedSamplers, sizeof(m_predefinedSamplers));
        }

        Driver::~Driver()
        {
            ASSERT(m_thread == nullptr);
            ASSERT(m_windows == nullptr);
        }
        
        base::StringBuf Driver::runtimeDescription() const
        {
            return m_desc;
        }

        base::Point Driver::maxRenderTargetSize() const
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

        bool Driver::isVerticalFlipRequired() const
        {
            return true;
        }

        bool Driver::supportsAsyncCommandBufferBuilding() const
        {
            return false;
        }

        bool Driver::initialize(const base::app::CommandLine& cmdLine)
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
            m_transientAllocator = new TransientAllocator(this);
            m_objectCache = new ObjectCache(this);

            // create default objects - NOTE this requires running on device thread
            m_thread->run([this]()
                {
                    createPredefinedImages();
                    createPredefinedSamplers();
                });

            // we are initialized now
            ChangeActiveDevice(this);
            TRACE_INFO("OpenGL initialized");
            return true;
        }

        void Driver::shutdown()
        {
            // finish any high level rendering
            sync();

            // release all objects
            ChangeActiveDevice(nullptr);

            // delete object cache
            if (m_thread)
            {
                // NOTE: deletions of cached objects must be run from device thread
                m_thread->run([this]()
                    {
                        for (auto* image : m_predefinedImages)
                            delete image;
                        memset(m_predefinedImages, 0, sizeof(m_predefinedImages));
                        memset(m_predefinedSamplers, 0, sizeof(m_predefinedSamplers));

                        delete m_transientAllocator;
                        m_transientAllocator = nullptr;

                        delete m_objectCache;
                        m_objectCache = nullptr;
                    });
            }

            // sync to let the objects delete properly
            sync();

            // close the runtime thread
            delete m_thread;
            m_thread = nullptr;

            // close the window manager
            delete m_windows;
            m_windows = nullptr;
        }

        void Driver::advanceFrame()
        {
            PC_SCOPE_LVL1(AdvanceFrame);
            m_windows->updateWindows();
            m_thread->advanceFrame();
        }

        void Driver::sync()
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

        ObjectID Driver::createOutput(const DriverOutputInitInfo& info)
        {
            return m_thread->createOutput(info);
        }

        IDriverNativeWindowInterface* Driver::queryOutputWindow(ObjectID output) const
        {
            return m_thread->queryOutputWindow(output);
        }

        bool Driver::prepareOutputFrame(ObjectID output, DriverOutputFrameInfo& outFrameInfo)
        {
            return m_thread->prepareOutputFrame(output, outFrameInfo);
        }

        void Driver::enumMonitorAreas(base::Array<base::Rect>& outMonitorAreas) const
        {
            m_windows->enumMonitorAreas(outMonitorAreas);
        }

        void Driver::enumDisplays(base::Array<DriverDisplayInfo>& outDisplayInfos) const
        {
            m_windows->enumDisplays(outDisplayInfos);
        }

        void Driver::enumResolutions(uint32_t displayIndex, base::Array<DriverResolutionInfo>& outResolutions) const
        {
            m_windows->enumResolutions(displayIndex, outResolutions);
        }

        void Driver::enumVSyncModes(uint32_t displayIndex, base::Array<DriverResolutionSyncInfo>& outVSyncModes) const
        {
            m_windows->enumVSyncModes(displayIndex, outVSyncModes);
        }

        void Driver::enumRefreshRates(uint32_t displayIndex, const DriverResolutionInfo& info, base::Array<int>& outRefreshRates) const
        {
            m_windows->enumRefreshRates(displayIndex, info, outRefreshRates);
        }

        void Driver::submitWork(command::CommandBuffer* masterCommandBuffer, bool background)
        {
            (void)background; // no background rendering jobs on OpenGL :(
            m_thread->submit(masterCommandBuffer);
        }

        //---

    } // gl4
} // driver
