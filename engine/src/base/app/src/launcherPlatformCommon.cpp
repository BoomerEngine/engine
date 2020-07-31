/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\common #]
***/

#include "build.h"
#include "launcherPlatformCommon.h"
#include "application.h"
#include "localServiceContainer.h"

#include "base/app/include/commandline.h"
#include "base/io/include/ioSystem.h"
#include "base/io/include/timestamp.h"
#include "base/system/include/thread.h"
#include "base/system/include/debug.h"
#include "base/memory/include/poolStats.h"
#include "base/containers/include/stringBuf.inl"

#include <Windows.h>

namespace base
{
    namespace platform
    {

        //--

        CommonPlatform::CommonPlatform()
        {}

        CommonPlatform::~CommonPlatform()
        {}

        #undef GetUserName

        bool CommonPlatform::handleStart(const app::CommandLine& systemCmdline, app::IApplication* application)
        {
            // header
            TRACE_INFO("Boomer Engine v4");
            TRACE_INFO("Written by Tomasz \"Rex Dex\" Jonarski");
            TRACE_INFO("Compiled at {}, {}", __DATE__, __TIME__);

            // get time of day
            auto currentTime = io::TimeStamp::GetNow();
            TRACE_INFO("Started at {}", currentTime.toDisplayString());

            // system information
            TRACE_INFO("User name: {}", base::GetUserName());
            TRACE_INFO("Host name: {}", base::GetHostName());
            TRACE_INFO("System name: {}", base::GetSystemName());

            // reset random number generator
            srand((uint32_t)currentTime.value());

            // if no command line was specified load the default "exename.commandline.txt" file
            // this is useful in cases when we want to have some default behavior for an executable that otherwise requires parameters
            auto cmdLine  = &systemCmdline;
            app::CommandLine defaultCommandLine;
            if (systemCmdline.empty())
            {
                // prepare the special file name
                // we want the commandline.txt file to be related to the app
                auto commandlinePath = base::io::SystemPath(base::io::PathCategory::ExecutableFile).changeExtension(UTF16StringBuf(L"commandline.txt"));
                TRACE_WARNING("Command line not specified, loading stuff from '{}'", commandlinePath);

                // open the file using most basic function
                StringBuf text;
                if (io::LoadFileToString(commandlinePath, text))
                { 
                    if (!defaultCommandLine.parse(text.c_str(), false))
                    {
                        TRACE_ERROR("Failed to parse valid commandline from the commandline.txt file");
                        return false;
                    }
                }
                
                // use the loaded command line
                cmdLine = &defaultCommandLine;
            }

            // initialize fibers
            if (!Fibers::GetInstance().initialize(*cmdLine))
                return false;

			// initialize network
			base::socket::Initialize();

            // initialize all services
            if (!app::LocalServiceContainer::GetInstance().init(*cmdLine))
                return false;

            // no app
            if (cmdLine->hasParam("noapp") && application)
            {
                TRACE_WARNING("Application will not be run because we were started with -noapp");
                application = nullptr;
            }

#if !defined(NO_PROFILING)
            uint32_t level = cmdLine->singleValueInt("profile", 0);
            if (level > 0)
            {
                TRACE_INFO("Profiling ENABLED at LEVEL {}", level);
                base::profiler::Block::Initialize((uint8_t)level);
            }
            else
            {
                TRACE_INFO("Profiling DISABLED", level);
                base::profiler::Block::InitializeDisabled();

            }
#endif
            // initialize RTTI caches
            rtti::TypeSystem::GetInstance().updateCaches();

            // if we have the app run it
            if (application)
            {
                if (!application->initialize(*cmdLine))
                {
                    application->cleanup();
                    if (cmdLine->command().empty())
                        TRACE_ERROR("Application failed to initialize, we will report failure and most likely exit the process");
                    return false;
                }
            }

            m_application = application;
            return true;
        }

        void CommonPlatform::handleUpdate()
        {
            PC_SCOPE_LVL0(PlatformUpdate);

            app::LocalServiceContainer::GetInstance().update();

            if (m_application)
                m_application->update();
        }

        void CommonPlatform::handleCleanup()
        {
            // close the application
            if (m_application)
            {
                TRACE_INFO("Closing application...");
                m_application->cleanup();
                m_application = nullptr;
                TRACE_INFO("Application closed");
            }

            // close all services
            app::LocalServiceContainer::GetInstance().shutdown();

            // deinitialize any global singletons and free any non-persistent content acquired by them
            // NOTE: this step is mostly so we can report actual leaks more accurately
            TRACE_INFO("Deinitializing singletons...");
            base::ISingleton::DeinitializeAll();
            TRACE_INFO("Singletons deinitialized");

            // dump high-level shared pointer leaks
            //base::prv::SharedHolder::DumpLeaks();
            base::DumpLiveRefCountedObjects();

            // dump list of all unreleased memory blocks (leaks)
            base::mem::DumpMemoryLeaks();
        }

    } // platform
} // base