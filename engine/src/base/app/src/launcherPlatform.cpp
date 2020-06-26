/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher #]
***/

#include "build.h"
#include "launcherPlatform.h"

#ifdef PLATFORM_WINDOWS
    #include "winPlatform.h"
#elif defined(PLATFORM_POSIX)
    #include "posixPlatform.h"
#else
    #include "launcherPlatformCommon.h"
#endif

#include "application.h"

namespace base
{
    namespace platform
    {

        //---

        Platform::Platform()
            : m_exitRequestsed(0)
            , m_application(nullptr)
        {
        }

        Platform::~Platform()
        {
        }

        bool Platform::platformStart(const app::CommandLine& cmdline, app::IApplication* localApplication)
        {
            ScopeTimer timer;

            if (!handleStart(cmdline, localApplication))
                return false;

            TRACE_INFO("Platform initialized in {}", TimeInterval(timer.timeElapsed()));
            return true;
        }

        bool Platform::platformUpdate()
        {
            handleUpdate();

            if (1 == m_exitRequestsed.exchange(0))
            {
                TRACE_INFO("Platform serviced exit request");
                return false;
            }

            return true;
        }

        void Platform::platformIdle()
        {
            // TODO: max tick rate ? CPU limiting ?
        }

        void Platform::platformCleanup()
        {
    

            handleCleanup();
        }

        void Platform::requestExit(const char* reason)
        {
            if (0 == m_exitRequestsed.exchange(1))
            {
                TRACE_INFO("Request application exit{}{}", reason ? ": " : "", reason);
            }
        }

        //---

        Platform& GetLaunchPlatform()
        {
#ifdef PLATFORM_WINDOWS
            static auto thePlatform  = new win::Platform();
#elif defined(PLATFORM_POSIX)
            static auto thePlatform  = new posix::Platform();
#else
            static auto thePlatform  = new CommonPlatform();
#endif
            return *thePlatform;
        }

    }
}
