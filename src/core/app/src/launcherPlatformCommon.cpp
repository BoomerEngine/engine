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

#include "core/app/include/commandline.h"
#include "core/io/include/io.h"
#include "core/io/include/timestamp.h"
#include "core/system/include/thread.h"
#include "core/system/include/debug.h"
#include "core/memory/include/poolStats.h"
#include "core/containers/include/stringBuf.inl"

#include <Windows.h>

BEGIN_BOOMER_NAMESPACE()

//--

CommonPlatform::CommonPlatform()
{}

CommonPlatform::~CommonPlatform()
{}

#undef GetUserName

bool CommonPlatform::handleStart(const CommandLine& systemCmdline, IApplication* application)
{
    // header
    TRACE_INFO("Boomer Engine v4");
    TRACE_INFO("Written by Tomasz \"Rex Dex\" Jonarski");
    TRACE_INFO("Compiled at {}, {}", __DATE__, __TIME__);

    // get time of day
    auto currentTime = TimeStamp::GetNow();
    TRACE_INFO("Started at {}", currentTime.toDisplayString());

    // system information
    TRACE_INFO("User name: {}", GetUserName());
    TRACE_INFO("Host name: {}", GetHostName());
    TRACE_INFO("System name: {}", GetSystemName());

    // reset random number generator
    srand((uint32_t)currentTime.value());

    // if no command line was specified load the default "exename.commandline.txt" file
    // this is useful in cases when we want to have some default behavior for an executable that otherwise requires parameters
    auto cmdLine  = &systemCmdline;
    CommandLine defaultCommandLine;
    if (systemCmdline.empty())
    {
        // prepare the special file name
        // we want the commandline.txt file to be related to the app
        auto executablePath = SystemPath(PathCategory::ExecutableFile);
        auto commandlinePath = StringBuf(TempString("{}.commandline.txt", executablePath));
        TRACE_WARNING("Command line not specified, loading stuff from '{}'", commandlinePath);

        // open the file using most basic function
        StringBuf text;
        if (LoadFileToString(commandlinePath, text))
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
    if (!InitializeFibers(*cmdLine))
        return false;

	// initialize network
	socket::Initialize();

    // initialize all services
    if (!ServiceContainer::GetInstance().init(*cmdLine))
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
        profiler::Block::Initialize((uint8_t)level);
    }
    else
    {
        TRACE_INFO("Profiling DISABLED", level);
        profiler::Block::InitializeDisabled();

    }
#endif
    // initialize RTTI caches
    TypeSystem::GetInstance().updateCaches();

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

    ServiceContainer::GetInstance().update();

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
    ServiceContainer::GetInstance().shutdown();

    // deinitialize any global singletons and free any non-persistent content acquired by them
    // NOTE: this step is mostly so we can report actual leaks more accurately
    TRACE_INFO("Deinitializing singletons...");
    ISingleton::DeinitializeAll();
    TRACE_INFO("Singletons deinitialized");

    // dump high-level shared pointer leaks
    //prv::SharedHolder::DumpLeaks();
    DumpLiveRefCountedObjects();

    // dump list of all unreleased memory blocks (leaks)
    DumpMemoryLeaks();
}

END_BOOMER_NAMESPACE()
