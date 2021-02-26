/***
* Boomer Engine v4
* Written by Tomasz Jonarski (RexDex)
* Source code licensed under LGPL 3.0 license
*
* [# filter: launcher\windows #]
* [# platform: winapi #]
***/

#include "build.h"

#include "launcherPlatformCommon.h"
#include "core/app/include/commandline.h"
#include "core/io/include/pathBuilder.h"

#include <Windows.h>
#include "winPlatform.h"
#include "winOutput.h"

BEGIN_BOOMER_NAMESPACE_EX(platform)

namespace win
{

    Platform::Platform()
        : m_hasLog(false)
        , m_hasErrors(false)
        , m_output(nullptr)
    {}

    Platform::~Platform()
    {
        if (m_hasLog && m_output)
			logging::Log::DetachGlobalSink(m_output.get());

        if (m_hasErrors)
			logging::IErrorHandler::BindListener(nullptr);
	}

    bool Platform::handleStart(const app::CommandLine& cmdline, app::IApplication* app)
    {
        // register output handlers
        m_hasLog = !cmdline.hasParam("silent") && !cmdline.hasParam("nolog");
        m_hasErrors = !cmdline.hasParam("silent") && !cmdline.hasParam("noasserts");
        bool console = cmdline.hasParam("console");

        // create the output handler
        bool openConsole = m_hasLog && !console && cmdline.hasParam("tty");
        bool verboseLog = cmdline.hasParam("verbose");
        m_output = CreateUniquePtr<win::GenericOutput>(openConsole, verboseLog);

        // register the output handler
        if (m_hasLog)
			logging::Log::AttachGlobalSink(m_output.get());

        // register the error handler
        if (m_hasErrors)
            logging::IErrorHandler::BindListener(m_output.get());

        // enter the protected region
        return protectedStart(cmdline, app);
    }

    bool Platform::protectedStart(const app::CommandLine& cmdline, app::IApplication* app)
    {
        // start
        if (IsDebuggerPresent())
        {
            return CommonPlatform::handleStart(cmdline, app);
        }
        else
        {
            __try
            {
                return CommonPlatform::handleStart(cmdline, app);
            }
            __except (m_output->handleException(GetExceptionInformation()))
            {
                requestExit("PlatformException");
                return false;
            }
        }
    }

    void Platform::handleCleanup()
    {
        // pass to base class
        CommonPlatform::handleCleanup();

        // unregister the stuff
		if (m_hasLog && m_output)
			logging::Log::DetachGlobalSink(m_output.get());

		if (m_hasErrors)
			logging::IErrorHandler::BindListener(nullptr);

        // delete the handler
        m_output.reset();
    }

    void Platform::handleUpdate()
    {
        if (IsDebuggerPresent())
        {
            return CommonPlatform::handleUpdate();
        }
        else
        {
            __try
            {
                // start normal launcher
                return CommonPlatform::handleUpdate();
            }
            __except (m_output->handleException(GetExceptionInformation()))
            {
                requestExit("PlatformException");
            }
        }
    }

} // win

bool Platform::HasDebuggerAttached()
{
    return ::IsDebuggerPresent();
}

END_BOOMER_NAMESPACE_EX(platform)
